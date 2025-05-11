//main args: 
//argv[1] <- M_SIZE
//argv[2] <- B_SIZE quiza lo podemos dejar fijo en 4096 

//se define M_SIZE <- tamaño de la memoria principal: valor capturado desde los args
//se define B_SIZE <- tamaño del bloque: como tamaño real de bloque en un disco (512B en discos viejos, 4069B en discos modernos)
//se define N_SIZE <- tamaño del array que contiene a los elementos a ordenar (input)
//se define a <- valor que se debe determinar acá y define: 1) aridad del mergesort 2) Número de particiones que se van a realizar en el algoritmo de quicksort

//funcion que determina a
//experimentalmente se busca el a optimo usando mergesort, luego de tenerlo ya queda fijo y se usa de ahí en adelante
//por ello no forma parte del main

//main:
    
    //for(int i = 0; i < 15; i++) {
        //1) llamar a la función que genera los 5 archivos para un único tamaño N <- haciendo todo para no ocupar tanto espacio en disco

        //2) llamado a mergesort sobre cada uno de los 5 archivos de tamaño N. 

        //3) llamado a quicksort sobre cada uno de los 5 archivos de tamaño N. 

        //4) promediar los resultados para ese N

        //5) guardar los resultados en un archivo de salida (csv o txt)

        //6) liberar memoria

    //y más cosas a implementar...

#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include "sequence_generator.hpp"
#include "../headers/quicksort.hpp" 

using namespace std;

//vector de 15 tamaños N
vector<int> v = {4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};

struct AlgorithmResults {
    /* Estructura para almacenar los resultados de los algoritmos, tiempo y accesos a disco respectivamente
    campos:
        merge_time_ms: tiempo de ejecución de mergesort en milisegundos
        merge_disk_access: accesos a disco de mergesort
        quick_time_ms: tiempo de ejecución de quicksort en milisegundos
        quick_disk_access: accesos a disco de quicksort
    returns:
        struct // <- se almacena en los campos de la estructura
    */
    long long merge_time_ms;
    long long merge_disk_access;
    long long quick_time_ms;
    long long quick_disk_access;
};


AlgorithmResults process_sequence(const std::string& filename, long N_SIZE, int a, long B_SIZE, long M_SIZE) {
    AlgorithmResults results = {0, 0, 0, 0};
    /*
    Función para procesar la secuencia aleatoria generada (del tamaño definido como múltiplo de M), 
    ejecutando los algoritmos de ordenamiento y midiendo el tiempo y accesos a disco
    args:
        filename: nombre del archivo con la secuencia a ordenar
        N_SIZE: número total de bytes en el archivo
        a: número de particiones que se crearán
        B_SIZE: tamaño del bloque en bytes
        M_SIZE: tamaño de la memoria principal en bytes
    returns:
        results: estructura AlgorithmResults con los resultados de los algoritmos
    */
    
    // --------------------------- MERGESORT ---------------------------
    // auto merge_start_time = std::chrono::high_resolution_clock::now();
    // int merge_sort_disk_access = run_mergesort(filename, N_SIZE, a, B_SIZE, M_SIZE);
    // auto merge_end_time = std::chrono::high_resolution_clock::now();
    // auto merge_duration = std::chrono::duration_cast<std::chrono::milliseconds>(merge_end_time - merge_start_time);
    // results.merge_time_ms = merge_duration.count();
    // results.merge_disk_access = merge_sort_disk_access;
    // cout << "MergeSort completado en " << results.merge_time_ms << " ms, con " 
    //      << results.merge_disk_access << " accesos a disco" << endl;
    

    // --------------------------- QUICKSORT ---------------------------
    // Start measuring time for quicksort
    auto quick_start_time = std::chrono::high_resolution_clock::now();
    
    // Here we call run_quicksort and pass the necessary arguments
    int quick_sort_disk_access = run_quicksort(filename, N_SIZE, a, B_SIZE, M_SIZE);
    
    // Stop measuring time
    auto quick_end_time = std::chrono::high_resolution_clock::now();
    
    // Calculate duration in milliseconds
    auto quick_duration = std::chrono::duration_cast<std::chrono::milliseconds>(quick_end_time - quick_start_time);
    
    // Store quicksort results
    results.quick_time_ms = quick_duration.count();
    results.quick_disk_access = quick_sort_disk_access;
    
    // Print results
    cout << "QuickSort completado en " << results.quick_time_ms << " ms, con " 
         << results.quick_disk_access << " accesos a disco" << endl;
         
    return results;
}

int main(int argc, char* argv[]){
    if (argc != 4) {
        cerr << "Uso: " << argv[0]
                  << " <M_SIZE MB> <B_SIZE bytes> <a particiones>\n";
        return EXIT_FAILURE;
    }

    const int64_t M_BYTES = stol(argv[1]) * 1024L * 1024L; // Tamaño de la memoria principal en bytes
    const size_t B_SIZE = stol(argv[2]); // Tamaño del bloque en bytes
    const int a = stoi(argv[3]); // Número de particiones a realizar
    
    // Creación del archivo CSV para guardar los resultados
    ofstream results_csv("sorting_results.csv");
    if (!results_csv) {
        cerr << "Error: No se pudo crear el archivo CSV de resultados.\n";
        return EXIT_FAILURE;
    }
    
    // Encabezado del CSV
    results_csv << "Size_MB,Repetition,N_elements,MergeSort_Time_ms,MergeSort_Disk_Access,"
                << "QuickSort_Time_ms,QuickSort_Disk_Access\n";

    for (size_t i = 0; i < v.size(); ++i){
    // Recorremos el vector de tamaños N → 4,8,…,60
        int mult = static_cast<int>((i + 1) * 4);      
        int64_t N = static_cast<int64_t>(v[i]) * M_BYTES;

        cout << "\n===  Multiplicador " << mult
                  << " M  →  N = " << N << " bytes  ===\n";

        for (int rep = 1; rep <= 5; ++rep){
        // Generar  5  secuencias  de  números  enteros  de  64  bits  de  tamaño  total  𝑁  
            ostringstream fn;
            fn << "seq_" << setw(2) << setfill('0')
               << mult << "M_rep" << rep << ".bin";

            // 1. Generar archivo con secuencias aleatorias de int64_t                                                
            generate_sequence(N, fn.str(), B_SIZE);

            // 2. Procesar (algoritmos externos, medición, etc.)                  
            AlgorithmResults results = process_sequence(fn.str(), N, a, B_SIZE, M_BYTES);
            
            // Save results to CSV
            results_csv << mult*stol(argv[1]) << "," << rep << "," << N << "," 
                        << results.merge_time_ms << "," << results.merge_disk_access << ","
                        << results.quick_time_ms << "," << results.quick_disk_access << "\n";
                        
            cout << "Archivo " << fn.str() << " procesado.\n";
            
            // 3. Borrar para liberar espacio                                     
            filesystem::remove(fn.str());
        }
    }

    // Fin del experimento
    results_csv.close();
    cout << "\n✓ Experimento completo. Resultados guardados en sorting_results.csv" << endl;
    return EXIT_SUCCESS;
}