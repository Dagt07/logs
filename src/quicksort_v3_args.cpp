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

// --------------------------------- Variables globales ---------------------------------
long B_SIZE; // tamaño del bloque en bytes
long M_SIZE; //tamaño de memoria principal (50 MB)
long long disk_access = 0; // contador de accesos al disco

// --------------------------------- Funciones de I/O por bloque ---------------------------------


size_t readBlock(FILE* file, long blockOffset, vector<int64_t>& buffer, bool first_seek) {
    /* Lee un bloque de tamaño B_SIZE desde el archivo en el offset dado y lo almacena en el buffer 
    args:
        file: puntero al archivo desde el cual se leerá
        blockOffset: offset del bloque a leer
        buffer: vector donde se almacenarán los datos leídos
        first_seek: indica si es la primera lectura (para contar accesos a disco)
    returns:
        bytesRead: número de bytes leídos // en verdad no se usan luego esos bytes pero el compilador se quejaba
    */

    size_t numElements = B_SIZE / sizeof(int64_t);  // Número de elementos en un bloque
    buffer.resize(numElements);
    fseek(file, blockOffset * B_SIZE, SEEK_SET);  // Mover el puntero del archivo
    size_t bytesRead = fread(buffer.data(), sizeof(int64_t), numElements, file);
    if (bytesRead > 0 and first_seek){
        disk_access++;  // Contar acceso a disco
    }

    return bytesRead;
}


void flushBufferToFile(const string& filename, vector<int64_t>& buffer) {
    /* Volcar (flush) el contenido del buffer a un archivo en modo append 
    args:
        filename: nombre del archivo donde se volcará el buffer
        buffer: vector que contiene los datos a escribir
    returns:
        void
    */

    if (buffer.empty()) return;
    FILE* outFile = fopen(filename.c_str(), "ab");
    fwrite(buffer.data(), sizeof(int64_t), buffer.size(), outFile);
    fclose(outFile);
    buffer.clear();  // Limpiar buffer después de escribir
}


// --------------------------------- Funciones del algoritmo Quicksort Externo ---------------------------------

void selectPivots(vector<int64_t>& block, int numPivots) {
    /* Selecciona pivotes aleatorios dentro de un bloque y los coloca al inicio de este 
    args:
        block: bloque de datos del que se seleccionarán los pivotes
        numPivots: número de pivotes a seleccionar
    returns: 
        void    
    */

    // Check if block is empty or too small
    if (block.empty() || block.size() < numPivots - 1) {
        cerr << "Error: Cannot select pivots from empty or too small block" << endl;
        return;
    }

    srand(time(0));
    for (int i = 0; i < numPivots - 1; ++i) {
        int randIndex = rand() % block.size();
        swap(block[i], block[randIndex]);  // Intercambiar para agrupar los pivotes al inicio
    }
}


void externalQuicksort(const string& fileName, long N_SIZE, int a, bool first_run) {
    /* Función principal de Quicksort Externo de acuerdo al algoritmo descrito
    args:
        fileName: nombre del archivo con los datos a ordenar
        N_SIZE: número total de elementos en el archivo
        a: número de particiones que se crearán
        firts_run: indica si es la primera ejecución (para contar accesos a disco)
    returns:
        void
    */

    if (first_run) {
        cout << "Tamaño del archivo: " << N_SIZE << " bytes" << endl;
    }

    // Caso base: Si los datos caben en la memoria principal, ordenar directamente en memoria
    if (N_SIZE * sizeof(int64_t) <= M_SIZE) {
        vector<int64_t> buffer(N_SIZE);
        
        // Leer el archivo completo en memoria y ordenarlo gratis
        FILE* file = fopen(fileName.c_str(), "rb+");
        size_t _ = fread(buffer.data(), sizeof(int64_t), N_SIZE, file);
        sort(buffer.begin(), buffer.end());

        // Escribir el archivo ordenado de vuelta
        fseek(file, 0, SEEK_SET);
        fwrite(buffer.data(), sizeof(int64_t), N_SIZE, file);
        fclose(file);
        return;
    }

    // 1) Leer un bloque aleatorio para seleccionar pivotes
    FILE* file = fopen(fileName.c_str(), "rb");
    long blockCount = (N_SIZE * sizeof(int64_t) + B_SIZE - 1) / B_SIZE;

    // Leer un bloque aleatorio
    vector<int64_t> block;
    long randBlock = rand() % blockCount;
    size_t bytesRead = readBlock(file, randBlock, block, first_run);

    if (bytesRead == 0 || block.empty()) {
        cerr << "Error: Failed to read block or block is empty" << endl;
        fclose(file);
        return;
    }
    
    // 2) Seleccionar pivotes aleatorios dentro del bloque y ordenarlos
    selectPivots(block, a);
    sort(block.begin(), block.begin() + (a - 1));  // esto es gratis ya que a<B_SIZE
    
    fclose(file);

    // Declarar los arreglos a usar como archivos temporales para las particiones
    vector<string> partitionFiles(a);
    vector<vector<int64_t>> partitionBuffers(a);

    // Caso borde -> Eliminar archivos temporales si ya existían
    for (int i = 0; i < a; ++i) {
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
        // Debemos leer el archivo en bloques de tamaño B_SIZE para ordenar los elementos de acuerdo a los pivotes en sub arreglos
        // De lo contrario, si leemos solo el bloque seleccionado, no tendremos en cuenta el resto de los elementos
        size_t numElementsRead = readBlock(file, blockIndex++, currentBlock, first_run);

        // Distribuir los elementos en las particiones correspondientes
        for (size_t i = 0; i < numElementsRead; ++i) {
            int64_t value = currentBlock[i];
            int partitionIndex = a - 1;
            for (int p = 0; p < a - 1; ++p) {
                if (value < block[p]) { partitionIndex = p; break; }
            }
            partitionBuffers[partitionIndex].push_back(value); // agregar el elemento al buffer de la partición correspondiente

            // Si el buffer de la partición alcanzó el tamaño de bloque, volcarlo
            if (partitionBuffers[partitionIndex].size() == elemsPerBlock) {
                flushBufferToFile(partitionFiles[partitionIndex], partitionBuffers[partitionIndex]);
            }
        }
        totalReadElements += numElementsRead;
    }
    fclose(file);

    // Volcar cualquier contenido restante en los buffers a los archivos temporales
    for (int i = 0; i < a; ++i) {
        flushBufferToFile(partitionFiles[i], partitionBuffers[i]);
    }

    // 4) Recursivamente aplicar quicksort a cada partición
    for (int i = 0; i < a; ++i) {
        long partitionSize = filesystem::file_size(partitionFiles[i]) / sizeof(int64_t);
        if (partitionSize > 0) {
            externalQuicksort(partitionFiles[i], partitionSize, a, false); // first_run = false porque ya no queremos contar accesos a disco
        }
    }

    // 5) Unir las particiones de vuelta al archivo original
    file = fopen(fileName.c_str(), "wb");  // Abrir el archivo para reescribirlo
    vector<int64_t> mergeBuffer;
    for (int i = 0; i < a; ++i) {
        FILE* partitionFile = fopen(partitionFiles[i].c_str(), "rb");
        while (true) {
            vector<int64_t> tempBuffer(elemsPerBlock);
            size_t numElements = fread(tempBuffer.data(), sizeof(int64_t), elemsPerBlock, partitionFile);
            if (numElements == 0) break;
            fwrite(tempBuffer.data(), sizeof(int64_t), numElements, file);
            if (first_run)++ disk_access;  // Contar acceso a disco solo en first_run
        }
        fclose(partitionFile);

        // Eliminar archivo temporal
        filesystem::remove(partitionFiles[i]);  

        // Insertar pivote si es necesario
        if (i < a - 1) {
            int64_t pivot = block[i];
            fwrite(&pivot, sizeof(int64_t), 1, file);
        }
    }
    fclose(file);
}

// --------------------------------- Funciones auxiliares ---------------------------------


void printFirstElements(FILE* file, long numElements) {
    /* Imprime los primeros N elementos del archivo
    args:
        file: puntero al archivo que se va a imprimir
        numElements: número de elementos a imprimir
    returns:
        void
    */

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
        cout << "✅ Ordenado" << endl;
    } else {
        cout << "❌ No ordenado" << endl;
    }
}

// --------------------------------- Función para runnear en main.cpp ---------------------------------

#include "sequence_generator.hpp"

extern void externalQuicksort(const std::string& inputFile, long N_SIZE, int a, bool first_run);

long long run_quicksort(const std::string& inputFile, long N_SIZE, int a, long B_SIZE_arg, long M_SIZE_arg) {
    /* Función principal para ejecutar el Quicksort Externo
    args:
        inputFile: nombre del archivo de entrada
        N_SIZE: número total de elementos en el archivo
        a: número de particiones que se crearán
        B_SIZE_arg: tamaño del bloque en bytes
        M_SIZE_arg: tamaño de la memoria principal en bytes
    returns:
        disk_access: número de accesos al disco
    */

    // Variables globales
    B_SIZE = B_SIZE_arg; // tamaño del bloque (estandar 4096 bytes)
    M_SIZE = M_SIZE_arg; // tamaño de la memoria principal (50 MB)

    /*
    // Show some elements before sorting
    {
        FILE* file = fopen(inputFile.c_str(), "rb");
        if (!file) {
            cerr << "No puedo abrir " << inputFile << endl;
            return -1;
        }
        printFirstElements(file, 10);
        fclose(file);
    }
    */

    // Execute the external quicksort
    externalQuicksort(inputFile, N_SIZE, a, true);

    /*
    // Optionally, print the results after sorting (if needed)
    {
        FILE* file = fopen(inputFile.c_str(), "rb");
        if (!file) {
            cerr << "No puedo abrir " << inputFile << " después de ordenar" << endl;
            return -1;
        }
        printFirstElements(file, 25);
        fclose(file);
    }
    */
    return disk_access;
}
