#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * Genera una secuencia pseudo‑aleatoria de N enteros de 64 bit y la escribe en
 * disco **exclusivamente** en bloques de BLOCK_SIZE bytes.
 *
 * @param N_BYTES       Cantidad total de bytes para el archivo (N enteros de 64 bit).
 * @param file_name     Nombre del .bin de salida.
 * @param BLOCK_SIZE    Tamaño de bloque en bytes (debe ser múltiplo de 8).
 */
inline void generate_sequence(std::int64_t N_BYTES,
                             const std::string& file_name,
                             std::size_t BLOCK_SIZE)
{
    if (BLOCK_SIZE % sizeof(std::int64_t) != 0)
        throw std::invalid_argument("BLOCK_SIZE debe ser múltiplo de 8");

    // Calculamos la cantidad real de elementos
    std::int64_t num_elements = N_BYTES / sizeof(std::int64_t);
    const std::size_t ints_per_block = BLOCK_SIZE / sizeof(std::int64_t);
    std::vector<std::int64_t> buffer(ints_per_block);

    // Inicializar generador de números pseudo-aleatorios
    std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::int64_t> dist;

    FILE* fp = std::fopen(file_name.c_str(), "wb");
    if (!fp) throw std::runtime_error("No se pudo abrir " + file_name);

    std::cout << "Generando archivo '" << file_name << "' con " 
              << num_elements << " elementos (" << N_BYTES << " bytes)..." << std::endl;
    
    // Para mostrar progreso
    std::int64_t step = num_elements / 10;
    if (step == 0) step = 1;

    std::int64_t elements_written = 0;
    while (elements_written < num_elements) {
        // Determinar cuántos elementos quedan por escribir en este bloque
        size_t elements_to_write = std::min(ints_per_block, 
                                           static_cast<size_t>(num_elements - elements_written));
        
        // Llenar buffer con valores aleatorios
        for (size_t i = 0; i < elements_to_write; ++i) {
            buffer[i] = dist(rng);
        }
        
        // Escribir al archivo
        size_t items_written = std::fwrite(buffer.data(), sizeof(std::int64_t), 
                                          elements_to_write, fp);
        
        if (items_written != elements_to_write)
            throw std::runtime_error("Error de escritura en " + file_name);
        
        elements_written += items_written;
        
        // Mostrar progreso
        if (elements_written % step == 0 || elements_written == num_elements) {
            std::cout << "\rGenerado " << elements_written << "/" 
                      << num_elements << " elementos..." << std::flush;
        }
    }
    
    std::fclose(fp);
    std::cout << "\nArchivo generado exitosamente." << std::endl;
}


//NO SE ESTA USANDO AHORA, SE HACE DIRECTO EN EL MAIN
/*Función que retorna un vector con los 15 tamaños N dados M en bytes   
// Vector inicializado con los multiplicadores directamente
    return {ints_in_M * 4,  ints_in_M * 8,  ints_in_M * 12, ints_in_M * 16, 
            ints_in_M * 20, ints_in_M * 24, ints_in_M * 28, ints_in_M * 32, 
            ints_in_M * 36, ints_in_M * 40, ints_in_M * 44, ints_in_M * 48, 
            ints_in_M * 52, ints_in_M * 56, ints_in_M * 60};
*/
/*
inline std::vector<std::int64_t> dataset_sizes(std::int64_t M_bytes)
{
    if (M_bytes % 8 != 0)
        throw std::invalid_argument("M_SIZE debe ser múltiplo de 8 bytes");

    const std::int64_t ints_in_M = M_bytes / 8;       // 64‑bit integers
    std::vector<std::int64_t> v;
    for (int mult = 4; mult <= 60; mult += 4)
        v.push_back(ints_in_M * mult);                 // {4M,8M,…,60M}
    return v;
}
*/