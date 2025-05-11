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
    
    std::cout << "DEBUG: Iniciando mergeRuns con input=" << input_filename 
              << ", output=" << output_filename << std::endl;
    
    // Calcular número real de ejecuciones
    size_t num_runs = (total_elements + run_size - 1) / run_size;
    std::cout << "DEBUG: Número total de ejecuciones: " << num_runs << std::endl;
    
    // Procesar ejecuciones en grupos de num_runs_to_merge
    for (size_t base_run = 0; base_run < num_runs; base_run += num_runs_to_merge) {
        size_t runs_in_this_merge = std::min(num_runs_to_merge, num_runs - base_run);
        std::cout << "Uniendo ejecuciones " << base_run << " a " << (base_run + runs_in_this_merge - 1) << std::endl;
        
        try {
            // Abrir archivos
            std::cout << "DEBUG: Intentando abrir archivos..." << std::endl;
            BufferedFile input_file(input_filename, true, block_size);
            BufferedFile output_file(output_filename, false, block_size);
            std::cout << "DEBUG: Archivos abiertos correctamente" << std::endl;
            
            std::cout << "Leyendo ejecuciones de " << input_filename << std::endl;
            std::cout << "Escribiendo en " << output_filename << std::endl;
            std::cout << "Tamaño de bloque: " << block_size << " elementos" << std::endl;
            std::cout << "Número de ejecuciones a unir: " << runs_in_this_merge << std::endl;
            std::cout << "Tamaño de cada ejecución: " << run_size << " elementos" << std::endl;
            std::cout << "Total de elementos: " << total_elements << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            std::cout << "Leyendo ejecuciones..." << std::endl;
            
            // Estructuras para unir las ejecuciones
            std::vector<std::vector<int64_t>> run_buffers(runs_in_this_merge);
            std::vector<size_t> buffer_positions(runs_in_this_merge, 0);
            std::vector<size_t> elements_processed(runs_in_this_merge, 0); // Nuevo: contador de elementos procesados
            std::vector<size_t> run_sizes(runs_in_this_merge);
            
            std::cout << "DEBUG: Inicializando buffers para " << runs_in_this_merge << " ejecuciones" << std::endl;
            
            // Inicializar buffers y cargar el primer bloque de cada ejecución
            for (size_t i = 0; i < runs_in_this_merge; i++) {
                std::cout << "DEBUG: Preparando ejecución " << i << " de " << runs_in_this_merge << std::endl;
                
                // Calcular posición y tamaño de esta ejecución
                size_t run_start = (base_run + i) * run_size;
                run_sizes[i] = std::min(run_size, total_elements - run_start);
                
                std::cout << "DEBUG: Ejecución " << i << " comienza en posición " << run_start 
                          << " con tamaño " << run_sizes[i] << std::endl;
                
                try {
                    run_buffers[i].resize(block_size);
                    std::cout << "DEBUG: Buffer redimensionado a " << block_size << " elementos" << std::endl;
                    
                    input_file.seek(run_start);
                    std::cout << "DEBUG: Posicionado en " << run_start << " para lectura" << std::endl;
                    
                    size_t elements_to_read = std::min(run_sizes[i], block_size);
                    std::cout << "DEBUG: Intentando leer " << elements_to_read << " elementos" << std::endl;
                    
                    size_t elements_read = input_file.readBlock(run_buffers[i], elements_to_read);
                    std::cout << "DEBUG: Leídos " << elements_read << " elementos" << std::endl;
                    
                    if (elements_read == 0) {
                        run_sizes[i] = 0;  // Ejecución vacía
                        std::cout << "DEBUG: ¡Advertencia! Ejecución " << i << " está vacía" << std::endl;
                    }
                    
                    if (elements_read > 0) {
                        std::cout << "DEBUG: Primeros elementos de ejecución " << i << ": ";
                        for (size_t j = 0; j < std::min(elements_read, size_t(5)); j++) {
                            std::cout << run_buffers[i][j] << " ";
                        }
                        std::cout << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "ERROR al inicializar buffer " << i << ": " << e.what() << std::endl;
                    throw;
                }
            }
            
            // Cola de prioridad para la unión
            std::cout << "DEBUG: Inicializando cola de prioridad" << std::endl;
            std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> pq;
            
            // Inicializar cola con el primer elemento de cada ejecución
            for (size_t i = 0; i < runs_in_this_merge; i++) {
                if (run_sizes[i] > 0) {
                    std::cout << "DEBUG: Añadiendo primer elemento de ejecución " << i 
                              << " a la cola: " << run_buffers[i][0] << std::endl;
                    pq.push({run_buffers[i][0], i});
                    buffer_positions[i] = 1;
                } else {
                    std::cout << "DEBUG: Ejecución " << i << " sin elementos, ignorando" << std::endl;
                }
            }
            
            std::cout << "DEBUG: Tamaño inicial de la cola: " << pq.size() << std::endl;
            size_t elements_merged = 0;
            
            // Realizar la unión
            while (!pq.empty()) {
                QueueElement min_element = pq.top();
                pq.pop();
                
                // Escribir el valor mínimo al archivo de salida
                output_file.writeElement(min_element.value);
                elements_merged++;
                
                if (elements_merged % 1000000 == 0) {
                    std::cout << "DEBUG: Procesados " << elements_merged << " elementos" << std::endl;
                }
                
                size_t run_idx = min_element.run_index;
                size_t run_start = (base_run + run_idx) * run_size;
                
                // Incrementar contador de elementos procesados de esta ejecución
                elements_processed[run_idx]++;
                
                // Si terminamos con el buffer actual, cargar más datos de esta ejecución
                if (buffer_positions[run_idx] >= run_buffers[run_idx].size() || 
                    buffer_positions[run_idx] >= block_size) { // Cambiado: compara con block_size
                    
                    // Si aún hay más elementos en esta ejecución, cargarlos
                    if (elements_processed[run_idx] < run_sizes[run_idx]) {
                        std::cout << "DEBUG: Recargando buffer para ejecución " << run_idx 
                                  << ", procesados hasta ahora: " << elements_processed[run_idx] 
                                  << " de " << run_sizes[run_idx] << std::endl;
                        
                        try {
                            // Calcular posición absoluta en el archivo
                            size_t absolute_position = run_start + elements_processed[run_idx];
                            input_file.seek(absolute_position);
                            std::cout << "DEBUG: Posicionado en " << absolute_position << " para recarga" << std::endl;
                            
                            // Calcular cuántos elementos quedan por leer en esta ejecución
                            size_t elements_remaining = run_sizes[run_idx] - elements_processed[run_idx];
                            size_t elements_to_read = std::min(block_size, elements_remaining);
                            
                            std::cout << "DEBUG: Intentando leer " << elements_to_read 
                                      << " elementos (quedan " << elements_remaining << ")" << std::endl;
                            
                            // Leer el siguiente bloque
                            size_t elements_read = input_file.readBlock(run_buffers[run_idx], elements_to_read);
                            
                            std::cout << "DEBUG: Leídos " << elements_read << " elementos adicionales" << std::endl;
                            
                            if (elements_read > 0) {
                                // Verificación para depuración
                                std::cout << "DEBUG: Nuevo bloque para ejecución " << run_idx << ": ";
                                for (size_t j = 0; j < std::min(elements_read, size_t(3)); j++) {
                                    std::cout << run_buffers[run_idx][j] << " ";
                                }
                                std::cout << std::endl;
                                
                                // Resetear posición en el buffer y añadir elemento a la cola
                                buffer_positions[run_idx] = 1;  // Ya usaremos el primer elemento
                                pq.push({run_buffers[run_idx][0], run_idx});
                                std::cout << "DEBUG: Añadido nuevo elemento " << run_buffers[run_idx][0] 
                                          << " de ejecución " << run_idx << " a la cola" << std::endl;
                            } else {
                                std::cout << "DEBUG: ATENCIÓN: No se pudieron leer más elementos de ejecución " 
                                          << run_idx << " (posiblemente EOF)" << std::endl;
                                
                                // Marcar esta ejecución como completada
                                elements_processed[run_idx] = run_sizes[run_idx];
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "ERROR al recargar buffer " << run_idx << ": " << e.what() << std::endl;
                            throw;
                        }
                    } else {
                        std::cout << "DEBUG: Ejecución " << run_idx << " completada, sin más elementos" << std::endl;
                    }
                } else {
                    // Todavía hay elementos en el buffer, usar el siguiente
                    int64_t next_value = run_buffers[run_idx][buffer_positions[run_idx]];
                    pq.push({next_value, run_idx});
                    buffer_positions[run_idx]++;
                    
                    if (buffer_positions[run_idx] % 100000 == 0) {
                        std::cout << "DEBUG: Posición en buffer " << run_idx << " avanzada a " 
                                  << buffer_positions[run_idx] << " (procesados: " 
                                  << elements_processed[run_idx] << ")" << std::endl;
                    }
                }
            }
            
            std::cout << "DEBUG: Merge completado, procesados " << elements_merged << " elementos" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "ERROR FATAL en mergeRuns: " << e.what() << std::endl;
            throw;
        }
    }
    
    std::cout << "DEBUG: mergeRuns finalizado exitosamente" << std::endl;
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
        std::cout << "Elements left: " << elements_left << ", read: " << elements_read << std::endl;

    }
    
    // Fase 2: Unir las ejecuciones
    // Calcular el número máximo de ejecuciones que podemos unir a la vez
    if (max_runs_to_merge == 0) {
        // Necesitamos 1 buffer de bloque para cada ejecución de entrada y 1 para la salida
        max_runs_to_merge = (memory_size / sizeof(int64_t) / block_size) - 1;
        if (max_runs_to_merge < 2) max_runs_to_merge = 2;  // Mínimo 2 ejecuciones para unir
        std::cout << "Max runs to merge: " << max_runs_to_merge << std::endl;
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
        std::cout << "Ejecuciones restantes: " << runs_created << std::endl;
        std::cout << "Tamaño de ejecución: " << run_size << std::endl;
        std::cout << "Total de elementos: " << total_elements << std::endl;
        std::cout << "Tamaño de archivo: " << file_size_bytes << std::endl;
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
        
        // Cronometrar la operación de ordenamiento
        auto start = std::chrono::high_resolution_clock::now();
        
        // Ejecutar el merge sort externo con este valor de 'a'
        externalMergeSort(temp_input, temp_output, memory_size, block_size, mid);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        std::cout << "Probado a = " << mid << ": " 
                 << (io_counter.reads + io_counter.writes) << " I/Os en " 
                 << elapsed.count() << " segundos" << std::endl;
        
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
    
    std::cout << "Valor óptimo de a: " << best_a << " con " << min_io << " I/Os" << std::endl;
    return best_a;
}

bool checkSorted(FILE* file, long input_size) {
    /* Verifica si el archivo completo está ordenado
    args:
        file: puntero al archivo que se va a verificar
        input_size: tamaño del archivo en bytes
    returns:
        true si el archivo está ordenado, false en caso contrario
    */

    long numElements = input_size / sizeof(int64_t);
    if (numElements <= 0) {
        std::cerr << "El tamaño del archivo no es suficiente para contener al menos un entero." << std::endl;
        return false;
    }

    std::vector<int64_t> buffer(numElements);
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(buffer.data(), sizeof(int64_t), numElements, file);

    /* //condición apagada para test grande
    if (bytesRead * sizeof(int64_t) != input_size) {
        cerr << "Error leyendo el archivo." << endl;
        return false;
    }
    */

    if (is_sorted(buffer.begin(), buffer.end())) {
        std::cout << "El archivo está ordenado." << std::endl;
        return true;
    } else {
        std::cout << "El archivo NO está ordenado." << std::endl;
        return false;
    }
}

/**
 * Función principal para demostrar el funcionamiento del algoritmo
 */
int main(int argc, char *argv[]) {
    // Verificar si se proporcionan argumentos
    if (argc < 4) {
        std::cout << "Uso: " << argv[0] 
                 << " <archivo_entrada> <archivo_salida> <tamano_memoria_MB> [buscar_a_optimo] [a_valor]" 
                 << std::endl;
        return 1;
    }
    
    const std::string input_file = argv[1];
    const std::string output_file = argv[2];
    size_t memory_size = static_cast<size_t>(std::stoll(argv[3])) * 1024 * 1024;  // Convertir MB a bytes
    
    // Determinar tamaño de bloque (típicamente 4KB)
    size_t block_size = 4096;
    
    // Verificar si debemos encontrar el valor óptimo de 'a'
    if (argc > 4 && std::string(argv[4]) == "find_optimal_a") {
        size_t optimal_a = findOptimalA(input_file, memory_size, block_size);
        std::cout << "Valor óptimo de a: " << optimal_a << std::endl;
        return 0;
    }
    
    // Comprobar si se proporciona un valor específico de 'a'
    size_t a_value = 0;  // 0 significa usar el cálculo por defecto
    if (argc > 4) {
        a_value = static_cast<size_t>(std::stoll(argv[4]));
    }
    
    // Resetear contador IO
    io_counter.reset();
    
    // Registrar tiempo de inicio
    auto start = std::chrono::high_resolution_clock::now();
    
    // Realizar external merge sort
    externalMergeSort(input_file, output_file, memory_size, block_size, a_value);
    
    // Registrar tiempo de finalización
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // Verificar si el archivo de salida está ordenado
    FILE* output_fp = fopen(output_file.c_str(), "rb");
    if (!output_fp) {
        std::cerr << "Error al abrir el archivo de salida." << std::endl;
        return 1;
    }
    long output_size = ftell(output_fp);
    fseek(output_fp, 0, SEEK_END);
    output_size = ftell(output_fp);
    fseek(output_fp, 0, SEEK_SET);
    bool is_sorted = checkSorted(output_fp, output_size);
    fclose(output_fp);
    if (!is_sorted) {
        std::cerr << "El archivo de salida NO está ordenado." << std::endl;
        return 1;
    }
    
    // Imprimir estadísticas
    std::cout << "Ordenamiento completado en " << elapsed.count() << " segundos" << std::endl;
    std::cout << "Total de lecturas a disco: " << io_counter.reads << std::endl;
    std::cout << "Total de escrituras a disco: " << io_counter.writes << std::endl;
    std::cout << "Total de operaciones I/O: " << (io_counter.reads + io_counter.writes) << std::endl;
    
    return 0;
}