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
#include <chrono>
#include <algorithm>
#include <string>
#include <cstring>
#include <filesystem>
#include <random>

using namespace std;
namespace fs = std::filesystem;

// Configuraciones y contadores
size_t B_SIZE = 4096;  // Tamaño del bloque de disco (4KB es común en sistemas modernos)
size_t M_SIZE = 50 * 1024 * 1024; // Memoria principal de 50 MB
size_t disk_access = 0; // Contador de accesos al disco

// Función para generar una secuencia aleatoria de enteros de 64 bits
void generate_sequence(size_t num_elements, const string& filename) {
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        cerr << "Error al crear el archivo " << filename << endl;
        return;
    }

    // Usar un generador de números aleatorios de alta calidad
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<int64_t> dis;

    // Calcular cuántos elementos por bloque
    size_t elements_per_block = B_SIZE / sizeof(int64_t);
    vector<int64_t> buffer(elements_per_block);

    // Generar y escribir los números en bloques
    for (size_t i = 0; i < num_elements; i += elements_per_block) {
        size_t block_size = min(elements_per_block, num_elements - i);
        
        // Llenar el buffer con números aleatorios
        for (size_t j = 0; j < block_size; j++) {
            buffer[j] = dis(gen);
        }
        
        // Escribir el bloque al archivo
        fwrite(buffer.data(), sizeof(int64_t), block_size, file);
    }

    fclose(file);
    cout << "Secuencia generada en " << filename << endl;
}

// Función para leer un bloque desde un archivo
size_t read_block(FILE* file, vector<int64_t>& buffer, size_t offset = 0, size_t max_elements = 0) {
    size_t elements_per_block = B_SIZE / sizeof(int64_t);
    size_t elements_to_read = (max_elements > 0) ? min(max_elements, elements_per_block) : elements_per_block;
    
    buffer.resize(elements_to_read);
    
    fseek(file, offset * sizeof(int64_t), SEEK_SET);
    size_t elements_read = fread(buffer.data(), sizeof(int64_t), elements_to_read, file);
    
    if (elements_read > 0) {
        disk_access++;
    }
    
    buffer.resize(elements_read);  // Ajustar al tamaño real leído
    return elements_read;
}

// Función para escribir un bloque a un archivo
void write_block(FILE* file, const vector<int64_t>& buffer, size_t offset = 0) {
    fseek(file, offset * sizeof(int64_t), SEEK_SET);
    fwrite(buffer.data(), sizeof(int64_t), buffer.size(), file);
    disk_access++;
}

// Función para seleccionar y ordenar pivotes de un bloque aleatorio
vector<int64_t> select_pivots(FILE* input_file, size_t file_size, int a) {
    // Calcular cuántos elementos hay en total en el archivo
    size_t total_elements = file_size / sizeof(int64_t);
    
    // Seleccionar un offset aleatorio para leer un bloque
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<size_t> dis(0, max(0UL, total_elements - (B_SIZE / sizeof(int64_t))));
    size_t random_offset = dis(gen);
    
    // Leer el bloque aleatorio
    vector<int64_t> block;
    size_t elements_read = read_block(input_file, block, random_offset);
    
    if (elements_read < a - 1) {
        cerr << "No se pudieron leer suficientes elementos para seleccionar pivotes" << endl;
        return {};
    }
    
    // Seleccionar a-1 elementos como pivotes
    vector<int64_t> pivots;
    uniform_int_distribution<size_t> pivot_dis(0, elements_read - 1);
    
    for (int i = 0; i < a - 1; i++) {
        size_t idx = pivot_dis(gen);
        pivots.push_back(block[idx]);
    }
    
    // Ordenar los pivotes
    sort(pivots.begin(), pivots.end());
    
    return pivots;
}

// Función para encontrar en qué partición va un elemento
int find_partition(const vector<int64_t>& pivots, int64_t value) {
    for (size_t i = 0; i < pivots.size(); i++) {
        if (value < pivots[i]) {
            return i;
        }
    }
    return pivots.size();  // La última partición
}

// Función para particionar el archivo de entrada completo usando los pivotes
void partition_file(FILE* input_file, size_t file_size, const vector<int64_t>& pivots, 
                    vector<string>& partition_files) {
    
    int a = pivots.size() + 1;  // Número de particiones
    
    // Crear archivos temporales para cada partición
    vector<FILE*> partition_fps(a);
    for (int i = 0; i < a; i++) {
        string partition_name = "partition_" + to_string(i) + ".tmp";
        partition_files[i] = partition_name;
        partition_fps[i] = fopen(partition_name.c_str(), "wb");
        if (!partition_fps[i]) {
            cerr << "Error al crear archivo de partición " << partition_name << endl;
            exit(1);
        }
        
        // Escribir al menos un byte en cada archivo para asegurar que exista
        int64_t dummy = 0;
        fwrite(&dummy, sizeof(int64_t), 1, partition_fps[i]);
        fseek(partition_fps[i], 0, SEEK_SET); // Volver al inicio del archivo
    }
    
    // Buffers para cada partición (para minimizar escrituras)
    vector<vector<int64_t>> partition_buffers(a);
    for (int i = 0; i < a; i++) {
        partition_buffers[i].reserve(B_SIZE / sizeof(int64_t));
    }
    
    // Procesar el archivo de entrada por bloques
    fseek(input_file, 0, SEEK_SET);
    size_t total_elements = file_size / sizeof(int64_t);
    size_t elements_per_block = B_SIZE / sizeof(int64_t);
    
    for (size_t offset = 0; offset < total_elements; offset += elements_per_block) {
        // Leer un bloque del archivo original
        vector<int64_t> block;
        size_t elements_read = read_block(input_file, block, offset, elements_per_block);
        
        // Distribuir elementos a las particiones correspondientes
        for (size_t i = 0; i < elements_read; i++) {
            int partition_idx = find_partition(pivots, block[i]);
            
            partition_buffers[partition_idx].push_back(block[i]);
            
            // Si el buffer está lleno, escribirlo al archivo de partición
            if (partition_buffers[partition_idx].size() >= elements_per_block) {
                fwrite(partition_buffers[partition_idx].data(), sizeof(int64_t), 
                       partition_buffers[partition_idx].size(), partition_fps[partition_idx]);
                disk_access++;
                partition_buffers[partition_idx].clear();
            }
        }
    }
    
    // Vaciar los buffers restantes
    for (int i = 0; i < a; i++) {
        if (!partition_buffers[i].empty()) {
            fwrite(partition_buffers[i].data(), sizeof(int64_t), 
                   partition_buffers[i].size(), partition_fps[i]);
            disk_access++;
        }
        fclose(partition_fps[i]);
    }
}

// Improved version of the external_quicksort function to prevent freezing
void external_quicksort(const string& input_filename, const string& output_filename, int a) {
    // Verify if the file exists and get its size
    if (!fs::exists(input_filename)) {
        cerr << "File not found: " << input_filename << endl;
        return;
    }
    
    size_t file_size = fs::file_size(input_filename);
    
    // Handle empty files
    if (file_size == 0) {
        FILE* output_file = fopen(output_filename.c_str(), "wb");
        fclose(output_file);
        return;
    }
    
    // In-memory sort for small files
    if (file_size <= M_SIZE) {
        FILE* input_file = fopen(input_filename.c_str(), "rb");
        FILE* output_file = fopen(output_filename.c_str(), "wb");
        
        if (!input_file || !output_file) {
            cerr << "Error opening files for in-memory sort" << endl;
            if (input_file) fclose(input_file);
            if (output_file) fclose(output_file);
            return;
        }
        
        size_t num_elements = file_size / sizeof(int64_t);
        vector<int64_t> data(num_elements);
        
        size_t read_elements = fread(data.data(), sizeof(int64_t), num_elements, input_file);
        disk_access++;
        
        if (read_elements != num_elements) {
            cerr << "Warning: Couldn't read all elements from " << input_filename << endl;
        }
        
        sort(data.begin(), data.begin() + read_elements);
        
        fwrite(data.data(), sizeof(int64_t), read_elements, output_file);
        disk_access++;
        
        fclose(input_file);
        fclose(output_file);
        return;
    }
    
    // External sort for larger files
    FILE* input_file = fopen(input_filename.c_str(), "rb");
    if (!input_file) {
        cerr << "Error opening input file: " << input_filename << endl;
        return;
    }
    
    // Generate a unique prefix for temporary files to avoid conflicts in recursion
    string temp_prefix = "temp_" + to_string(rand()) + "_";
    
    // Select pivots
    vector<int64_t> pivots = select_pivots(input_file, file_size, a);
    
    if (pivots.empty() && a > 1) {
        // Fall back to just copying the file if we can't select pivots
        fclose(input_file);
        fs::copy_file(input_filename, output_filename, fs::copy_options::overwrite_existing);
        return;
    }
    
    // Create temporary partition files with unique names
    vector<string> partition_files(a);
    for (int i = 0; i < a; i++) {
        partition_files[i] = temp_prefix + "partition_" + to_string(i) + ".tmp";
        // Make sure any existing files with the same name are removed
        fs::remove(partition_files[i]);
    }
    
    // Partition the input file
    partition_file(input_file, file_size, pivots, partition_files);
    fclose(input_file);
    
    // Check partitions and recursively sort them
    bool all_partitions_empty = true;
    for (int i = 0; i < a; i++) {
        if (fs::exists(partition_files[i]) && fs::file_size(partition_files[i]) > 0) {
            all_partitions_empty = false;
            
            string sorted_partition = temp_prefix + "sorted_" + to_string(i) + ".tmp";
            fs::remove(sorted_partition); // Ensure it doesn't exist
            
            // Recursively sort this partition
            external_quicksort(partition_files[i], sorted_partition, a);
            
            // Replace the original partition with the sorted one
            fs::remove(partition_files[i]);
            if (fs::exists(sorted_partition)) {
                fs::rename(sorted_partition, partition_files[i]);
            } else {
                // Create empty file if sorted partition doesn't exist
                FILE* empty = fopen(partition_files[i].c_str(), "wb");
                if (empty) fclose(empty);
            }
        }
    }
    
    // If all partitions are empty, create an empty output file and return
    if (all_partitions_empty) {
        FILE* output_file = fopen(output_filename.c_str(), "wb");
        if (output_file) fclose(output_file);
        return;
    }
    
    // Combine sorted partitions
    FILE* output_file = fopen(output_filename.c_str(), "wb");
    if (!output_file) {
        cerr << "Error creating output file: " << output_filename << endl;
        return;
    }
    
    for (int i = 0; i < a; i++) {
        if (!fs::exists(partition_files[i]) || fs::file_size(partition_files[i]) == 0) {
            continue; // Skip empty partitions
        }
        
        FILE* partition_file = fopen(partition_files[i].c_str(), "rb");
        if (!partition_file) {
            cerr << "Error opening partition file: " << partition_files[i] << endl;
            continue;
        }
        
        // Copy partition content to output file
        vector<int64_t> buffer;
        while (true) {
            size_t elements_read = read_block(partition_file, buffer);
            if (elements_read == 0) break;
            
            write_block(output_file, buffer);
        }
        
        fclose(partition_file);
        fs::remove(partition_files[i]);
        
        // Insert pivot after each partition except the last one
        if (i < a - 1 && i < pivots.size()) {
            vector<int64_t> pivot_buffer = {pivots[i]};
            write_block(output_file, pivot_buffer);
        }
    }
    
    fclose(output_file);
}

// Función para verificar si un archivo está ordenado
bool check_sorted(const string& filename) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        cerr << "Error al abrir el archivo " << filename << endl;
        return false;
    }

    int64_t prev, curr;
    bool first = true;

    // Leer un elemento a la vez para ahorrar memoria
    while (fread(&curr, sizeof(int64_t), 1, file) == 1) {
        if (!first && prev > curr) {
            fclose(file);
            return false;
        }
        prev = curr;
        first = false;
    }

    fclose(file);
    return true;
}

// Función para imprimir los primeros N elementos de un archivo
void print_first_elements(const string& filename, int n) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        cerr << "Error al abrir el archivo " << filename << endl;
        return;
    }

    vector<int64_t> elements(n);
    size_t elements_read = fread(elements.data(), sizeof(int64_t), n, file);

    cout << "Primeros " << elements_read << " elementos:" << endl;
    for (size_t i = 0; i < elements_read; i++) {
        cout << elements[i] << " ";
        if ((i + 1) % 10 == 0) cout << endl;
    }
    cout << endl;

    fclose(file);
}

int main(int argc, char* argv[]) {
    // Parámetros por defecto
    size_t N = 4 * M_SIZE;  // 4M elementos por defecto
    int a = 4;  // 4 particiones por defecto
    
    // Procesar argumentos de línea de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) {
            N = stoull(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            a = stoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "-M") == 0 && i + 1 < argc) {
            M_SIZE = stoull(argv[i+1]) * 1024 * 1024;  // Convertir de MB a bytes
            i++;
        } else if (strcmp(argv[i], "-B") == 0 && i + 1 < argc) {
            B_SIZE = stoull(argv[i+1]);
            i++;
        }
    }
    
    string input_file = "input.bin";
    string output_file = "output.bin";
    
    cout << "Generando secuencia de " << N << " elementos..." << endl;
    generate_sequence(N, input_file);
    
    cout << "Elementos originales:" << endl;
    print_first_elements(input_file, 20);
    
    // Reiniciar contador de accesos a disco
    disk_access = 0;
    
    cout << "Ordenando con Quicksort externo (a=" << a << ")..." << endl;
    auto start_time = chrono::high_resolution_clock::now();
    
    external_quicksort(input_file, output_file, a);
    
    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;
    
    cout << "Tiempo de ejecución: " << elapsed.count() << " segundos" << endl;
    cout << "Accesos a disco: " << disk_access << endl;
    
    cout << "Verificando si el archivo está ordenado: ";
    if (check_sorted(output_file)) {
        cout << "¡Ordenado correctamente!" << endl;
    } else {
        cout << "¡Error! El archivo no está ordenado" << endl;
    }
    
    cout << "Elementos ordenados:" << endl;
    print_first_elements(output_file, 20);
    
    return 0;
}