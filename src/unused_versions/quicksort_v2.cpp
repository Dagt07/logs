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

#define B_SIZE 4096                        // bytes por bloque
#define M_SIZE (50 * 1024 * 1024)         // 50 MB

int disk_access = 0;                       // contador de accesos disco

// --- helpers para I/O por bloque ----------------------------------------

// lee exactamente un bloque en buffer; offset en bloques
size_t readBlock(FILE* f, long blockOffset, vector<int64_t>& buffer) {
    size_t elems = B_SIZE / sizeof(int64_t);
    buffer.resize(elems);
    fseek(f, blockOffset * B_SIZE, SEEK_SET);
    size_t n = fread(buffer.data(), sizeof(int64_t), elems, f);
    if (n>0) disk_access++;
    return n;
}

// escribe buffer completo (== B_SIZE) en offset en bloques
void writeBlock(FILE* f, long blockOffset, const vector<int64_t>& buffer) {
    fseek(f, blockOffset * B_SIZE, SEEK_SET);
    fwrite(buffer.data(), sizeof(int64_t), buffer.size(), f);
    disk_access++;
}

// Función para obtener pivotes aleatorios dentro de un bloque
void selectPivots(vector<int64_t>& block, int a) {
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

// vuelca (flush) un buffer parcial a disco en modo append
void flushBufferToFile(const string& filename, vector<int64_t>& buf) {
    if (buf.empty()) return;
    FILE* out = fopen(filename.c_str(), "ab");
    fwrite(buf.data(), sizeof(int64_t), buf.size(), out);
    fclose(out);
    disk_access++;   // contamos como I/O
    buf.clear();
}

// --- función recursiva de Quicksort Externo -----------------------------

// fileName: archivo con N elementos; N_SIZE en #elementos; a = aridad
void quicksort_externo(const string& fileName, long N_SIZE, int a) {
    // caso base: cabe en M_SIZE → sort in-memory
    if (N_SIZE * sizeof(int64_t) <= M_SIZE) {
        vector<int64_t> mem(N_SIZE);
        FILE* f = fopen(fileName.c_str(), "rb+");
        size_t unused_bytes = fread(mem.data(), sizeof(int64_t), N_SIZE, f);
        sort(mem.begin(), mem.end());
        fseek(f, 0, SEEK_SET);
        fwrite(mem.data(), sizeof(int64_t), N_SIZE, f);
        fclose(f);
        disk_access++;
        return;
    }

    // 1) elegir pivotes leyendo un bloque al azar
    FILE* f = fopen(fileName.c_str(), "rb");
    long blockCount = (N_SIZE * sizeof(int64_t) + B_SIZE - 1) / B_SIZE;
    srand(time(0));
    long randBlock = rand() % blockCount;
    vector<int64_t> sample;
    readBlock(f, randBlock, sample);
    selectPivots(sample, a);
    sort(sample.begin(), sample.begin() + (a-1));
    fclose(f);

    // 2) preparar archivos y buffers por partición
    vector<string> partFiles(a);
    vector<vector<int64_t>> buffers(a);
    for (int i = 0; i < a; ++i) {
        partFiles[i] = fileName + ".part" + to_string(i);
        // limpiar si existían de previas ejecuciones
        filesystem::remove(partFiles[i]);
    }

    // 3) recorrer todo A por bloques y particionar a buffers
    f = fopen(fileName.c_str(), "rb");
    vector<int64_t> block;
    size_t elemsPorBloque = B_SIZE / sizeof(int64_t);
    long totalReadElems = 0;
    long blockIdx = 0;
    while (totalReadElems < N_SIZE) {
        size_t n = readBlock(f, blockIdx++, block);
        for (size_t i = 0; i < n; ++i) {
            int64_t v = block[i];
            // encontrar partición j
            int j = a-1;
            for (int p = 0; p < a-1; ++p) {
                if (v < sample[p]) { j = p; break; }
            }
            buffers[j].push_back(v);
            // si buffer llegó a bloque, lo vuelca
            if (buffers[j].size() == elemsPorBloque) {
                flushBufferToFile(partFiles[j], buffers[j]);
            }
        }
        totalReadElems += n;
    }
    fclose(f);
    // flush final de buffers restantes
    for (int i = 0; i < a; ++i) {
        flushBufferToFile(partFiles[i], buffers[i]);
    }

    // 4) recursión sobre cada partición
    for (int i = 0; i < a; ++i) {
        // contar cuántos elementos hay en el archivo .parti
        long partBytes = filesystem::file_size(partFiles[i]);
        long partElems = partBytes / sizeof(int64_t);
        if (partElems > 0) {
            quicksort_externo(partFiles[i], partElems, a);
        }
    }

    // 5) concatenar de vuelta a fileName
    f = fopen(fileName.c_str(), "wb");  // vaciar
    vector<int64_t> mergeBuf;
    // leer secuencialmente cada parte y escribir en bloques
    for (int i = 0; i < a; ++i) {
        FILE* pf = fopen(partFiles[i].c_str(), "rb");
        while (true) {
            vector<int64_t> tmp(elemsPorBloque);
            size_t r = fread(tmp.data(), sizeof(int64_t), elemsPorBloque, pf);
            if (r == 0) break;
            fwrite(tmp.data(), sizeof(int64_t), r, f);
            disk_access++;
        }
        fclose(pf);
        filesystem::remove(partFiles[i]);
        // si es partición intermedia, insertar pivote ordenado
        if (i < a-1) {
            int64_t piv = sample[i];
            fwrite(&piv, sizeof(int64_t), 1, f);
        }
    }
    fclose(f);
}



bool check_sorted(FILE* file, long input_size) {
    // Calculamos cuántos enteros caben en input_size bytes
    long num_elements = input_size / sizeof(int64_t);
    
    if (num_elements <= 0) {
        cerr << "El tamaño del archivo no es suficiente para contener al menos un entero." << endl;
        return false;
    }

    vector<int64_t> v(num_elements);
    
    // Nos aseguramos de empezar desde el inicio del archivo
    fseek(file, 0, SEEK_SET);
    
    // Leemos la cantidad de bytes especificada
    size_t bytesRead = fread(v.data(), sizeof(int64_t), num_elements, file);
    
    /*
    // Verificamos si se leyeron la cantidad correcta de bytes
    cout << "Esperado leer: " << input_size << " bytes." << endl;
    cout << "Leídos: " << bytesRead * sizeof(int64_t) << " bytes." << endl;
    */

    
    if (bytesRead * sizeof(int64_t) != input_size) {
        cerr << "Error leyendo el archivo. Se esperaban " << input_size << " bytes, pero solo se leyeron " << bytesRead * sizeof(int64_t) << " bytes." << endl;
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
    vector<int64_t> buffer(elements);
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(buffer.data(), sizeof(int64_t), elements, file);
    
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
    
}

#include "sequence_generator.hpp"

int main(int argc, char* argv[]) {
    // 1) Parámetros de entrada
    long N_SIZE = 4 * M_SIZE;       // número de elementos a generar / ordenar
    int a      = 50;            // número de particiones

    const string inputFile = "input.bin";

    // 2) Generar la secuencia aleatoria (solo la primera vez)
    generate_sequence(N_SIZE, inputFile, B_SIZE);

    // 3) Mostrar un preview de los primeros elementos
    {
        FILE* f = fopen(inputFile.c_str(), "rb");
        if (!f) { 
            cerr << "No puedo abrir " << inputFile << endl; 
            return 1; 
        }
        printFirstElements(f, 10);
        fclose(f);
    }

    // 4) Ejecutar el quicksort externo
    quicksort_externo(inputFile, N_SIZE, a);

    // 5) Mostrar número de accesos a disco
    extern int disk_access;
    cout << "Accesos al disco: " << disk_access << endl;

    

    // 6) Mostrar primer bloque ordenado
    {
        FILE* f = fopen(inputFile.c_str(), "rb");
        if (!f) {
            cerr << "No puedo abrir " << inputFile << " tras ordenar\n";
            return 1;
        }
        printFirstElements(f, 1000);
        cout << "Sorted? " << check_sorted(f, N_SIZE) << endl;
        fclose(f);
    }

    return 0;
}

