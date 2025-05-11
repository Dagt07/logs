#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <mergesort.hpp>

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