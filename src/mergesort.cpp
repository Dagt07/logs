
// Pseudo Codigo

/*
4.2.1. Implementación de algoritmos
No se aceptará el uso de librerías externas para la implementación de los algoritmos. Se espera que
cada uno de los algoritmos sea implementado desde cero. Para Mergesort externo, básese en lo expuesto
en cátedra y en el apunte del curso, en la siguiente sección explicaremos cómo definir la aridad. Para
Quicksort externo, básese en la descripción del algoritmo dada en este enunciado.
4.2.2. Trabajar en disco
Para trabajar en disco, se guardará como un archivo binario (.bin) el arreglo de tamaño 𝑁 . Este archivo
se deberá leer y escribir en bloques de tamaño 𝐵. Es incorrecto realizar una lectura o escritura de tamaño
distinto a 𝐵. Para esto, se recomienda usar las funciones fread, fwrite y fseek de C. Se recomienda
también usar un buffer para evitar leer y escribir cada vez que se accede a un elemento del arreglo. El
buffer debe ser de tamaño 𝐵 es decir, tendremos en memoria principal todo el bloque solicitado.
4.2.3. Arreglo
Ya como el arreglo se encuentra en un archivo .bin en memoria secundaria, se requerirá que implementen
una función que pueda interpretar un bloque del archivo binario y llevarlo a un arreglo de números, esto
para poder trabajar en memoria principal.

4.2.5. Valores de M y B
El valor de 𝑀 para la experimentación final será de 50 MB, el valor de 𝐵 será el tamaño real de un bloque
en disco. Debido al contexto de esta tarea, no debería existir ningún número que los bits que lo representan
queden en dos bloques distintos.
*/

/*
La funcion principal tiene que recibir nombre de archivo de entrada y salida, tamaño de memoria (aridad default, calcular
mejor aridad o buscar la aridad optima)

Tamaño de Bloque lo definimos como 4KB (4096 bytes)

Tenemos que implementar un mergesort externo, el cual consiste en:
1. Revisamos si podemos implementar mergesort en memoria, si no podemos, tenemos que dividir el archivo en partes

Solo podemos tener un bloque en memoria, por lo que tenemos que leer el archivo en partes
2. Si no podemos, tenemos que dividir el archivo por la aridad recursivamente hasta que los arreglos a ordenar sean de tamaño B

3. Una vez que tenemos los arreglos de tamaño B, tenemos que escribirlos en archivos temporales
4. La union se hace leyendo secuencialmente los dos subarreglos, usando un buffer de tamaño B en memoria para
cada subarreglo y otro para el merge

5. Una vez que tenemos los subarreglos ordenados, tenemos que unirlos en un solo archivo

*/

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
#include <mergesort.hpp>


#define BLOCK_SIZE 4096 // Tamaño del bloque en bytes (4 KB)

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
    // run_index es el índice de la ejecución de entrada
    size_t run_index;

    // Operador de comparación para la cola de prioridad (min-heap)
    bool operator>(const QueueElement &other) const
    {
        return value > other.value;
    }
};

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
void mergeRuns(const std::vector<std::string> &input_files,
                const std::string &output_file,
                size_t block_size,
                size_t max_runs_to_merge)
{
    // Crear un min-heap para la unión
    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> pq;

    // Abrir los archivos de entrada
    std::vector<BufferedFile *> input_files_ptrs;
    for (const auto &file : input_files)
    {
        input_files_ptrs.push_back(new BufferedFile(file, true, block_size));
    }

    // Abrir el archivo de salida
    BufferedFile output_file_ptr(output_file, false, block_size);

    // Leer el primer elemento de cada archivo y agregarlo a la cola de prioridad
    for (size_t i = 0; i < input_files_ptrs.size(); ++i)
    {
        int64_t value;
        if (input_files_ptrs[i]->readElement(value))
        {
            pq.push({value, i});
        }
    }

    // Realizar la unión
    while (!pq.empty())
    {
        QueueElement min_element = pq.top();
        pq.pop();

        // Escribir el elemento mínimo en el archivo de salida
        output_file_ptr.writeElement(min_element.value);

        // Leer el siguiente elemento del archivo correspondiente
        int64_t next_value;
        if (input_files_ptrs[min_element.run_index]->readElement(next_value))
        {
            pq.push({next_value, min_element.run_index});
        }
    }

    // Cerrar los archivos de entrada
    for (auto file : input_files_ptrs)
    {
        delete file;
    }
}


// external merge sort
void externalMergeSort(const std::string &input_filename,
                       const std::string &output_filename,
                       size_t memory_size,
                       size_t max_runs_to_merge)
{
    // Revisar si el tamaño de memoria es suficiente para ordenar el archivo completo
    std::ifstream input_file(input_filename, std::ios::binary | std::ios::ate);
    if (!input_file)
    {
        throw std::runtime_error("No se pudo abrir el archivo de entrada: " + input_filename);
    }
    size_t file_size_bytes = input_file.tellg();
    size_t total_elements = file_size_bytes / sizeof(int64_t);
    input_file.close();
    size_t M = memory_size / sizeof(int64_t);

    if (total_elements <= M)
    {
        // Si el archivo cabe en memoria, ordenarlo directamente
        std::vector<int64_t> buffer(total_elements);
        BufferedFile in_file(input_filename, true, total_elements);
        in_file.readBlock(buffer, total_elements);
        std::sort(buffer.begin(), buffer.end());
        BufferedFile out_file(output_filename, false, total_elements);
        out_file.writeBlock(buffer, total_elements);
        return;
    }

    // Fase 1: Crear ejecuciones ordenadas iniciales
    std::vector<std::string> initial_runs_files;
    BufferedFile in_file(input_filename, true, BLOCK_SIZE);
    BufferedFile out_file("initial_runs.bin", false, BLOCK_SIZE);
    std::vector<int64_t> buffer(M);
    size_t elements_left = total_elements;
    size_t run_index = 0;
    while (elements_left > 0)
    {
        size_t run_size = std::min(elements_left, M);
        size_t read = in_file.readBlock(buffer, run_size);
        std::sort(buffer.begin(), buffer.begin() + read);
        out_file.writeBlock(buffer, read);
        initial_runs_files.push_back("run_" + std::to_string(run_index++) + ".bin");
        elements_left -= read;
    }
    out_file.flush();
    // Fase 2: Realizar múltiples pasadas de fusión
    size_t run_size = M;
    std::vector<std::string> current_run_files = initial_runs_files;
    size_t pass = 1;
    while (current_run_files.size() > 1 ||
           (current_run_files.size() == 1 && current_run_files[0] != output_filename))
    {
        std::vector<std::string> new_run_files;
        for (size_t i = 0; i < current_run_files.size(); i += max_runs_to_merge)
        {
            size_t runs_to_merge = std::min(max_runs_to_merge, current_run_files.size() - i);
            std::vector<std::string> files_to_merge(current_run_files.begin() + i,
                                                    current_run_files.begin() + i + runs_to_merge);
            std::string merged_file = "merged_" + std::to_string(pass) + "_" + std::to_string(i) + ".bin";
            mergeRuns(files_to_merge, merged_file, BLOCK_SIZE, max_runs_to_merge);
            new_run_files.push_back(merged_file);
        }
        current_run_files = new_run_files;
        run_size *= max_runs_to_merge;
        ++pass;

        // Si solo queda un archivo y hemos terminado todas las pasadas
        if (current_run_files.size() == 1)
        {
            if (current_run_files[0] != output_filename)
            {
                std::rename(current_run_files[0].c_str(), output_filename.c_str());
                current_run_files[0] = output_filename;
            }
            break;
        }
    }
    // Limpiar archivos temporales
    for (const auto &file : initial_runs_files)
    {
        std::remove(file.c_str());
    }
    for (const auto &file : current_run_files)
    {
        if (file != output_filename)
        {
            std::remove(file.c_str());
        }
    }
    std::cout << "Ordenación completada. Archivo de salida: " << output_filename << std::endl;
    std::cout << "Operaciones de I/O: Lecturas = " << io_counter.reads << ", Escrituras = " << io_counter.writes << std::endl;
    std::cout << "Tamaño de bloque: " << BLOCK_SIZE << " bytes" << std::endl;
    std::cout << "Tamaño de memoria: " << memory_size << " bytes" << std::endl;
    std::cout << "Número total de elementos: " << total_elements << std::endl;
    std::cout << "Número de ejecuciones iniciales: " << initial_runs_files.size() << std::endl;
    std::cout << "Número de pasadas de fusión: " << pass << std::endl;
}

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
