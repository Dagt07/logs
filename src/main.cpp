#include "mergesort.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>

// Contador global para realizar seguimiento de las operaciones de I/O
size_t disk_access_count = 0;

// Función para generar un archivo con números aleatorios
void generateRandomFile(const char* filename, size_t numElements) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        std::cerr << "Error creating file: " << filename << std::endl;
        exit(1);
    }

    // Inicializar generador de números aleatorios
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> dist(
        std::numeric_limits<int64_t>::min(), 
        std::numeric_limits<int64_t>::max()
    );

    // Generar y escribir números en bloques
    const size_t blockSize = 1024 * 1024 * 4; // Tamaño del bloque en bytes
    std::vector<int64_t> buffer(blockSize);

    for (size_t i = 0; i < numElements; i += blockSize) {
        size_t elementsToWrite = std::min(blockSize, numElements - i);
        
        // Generar números aleatorios
        for (size_t j = 0; j < elementsToWrite; j++) {
            buffer[j] = dist(gen);
        }
        
        // Escribir bloque
        fwrite(buffer.data(), sizeof(int64_t), elementsToWrite, file);
    }

    fclose(file);
}

// Función para encontrar la mejor aridad para MergeSort usando búsqueda binaria
size_t findBestArityForMergeSort(const char* inputFileName) {
    std::cout << "Finding best arity for MergeSort..." << std::endl;
    
    // Calcular cuántos números de 64 bits caben en un bloque
    const size_t elementsPerBlock = B / sizeof(int64_t);
    
    // Definir rango de búsqueda para la aridad
    size_t min_a = 2;
    size_t max_a = elementsPerBlock;  // La aridad máxima es el número de elementos por bloque
    
    size_t best_a = 2;
    double best_time = std::numeric_limits<double>::max();
    
    // Búsqueda binaria para encontrar la mejor aridad
    while (min_a <= max_a) {
        size_t mid_a = min_a + (max_a - min_a) / 2;
        
        // Crear archivo temporal para la salida
        const char* outputFileName = "binary_search_output.bin";
        
        // Medir tiempo con esta aridad
        auto start = std::chrono::high_resolution_clock::now();
        disk_access_count = 0;  // Reiniciar contador de accesos a disco
        
        performExternalMergeSort(inputFileName, outputFileName, mid_a);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        
        std::cout << "Tested arity " << mid_a << ": " << diff.count() << " seconds, " 
                  << disk_access_count << " disk accesses" << std::endl;
        
        // Actualizar mejor aridad si es necesario
        if (diff.count() < best_time) {
            best_time = diff.count();
            best_a = mid_a;
            
            // Si estamos explorando la primera mitad
            if (mid_a < max_a) {
                max_a = mid_a - 1;
            } else {
                // Si estamos explorando la segunda mitad
                min_a = mid_a + 1;
            }
        } else {
            // Si el tiempo está empeorando
            if (mid_a > best_a) {
                max_a = mid_a - 1;
            } else {
                min_a = mid_a + 1;
            }
        }
        
        // Eliminar archivo temporal
        remove(outputFileName);
    }
    
    std::cout << "Best arity found: " << best_a << " with time: " << best_time << " seconds" << std::endl;
    return best_a;
}

// Función principal
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " [find-arity | run-tests]" << std::endl;
        return 1;
    }

    // Comando para encontrar la mejor aridad
    if (strcmp(argv[1], "find-arity") == 0) {
        // Generar archivo grande (60M elementos)
        const size_t largeSize = 60 * 1024 * 1024 / sizeof(int64_t);  // 60M/8 = 7.5M números de 64 bits
        const char* largefile = "large_test.bin";
        
        std::cout << "Generating large test file with " << largeSize << " elements..." << std::endl;
        generateRandomFile(largefile, largeSize);
        
        size_t best_arity = findBestArityForMergeSort(largefile);
        std::cout << "Optimal arity for MergeSort: " << best_arity << std::endl;
        
        // Limpiar archivos temporales
        remove(largefile);
        
        return 0;
    }
    
    // Comando para ejecutar las pruebas
    else if (strcmp(argv[1], "run-tests") == 0) {
        if (argc < 3) {
            std::cout << "Please specify arity for tests" << std::endl;
            return 1;
        }
        
        size_t arity = std::stoul(argv[2]);
        std::cout << "Running tests with arity: " << arity << std::endl;
        
        // Tamaños de prueba: 4M, 8M, ..., 60M (en bytes)
        const std::vector<size_t> testSizes = {
            4 * 1024 * 1024, 8 * 1024 * 1024, 12 * 1024 * 1024, 16 * 1024 * 1024, 
            20 * 1024 * 1024, 24 * 1024 * 1024, 28 * 1024 * 1024, 32 * 1024 * 1024,
            36 * 1024 * 1024, 40 * 1024 * 1024, 44 * 1024 * 1024, 48 * 1024 * 1024,
            52 * 1024 * 1024, 56 * 1024 * 1024, 60 * 1024 * 1024
        };
        
        // Resultados: [tamaño][algoritmo][repetición]
        std::vector<std::vector<std::vector<std::pair<double, size_t>>>> results(
            testSizes.size(),
            std::vector<std::vector<std::pair<double, size_t>>>(
                2,  // 2 algoritmos: MergeSort y QuickSort
                std::vector<std::pair<double, size_t>>(5)  // 5 repeticiones
            )
        );
        
        // Para cada tamaño de prueba
        for (size_t sizeIdx = 0; sizeIdx < testSizes.size(); sizeIdx++) {
            size_t testSize = testSizes[sizeIdx];
            size_t numElements = testSize / sizeof(int64_t);
            
            std::cout << "\nTesting size: " << testSize / (1024 * 1024) << "MB (" 
                      << numElements << " elements)" << std::endl;
            
            // Realizar 5 repeticiones para cada algoritmo
            for (int rep = 0; rep < 5; rep++) {
                std::cout << "  Repetition " << rep + 1 << ":" << std::endl;
                
                // Generar archivo de prueba
                std::string inputFile = "test_input_" + std::to_string(sizeIdx) + "_" + std::to_string(rep) + ".bin";
                std::string mergeOutput = "merge_output_" + std::to_string(sizeIdx) + "_" + std::to_string(rep) + ".bin";
                std::string quickOutput = "quick_output_" + std::to_string(sizeIdx) + "_" + std::to_string(rep) + ".bin";
                
                generateRandomFile(inputFile.c_str(), numElements);
                
                // Test MergeSort
                std::cout << "    Testing MergeSort... " << std::flush;
                auto mergeStart = std::chrono::high_resolution_clock::now();
                disk_access_count = 0;
                
                performExternalMergeSort(inputFile.c_str(), mergeOutput.c_str(), arity);
                
                auto mergeEnd = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> mergeDiff = mergeEnd - mergeStart;
                
                bool mergeSorted = checkSorted(mergeOutput);
                std::cout << (mergeSorted ? "OK" : "FAILED") << " - Time: " << mergeDiff.count() 
                          << "s, Disk accesses: " << disk_access_count << std::endl;
                
                results[sizeIdx][0][rep] = {mergeDiff.count(), disk_access_count};
                
                // Test QuickSort (asumimos que ya está implementado en otro archivo)
                // Aquí deberías incluir la llamada a tu implementación de QuickSort externo
                /* Ejemplo:
                std::cout << "    Testing QuickSort... " << std::flush;
                auto quickStart = std::chrono::high_resolution_clock::now();
                disk_access_count = 0;
                
                performExternalQuickSort(inputFile.c_str(), quickOutput.c_str(), arity - 1);  // a-1 pivotes
                
                auto quickEnd = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> quickDiff = quickEnd - quickStart;
                
                bool quickSorted = checkSorted(quickOutput);
                std::cout << (quickSorted ? "OK" : "FAILED") << " - Time: " << quickDiff.count() 
                          << "s, Disk accesses: " << disk_access_count << std::endl;
                
                results[sizeIdx][1][rep] = {quickDiff.count(), disk_access_count};
                */
                
                // Limpiar archivos temporales
                remove(inputFile.c_str());
                remove(mergeOutput.c_str());
                remove(quickOutput.c_str());
            }
        }
        
        // Calcular y mostrar promedios
        std::cout << "\n--- RESULTS SUMMARY ---" << std::endl;
        std::cout << "Size(MB),MergeSort_Time(s),MergeSort_IO,QuickSort_Time(s),QuickSort_IO" << std::endl;
        
        for (size_t sizeIdx = 0; sizeIdx < testSizes.size(); sizeIdx++) {
            double avgMergeTime = 0, avgQuickTime = 0;
            size_t avgMergeIO = 0, avgQuickIO = 0;
            
            // Calcular promedio para MergeSort
            for (int rep = 0; rep < 5; rep++) {
                avgMergeTime += results[sizeIdx][0][rep].first;
                avgMergeIO += results[sizeIdx][0][rep].second;
                
                // Descomentar cuando se implemente QuickSort
                /*
                avgQuickTime += results[sizeIdx][1][rep].first;
                avgQuickIO += results[sizeIdx][1][rep].second;
                */
            }
            
            avgMergeTime /= 5;
            avgMergeIO /= 5;
            avgQuickTime /= 5;
            avgQuickIO /= 5;
            
            std::cout << testSizes[sizeIdx] / (1024 * 1024) << ","
                      << avgMergeTime << ","
                      << avgMergeIO << ","
                      << avgQuickTime << ","
                      << avgQuickIO << std::endl;
        }
        
        return 0;
    }
    
    else {
        std::cout << "Unknown command: " << argv[1] << std::endl;
        return 1;
    }
}