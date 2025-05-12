#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdint>
#include <string>
#include <chrono>
#include <random>
#include <queue>
#include <limits>
#include <filesystem>
#include "../headers/mergesort.hpp"

/**
 * Estructura para mantener el seguimiento de operaciones de I/O
 */
struct IOCounter {
    size_t reads = 0;
    size_t writes = 0;
    
    void reset() {
        reads = 0;
        writes = 0;
    }
};

// Contador global para operaciones I/O
IOCounter io_counter;

/**
 * Estructura para representar un elemento en la cola de prioridad durante la unión
 */
struct QueueElement {
    int64_t value;
    size_t run_index;
    
    // Operador de comparación para la cola de prioridad (min-heap)
    bool operator>(const QueueElement& other) const {
        return value > other.value;
    }
};

/**
 * Clase para manejar operaciones de archivo con buffer
 */
class BufferedFile {
private:
    std::fstream file;
    std::vector<int64_t> buffer;
    size_t buffer_pos;
    size_t buffer_size;
    bool is_read_mode;
    
public:
    BufferedFile(const std::string& filename, bool read_mode, size_t buffer_size = 4096 / sizeof(int64_t)) 
        : buffer(buffer_size), buffer_pos(0), buffer_size(buffer_size), is_read_mode(read_mode) {
        
        if (read_mode) {
            file.open(filename, std::ios::in | std::ios::binary);
        } else {
            file.open(filename, std::ios::out | std::ios::binary);
        }
        
        if (!file) {
            throw std::runtime_error("Error al abrir el archivo: " + filename);
        }
        
        if (read_mode) {
            // Inicialmente el buffer está vacío para lectura
            buffer_pos = buffer_size;
        }
    }
    
    ~BufferedFile() {
        if (!is_read_mode && buffer_pos > 0) {
            // Escribir cualquier dato restante en el buffer
            file.write(reinterpret_cast<char*>(buffer.data()), buffer_pos * sizeof(int64_t));
            io_counter.writes++;
        }
        file.close();
    }
    
    // Lee un bloque completo y devuelve cuántos elementos se leyeron realmente
    size_t readBlock(std::vector<int64_t>& dest, size_t count) {
        size_t read_count = 0;
        
        while (read_count < count) {
            // Si el buffer está vacío, llenarlo
            if (buffer_pos >= buffer_size) {
                file.read(reinterpret_cast<char*>(buffer.data()), buffer_size * sizeof(int64_t));
                size_t elements_read = file.gcount() / sizeof(int64_t);
                if (elements_read == 0) {
                    break;  // Fin de archivo
                }
                buffer_pos = 0;
                io_counter.reads++;
            }
            
            // Copiar datos del buffer al destino
            size_t available = std::min(buffer_size - buffer_pos, count - read_count);
            std::copy(buffer.begin() + buffer_pos, buffer.begin() + buffer_pos + available, 
                    dest.begin() + read_count);
            
            buffer_pos += available;
            read_count += available;
        }
        
        return read_count;
    }
    
    // Lee un solo elemento
    bool readElement(int64_t& value) {
        if (buffer_pos >= buffer_size) {
            file.read(reinterpret_cast<char*>(buffer.data()), buffer_size * sizeof(int64_t));
            size_t elements_read = file.gcount() / sizeof(int64_t);
            if (elements_read == 0) {
                return false;  // Fin de archivo
            }
            buffer_pos = 0;
            io_counter.reads++;
        }
        
        value = buffer[buffer_pos++];
        return true;
    }
    
    // Escribe un bloque completo
    void writeBlock(const std::vector<int64_t>& src, size_t count) {
        size_t written = 0;
        
        while (written < count) {
            // Si el buffer está lleno, vaciarlo
            if (buffer_pos >= buffer_size) {
                file.write(reinterpret_cast<const char*>(buffer.data()), buffer_size * sizeof(int64_t));
                buffer_pos = 0;
                io_counter.writes++;
            }
            
            // Copiar datos del origen al buffer
            size_t available = std::min(buffer_size - buffer_pos, count - written);
            std::copy(src.begin() + written, src.begin() + written + available, 
                   buffer.begin() + buffer_pos);
            
            buffer_pos += available;
            written += available;
        }
    }
    
    // Escribe un solo elemento
    void writeElement(int64_t value) {
        if (buffer_pos >= buffer_size) {
            file.write(reinterpret_cast<const char*>(buffer.data()), buffer_size * sizeof(int64_t));
            buffer_pos = 0;
            io_counter.writes++;
        }
        
        buffer[buffer_pos++] = value;
    }
    
    // Posiciona el puntero del archivo
    void seek(size_t position) {
        // Vaciar o invalidar el buffer
        if (!is_read_mode && buffer_pos > 0) {
            file.write(reinterpret_cast<char*>(buffer.data()), buffer_pos * sizeof(int64_t));
            io_counter.writes++;
        }
        buffer_pos = buffer_size; // Invalidar buffer
        
        file.seekg(position * sizeof(int64_t), std::ios::beg);
        file.seekp(position * sizeof(int64_t), std::ios::beg);
    }
    
    // Obtiene la posición actual del archivo
    size_t tell() {
        std::streampos file_pos;
        if (is_read_mode) {
            file_pos = file.tellg();
        } else {
            file_pos = file.tellp();
        }
        
        return file_pos / sizeof(int64_t);
    }
};

/**
 * Genera un archivo con enteros aleatorios
 */
void createRandomFile(const std::string& filename, size_t num_elements) {
    std::ofstream output(filename, std::ios::binary);
    if (!output) {
        throw std::runtime_error("No se pudo crear el archivo");
    }
    
    // Crear buffer para escrituras en bloque
    size_t block_size = 4096 / sizeof(int64_t); // Tamaño típico de bloque
    std::vector<int64_t> buffer(block_size);
    
    // Inicializar generador de números aleatorios
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> dist(INT64_MIN, INT64_MAX);
    
    // Escribir números aleatorios en bloques
    size_t remaining = num_elements;
    while (remaining > 0) {
        size_t current_block = std::min(remaining, block_size);
        
        // Llenar buffer con enteros aleatorios de 64 bits
        for (size_t i = 0; i < current_block; i++) {
            buffer[i] = dist(gen);
        }
        
        // Escribir buffer al archivo
        output.write(reinterpret_cast<char*>(buffer.data()), current_block * sizeof(int64_t));
        remaining -= current_block;
    }
    
    output.close();
}

/**
 * Ordena un bloque en memoria
 */
void sortBlock(std::vector<int64_t>& buffer, size_t size) {
    std::sort(buffer.begin(), buffer.begin() + size);
}

/**
 * Une múltiples ejecuciones ordenadas
 */
void mergeRuns(const std::string& input_filename, const std::string& output_filename,
               size_t total_elements, size_t run_size, size_t num_runs_to_merge,
               size_t block_size) {
    
    // Calcular número real de ejecuciones
    size_t num_runs = (total_elements + run_size - 1) / run_size;
    
    // Procesar ejecuciones en grupos de num_runs_to_merge
    for (size_t base_run = 0; base_run < num_runs; base_run += num_runs_to_merge) {
        size_t runs_in_this_merge = std::min(num_runs_to_merge, num_runs - base_run);
        
        try {
            // Abrir archivos
            BufferedFile input_file(input_filename, true, block_size);
            BufferedFile output_file(output_filename, false, block_size);
            
            // Estructuras para unir las ejecuciones
            std::vector<std::vector<int64_t>> run_buffers(runs_in_this_merge);
            std::vector<size_t> buffer_positions(runs_in_this_merge, 0);
            std::vector<size_t> elements_processed(runs_in_this_merge, 0);
            std::vector<size_t> run_sizes(runs_in_this_merge);
            
            // Inicializar buffers y cargar el primer bloque de cada ejecución
            for (size_t i = 0; i < runs_in_this_merge; i++) {
                // Calcular posición y tamaño de esta ejecución
                size_t run_start = (base_run + i) * run_size;
                run_sizes[i] = std::min(run_size, total_elements - run_start);
                
                try {
                    run_buffers[i].resize(block_size);
                    
                    input_file.seek(run_start);
                    
                    size_t elements_to_read = std::min(run_sizes[i], block_size);
                    
                    size_t elements_read = input_file.readBlock(run_buffers[i], elements_to_read);
                    
                    if (elements_read == 0) {
                        run_sizes[i] = 0;  // Ejecución vacía
                    }
                } catch (const std::exception& e) {
                    throw;
                }
            }
            
            // Cola de prioridad para la unión
            std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> pq;
            
            // Inicializar cola con el primer elemento de cada ejecución
            for (size_t i = 0; i < runs_in_this_merge; i++) {
                if (run_sizes[i] > 0) {
                    pq.push({run_buffers[i][0], i});
                    buffer_positions[i] = 1;
                }
            }
            
            size_t elements_merged = 0;
            
            // Realizar la unión
            while (!pq.empty()) {
                QueueElement min_element = pq.top();
                pq.pop();
                
                // Escribir el valor mínimo al archivo de salida
                output_file.writeElement(min_element.value);
                elements_merged++;
                
                size_t run_idx = min_element.run_index;
                size_t run_start = (base_run + run_idx) * run_size;
                
                // Incrementar contador de elementos procesados de esta ejecución
                elements_processed[run_idx]++;
                
                // Si terminamos con el buffer actual, cargar más datos de esta ejecución
                if (buffer_positions[run_idx] >= run_buffers[run_idx].size() || 
                    buffer_positions[run_idx] >= block_size) {
                    
                    // Si aún hay más elementos en esta ejecución, cargarlos
                    if (elements_processed[run_idx] < run_sizes[run_idx]) {
                        try {
                            // Calcular posición absoluta en el archivo
                            size_t absolute_position = run_start + elements_processed[run_idx];
                            input_file.seek(absolute_position);
                            
                            // Calcular cuántos elementos quedan por leer en esta ejecución
                            size_t elements_remaining = run_sizes[run_idx] - elements_processed[run_idx];
                            size_t elements_to_read = std::min(block_size, elements_remaining);
                            
                            // Leer el siguiente bloque
                            size_t elements_read = input_file.readBlock(run_buffers[run_idx], elements_to_read);
                            
                            if (elements_read > 0) {
                                // Resetear posición en el buffer y añadir elemento a la cola
                                buffer_positions[run_idx] = 1;  // Ya usaremos el primer elemento
                                pq.push({run_buffers[run_idx][0], run_idx});
                            } else {
                                // Marcar esta ejecución como completada
                                elements_processed[run_idx] = run_sizes[run_idx];
                            }
                        } catch (const std::exception& e) {
                            throw;
                        }
                    }
                } else {
                    // Todavía hay elementos en el buffer, usar el siguiente
                    int64_t next_value = run_buffers[run_idx][buffer_positions[run_idx]];
                    pq.push({next_value, run_idx});
                    buffer_positions[run_idx]++;
                }
            }
        } catch (const std::exception& e) {
            throw;
        }
    }
}

/**
 * Implementación del algoritmo Mergesort Externo
 */
void externalMergeSort(const std::string& input_filename, const std::string& output_filename,
                      size_t memory_size, size_t block_size, size_t max_runs_to_merge) {
    
    // Determinar tamaño total del archivo
    std::ifstream input_file(input_filename, std::ios::binary | std::ios::ate);
    if (!input_file) {
        throw std::runtime_error("No se pudo abrir el archivo de entrada");
    }
    
    size_t file_size_bytes = input_file.tellg();
    size_t total_elements = file_size_bytes / sizeof(int64_t);
    input_file.close();
    
    // Si el archivo cabe en memoria, ordenarlo directamente
    if (total_elements * sizeof(int64_t) <= memory_size) {
        std::vector<int64_t> buffer(total_elements);
        
        std::ifstream in_file(input_filename, std::ios::binary);
        in_file.read(reinterpret_cast<char*>(buffer.data()), total_elements * sizeof(int64_t));
        io_counter.reads++;
        in_file.close();
        
        sortBlock(buffer, total_elements);
        
        std::ofstream out_file(output_filename, std::ios::binary);
        out_file.write(reinterpret_cast<char*>(buffer.data()), total_elements * sizeof(int64_t));
        io_counter.writes++;
        out_file.close();
        
        return;
    }
    
    // Fase 1: Crear ejecuciones ordenadas
    // Calcular M - tamaño que podemos ordenar en memoria a la vez
    size_t M = memory_size / sizeof(int64_t);
    
    // Generar ejecuciones iniciales ordenadas
    const std::string temp_runs_file = "temp_runs.bin";
    BufferedFile in_file(input_filename, true, block_size);
    BufferedFile runs_file(temp_runs_file, false, block_size);
    
    std::vector<int64_t> buffer(M);
    size_t runs_created = 0;
    size_t elements_left = total_elements;
    
    while (elements_left > 0) {
        // Determinar tamaño de esta ejecución
        size_t run_size = std::min(elements_left, M);
        
        // Leer elementos para esta ejecución
        size_t elements_read = in_file.readBlock(buffer, run_size);
        
        // Ordenar la ejecución en memoria
        sortBlock(buffer, elements_read);
        
        // Escribir ejecución ordenada de vuelta al disco
        runs_file.writeBlock(buffer, elements_read);
        
        elements_left -= elements_read;
        runs_created++;
    }
    
    // Fase 2: Unir las ejecuciones
    // Calcular el número máximo de ejecuciones que podemos unir a la vez
    if (max_runs_to_merge == 0) {
        // Necesitamos 1 buffer de bloque para cada ejecución de entrada y 1 para la salida
        max_runs_to_merge = (memory_size / sizeof(int64_t) / block_size) - 1;
        if (max_runs_to_merge < 2) max_runs_to_merge = 2;  // Mínimo 2 ejecuciones para unir
    }
    
    // Unir iterativamente ejecuciones hasta tener solo una
    std::string input_file_name = temp_runs_file;
    std::string output_file_name = "temp_merged.bin";
    std::string temp;
    size_t run_size = M;
    
    while (runs_created > 1) {
        mergeRuns(input_file_name, output_file_name, total_elements, run_size, max_runs_to_merge, block_size);
        
        // Intercambiar entrada y salida para la siguiente iteración
        temp = input_file_name;
        input_file_name = output_file_name;
        output_file_name = (temp == temp_runs_file) ? "temp_merged.bin" : temp_runs_file;
        
        // Actualizar tamaño de ejecución y conteo para la siguiente iteración
        run_size *= max_runs_to_merge;
        runs_created = (runs_created + max_runs_to_merge - 1) / max_runs_to_merge;
    }
    
    // Renombrar el archivo de salida final al nombre de salida solicitado
    if (input_file_name != output_filename) {
        std::ifstream src(input_file_name, std::ios::binary);
        std::ofstream dst(output_filename, std::ios::binary);
        
        dst << src.rdbuf();
        
        src.close();
        dst.close();
        
        // Eliminar el archivo temporal
        std::remove(input_file_name.c_str());
    }
    
    // Limpiar archivos temporales restantes
    if (std::string(temp_runs_file) != output_filename) {
        std::remove(temp_runs_file.c_str());
    }
    if (std::string("temp_merged.bin") != output_filename) {
        std::remove("temp_merged.bin");
    }
}

/**
 * Encuentra el valor óptimo de 'a' (aridad)
 */
size_t findOptimalA(const std::string& input_filename, size_t memory_size, size_t block_size) {
    // Determinar el tamaño total del archivo
    std::ifstream input_file(input_filename, std::ios::binary | std::ios::ate);
    if (!input_file) {
        throw std::runtime_error("No se pudo abrir el archivo de entrada");
    }
    
    size_t file_size_bytes = input_file.tellg();
    size_t total_elements = file_size_bytes / sizeof(int64_t);
    input_file.close();
    
    // Número máximo de enteros de 64 bits en un bloque
    size_t b = block_size / sizeof(int64_t);
    if (b == 0) b = 1;  // Asegurar al menos 1 elemento por bloque
    
    // Búsqueda binaria para 'a' óptimo
    size_t left = 2;
    size_t right = b;
    size_t best_a = 2;
    size_t min_io = std::numeric_limits<size_t>::max();
    
    // Para cada valor candidato de 'a', ejecutar el algoritmo y medir IOs
    while (left <= right) {
        size_t mid = left + (right - left) / 2;
        
        // Resetear contador IO
        io_counter.reset();
        
        // Crear una copia del archivo de entrada para preservar el original
        const std::string temp_input = "temp_input_copy.bin";
        std::ifstream src(input_filename, std::ios::binary);
        std::ofstream dst(temp_input, std::ios::binary);
        dst << src.rdbuf();
        src.close();
        dst.close();
        
        // Ejecutar Merge Sort con este valor de 'a'
        const std::string temp_output = "temp_output.bin";
        
        // Comprobar si este 'a' requiere más memoria de la disponible
        size_t old_max_runs = (memory_size / sizeof(int64_t) / block_size) - 1;
        if (mid > old_max_runs) {
            std::remove(temp_input.c_str());
            left = mid + 1;
            continue;
        }
        
        // Ejecutar el merge sort externo con este valor de 'a'
        externalMergeSort(temp_input, temp_output, memory_size, block_size, mid);
        
        // Comprobar si este 'a' es mejor
        if (io_counter.reads + io_counter.writes < min_io) {
            min_io = io_counter.reads + io_counter.writes;
            best_a = mid;
        }
        
        // Limpiar archivos temporales
        std::remove(temp_input.c_str());
        std::remove(temp_output.c_str());
        
        // Continuar búsqueda binaria
        if (min_io == io_counter.reads + io_counter.writes) {
            // Si el rendimiento mejora o se mantiene igual, probar un 'a' mayor
            left = mid + 1;
        } else {
            // Si el rendimiento empeoró, probar un 'a' menor
            right = mid - 1;
        }
    }
    
    return best_a;
}

// Function to expose for main.cpp
long long run_mergesort(const std::string& inputFile, long N_SIZE, int a, long B_SIZE, long M_SIZE) {
    // Reset IO counter
    io_counter.reset();
    
    // Create a temporary output filename
    std::string outputFile = inputFile + ".sorted";

    M_SIZE *= 1024 * 1024; // Convert M_SIZE from MB to bytes

    
    // Run external merge sort
    externalMergeSort(inputFile, outputFile, M_SIZE, B_SIZE, a);
    
    // Copy the sorted result back to the input file
    std::ifstream src(outputFile, std::ios::binary);
    std::ofstream dst(inputFile, std::ios::binary | std::ios::trunc);
    dst << src.rdbuf();
    src.close();
    dst.close();
    
    // Return total number of disk accesses
    return io_counter.reads + io_counter.writes;
}