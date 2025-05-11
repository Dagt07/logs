#include "mergesort.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <queue>
#include <string>
#include <memory>
#include <limits>
#include <cassert>

// Estructura para manejar buffers en el merge
struct BufferHandler {
    std::vector<int64_t> buffer;  // Buffer para almacenar datos
    size_t position;              // Posición actual en el buffer
    FILE* file;                   // Archivo de donde se lee
    bool empty;                   // Indica si no hay más datos para leer

    BufferHandler(FILE* f) : position(0), file(f), empty(false) {
        buffer.resize(B / sizeof(int64_t));
    }

    // Obtiene el siguiente valor del buffer
    int64_t next() {
        if (position >= buffer.size() || empty) {
            return std::numeric_limits<int64_t>::max();
        }
        return buffer[position++];
    }

    // Verifica si hay más elementos disponibles
    bool hasNext() {
        return !empty && position < buffer.size();
    }

    // Recarga el buffer desde el archivo
    bool reload() {
        if (empty) return false;
        
        position = 0;
        size_t itemsRead = fread(buffer.data(), sizeof(int64_t), buffer.size(), file);
        
        if (itemsRead == 0) {
            empty = true;
            return false;
        }
        
        // Si leímos menos elementos que el tamaño del buffer, ajustamos el tamaño
        if (itemsRead < buffer.size()) {
            buffer.resize(itemsRead);
        }
        
        return true;
    }
};

// Función para leer un bloque de tamaño B desde el archivo
bool readBlock(FILE* file, std::vector<int64_t>& buffer) {
    if (!file) return false;
    
    // Calcular cuántos elementos de 64 bits caben en un bloque de tamaño B
    size_t elementsPerBlock = B / sizeof(int64_t);
    buffer.resize(elementsPerBlock);
    
    // Leer el bloque
    size_t elementsRead = fread(buffer.data(), sizeof(int64_t), elementsPerBlock, file);
    
    // Ajustar el tamaño del buffer si leímos menos elementos
    if (elementsRead < elementsPerBlock) {
        buffer.resize(elementsRead);
    }
    
    return elementsRead > 0;
}

// Función para escribir un bloque al archivo
void writeBlock(FILE* file, const std::vector<int64_t>& buffer) {
    if (!file) return;
    
    // Escribir el bloque completo
    fwrite(buffer.data(), sizeof(int64_t), buffer.size(), file);
}

// Función de fusión de varios bloques ordenados con aridad 'a'
void mergeBlocks(FILE* output, std::vector<FILE*>& inputFiles) {
    const size_t bufferSize = B / sizeof(int64_t);
    std::vector<int64_t> outputBuffer;
    outputBuffer.reserve(bufferSize);
    
    // Crear handlers para cada archivo de entrada
    std::vector<std::unique_ptr<BufferHandler>> handlers;
    for (FILE* file : inputFiles) {
        handlers.push_back(std::make_unique<BufferHandler>(file));
        handlers.back()->reload();  // Cargar datos iniciales
    }
    
    // Usar una cola de prioridad para encontrar el mínimo elemento
    auto compare = [&handlers](size_t a, size_t b) {
        return handlers[a]->next() > handlers[b]->next();
    };
    
    // Realizar el merge mientras haya elementos
    bool hasMoreData = true;
    while (hasMoreData) {
        hasMoreData = false;
        
        // Encontrar el elemento mínimo entre todos los buffers
        int64_t minValue = std::numeric_limits<int64_t>::max();
        size_t minIndex = handlers.size();
        
        for (size_t i = 0; i < handlers.size(); i++) {
            if (handlers[i]->hasNext()) {
                hasMoreData = true;
                int64_t value = handlers[i]->buffer[handlers[i]->position];
                if (value < minValue) {
                    minValue = value;
                    minIndex = i;
                }
            } else if (!handlers[i]->empty) {
                // Recargar buffer si está vacío pero el archivo aún tiene datos
                if (handlers[i]->reload()) {
                    hasMoreData = true;
                    int64_t value = handlers[i]->buffer[handlers[i]->position];
                    if (value < minValue) {
                        minValue = value;
                        minIndex = i;
                    }
                }
            }
        }
        
        // Si encontramos un elemento mínimo, lo agregamos al buffer de salida
        if (minIndex < handlers.size()) {
            outputBuffer.push_back(minValue);
            handlers[minIndex]->position++;
            
            // Si el buffer de salida está lleno, lo escribimos a disco
            if (outputBuffer.size() >= bufferSize) {
                writeBlock(output, outputBuffer);
                outputBuffer.clear();
            }
        }
    }
    
    // Escribir el resto del buffer de salida si quedan elementos
    if (!outputBuffer.empty()) {
        writeBlock(output, outputBuffer);
    }
}

// Función para realizar el MergeSort Externo con aridad a
void externalMergeSort(FILE* inputFile, FILE* outputFile, size_t start, size_t end, size_t a) {
    // Calcular el tamaño total en bytes
    fseek(inputFile, 0, SEEK_END);
    size_t fileSize = ftell(inputFile);
    size_t totalElements = fileSize / sizeof(int64_t);
    
    // Si el archivo es suficientemente pequeño para caber en memoria, ordenarlo directamente
    if (totalElements * sizeof(int64_t) <= M) {
        std::vector<int64_t> buffer;
        buffer.reserve(totalElements);
        
        // Leer todo el archivo
        fseek(inputFile, 0, SEEK_SET);
        for (size_t i = 0; i < totalElements; i += B / sizeof(int64_t)) {
            std::vector<int64_t> tempBuffer;
            readBlock(inputFile, tempBuffer);
            buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.end());
        }
        
        // Ordenar en memoria
        std::sort(buffer.begin(), buffer.end());
        
        // Escribir de vuelta ordenado
        fseek(outputFile, 0, SEEK_SET);
        for (size_t i = 0; i < buffer.size(); i += B / sizeof(int64_t)) {
            size_t blockSize = std::min(B / sizeof(int64_t), buffer.size() - i);
            std::vector<int64_t> tempBuffer(buffer.begin() + i, buffer.begin() + i + blockSize);
            writeBlock(outputFile, tempBuffer);
        }
        
        return;
    }
    
    // Crear archivos temporales para las particiones
    std::vector<FILE*> tempFiles;
    std::vector<std::string> tempFileNames;
    
    // Calcular tamaño aproximado de cada partición
    size_t partitionSize = (totalElements + a - 1) / a;
    
    // Crear archivos temporales
    for (size_t i = 0; i < a; i++) {
        std::string tempFileName = "temp_" + std::to_string(i) + ".bin";
        tempFileNames.push_back(tempFileName);
        FILE* tempFile = fopen(tempFileName.c_str(), "wb+");
        if (!tempFile) {
            std::cerr << "Error creating temporary file: " << tempFileName << std::endl;
            exit(1);
        }
        tempFiles.push_back(tempFile);
    }
    
    // Distribuir los elementos en los archivos temporales
    fseek(inputFile, 0, SEEK_SET);
    for (size_t i = 0; i < a; i++) {
        size_t elementsToRead = std::min(partitionSize, totalElements - i * partitionSize);
        
        // Si esta partición cabe en memoria, la ordenamos directamente
        if (elementsToRead * sizeof(int64_t) <= M) {
            std::vector<int64_t> buffer;
            buffer.reserve(elementsToRead);
            
            // Leer la partición
            for (size_t j = 0; j < elementsToRead; j += B / sizeof(int64_t)) {
                std::vector<int64_t> tempBuffer;
                readBlock(inputFile, tempBuffer);
                buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.end());
            }
            
            // Ordenar en memoria
            std::sort(buffer.begin(), buffer.end());
            
            // Escribir ordenado al archivo temporal
            fseek(tempFiles[i], 0, SEEK_SET);
            for (size_t j = 0; j < buffer.size(); j += B / sizeof(int64_t)) {
                size_t blockSize = std::min(B / sizeof(int64_t), buffer.size() - j);
                std::vector<int64_t> tempBuffer(buffer.begin() + j, buffer.begin() + j + blockSize);
                writeBlock(tempFiles[i], tempBuffer);
            }
        } else {
            // Si no cabe en memoria, crear un archivo temporal para esta partición
            // y llamar recursivamente a externalMergeSort
            std::string subTempFileName = "sub_temp_" + std::to_string(i) + ".bin";
            FILE* subTempFile = fopen(subTempFileName.c_str(), "wb+");
            
            // Copiar los elementos al archivo temporal
            for (size_t j = 0; j < elementsToRead; j += B / sizeof(int64_t)) {
                std::vector<int64_t> tempBuffer;
                readBlock(inputFile, tempBuffer);
                writeBlock(subTempFile, tempBuffer);
            }
            
            // Ordenar recursivamente esta partición
            rewind(subTempFile);
            externalMergeSort(subTempFile, tempFiles[i], 0, elementsToRead, a);
            
            // Cerrar y eliminar el archivo temporal
            fclose(subTempFile);
            remove(subTempFileName.c_str());
        }
    }
    
    // Reposicionar todos los archivos temporales al inicio para el merge
    for (FILE* file : tempFiles) {
        rewind(file);
    }
    
    // Fusionar los archivos temporales ordenados
    mergeBlocks(outputFile, tempFiles);
    
    // Cerrar y eliminar los archivos temporales
    for (size_t i = 0; i < tempFiles.size(); i++) {
        fclose(tempFiles[i]);
        remove(tempFileNames[i].c_str());
    }
}

// Función principal de MergeSort Externo
void performExternalMergeSort(const char* inputFileName, const char* outputFileName, size_t a) {
    FILE* inputFile = fopen(inputFileName, "rb");
    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFileName << std::endl;
        exit(1);
    }
    
    FILE* outputFile = fopen(outputFileName, "wb");
    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFileName << std::endl;
        fclose(inputFile);
        exit(1);
    }
    
    // Calcular el tamaño del archivo
    fseek(inputFile, 0, SEEK_END);
    size_t fileSize = ftell(inputFile);
    size_t totalElements = fileSize / sizeof(int64_t);
    
    // Ordenar el archivo
    externalMergeSort(inputFile, outputFile, 0, totalElements, a);
    
    // Cerrar archivos
    fclose(inputFile);
    fclose(outputFile);
}

// Función para verificar si un archivo está ordenado
bool checkSorted(const std::string &filename) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        std::cerr << "Error opening file for checking: " << filename << std::endl;
        return false;
    }
    
    int64_t prevValue = std::numeric_limits<int64_t>::min();
    std::vector<int64_t> buffer;
    
    while (readBlock(file, buffer)) {
        for (int64_t value : buffer) {
            if (value < prevValue) {
                fclose(file);
                return false;
            }
            prevValue = value;
        }
    }
    
    fclose(file);
    return true;
}