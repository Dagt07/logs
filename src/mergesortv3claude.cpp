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
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * Estructura para mantener el seguimiento de operaciones de I/O
 * Cuenta tanto lecturas como escrituras a disco
 */
struct IOCounter
{
    size_t reads = 0;
    size_t writes = 0;

    void reset()
    {
        reads = 0;
        writes = 0;
    }
};

// Contador global para operaciones I/O
IOCounter io_counter;

/**
 * Estructura para representar un elemento en la cola de prioridad durante la unión
 */
struct QueueElement
{
    int64_t value;
    size_t run_index;

    // Operador de comparación para la cola de prioridad (min-heap)
    bool operator>(const QueueElement &other) const
    {
        return value > other.value;
    }
};

/**
 * Clase para manejar operaciones de archivo con buffer
 * Garantiza que las lecturas y escrituras sean siempre del tamaño del bloque
 */
class BufferedFile
{
private:
    std::fstream file;
    std::vector<int64_t> buffer;
    size_t buffer_pos;
    size_t buffer_size;
    bool is_read_mode;
    std::string filename;

public:
    /**
     * Constructor para BufferedFile
     * @param filename Nombre del archivo a manipular
     * @param read_mode Si es true, abre el archivo en modo lectura, si es false, en modo escritura
     * @param buffer_size Tamaño del buffer en número de elementos int64_t
     */
    BufferedFile(const std::string &filename, bool read_mode, size_t buffer_size)
        : buffer(buffer_size), buffer_pos(0), buffer_size(buffer_size), is_read_mode(read_mode), filename(filename)
    {
        // Asegurar que el directorio del archivo exista
        if (!read_mode) {
            fs::path file_path(filename);
            fs::path dir_path = file_path.parent_path();
            if (!dir_path.empty() && !fs::exists(dir_path)) {
                fs::create_directories(dir_path);
            }
        }

        if (read_mode)
        {
            file.open(filename, std::ios::in | std::ios::binary);
        }
        else
        {
            file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
        }

        if (!file)
        {
            throw std::runtime_error("Error al abrir el archivo: " + filename);
        }

        if (read_mode)
        {
            // Inicialmente el buffer está vacío para lectura
            buffer_pos = buffer_size;
        }
    }

    ~BufferedFile()
    {
        if (!is_read_mode && buffer_pos > 0)
        {
            // Escribir cualquier dato restante en el buffer
            file.write(reinterpret_cast<char *>(buffer.data()), buffer_pos * sizeof(int64_t));
            io_counter.writes++;
        }
        file.close();
    }

    /**
     * Lee un bloque completo y devuelve cuántos elementos se leyeron realmente
     * @param dest Vector destino donde se copiarán los elementos leídos
     * @param count Número de elementos a leer
     * @return Número de elementos efectivamente leídos
     */
    size_t readBlock(std::vector<int64_t> &dest, size_t count)
    {
        size_t read_count = 0;

        while (read_count < count)
        {
            // Si el buffer está vacío, llenarlo
            if (buffer_pos >= buffer_size)
            {
                file.read(reinterpret_cast<char *>(buffer.data()), buffer_size * sizeof(int64_t));
                size_t elements_read = file.gcount() / sizeof(int64_t);
                if (elements_read == 0)
                {
                    break; // Fin de archivo
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

    /**
     * Lee un solo elemento
     * @param value Referencia donde se almacenará el valor leído
     * @return true si se leyó exitosamente, false si se llegó al fin del archivo
     */
    bool readElement(int64_t &value)
    {
        if (buffer_pos >= buffer_size)
        {
            file.read(reinterpret_cast<char *>(buffer.data()), buffer_size * sizeof(int64_t));
            size_t elements_read = file.gcount() / sizeof(int64_t);
            if (elements_read == 0)
            {
                return false; // Fin de archivo
            }
            buffer_pos = 0;
            io_counter.reads++;
        }

        value = buffer[buffer_pos++];
        return true;
    }

    /**
     * Escribe un bloque completo
     * @param src Vector fuente desde donde se copiarán los elementos
     * @param count Número de elementos a escribir
     */
    void writeBlock(const std::vector<int64_t> &src, size_t count)
    {
        size_t written = 0;

        while (written < count)
        {
            // Si el buffer está lleno, vaciarlo
            if (buffer_pos >= buffer_size)
            {
                file.write(reinterpret_cast<const char *>(buffer.data()), buffer_size * sizeof(int64_t));
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

    /**
     * Escribe un solo elemento
     * @param value Valor a escribir
     */
    void writeElement(int64_t value)
    {
        if (buffer_pos >= buffer_size)
        {
            file.write(reinterpret_cast<const char *>(buffer.data()), buffer_size * sizeof(int64_t));
            buffer_pos = 0;
            io_counter.writes++;
        }

        buffer[buffer_pos++] = value;
    }

    /**
     * Posiciona el puntero del archivo
     * @param position Posición en número de elementos int64_t desde el inicio
     */
    void seek(size_t position)
    {
        // Vaciar o invalidar el buffer
        if (!is_read_mode && buffer_pos > 0)
        {
            file.write(reinterpret_cast<char *>(buffer.data()), buffer_pos * sizeof(int64_t));
            io_counter.writes++;
        }
        buffer_pos = buffer_size; // Invalidar buffer

        file.seekg(position * sizeof(int64_t), std::ios::beg);
        file.seekp(position * sizeof(int64_t), std::ios::beg);
    }

    /**
     * Obtiene la posición actual del archivo
     * @return Posición actual en número de elementos int64_t
     */
    size_t tell()
    {
        std::streampos file_pos;
        if (is_read_mode)
        {
            file_pos = file.tellg();
        }
        else
        {
            file_pos = file.tellp();
        }

        return file_pos / sizeof(int64_t);
    }

    /**
     * Vacía el buffer escribiendo el contenido pendiente al disco
     */
    void flush()
    {
        if (!is_read_mode && buffer_pos > 0)
        {
            file.write(reinterpret_cast<char *>(buffer.data()), buffer_pos * sizeof(int64_t));
            buffer_pos = 0;
            io_counter.writes++;
        }
    }

    /**
     * Devuelve el nombre del archivo
     */
    std::string getFilename() const
    {
        return filename;
    }
};

/**
 * Ordena un bloque en memoria
 * @param buffer Buffer con los elementos a ordenar
 * @param size Número de elementos a ordenar
 */
void sortBlock(std::vector<int64_t> &buffer, size_t size)
{
    std::sort(buffer.begin(), buffer.begin() + size);
}

/**
 * Une múltiples ejecuciones ordenadas utilizando una cola de prioridad
 * @param input_filename Archivo de entrada con ejecuciones ordenadas
 * @param output_filename Archivo de salida donde se guardarán las ejecuciones unidas
 * @param total_elements Número total de elementos en el archivo
 * @param run_size Tamaño de cada ejecución en número de elementos
 * @param num_runs_to_merge Número de ejecuciones a unir a la vez (aridad)
 * @param block_size Tamaño del bloque en número de elementos
 */
std::vector<std::string> mergeRuns(const std::string &input_filename,
                                   size_t total_elements,
                                   size_t run_size,
                                   size_t num_runs_to_merge,
                                   size_t block_size,
                                   size_t pass_num)
{
    size_t num_runs = (total_elements + run_size - 1) / run_size;
    std::vector<std::string> output_files;
    
    // Crear directorio temporal si no existe
    std::string temp_dir = "./temp_mergesort";
    if (!fs::exists(temp_dir)) {
        fs::create_directories(temp_dir);
    }

    for (size_t base_run = 0, group = 0; base_run < num_runs; base_run += num_runs_to_merge, ++group)
    {
        size_t runs_in_this_merge = std::min(num_runs_to_merge, num_runs - base_run);

        // Archivo de salida para este grupo - usar directorio temporal
        std::string output_filename = temp_dir + "/merge_pass" + std::to_string(pass_num) + "_group" + std::to_string(group) + ".bin";
        output_files.push_back(output_filename);

        try
        {
            // Crear múltiples archivos de entrada para cada ejecución
            std::vector<BufferedFile*> input_files;
            BufferedFile output_file(output_filename, false, block_size);

            std::vector<std::vector<int64_t>> run_buffers(runs_in_this_merge);
            std::vector<size_t> buffer_positions(runs_in_this_merge, 0);
            std::vector<size_t> elements_processed(runs_in_this_merge, 0);
            std::vector<size_t> run_sizes(runs_in_this_merge);
            std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> pq;

            // Inicializar cada archivo de entrada
            for (size_t i = 0; i < runs_in_this_merge; i++)
            {
                input_files.push_back(new BufferedFile(input_filename, true, block_size));
                
                size_t run_start = (base_run + i) * run_size;
                run_sizes[i] = std::min(run_size, total_elements - run_start);

                run_buffers[i].resize(block_size);
                input_files[i]->seek(run_start);
                size_t elements_to_read = std::min(run_sizes[i], block_size);
                size_t elements_read = input_files[i]->readBlock(run_buffers[i], elements_to_read);

                buffer_positions[i] = 0;
                elements_processed[i] = 0;

                if (elements_read > 0)
                {
                    pq.push({run_buffers[i][0], i});
                    buffer_positions[i] = 1;
                }
            }

            while (!pq.empty())
            {
                QueueElement min_element = pq.top();
                pq.pop();

                output_file.writeElement(min_element.value);
                size_t run_idx = min_element.run_index;
                elements_processed[run_idx]++;

                if (buffer_positions[run_idx] >= block_size ||
                    buffer_positions[run_idx] >= run_buffers[run_idx].size())
                {
                    if (elements_processed[run_idx] < run_sizes[run_idx])
                    {
                        // Leer más elementos de esta ejecución
                        size_t elements_remaining = run_sizes[run_idx] - elements_processed[run_idx];
                        size_t elements_to_read = std::min(block_size, elements_remaining);

                        // Importante: mantener un puntero de archivo separado para cada ejecución
                        size_t elements_read = input_files[run_idx]->readBlock(run_buffers[run_idx], elements_to_read);

                        if (elements_read > 0)
                        {
                            buffer_positions[run_idx] = 0;
                            pq.push({run_buffers[run_idx][buffer_positions[run_idx]++], run_idx});
                        }
                    }
                }
                else if (elements_processed[run_idx] < run_sizes[run_idx])
                {
                    pq.push({run_buffers[run_idx][buffer_positions[run_idx]++], run_idx});
                }
            }

            output_file.flush();
            
            // Liberar los archivos de entrada
            for (auto file : input_files) {
                delete file;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "ERROR en mergeRuns: " << e.what() << std::endl;
            throw;
        }
    }

    return output_files;
}

/**
 * Implementación del algoritmo Mergesort Externo
 * @param input_filename Archivo de entrada con datos a ordenar
 * @param output_filename Archivo de salida donde se guardarán los datos ordenados
 * @param memory_size Tamaño de memoria disponible en bytes
 * @param block_size Tamaño del bloque en número de elementos int64_t
 * @param max_runs_to_merge Número máximo de ejecuciones a unir a la vez (aridad)
 */
void externalMergeSort(const std::string &input_filename,
                       const std::string &output_filename,
                       size_t memory_size,
                       size_t block_size,
                       size_t max_runs_to_merge)
{
    // Convertir tamaño de bloque de bytes a elementos
    size_t block_size_elements = block_size / sizeof(int64_t);
    if (block_size_elements == 0) block_size_elements = 1;
    
    size_t M = memory_size / sizeof(int64_t);

    // Crear directorio temporal si no existe
    std::string temp_dir = "./temp_mergesort";
    if (!fs::exists(temp_dir)) {
        fs::create_directories(temp_dir);
    }

    std::ifstream input_file(input_filename, std::ios::binary | std::ios::ate);
    if (!input_file) {
        throw std::runtime_error("No se pudo abrir el archivo de entrada: " + input_filename);
    }
    size_t file_size_bytes = input_file.tellg();
    size_t total_elements = file_size_bytes / sizeof(int64_t);
    input_file.close();

    const std::string initial_runs_file = temp_dir + "/runs_initial.bin";

    // Fase 1: Crear ejecuciones ordenadas iniciales
    {
        BufferedFile in_file(input_filename, true, block_size_elements);
        BufferedFile out_file(initial_runs_file, false, block_size_elements);

        std::vector<int64_t> buffer(M);
        size_t elements_left = total_elements;

        while (elements_left > 0)
        {
            size_t run_size = std::min(elements_left, M);
            size_t read = in_file.readBlock(buffer, run_size);

            sortBlock(buffer, read);
            out_file.writeBlock(buffer, read);
            elements_left -= read;
        }

        out_file.flush();
    }

    // Fase 2: Realizar múltiples pasadas de fusión
    size_t run_size = M;
    std::vector<std::string> current_run_files = {initial_runs_file};
    size_t pass = 1;

    while (current_run_files.size() > 1 || 
          (current_run_files.size() == 1 && current_run_files[0] != output_filename))
    {
        std::vector<std::string> new_run_files;

        for (const std::string &run_file : current_run_files)
        {
            auto result_files = mergeRuns(run_file, total_elements, run_size, max_runs_to_merge, block_size_elements, pass);
            new_run_files.insert(new_run_files.end(), result_files.begin(), result_files.end());

            // Eliminar el archivo ya procesado si no es el original
            if (run_file != input_filename && fs::exists(run_file)) {
                fs::remove(run_file);
            }
        }

        current_run_files = new_run_files;
        run_size *= max_runs_to_merge;
        ++pass;

        // Si solo queda un archivo y hemos terminado todas las pasadas
        if (current_run_files.size() == 1) {
            if (current_run_files[0] != output_filename) {
                // Copiar el contenido en lugar de renombrar, para manejar dispositivos diferentes
                std::ifstream src(current_run_files[0], std::ios::binary);
                std::ofstream dst(output_filename, std::ios::binary | std::ios::trunc);
                if (!src || !dst) {
                    throw std::runtime_error("Error al copiar el archivo final: " + current_run_files[0] + " -> " + output_filename);
                }
                
                dst << src.rdbuf();
                src.close();
                dst.close();
                
                if (fs::exists(current_run_files[0])) {
                    fs::remove(current_run_files[0]);
                }
                
                current_run_files[0] = output_filename;
            }
            break;
        }
    }
    
    // Limpiar directorio temporal
    try {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Advertencia: No se pudo eliminar el directorio temporal: " << e.what() << std::endl;
    }
}

/**
 * Encuentra el valor óptimo de 'a' (aridad) para Mergesort externo
 * @param input_filename Archivo de entrada para realizar las pruebas
 * @param memory_size Tamaño de memoria disponible en bytes
 * @param block_size Tamaño del bloque en número de elementos int64_t
 * @return El valor óptimo de 'a'
 */
size_t findOptimalA(const std::string &input_filename, size_t memory_size, size_t block_size)
{
    std::cout << "Buscando aridad óptima para Mergesort externo..." << std::endl;

    // Determinar el tamaño total del archivo
    std::ifstream input_file(input_filename, std::ios::binary | std::ios::ate);
    if (!input_file)
    {
        throw std::runtime_error("No se pudo abrir el archivo de entrada: " + input_filename);
    }

    size_t file_size_bytes = input_file.tellg();
    size_t total_elements = file_size_bytes / sizeof(int64_t);
    input_file.close();

    // Número máximo de enteros de 64 bits en un bloque
    size_t b = block_size / sizeof(int64_t);
    if (b == 0)
        b = 1; // Asegurar al menos 1 elemento por bloque

    // Número máximo teórico de ejecuciones que podemos unir a la vez
    size_t max_fanout = (memory_size / (sizeof(int64_t) * block_size)) - 1;
    if (max_fanout < 2)
        max_fanout = 2;

    std::cout << "Fanout máximo teórico: " << max_fanout << std::endl;

    // Búsqueda binaria para 'a' óptimo
    size_t left = 2;
    size_t right = std::min(b, max_fanout); // No tiene sentido probar más que el máximo fanout
    size_t best_a = 2;
    size_t min_io = std::numeric_limits<size_t>::max();

    std::cout << "Iniciando búsqueda binaria entre a=2 y a=" << right << std::endl;

    // Para cada valor candidato de 'a', ejecutar el algoritmo y medir IOs
    while (left <= right)
    {
        size_t mid = left + (right - left) / 2;

        // Resetear contador IO
        io_counter.reset();

        // Crear una copia del archivo de entrada para preservar el original
        const std::string temp_input = "./temp_mergesort/temp_input_copy_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".bin";
        {
            // Asegurar que el directorio existe
            std::string temp_dir = "./temp_mergesort";
            if (!fs::exists(temp_dir)) {
                fs::create_directories(temp_dir);
            }
            
            std::ifstream src(input_filename, std::ios::binary);
            std::ofstream dst(temp_input, std::ios::binary);
            if (!src || !dst) {
                throw std::runtime_error("Error al crear archivos temporales para la prueba");
            }
            dst << src.rdbuf();
        }

        // Ejecutar Merge Sort con este valor de 'a'
        const std::string temp_output = "./temp_mergesort/temp_output_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".bin";

        std::cout << "Probando a = " << mid << "... " << std::flush;

        // Cronometrar la operación de ordenamiento
        auto start = std::chrono::high_resolution_clock::now();

        try
        {
            // Ejecutar el merge sort externo con este valor de 'a'
            externalMergeSort(temp_input, temp_output, memory_size, block_size, mid);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;

            size_t total_ios = io_counter.reads + io_counter.writes;

            std::cout << total_ios << " I/Os en " << elapsed.count() << " segundos" << std::endl;

            // Comprobar si este 'a' es mejor
            if (total_ios < min_io)
            {
                min_io = total_ios;
                best_a = mid;
                std::cout << "  -> Nuevo mejor a = " << best_a << " con " << min_io << " I/Os" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error durante la prueba con a = " << mid << ": " << e.what() << std::endl;
        }

        // Limpiar archivos temporales
        try {
            if (fs::exists(temp_input)) fs::remove(temp_input);
            if (fs::exists(temp_output)) fs::remove(temp_output);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Advertencia: No se pudo eliminar archivos temporales: " << e.what() << std::endl;
        }

        // Continuar búsqueda binaria
        if (min_io == io_counter.reads + io_counter.writes)
        {
            // Si el rendimiento mejora o se mantiene igual, probar un 'a' mayor
            left = mid + 1;
        }
        else
        {
            // Si el rendimiento empeoró, probar un 'a' menor
            right = mid - 1;
        }
    }

    std::cout << "RESULTADO FINAL: Valor óptimo de a: " << best_a << " con " << min_io << " I/Os" << std::endl;
    return best_a;
}

/**
 * Verifica si un archivo está ordenado
 * @param filename Nombre del archivo a verificar
 * @return true si el archivo está ordenado, false en caso contrario
 */
bool checkSorted(const std::string &filename)
{
    // Abrir el archivo y determinar su tamaño
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::cerr << "Error al abrir el archivo: " << filename << std::endl;
        return false;
    }

    size_t file_size_bytes = file.tellg();
    size_t total_elements = file_size_bytes / sizeof(int64_t);

    if (total_elements <= 0)
    {
        std::cerr << "El archivo está vacío o no contiene enteros de 64 bits." << std::endl;
        return false;
    }

    // Volver al inicio del archivo
    file.seekg(0, std::ios::beg);

    // Leer los elementos de a pares para verificar que estén ordenados
    int64_t prev_value, current_value;

    // Leer el primer valor
    file.read(reinterpret_cast<char *>(&prev_value), sizeof(int64_t));

    // Verificar el resto de valores
    for (size_t i = 1; i < total_elements; i++)
    {
        file.read(reinterpret_cast<char *>(&current_value), sizeof(int64_t));

        if (current_value < prev_value)
        {
            std::cout << "Archivo NO ordenado: elemento " << i << " (" << current_value
                      << ") < elemento " << (i - 1) << " (" << prev_value << ")" << std::endl;
            return false;
        }

        prev_value = current_value;
    }

    std::cout << "Verificación completada: El archivo está correctamente ordenado." << std::endl;
    return true;
}

/**
 * Verifica si dos archivos contienen exactamente los mismos números
 * (aunque estén en orden diferente)
 */
bool checkSameContentsSortedCompare(const std::string &input_filename, const std::string &output_filename)
{
    std::ifstream input_file(input_filename, std::ios::binary);
    std::ifstream output_file(output_filename, std::ios::binary);

    if (!input_file || !output_file)
    {
        std::cerr << "Error al abrir los archivos: " << input_filename << " o " << output_filename << std::endl;
        return false;
    }

    std::vector<int64_t> input_values;
    std::vector<int64_t> output_values;
    int64_t value;

    // Leer archivo de entrada
    while (input_file.read(reinterpret_cast<char *>(&value), sizeof(int64_t)))
    {
        input_values.push_back(value);
    }

    // Leer archivo de salida
    while (output_file.read(reinterpret_cast<char *>(&value), sizeof(int64_t)))
    {
        output_values.push_back(value);
    }

    // Ordenar ambos vectores
    std::sort(input_values.begin(), input_values.end());
    std::sort(output_values.begin(), output_values.end());

    // Comparar vectores ordenados
    if (input_values == output_values)
    {
        std::cout << "Ambos archivos contienen los mismos números (comparación tras ordenar)." << std::endl;
        return true;
    }
    else
    {
        std::cout << "Los archivos contienen diferentes números (después de ordenar)." << std::endl;
        return false;
    }
}

/**
 * Función principal para demostrar el funcionamiento del algoritmo
 */
int main(int argc, char *argv[])
{
    // Verificar si se proporcionan argumentos
    if (argc < 4)
    {
        std::cout << "Uso: " << argv[0]
                  << " <archivo_entrada> <archivo_salida> <tamano_memoria_MB> [buscar_a_optimo] [a_valor]"
                  << std::endl;
        return 1;
    }

    const std::string input_file = argv[1];
    const std::string output_file = argv[2];
    size_t memory_size_mb = std::stoul(argv[3]);
    size_t memory_size = memory_size_mb * 1024 * 1024; // Convertir a bytes
    size_t block_size = 1024 * 1024;                   // Tamaño del bloque en bytes (1 MB)

    // Verificar si se debe buscar el valor óptimo de 'a'
    bool find_optimal_a = (argc > 4 && std::string(argv[4]) == "true");
    size_t optimal_a = 0;

    if (find_optimal_a)
    {
        optimal_a = findOptimalA(input_file, memory_size, block_size);
        std::cout << "Valor óptimo de 'a' encontrado: " << optimal_a << std::endl;
    }
    else
    {
        if (argc > 5)
        {
            optimal_a = std::stoul(argv[5]);
            std::cout << "Usando valor de 'a' proporcionado: " << optimal_a << std::endl;
        }
        else
        {
            optimal_a = 2; // Valor por defecto
            std::cout << "Usando valor por defecto de 'a': " << optimal_a << std::endl;
        }
    }

    // Ejecutar el algoritmo de ordenamiento externo
    try
    {
        externalMergeSort(input_file, output_file, memory_size, block_size, optimal_a);

        // Verificar si el archivo de salida está ordenado
        if (checkSorted(output_file))
        {
            std::cout << "El archivo de salida está correctamente ordenado." << std::endl;
        }
        else
        {
            std::cerr << "El archivo de salida NO está ordenado." << std::endl;
        }

        if (checkSameContentsSortedCompare(input_file, output_file))
        {
            std::cout << "Los archivos de entrada y salida contienen los mismos números." << std::endl;
        }
        else
        {
            std::cerr << "Los archivos de entrada y salida NO contienen los mismos números." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error durante la ejecución: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
