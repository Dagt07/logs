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
 * Genera una secuencia pseudo‑aleatoria de N enteros de 64 bit y la escribe en
 * disco **exclusivamente** en bloques de BLOCK_SIZE bytes.
 *
 * @param N             Cantidad de enteros de 64 bit.
 * @param file_name     Nombre del .bin de salida.
 * @param BLOCK_SIZE    Tamaño de bloque (debe ser múltiplo de 8).
 */
inline void generate_sequence(std::int64_t N,
                              const std::string& file_name,
                              std::size_t BLOCK_SIZE)
{
    if (BLOCK_SIZE % sizeof(std::int64_t) != 0)
        throw std::invalid_argument("BLOCK_SIZE debe ser múltiplo de 8");

    const std::size_t ints_per_block = BLOCK_SIZE / sizeof(std::int64_t);
    std::vector<std::int64_t> buffer(ints_per_block);

    std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::int64_t> dist;

    FILE* fp = std::fopen(file_name.c_str(), "wb");
    if (!fp) throw std::runtime_error("No se pudo abrir " + file_name);

    std::int64_t written = 0;
    while (written < N)
    {
        /* Llenar bloque */
        for (std::size_t i = 0; i < ints_per_block; ++i)
            buffer[i] = dist(rng);

        /* Escribir un bloque exacto */
        if (std::fwrite(buffer.data(), BLOCK_SIZE, 1, fp) != 1)
            throw std::runtime_error("Error de escritura en " + file_name);

        written += ints_per_block;
    }

    /* Relleno: si N no es múltiplo del bloque, *sobran* ints aleatorios;
       la tarea lo permite mientras cada fwrite sea de tamaño BLOCK_SIZE. */

    std::fclose(fp);
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