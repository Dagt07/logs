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
#include "sequence_generator.hpp"
#include <filesystem>

using namespace std;

#define B_SIZE 4096  // Tamaño del bloque de disco
#define M_SIZE (50 * 1024 * 1024) // Memoria principal de 50 MB

int disk_access = 0; // Contador de accesos al disco

// Función auxiliar para leer un bloque desde el archivo binario
size_t readBlock(FILE* file, long offset, vector<int>& buffer) {
    size_t elementos = B_SIZE / sizeof(int);  // Calcular cuántos elementos caben en el bloque
    buffer.resize(elementos);  // Redimensionar el buffer
    fseek(file, offset * B_SIZE, SEEK_SET);
    size_t bytesRead = fread(buffer.data(), sizeof(int), elementos, file);
    
    if (bytesRead > 0){
        disk_access ++;
    }

    return bytesRead;
}


// Función auxiliar para escribir un bloque en el archivo binario
void writeBlock(FILE* file, long offset, const vector<int>& buffer) {
    fseek(file, offset * B_SIZE, SEEK_SET);
    fwrite(buffer.data(), sizeof(int), B_SIZE / sizeof(int), file);
    disk_access ++;
}

// Función para obtener pivotes aleatorios dentro de un bloque
void selectPivots(vector<int>& block, int a) {
    srand(time(0));
    for (int i = 0; i < a - 1; ++i) {
        int randIndex = rand() % block.size();
        //hace swap para que aleatoriamente los pivotes seleccionados queden juntos al principio del bloque
        //ya que posteriormente necesitamos manipular los pivotes para que queden ordenados, 
        //al ser a<B_SIZE basicamente dejamos todos los pivotes juntos al principio del bloque
        //y el resto de los elementos quedan al final
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

    // Leer un bloque de tamaño B_SIZE
    vector<int> block(B_SIZE / sizeof(int));
    readBlock(file, offset, block);

    // Seleccionar pivotes aleatorios y dejarlos al principio del bloque
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
    long current_offset = 0;  // Mantener un offset de lectura para los subarreglos
    for (int i = 0; i < a; ++i) {
        if (!subarrays[i].empty()) {
            // En lugar de escribir y reabrir archivos, almacenamos los subarreglos en memoria
            FILE* subFile = fopen("subarray_temp.bin", "wb");
            fwrite(subarrays[i].data(), sizeof(int), subarrays[i].size(), subFile);
            fclose(subFile);

            // Llamar recursivamente al quicksort con el nuevo subarreglo
            quicksort(subFile, subarrays[i].size(), a, current_offset);
            current_offset++; // Actualizamos el offset para los subarreglos
        }
    }

    // Finalmente, combinar los subarreglos en el bloque original y escribir al archivo
    vector<int> sortedArray;
    for (int i = 0; i < a; ++i) {
        sortedArray.insert(sortedArray.end(), subarrays[i].begin(), subarrays[i].end());
    }

    // Escribir los subarreglos combinados de vuelta al archivo
    writeBlock(file, offset, sortedArray);
}

bool check_sorted(FILE* file, long input_size) {
    // Calculamos cuántos enteros caben en input_size bytes
    long num_elements = input_size / sizeof(int);
    
    if (num_elements <= 0) {
        cerr << "El tamaño del archivo no es suficiente para contener al menos un entero." << endl;
        return false;
    }

    vector<int> v(num_elements);
    
    // Nos aseguramos de empezar desde el inicio del archivo
    fseek(file, 0, SEEK_SET);
    
    // Leemos la cantidad de bytes especificada
    size_t bytesRead = fread(v.data(), sizeof(int), num_elements, file);
    
    /*
    // Verificamos si se leyeron la cantidad correcta de bytes
    cout << "Esperado leer: " << input_size << " bytes." << endl;
    cout << "Leídos: " << bytesRead * sizeof(int) << " bytes." << endl;
    */

    
    if (bytesRead * sizeof(int) != input_size) {
        cerr << "Error leyendo el archivo. Se esperaban " << input_size << " bytes, pero solo se leyeron " << bytesRead * sizeof(int) << " bytes." << endl;
        return false;  // Si no se leen todos los datos correctamente, devolvemos false
    }

    // Verificamos si está ordenado
    if (is_sorted(v.begin(), v.end())) {
        cout << "El archivo está ordenado." << endl;
        return true;
    } else {
        cout << "El archivo NO está ordenado." << endl;
        return false;
    }
}


// Función para imprimir los primeros N elementos de un archivo binario
void printFirstElements(FILE* file, long elements) {//, long input_size) {
    vector<int> buffer(elements);
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(buffer.data(), sizeof(int), elements, file);
    
    if (bytesRead != elements) {
        cerr << "Error al leer los primeros elementos del archivo." << endl;
        exit(1);
    }

    cout << "Primeros " << elements << " elementos del archivo:" << endl;
    for (long i = 0; i < elements; ++i) {
        cout << buffer[i] << " ";
    }

    cout << endl;
      // Checking if vector v is sorted or not
    if (is_sorted(buffer.begin(), buffer.end()))
      cout << "Sorted";
    else
      cout << "Not Sorted";
    cout << endl;
    
    /*
    vector<int> totalbuffer(input_size / sizeof(int));
    fseek(file, 0, SEEK_SET);
    size_t totalbytesRead = fread(totalbuffer.data(), sizeof(int), input_size / sizeof(int), file);
    
    // Checking if vector v is sorted or not
    if (is_sorted(totalbuffer.begin(), totalbuffer.end()))
        cout << "Sorted";
    else
        cout << "Not Sorted";
    cout << endl;
    */
}

int main(int argc, char* argv[]) {

    long N_SIZE = 100000; // Este valor debe ser determinado por el archivo de entrada
    int a = 12; // Número de particiones (se puede ajustar dependiendo de M y B)
    
    // Generar la secuencia aleatoria y guardarla en "input.bin"
    generate_sequence(N_SIZE, "input.bin", B_SIZE);

    // Abrir archivo binario con el arreglo
    FILE* file = fopen("input.bin", "rb+"); // Abre el archivo en modo lectura/escritura binaria

    if (!file) {
        cerr << "Error al abrir el archivo" << endl;
        return 1;
    }

    // Imprimir los primeros X elementos antes de ordenar
    printFirstElements(file, 10); //, N_SIZE); 
    
    //cout << "Sorted? " << check_sorted(file, N_SIZE) << endl;

    // Llamar al quicksort externo
    quicksort(file, N_SIZE, a, 0);

    cout << "Accesos al disco: " << disk_access << endl;

    // Imprimir los primeros X elementos después de ordenar
    fseek(file, 0, SEEK_SET);  // Volver al principio del archivo para leer de nuevo
    printFirstElements(file, 2500); //, N_SIZE);
    //cout << "Sorted? " << check_sorted(file, N_SIZE) << endl;

    filesystem::remove("subarray_temp.bin"); // Eliminar el archivo temporal

    fclose(file);
    return 0;
}
