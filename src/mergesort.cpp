#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <cstdio>
#include <cstdlib>
#include <climits> // Para LLONG_MAX
#include <cstring> // Para memcpy
#include "../headers/mergesort.hpp"

using namespace std;

// --- Constantes y Configuraciones (AJUSTAR ESTOS VALORES) ---
// M: Tamaño de la memoria principal disponible para ordenar un tramo (en bytes)
const long long M_BYTES = 50 * 1024 * 1024; // 50 MB según la tarea

// B: Tamaño del bloque de disco (en bytes)
const int B_BYTES = 4 * 1024; // Ejemplo: 4KB

// Calcula cuántos long long caben en M y B
const int RUN_SIZE_ELEMENTS = M_BYTES / sizeof(long long); 
const int BLOCK_SIZE_ELEMENTS = B_BYTES / sizeof(long long);

// Estructura para el MinHeap
struct MinHeapNode {
    long long element; 
    int i;             
    FILE* file_ptr;    
    vector<long long> buffer; 
    int buffer_pos;    
    long long elements_remaining_in_file;
};

// Comparador para el MinHeap
struct CompareMinHeapNode {
    bool operator()(MinHeapNode const& a, MinHeapNode const& b) {
        return a.element > b.element;
    }
};

// Prototipos
void sortBlock(vector<long long>& arr);
FILE* openFile(const char* fileName, const char* mode);

 // Contador global de operaciones de E/S
long long countIO = 0;

// Función de ordenamiento en memoria (Merge Sort simple)
void merge(vector<long long>& arr, int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;
    vector<long long> L(n1), R(n2);
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int j = 0; j < n2; j++) R[j] = arr[m + 1 + j];
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) arr[k++] = L[i++];
        else arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
}

// Función de ordenamiento recursivo
void mergeSortRecursive(vector<long long>& arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortRecursive(arr, l, m);
        mergeSortRecursive(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

// Función para ordenar un bloque de datos en memoria
void sortBlock(vector<long long>& arr) {
    std::sort(arr.begin(), arr.end());
}

// Función para abrir un archivo y manejar errores
FILE* openFile(const char* fileName, const char* mode) {
    FILE* fp = fopen(fileName, mode);
    if (fp == NULL) {
        perror("Error while opening the file");
        exit(EXIT_FAILURE);
    }
    return fp;
}


// input_file: nombre del archivo de entrada
// arity: aridad 'a', también el número de archivos temporales a generar (aproximadamente)
// run_size_elements: M, capacidad de la memoria en número de elementos long long
// block_size_elements: B, tamaño del bloque en disco en número de elementos long long
int createInitialRuns(const char* input_file, int arity, int run_size_elements, int block_size_elements) {
    FILE* in = openFile(input_file, "rb"); // Leer en modo binario

    vector<FILE*> out_files(arity);
    char fileName[32];
    for (int i = 0; i < arity; i++) {
        snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
        out_files[i] = openFile(fileName, "wb"); // Escribir en modo binario
    }

    vector<long long> run_buffer(run_size_elements); // Buffer para un tramo completo en memoria (M)
    vector<long long> block_read_buffer(block_size_elements); // Buffer para leer un bloque (B)

    bool more_input = true;
    int next_output_file_idx = 0;
    long long total_elements_processed = 0;

    fseek(in, 0, SEEK_END);
    long long total_file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    long long total_elements_in_file = total_file_size / sizeof(long long);

    while (more_input && total_elements_processed < total_elements_in_file) {
        int elements_in_current_run = 0;
        for (int i = 0; i < run_size_elements && more_input; ) {
            if (total_elements_processed >= total_elements_in_file) {
                more_input = false;
                break;
            }
            // Leer un bloque B si es necesario o lo que quede del archivo
            long long elements_to_read_this_block = min((long long)block_size_elements, total_elements_in_file - total_elements_processed);
            elements_to_read_this_block = min(elements_to_read_this_block, (long long)run_size_elements - elements_in_current_run);


            if (elements_to_read_this_block <= 0) { // No hay más espacio en el run_buffer o no hay más elementos en el archivo
                 more_input = (total_elements_processed < total_elements_in_file);
                 break;
            }

            size_t read_count = fread(block_read_buffer.data(), sizeof(long long), elements_to_read_this_block, in);
            countIO++; // Contar lectura

            if (read_count == 0 && ferror(in)) {
                perror("Error reading input file");
                exit(EXIT_FAILURE);
            }
            if (read_count == 0 && feof(in) && elements_to_read_this_block > 0) {
                 more_input = false; // Fin de archivo
            }


            for (size_t j = 0; j < read_count; ++j) {
                if (elements_in_current_run < run_size_elements) {
                    run_buffer[elements_in_current_run++] = block_read_buffer[j];
                }
            }
            total_elements_processed += read_count;
             if (read_count < (size_t)elements_to_read_this_block) { //Si se leyeron menos elementos de los esperados (EOF)
                more_input = false;
            }
            i += read_count;
        }

        if (elements_in_current_run > 0) {
            vector<long long> current_run_data(run_buffer.begin(), run_buffer.begin() + elements_in_current_run);
            sortBlock(current_run_data); // Ordenar el tramo en memoria

            // Escribir el tramo ordenado al archivo temporal correspondiente, por bloques B
            FILE* current_out_file = out_files[next_output_file_idx];
            int elements_written_in_run = 0;
            while(elements_written_in_run < elements_in_current_run) {
                long long elements_to_write_this_block = min((long long)block_size_elements, (long long)elements_in_current_run - elements_written_in_run);
                if (elements_to_write_this_block <= 0) break;

                fwrite(current_run_data.data() + elements_written_in_run, sizeof(long long), elements_to_write_this_block, current_out_file);
                countIO++; // Contar escritura
                elements_written_in_run += elements_to_write_this_block;
            }
            next_output_file_idx = (next_output_file_idx + 1) % arity;
        }
         if (total_elements_processed >= total_elements_in_file) {
            more_input = false;
        }
    }

    for (int i = 0; i < arity; i++) {
        fclose(out_files[i]);
    }
    fclose(in);
    return arity;
}


// Mezcla a archivos ordenados 
// output_file_name: nombre del archivo de salida final
// num_runs: numero de bloques a mezclar 
// block_size_elements: B
void mergeFiles(const char* output_file_name, int num_runs, int block_size_elements) {
    FILE* out = openFile(output_file_name, "wb");

    priority_queue<MinHeapNode, vector<MinHeapNode>, CompareMinHeapNode> min_heap;
    vector<FILE*> in_files(num_runs);
    vector<long long> output_buffer(block_size_elements);
    int output_buffer_pos = 0;

    for (int i = 0; i < num_runs; i++) {
        char fileName[32];
        snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
        in_files[i] = fopen(fileName, "rb");

        if (in_files[i] == NULL) {
            num_runs = i; 
            break;
        }

        fseek(in_files[i], 0, SEEK_END);
        long long file_size = ftell(in_files[i]);
        fseek(in_files[i], 0, SEEK_SET);

        if (file_size == 0) { 
            fclose(in_files[i]);
            in_files[i] = nullptr; 
            continue;
        }


        MinHeapNode node;
        node.i = i;
        node.file_ptr = in_files[i];
        node.buffer.resize(block_size_elements);
        node.buffer_pos = 0;
        node.elements_remaining_in_file = file_size / sizeof(long long);


        if (node.elements_remaining_in_file > 0) {
            size_t read_count = fread(node.buffer.data(), sizeof(long long), min((long long)block_size_elements, node.elements_remaining_in_file), node.file_ptr);
            countIO++;
            if (read_count > 0) {
                node.element = node.buffer[0];
                node.buffer_pos = 1;
                node.elements_remaining_in_file -= read_count; 
                node.buffer.resize(read_count); 
                min_heap.push(node);
            } else {
                 fclose(in_files[i]); 
                 in_files[i] = nullptr;
            }
        } else {
            fclose(in_files[i]); 
            in_files[i] = nullptr;
        }
    }
    

    while (!min_heap.empty()) {
        MinHeapNode root = min_heap.top();
        min_heap.pop();

        output_buffer[output_buffer_pos++] = root.element;

        if (output_buffer_pos == block_size_elements) {
            fwrite(output_buffer.data(), sizeof(long long), block_size_elements, out);
            countIO++;
            output_buffer_pos = 0;
        }

        if (root.buffer_pos < (int)root.buffer.size()) {
            root.element = root.buffer[root.buffer_pos++];
            min_heap.push(root);
        } else { 
            if (root.elements_remaining_in_file > 0) {
                long long elements_to_read_next_block = min((long long)block_size_elements, root.elements_remaining_in_file);
                size_t read_count = fread(root.buffer.data(), sizeof(long long), elements_to_read_next_block, root.file_ptr);
                countIO++;
                if (read_count > 0) {
                    root.buffer.resize(read_count); 
                    root.element = root.buffer[0];
                    root.buffer_pos = 1;
                    root.elements_remaining_in_file -= read_count;
                    min_heap.push(root);
                } else { 
                    fclose(root.file_ptr);
                    root.file_ptr = nullptr;
                }
            } else { 
                 if(root.file_ptr) fclose(root.file_ptr);
                 root.file_ptr = nullptr;
            }
        }
    }

    // Escribir cualquier elemento restante en el buffer de salida
    if (output_buffer_pos > 0) {
        fwrite(output_buffer.data(), sizeof(long long), output_buffer_pos, out);
        countIO++;
    }

    fclose(out);
    
}

int externalMergeSort(const char* input_file, const char* output_file, int num_ways_k, int run_size_M_elements, int block_size_B_elements) {
    cout << "Iniciando ordenamiento externo..." << endl;
    cout << "Tamaño de tramo en memoria (M): " << run_size_M_elements << " elementos (" << (long long)run_size_M_elements * sizeof(long long) / (1024*1024) << " MB)" << endl;
    cout << "Tamaño de bloque de disco (B): " << block_size_B_elements << " elementos (" << (long long)block_size_B_elements * sizeof(long long) / 1024 << " KB)" << endl;
    cout << "Aridad (k/a): " << num_ways_k << endl;
    countIO = 0;

    int actual_num_runs = createInitialRuns(input_file, num_ways_k, run_size_M_elements, block_size_B_elements);
    cout << "Fase de creación de tramos iniciales completada. Tramos creados: " << actual_num_runs << endl;
    cout << "Operaciones de E/S hasta ahora: " << countIO << endl;

    // Si createInitialRuns devuelve 0 tramos (ej. archivo de entrada vacío), no hay nada que mezclar.
    if (actual_num_runs > 0) {
        mergeFiles(output_file, actual_num_runs, block_size_B_elements);
        cout << "Fase de mezcla completada." << endl;
    } else {
        cout << "No se crearon tramos iniciales (posiblemente archivo de entrada vacío). Creando archivo de salida vacío." << endl;
        FILE* out = openFile(output_file, "wb"); // Crea un archivo de salida vacío
        fclose(out);
    }
    cout << "Ordenamiento externo finalizado." << endl;
    cout << "Total de operaciones de E/S (aproximado): " << countIO << endl;

    // Eliminar archivos temporales
    for (int i = 0; i < num_ways_k; i++) {
        char fileName[32];
        snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
        remove(fileName);
    }

    return countIO; 
}

// Modificar la declaración de run_mergesort para que coincida con el encabezado
int run_mergesort(const std::string& inputFile, long N_SIZE, int a, long B_SIZE_arg, long M_SIZE_arg) {
    // Convertir los argumentos a enteros
    int B_SIZE = static_cast<int>(B_SIZE_arg);
    int M_SIZE = static_cast<int>(M_SIZE_arg);

    // Crear nombre de archivo para la salida
    std::string outputFile = inputFile + ".sorted";

    // Llamar a la función de ordenamiento externo
    return externalMergeSort(inputFile.c_str(), outputFile.c_str(), a, M_SIZE / sizeof(long long), B_SIZE / sizeof(long long));
}