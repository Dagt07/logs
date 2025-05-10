//se define M_SIZE <- tamaño de la memoria principal: valor capturado desde los args
//se define B_SIZE <- tamaño del bloque: como tamaño real de bloque en un disco (512B en discos viejos, 4069B en discos modernos) 
//se define N_SIZE <- tamaño del array que contiene a los elementos a ordenar (input), recibido por quicksort
//se define a <- cantidad de particiones que se van a realizar en el algoritmo de quicksort, recibido por quicksort y no cambia

//variables globales
//M_SIZE recibida en el main
//B_SIZE recibida en el main

//quicksort args:
//a: cantidad de particiones a realizar
//N: array a ordenar

//quicksort version 3:
//si N_SIZE <= M_SIZE
// se ordena el bloque completo gratis en memoria principal con sort
//else N_SIZE > M_SIZE
// 1) Se selecciona un bloque de tamaño B_SIZE aleatorio de nuestro input N de N_SIZE (ya que solo podemos leer B_SIZE bytes a la vez)
// 2) Se seleccionan aleatoriamente a-1 pivotes dentro del bloque seleccionado en el paso anterior (ya son elementos al azar, por lo que son buenos candidatos como pivotes)
// 2.1) en un buffer ordeno gratis los pivotes seleccionados en el paso anterior (es gratis, ya que en general a es menor que B_SIZE = 4096)
// 3) Uso secuencialmente los pivotes ordenados para definir en que (a-1 archivos temporales usados como buffers) se almacenan los elementos menores, entre medio y mayores a los pivotes seleccionados en el paso 2 leyendo el input N por bloques de tamaño B_SIZE
// 4) Se llama recursivamente a quicksort para cada uno de los buffers generados en el paso 3
// 5) Finalmente se unen los buffers generados de acuerdo a los pivotes, ejemplo:
// el buffer de menores a pivote 1, pivote 1, el buffer de elementos entre pivote 1 y 2, pivote 2 y así sucesivamente
//retorna el array ordenado

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <filesystem>

using namespace std;

#define B_SIZE 4096                        // tamaño del bloque en bytes
#define M_SIZE (50 * 1024 * 1024)         // tamaño de memoria principal (50 MB)

int disk_access = 0;                       // contador de accesos al disco

// --- Funciones de I/O por bloque ----------------------------------------

// Lee un bloque de tamaño B_SIZE desde el archivo en el offset dado y lo almacena en el buffer
size_t readBlock(FILE* file, long blockOffset, vector<int64_t>& buffer) {
    size_t numElements = B_SIZE / sizeof(int64_t);  // Número de elementos en un bloque
    buffer.resize(numElements);
    fseek(file, blockOffset * B_SIZE, SEEK_SET);  // Mover el puntero del archivo
    size_t bytesRead = fread(buffer.data(), sizeof(int64_t), numElements, file);
    if (bytesRead > 0) disk_access++;  // Contar acceso a disco
    return bytesRead;
}

// Escribe un bloque completo (tamaño B_SIZE) al archivo en el offset dado
void writeBlock(FILE* file, long blockOffset, const vector<int64_t>& buffer) {
    fseek(file, blockOffset * B_SIZE, SEEK_SET);  // Mover el puntero del archivo
    fwrite(buffer.data(), sizeof(int64_t), buffer.size(), file);
    disk_access++;  // Contar acceso a disco
}

// Volcar (flush) el contenido del buffer a un archivo en modo append
void flushBufferToFile(const string& filename, vector<int64_t>& buffer) {
    if (buffer.empty()) return;
    FILE* outFile = fopen(filename.c_str(), "ab");
    fwrite(buffer.data(), sizeof(int64_t), buffer.size(), outFile);
    fclose(outFile);
    disk_access++;   // Contar acceso a disco
    buffer.clear();  // Limpiar buffer después de escribir
}

// --- Funciones del algoritmo Quicksort Externo ---------------------------

// Selecciona pivotes aleatorios dentro de un bloque
void selectPivots(vector<int64_t>& block, int numPivots) {
    srand(time(0));
    for (int i = 0; i < numPivots - 1; ++i) {
        int randIndex = rand() % block.size();
        swap(block[i], block[randIndex]);  // Intercambiar para agrupar los pivotes al inicio
    }
}

// Función principal de Quicksort Externo
// fileName: nombre del archivo con los datos a ordenar
// N_SIZE: número total de elementos en el archivo
// numPartitions: número de particiones que se crearán
void quicksort_externo(const string& fileName, long N_SIZE, int numPartitions) {
    // Caso base: Si los datos caben en la memoria principal, ordenar directamente en memoria
    if (N_SIZE * sizeof(int64_t) <= M_SIZE) {
        vector<int64_t> buffer(N_SIZE);
        FILE* file = fopen(fileName.c_str(), "rb+");
        size_t unused_bytes = fread(buffer.data(), sizeof(int64_t), N_SIZE, file);
        sort(buffer.begin(), buffer.end());
        fseek(file, 0, SEEK_SET);
        fwrite(buffer.data(), sizeof(int64_t), N_SIZE, file);
        fclose(file);
        disk_access++;  // Contar acceso a disco
        return;
    }

    // 1) Leer un bloque aleatorio para seleccionar pivotes
    FILE* file = fopen(fileName.c_str(), "rb");
    long blockCount = (N_SIZE * sizeof(int64_t) + B_SIZE - 1) / B_SIZE;
    long randBlock = rand() % blockCount;
    vector<int64_t> block;
    readBlock(file, randBlock, block);
    selectPivots(block, numPartitions);
    sort(block.begin(), block.begin() + (numPartitions - 1));  // Ordenar los pivotes
    fclose(file);

    // 2) Preparar los archivos temporales para las particiones
    vector<string> partitionFiles(numPartitions);
    vector<vector<int64_t>> partitionBuffers(numPartitions);

    // Eliminar archivos temporales si ya existían
    for (int i = 0; i < numPartitions; ++i) {
        partitionFiles[i] = fileName + ".part" + to_string(i);
        filesystem::remove(partitionFiles[i]);
    }

    // 3) Leer el archivo y particionar los elementos en función de los pivotes
    file = fopen(fileName.c_str(), "rb");
    vector<int64_t> currentBlock;
    size_t elemsPerBlock = B_SIZE / sizeof(int64_t);
    long totalReadElements = 0;
    long blockIndex = 0;

    while (totalReadElements < N_SIZE) {
        size_t numElementsRead = readBlock(file, blockIndex++, currentBlock);
        for (size_t i = 0; i < numElementsRead; ++i) {
            int64_t value = currentBlock[i];
            int partitionIndex = numPartitions - 1;
            for (int p = 0; p < numPartitions - 1; ++p) {
                if (value < block[p]) { partitionIndex = p; break; }
            }
            partitionBuffers[partitionIndex].push_back(value);

            // Si el buffer de la partición alcanzó el tamaño de bloque, volcarlo
            if (partitionBuffers[partitionIndex].size() == elemsPerBlock) {
                flushBufferToFile(partitionFiles[partitionIndex], partitionBuffers[partitionIndex]);
            }
        }
        totalReadElements += numElementsRead;
    }
    fclose(file);

    // Volcar cualquier contenido restante en los buffers a los archivos temporales
    for (int i = 0; i < numPartitions; ++i) {
        flushBufferToFile(partitionFiles[i], partitionBuffers[i]);
    }

    // 4) Recursivamente aplicar quicksort a cada partición
    for (int i = 0; i < numPartitions; ++i) {
        long partitionSize = filesystem::file_size(partitionFiles[i]) / sizeof(int64_t);
        if (partitionSize > 0) {
            quicksort_externo(partitionFiles[i], partitionSize, numPartitions);
        }
    }

    // 5) Fusionar las particiones de vuelta al archivo original
    file = fopen(fileName.c_str(), "wb");  // Abrir el archivo para reescribirlo
    vector<int64_t> mergeBuffer;
    for (int i = 0; i < numPartitions; ++i) {
        FILE* partitionFile = fopen(partitionFiles[i].c_str(), "rb");
        while (true) {
            vector<int64_t> tempBuffer(elemsPerBlock);
            size_t numElements = fread(tempBuffer.data(), sizeof(int64_t), elemsPerBlock, partitionFile);
            if (numElements == 0) break;
            fwrite(tempBuffer.data(), sizeof(int64_t), numElements, file);
            disk_access++;  // Contar acceso a disco
        }
        fclose(partitionFile);
        filesystem::remove(partitionFiles[i]);  // Eliminar archivo temporal

        // Insertar pivote si es necesario
        if (i < numPartitions - 1) {
            int64_t pivot = block[i];
            fwrite(&pivot, sizeof(int64_t), 1, file);
        }
    }
    fclose(file);
}

// --- Funciones auxiliares ------------------------------------------------

// Verifica si el archivo está ordenado
bool check_sorted(FILE* file, long input_size) {
    long numElements = input_size / sizeof(int64_t);
    if (numElements <= 0) {
        cerr << "El tamaño del archivo no es suficiente para contener al menos un entero." << endl;
        return false;
    }

    vector<int64_t> buffer(numElements);
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(buffer.data(), sizeof(int64_t), numElements, file);

    if (bytesRead * sizeof(int64_t) != input_size) {
        cerr << "Error leyendo el archivo." << endl;
        return false;
    }

    if (is_sorted(buffer.begin(), buffer.end())) {
        cout << "El archivo está ordenado." << endl;
        return true;
    } else {
        cout << "El archivo NO está ordenado." << endl;
        return false;
    }
}

// Imprime los primeros N elementos del archivo
void printFirstElements(FILE* file, long numElements) {
    vector<int64_t> buffer(numElements);
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(buffer.data(), sizeof(int64_t), numElements, file);

    if (bytesRead != numElements) {
        cerr << "Error al leer los primeros elementos del archivo." << endl;
        exit(1);
    }

    cout << "Primeros " << numElements << " elementos del archivo:" << endl;
    for (long i = 0; i < numElements; ++i) {
        cout << buffer[i] << " ";
    }
    cout << endl;

    if (is_sorted(buffer.begin(), buffer.end())) {
        cout << "Ordenado" << endl;
    } else {
        cout << "No ordenado" << endl;
    }
}

// --- Función principal ---------------------------------------------------

#include "sequence_generator.hpp"

int main(int argc, char* argv[]) {
    long N_SIZE = 4 * M_SIZE;       // número de elementos a generar
    int numPartitions = 50;         // número de particiones

    const string inputFile = "input.bin";

    // 1) Generar secuencia aleatoria
    generate_sequence(N_SIZE, inputFile, B_SIZE);

    // 2) Mostrar algunos elementos antes de ordenar
    {
        FILE* file = fopen(inputFile.c_str(), "rb");
        if (!file) {
            cerr << "No puedo abrir " << inputFile << endl;
            return 1;
        }
        printFirstElements(file, 10);
        fclose(file);
    }

    // 3) Ejecutar el Quicksort Externo
    quicksort_externo(inputFile, N_SIZE, numPartitions);

    // 4) Mostrar el número de accesos al disco
    cout << "Accesos al disco: " << disk_access << endl;

    // 5) Mostrar el resultado final
    {
        FILE* file = fopen(inputFile.c_str(), "rb");
        if (!file) {
            cerr << "No puedo abrir " << inputFile << " después de ordenar" << endl;
            return 1;
        }
        printFirstElements(file, 1000);
        cout << "¿Está ordenado? " << check_sorted(file, N_SIZE) << endl;
        fclose(file);
    }

    return 0;
}