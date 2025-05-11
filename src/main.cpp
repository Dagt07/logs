#include <iostream>
#include <cstdlib>
#include <ctime>
#include "../include/mergesort.hpp" 

using namespace std;

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

    cout << "El ordenamiento ha sido completado con éxito." << endl;

    return 0;
}