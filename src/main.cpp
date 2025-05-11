//main args: 
//argv[1] <- M_SIZE
//argv[2] <- B_SIZE quiza lo podemos dejar fijo en 4096 

//se define M_SIZE <- tamaño de la memoria principal: valor capturado desde los args
//se define B_SIZE <- tamaño del bloque: como tamaño real de bloque en un disco (512B en discos viejos, 4069B en discos modernos)
//se define N_SIZE <- tamaño del array que contiene a los elementos a ordenar (input)
//se define a <- valor que se debe determinar acá y define: 1) aridad del mergesort 2) Número de particiones que se van a realizar en el algoritmo de quicksort

//funcion que genera 5  secuencias  de  números  enteros  de  64  bits  de  tamaño  total   𝑁,  con
//𝑁 ∈ {4𝑀, 8𝑀, ...60𝑀} (es decir, 5 secuencias de tamaño 4𝑀, 5 secuencias de tamaño 8𝑀, ...), 
//insertarlos desordenadamente en un arreglo y guardarlo en binario en disco. 
//Para trabajar en disco, se guardará como un archivo binario (.bin) el arreglo de tamaño 𝑁. Este archivo
//se deberá leer y escribir en bloques de tamaño 𝐵. Es incorrecto realizar una lectura o escritura de tamaño
//distinto a 𝐵. Para esto, se recomienda usar las funciones fread, fwrite y fseek de C. Se recomienda
//también usar un buffer para evitar leer y escribir cada vez que se accede a un elemento del arreglo. El
//buffer debe ser de tamaño 𝐵 es decir, tendremos en memoria principal todo el bloque solicitado


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
#include <chrono>  // Add this include at the top of the file
#include "sequence_generator.hpp"
#include "../headers/quicksort.hpp" 

using namespace std;

//vector de 15 tamaños N
vector<int> v = {4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};

// Define a struct to hold algorithm results
struct AlgorithmResults {
    long long merge_time_ms;
    int merge_disk_access;
    long long quick_time_ms;
    int quick_disk_access;
};

// Modified process_sequence to return the results
AlgorithmResults process_sequence(const std::string& filename, long N_SIZE, int a, long B_SIZE, long M_SIZE) {
    AlgorithmResults results = {0, 0, 0, 0};
    
    // MERGESORT IMPLEMENTATION WOULD GO HERE
    // auto merge_start_time = std::chrono::high_resolution_clock::now();
    // int merge_sort_disk_access = run_mergesort(filename, N_SIZE, a, B_SIZE, M_SIZE);
    // auto merge_end_time = std::chrono::high_resolution_clock::now();
    // auto merge_duration = std::chrono::duration_cast<std::chrono::milliseconds>(merge_end_time - merge_start_time);
    // results.merge_time_ms = merge_duration.count();
    // results.merge_disk_access = merge_sort_disk_access;
    // cout << "MergeSort completado en " << results.merge_time_ms << " ms, con " 
    //      << results.merge_disk_access << " accesos a disco" << endl;
    
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
    if (argc != 3) {
        cerr << "Uso: " << argv[0]
                  << " <M_SIZE_MB> <B_SIZE_bytes>\n";
        return EXIT_FAILURE;
    }

    const int64_t M_BYTES = stol(argv[1]) * 1024L * 1024L;
    const size_t B_SIZE = stol(argv[2]);
    
    // Create and initialize CSV file
    ofstream results_csv("sorting_results.csv");
    if (!results_csv) {
        cerr << "Error: No se pudo crear el archivo CSV de resultados.\n";
        return EXIT_FAILURE;
    }
    
    // Write CSV header
    results_csv << "Size_MB,Repetition,N_elements,MergeSort_Time_ms,MergeSort_Disk_Access,"
                << "QuickSort_Time_ms,QuickSort_Disk_Access\n";

    for (size_t i = 0; i < v.size(); ++i)
    {
        int mult = static_cast<int>((i + 1) * 4);      // 4,8,…,60
        int64_t N = static_cast<int64_t>(v[i]) * M_BYTES;

        cout << "\n===  Multiplicador " << mult
                  << " M  →  N = " << N << " elementos  ===\n";

        for (int rep = 1; rep <= 5; ++rep)
        {
            ostringstream fn;
            fn << "seq_" << setw(2) << setfill('0')
               << mult << "M_rep" << rep << ".bin";

            // 1. Generar                                                         
            generate_sequence(N, fn.str(), B_SIZE);

            // 2. Procesar (algoritmos externos, medición, etc.)                  
            AlgorithmResults results = process_sequence(fn.str(), N, 30, B_SIZE, M_BYTES);
            
            // Save results to CSV
            results_csv << mult*stol(argv[1]) << "," << rep << "," << N << "," 
                        << results.merge_time_ms << "," << results.merge_disk_access << ","
                        << results.quick_time_ms << "," << results.quick_disk_access << "\n";
                        
            cout << "Archivo " << fn.str() << " procesado.\n";
            
            // 3. Borrar para liberar espacio                                     
            filesystem::remove(fn.str());
        }
    }

    // Close the CSV file
    results_csv.close();
    cout << "\n✓ Experimento completo. Resultados guardados en sorting_results.csv\n";
    return EXIT_SUCCESS;
}