//se define M_SIZE <- tamaño de la memoria principal: valor capturado desde los args
//se define B_SIZE <- tamaño del bloque: como tamaño real de bloque en un disco (512B en discos viejos, 4069B en discos modernos) 
//se define N_SIZE <- tamaño del array que contiene a los elementos a ordenar (input), recibido por quicksort
//se define a <- cantidad de particiones que se van a realizar en el algoritmo de quicksort, recibido por quicksort y no cambia

//variables globales
//M_SIZE recibida en el main
//B_SIZE recibida en el main

//quicksort args:
//a: cantidad de particiones a realizar
//N: array a ordenar <- también representa el tamaño del bloque a seleccionar cuando N_SIZE > M_SIZE

//quicksort version 1:
//si N_SIZE <= M_SIZE
// se ordena el bloque completo gratis en memoria principal con sort
//else N_SIZE > M_SIZE
// 1) Se selecciona un bloque aleatorio para ser ordenado, presuntamente N pero seguir pensando en B_A valor más chico que N_SIZE pero > M_SIZE y B
// porque sino se ordenaría en memoria principal gratis
// 2) Se seleccionan aleatoriamente a-1 pivotes dentro del bloque seleccionado en el paso anterior
// 3) en un buffer de tamaño B_SIZE se ordenan los elementos seleccionados en el paso anterior gratis (pensar más porque el tamaño de a permite que sea gratis, ie, a<M_SIZE)
// 4) En otros buffers (a-1 buffers) se almacenan los elementos menores, entre medio y mayores a los pivotes seleccionados en el paso 2 recorriendo N de forma lineal
// 5) Se llama recursivamente a quicksort para cada uno de los buffers generados en el paso 4
// 6) Finalmente se unen los buffers generados de acuerdo a los pivotes, ejemplo:
// el buffer de menores a pivote 1, pivote 1, el buffer de elementos entre pivote 1 y 2, pivote 2 y así sucesivamente

//quicksort args:
//a: cantidad de particiones a realizar
//N: array a ordenar

//quicksort version 2:
//si N_SIZE <= M_SIZE
// se ordena el bloque completo gratis en memoria principal con sort
//else N_SIZE > M_SIZE
// 1) Se selecciona un bloque de tamaño B_SIZE aleatorio
// 2) Se seleccionan aleatoriamente a-1 pivotes dentro del bloque seleccionado en el paso anterior
// 3) en un buffer ordeno gratis los pivotes seleccionados en el paso anterior (ahora si es gratis, en general a es menor que B_SIZE = 4096)
// 4) En otros buffers (a-1 buffers) se almacenan los elementos menores, entre medio y mayores a los pivotes seleccionados en el paso 2 leyendo el input N por bloques de tamaño B_SIZE
// 5) Se llama recursivamente a quicksort para cada uno de los buffers generados en el paso 4
// 6) Finalmente se unen los buffers generados de acuerdo a los pivotes, ejemplo:
// el buffer de menores a pivote 1, pivote 1, el buffer de elementos entre pivote 1 y 2, pivote 2 y así sucesivamente

//quicksort args:
//a: cantidad de particiones a realizar
//N: array a ordenar

//quicksort version 3:
//si N_SIZE <= M_SIZE
// se ordena el bloque completo gratis en memoria principal con sort
//else N_SIZE > M_SIZE
// 1) Se selecciona un bloque de tamaño B_SIZE aleatorio de nuestro input N de N_SIZE (ya que solo podemos leer B_SIZE bytes a la vez)
// 2) Se seleccionan aleatoriamente a-1 pivotes dentro del bloque seleccionado en el paso anterior
// 2.1) en un buffer ordeno gratis los pivotes seleccionados en el paso anterior (es gratis, ya que en general a es menor que B_SIZE = 4096)
// 3) Uso secuencialmente los pivotes ordenados para definir en que (a-1 buffers) se almacenan los elementos menores, entre medio y mayores a los pivotes seleccionados en el paso 2 leyendo el input N por bloques de tamaño B_SIZE
// 4) Se llama recursivamente a quicksort para cada uno de los buffers generados en el paso 3
// 5) Finalmente se unen los buffers generados de acuerdo a los pivotes, ejemplo:
// el buffer de menores a pivote 1, pivote 1, el buffer de elementos entre pivote 1 y 2, pivote 2 y así sucesivamente
//retorna el array ordenado

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm> // For std::sort

using namespace std;

#define B_SIZE 4096  // Tamaño del bloque de disco
#define M_SIZE (50 * 1024 * 1024)  // Memoria principal de 50 MB

// Función auxiliar para leer un bloque desde el archivo binario
void readBlock(FILE* file, long offset, vector<int>& buffer) {
    fseek(file, offset * B_SIZE, SEEK_SET);
    fread(buffer.data(), sizeof(int), B_SIZE / sizeof(int), file);
}

// Función auxiliar para escribir un bloque en el archivo binario
void writeBlock(FILE* file, long offset, const vector<int>& buffer) {
    fseek(file, offset * B_SIZE, SEEK_SET);
    fwrite(buffer.data(), sizeof(int), B_SIZE / sizeof(int), file);
}

// Función para obtener pivotes aleatorios dentro de un bloque
void selectPivots(vector<int>& block, int a) {
    srand(time(0));
    for (int i = 0; i < a - 1; ++i) {
        int randIndex = rand() % block.size();
        swap(block[i], block[randIndex]);
    }
}

// Función para realizar el quicksort externo
void quicksort(FILE* file, long N_SIZE, int a, long offset) {
    if (N_SIZE <= M_SIZE) {
        // Caso base: ordenar en memoria
        vector<int> block(N_SIZE);
        readBlock(file, offset, block);
        sort(block.begin(), block.end());
        writeBlock(file, offset, block);
        return;
    }

    // Leer un bloque aleatorio
    vector<int> block(B_SIZE / sizeof(int));
    readBlock(file, offset, block);

    // Seleccionar pivotes aleatorios
    selectPivots(block, a);

    // Ordenar pivotes en memoria
    sort(block.begin(), block.begin() + a - 1);

    // Particionar el arreglo en subarreglos basados en los pivotes
    vector<vector<int>> subarrays(a);
    for (int i = 0; i < block.size(); ++i) {
        int value = block[i];
        for (int j = 0; j < a - 1; ++j) {
            if (value < block[j]) {
                subarrays[j].push_back(value);
                break;
            }
        }
    }

    // Recursión: llamar a quicksort en cada subarreglo
    for (int i = 0; i < a; ++i) {
        if (!subarrays[i].empty()) {
            // Aquí se debería escribir cada subarreglo en su propio archivo temporal
            FILE* subFile = fopen("subarray.bin", "wb");
            fwrite(subarrays[i].data(), sizeof(int), subarrays[i].size(), subFile);
            fclose(subFile);
            quicksort(subFile, subarrays[i].size(), a, 0);
        }
    }

    // Finalmente, combinar los subarreglos
    vector<int> sortedArray;
    for (int i = 0; i < a; ++i) {
        sortedArray.insert(sortedArray.end(), subarrays[i].begin(), subarrays[i].end());
    }

    // Escribir los subarreglos combinados de vuelta al archivo
    writeBlock(file, offset, sortedArray);
}

int main(int argc, char* argv[]) {
    // Abrir archivo binario con el arreglo
    FILE* file = fopen("input.bin", "rb+");

    if (!file) {
        cerr << "Error al abrir el archivo" << endl;
        return 1;
    }

    long N_SIZE = 10000000; // Este valor debe ser determinado por el archivo de entrada
    int a = 10; // Número de particiones (se puede ajustar dependiendo de M y B)

    // Llamar al quicksort externo
    quicksort(file, N_SIZE, a, 0);

    fclose(file);
    return 0;
}
