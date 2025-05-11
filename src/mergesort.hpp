#ifndef MERGESORT_H
#define MERGESORT_H
#include <vector>
#include <cstdio>
#include <cstdint> // For int64_t
#include <iostream>

#define M 32 // Tamaño de M (50MB)*
#define B 4096   // Tamaño de bloque B (1MB)*

// Función para leer un bloque de tamaño B desde el archivo
bool readBlock(FILE* file, std::vector<int64_t>& buffer);
// Función para escribir un bloque al archivo
void writeBlock(FILE* file, const std::vector<int64_t>& buffer);
// Función de fusión de varios bloques ordenados
void mergeBlocks(FILE* output, std::vector<FILE*>& inputFiles);
// Función para realizar el MergeSort Externo con aridad a
void externalMergeSort(FILE* inputFile, FILE* outputFile, size_t start, size_t end, size_t a);
// Función principal de MergeSort Externo
void performExternalMergeSort(const char* inputFileName, const char* outputFileName, size_t a);
// Función para verificar si un archivo está ordenado
bool checkSorted(const std::string &filename);
#endif