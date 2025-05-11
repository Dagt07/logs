#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdint> // For standard int64_t
#include <limits>

#define M 50 * 1024 * 1024 // Tamaño de M (50MB)
#define B 1024 * 1024      // Tamaño de bloque B (1MB)

using namespace std;

// Función para leer un bloque de tamaño B desde el archivo
bool readBlock(FILE* file, vector<int64_t>& buffer) {
    buffer.resize(B / sizeof(int64_t)); // Ajuste del tamaño del buffer
    size_t elementsRead = fread(buffer.data(), sizeof(int64_t), buffer.size(), file);
    return elementsRead == buffer.size(); // Devuelve true si el bloque se leyó correctamente
}

// Función para escribir un bloque al archivo
void writeBlock(FILE* file, const vector<int64_t>& buffer) {
    fwrite(buffer.data(), sizeof(int64_t), buffer.size(), file);
}

// Función de fusión de varios bloques ordenados
void mergeBlocks(FILE* output, vector<FILE*>& inputFiles) {
    vector<vector<int64_t>> buffers(inputFiles.size(), vector<int64_t>(B / sizeof(int64_t)));
    vector<size_t> indices(inputFiles.size(), 0);

    bool allFilesRead = true;
    
    // Leer los primeros bloques de todos los archivos
    for (size_t i = 0; i < inputFiles.size(); ++i) {
        if (!readBlock(inputFiles[i], buffers[i])) {
            allFilesRead = false;
        }
    }

    vector<int64_t> mergedBuffer(B / sizeof(int64_t));
    size_t mergeIndex = 0;

    // Mientras haya elementos por fusionar
    while (!allFilesRead) {
        size_t minIndex = -1;
        int64_t minValue = std::numeric_limits<int64_t>::max();

        // Encuentra el elemento mínimo entre los archivos
        for (size_t i = 0; i < inputFiles.size(); ++i) {
            if (indices[i] < buffers[i].size()) {
                if (buffers[i][indices[i]] < minValue) {
                    minValue = buffers[i][indices[i]];
                    minIndex = i;
                }
            }
        }

        // Si hay un valor mínimo encontrado, agregarlo al buffer de fusión
        if (minIndex != -1) {
            mergedBuffer[mergeIndex++] = minValue;
            indices[minIndex]++;

            // Si el buffer se llena, escribirlo al archivo de salida
            if (mergeIndex == mergedBuffer.size()) {
                writeBlock(output, mergedBuffer);
                mergeIndex = 0;
            }
        }

        // Leer el siguiente bloque si se ha terminado de procesar el bloque actual
        for (size_t i = 0; i < inputFiles.size(); ++i) {
            if (indices[i] == buffers[i].size()) {
                if (!readBlock(inputFiles[i], buffers[i])) {
                    allFilesRead = true;
                }
                indices[i] = 0;
            }
        }
    }

    // Escribir cualquier sobrante en el buffer de fusión
    if (mergeIndex > 0) {
        writeBlock(output, mergedBuffer);
    }
}

// Función para realizar el MergeSort Externo con aridad a
void externalMergeSort(FILE* inputFile, FILE* outputFile, size_t start, size_t end, size_t a) {
    // Caso base: si el bloque es suficientemente pequeño, ordenar en memoria
    if (end - start <= B) {
        fseek(inputFile, start * sizeof(int64_t), SEEK_SET);

        vector<int64_t> buffer(end - start);
        fread(buffer.data(), sizeof(int64_t), buffer.size(), inputFile);
        sort(buffer.begin(), buffer.end());

        fseek(outputFile, start * sizeof(int64_t), SEEK_SET);
        fwrite(buffer.data(), sizeof(int64_t), buffer.size(), outputFile);
        return;
    }

    // Dividir el arreglo en a subarreglos
    size_t segmentSize = (end - start) / a;
    vector<FILE*> subFiles(a, nullptr);

    for (size_t i = 0; i < a; ++i) {
        size_t segmentStart = start + i * segmentSize;
        size_t segmentEnd = (i == a - 1) ? end : segmentStart + segmentSize;

        subFiles[i] = fopen(("temp_sub_" + to_string(i)).c_str(), "wb");

        // Recursión sobre el subarreglo
        externalMergeSort(inputFile, subFiles[i], segmentStart, segmentEnd, a);
    }

    // Fusionar los subarreglos
    mergeBlocks(outputFile, subFiles);

    // Cerrar archivos temporales
    for (size_t i = 0; i < a; ++i) {
        fclose(subFiles[i]);
    }
}

// Función principal de MergeSort Externo
void performExternalMergeSort(const char* inputFileName, const char* outputFileName, size_t a) {
    // Abre el archivo de entrada
    FILE* inputFile = fopen(inputFileName, "rb");
    FILE* outputFile = fopen(outputFileName, "wb");

    if (!inputFile || !outputFile) {
        cerr << "Error al abrir archivos." << endl;
        return;
    }

    // Obtener el tamaño del archivo
    fseek(inputFile, 0, SEEK_END);
    size_t fileSize = ftell(inputFile) / sizeof(int64_t);
    fclose(inputFile);

    // Reabrir el archivo para ordenarlo
    inputFile = fopen(inputFileName, "rb");
    externalMergeSort(inputFile, outputFile, 0, fileSize, a);

    fclose(inputFile);
    fclose(outputFile);
}

// Función para generar datos aleatorios y guardarlos en un archivo binario
void generateTestData(const char* filename, size_t numElements) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        cerr << "Error al abrir el archivo para escribir datos." << endl;
        return;
    }

    for (size_t i = 0; i < numElements; ++i) {
        int64_t value = rand(); // Generamos números aleatorios de 64 bits
        fwrite(&value, sizeof(int64_t), 1, file);
    }

    fclose(file);
}

int main() {
    // Definir el nombre del archivo de entrada y salida
    const char* inputFileName = "input_4M.bin";
    const char* outputFileName = "output_4M.bin";

    // Definir el tamaño del arreglo, N = 4M (4 * 50MB en 64 bits)
    size_t numElements = 4 * M / sizeof(int64_t); // 4 * 50MB en números de 64 bits

    // Sembrar el generador de números aleatorios
    srand(time(0));

    // Generar datos de prueba y guardarlos en un archivo binario
    generateTestData(inputFileName, numElements);

    // Definir la aridad 'a' (puedes probar con diferentes valores, por ejemplo, a = 4)
    size_t a = 4;

    // Ejecutar MergeSort Externo con la aridad 'a'
    performExternalMergeSort(inputFileName, outputFileName, a);

    cout << "El ordenamiento ha sido completado con éxito." << endl;

    return 0;
}
