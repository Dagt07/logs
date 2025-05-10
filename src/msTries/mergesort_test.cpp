#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctime>


#include <fstream>
#include <cstdlib>
#include <filesystem>
using namespace std;


const size_t B_SIZE = 4096; // Tamaño del bloque en bytes (revisar en enuncciado de cuanto era esto)
const size_t M_SIZE = 50 * 1024 * 1024; // Memoria principal
int accesos_disco = 0;


//Función para el test
void generar_arreglo_binario(const char* nombre_archivo, size_t num_elementos) {
    FILE* archivo = fopen(nombre_archivo, "wb");
    if (!archivo) {
        printf("Error al abrir el archivo para escribir\n");
        return;
    }

    std::vector<int64_t> arreglo(num_elementos);
    
    // Semilla para números aleatorios
    std::srand(static_cast<unsigned int>(std::time(0)));

    // Llenar el arreglo con números aleatorios
    for (size_t i = 0; i < num_elementos; i++) {
        arreglo[i] = std::rand() % 100000;  // Números aleatorios entre 0 y 99999
    }

    // Escribir el arreglo en el archivo binario
    fwrite(arreglo.data(), sizeof(int64_t), num_elementos, archivo);
    fclose(archivo);
}

//Función para el test
// Función para leer el archivo binario y cargarlo en un vector
void leer_arreglo_binario(const char* nombre_archivo, std::vector<int64_t>& buffer) {
    FILE* archivo = fopen(nombre_archivo, "rb");
    if (!archivo) {
        printf("Error al abrir el archivo para leer\n");
        return;
    }

    fseek(archivo, 0, SEEK_END);
    size_t tamano_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    size_t num_elementos = tamano_archivo / sizeof(int64_t);
    buffer.resize(num_elementos);

    fread(buffer.data(), sizeof(int64_t), num_elementos, archivo);
    fclose(archivo);
}

// lee un bloque de tamaño B y cargarlo en el buffer
size_t leer_bloque(FILE* archivo, std::vector<int64_t>& buffer) {
    size_t elementos = B_SIZE / sizeof(int64_t);  // Número de enteros que caben en el bloque
    buffer.resize(elementos);
    size_t leidos = fread(buffer.data(), sizeof(int64_t), elementos, archivo);  // Leer el bloque
    if (leidos > 0) {
        accesos_disco++;  
    }
    return leidos;
}

// escribe un bloque de tamaño B desde el buffer al archivo
void escribir_bloque(FILE* archivo, const std::vector<int64_t>& buffer, size_t cantidad) {
    fwrite(buffer.data(), sizeof(int64_t), cantidad, archivo);
    if (cantidad > 0) {
        accesos_disco++;  
    }
}

// Función para verificar si el archivo está ordenado (mayor a menor)
bool check_sorted(FILE* file, long input_size) {
    long num_elements = input_size / sizeof(int64_t);
    
    if (num_elements <= 0) {
        std::cerr << "El tamaño del archivo no es suficiente para contener al menos un entero." << std::endl;
        return false;
    }

    std::vector<int64_t> prev_buffer;
    std::vector<int64_t> curr_buffer;
    
    // Nos aseguramos de empezar desde el inicio del archivo
    fseek(file, 0, SEEK_SET);
    
    size_t bytesRead = leer_bloque(file, curr_buffer);  // Leer el primer bloque
    if (bytesRead == 0) return false;  // Si no leemos nada, retornamos false

    // Establecer el primer bloque como "anterior" para la comparación
    prev_buffer = curr_buffer;

    // Leer el archivo en bloques y comparar elementos consecutivos
    while (bytesRead > 0) {
        // Leer el siguiente bloque
        bytesRead = leer_bloque(file, curr_buffer);
        
        // Si el bloque actual tiene elementos, lo comparamos con el bloque anterior
        if (bytesRead > 0) {
            // Comparar el último elemento del bloque anterior con el primer elemento del bloque actual
            if (prev_buffer.back() < curr_buffer.front()) {
                std::cout << "El archivo NO está ordenado." << std::endl;
                return false;  // Si encontramos que no está ordenado, retornamos false
            }
        }

        // Actualizar el bloque anterior
        prev_buffer = curr_buffer;
    }

    std::cout << "El archivo está ordenado." << std::endl;
    return true;
}


//La magia: 

// Función recursiva para generar subarreglos y ordenarlos
void generar_subarreglos_y_ordenar_recursivo(FILE* entrada, size_t tamano_archivo, size_t elementos_por_subarreglo) {
    // Condición base: si el tamaño de los elementos es pequeño y cabe en memoria, ordenar
    if (tamano_archivo <= M_SIZE) {
        std::vector<int64_t> buffer(tamano_archivo / sizeof(int64_t));
        fread(buffer.data(), sizeof(int64_t), buffer.size(), entrada);
        std::sort(buffer.begin(), buffer.end());  // Ordenar en memoria
        // Escribir el subarreglo ordenado
        char temp_filename[64];
        sprintf(temp_filename, "temp_sorted.bin");
        FILE* temp = fopen(temp_filename, "wb");
        if (temp) {
            fwrite(buffer.data(), sizeof(int64_t), buffer.size(), temp);
            fclose(temp);
        }
        return;
    }

    // Si el tamaño es demasiado grande, dividirlo en partes más pequeñas y aplicar recursión
    std::vector<int64_t> buffer(elementos_por_subarreglo);
    size_t leidos;
    while ((leidos = leer_bloque(entrada, buffer)) > 0) {
        std::sort(buffer.begin(), buffer.begin() + leidos);  // Ordenar en memoria
        char temp_filename[64];
        sprintf(temp_filename, "temp_%ld.bin", ftell(entrada)); // Utilizando la posición de archivo para el nombre
        FILE* temp = fopen(temp_filename, "wb");
        if (temp) {
            fwrite(buffer.data(), sizeof(int64_t), leidos, temp);
            fclose(temp);
        }
    }
}

// Función para dividir el archivo en `a` subarreglos, ordenarlos y guardarlos en archivos temporales
void generar_subarreglos_y_ordenar(const char* archivo_entrada, int a) {
    FILE* entrada = fopen(archivo_entrada, "rb");
    if (!entrada) {
        std::cerr << "Error abriendo archivo de entrada\n";
        return;
    }

    // Obtener el tamaño total del archivo
    fseek(entrada, 0, SEEK_END);
    size_t tamano_archivo = ftell(entrada);
    fseek(entrada, 0, SEEK_SET);

    size_t num_elementos = tamano_archivo / sizeof(int64_t);
    size_t elementos_por_subarreglo = num_elementos / a;

    // Llamar a la función recursiva para dividir y ordenar los subarreglos
    generar_subarreglos_y_ordenar_recursivo(entrada, tamano_archivo, elementos_por_subarreglo);

    fclose(entrada);
    std::cout << "Subarreglos generados y ordenados.\n";
}

// Función de merge k-way (fusionar múltiples archivos)
void merge_k_way(const char* arch_salida, int a) {
    FILE* salida = fopen(arch_salida, "wb");
    if (!salida) {
        std::cerr << "Error abriendo archivo de salida\n";
        return;
    }

    std::vector<FILE*> archivos_entrada;
    std::vector<std::vector<int64_t>> buffers(a);
    std::vector<size_t> indices(a);

    // Abrir todos los archivos temporales
    for (int i = 0; i < a; i++) {
        char temp_filename[64];
        sprintf(temp_filename, "temp_%d.bin", i);
        FILE* temp = fopen(temp_filename, "rb");
        if (!temp) {
            std::cerr << "Error abriendo archivo temporal: " << temp_filename << "\n";
            return;
        }
        archivos_entrada.push_back(temp);
        indices[i] = leer_bloque(archivos_entrada[i], buffers[i]);
    }

    std::vector<int64_t> out(B_SIZE / sizeof(int64_t)); // Buffer para los datos fusionados
    size_t io = 0;

    // Realizamos el merge de todos los archivos
    while (true) {
        int min_val = INT_MAX;
        int min_index = -1;

        // Encontramos el valor mínimo entre todos los archivos abiertos
        for (int i = 0; i < a; i++) {
            if (indices[i] > 0 && buffers[i][indices[i] - 1] < min_val) {
                min_val = buffers[i][indices[i] - 1];
                min_index = i;
            }
        }

        if (min_index == -1) break;  // Si todos los archivos están vacíos, terminamos

        // Escribir el valor mínimo en el archivo de salida
        out[io++] = min_val;
        if (io == B_SIZE / sizeof(int64_t)) {
            escribir_bloque(salida, out, io);
            io = 0;
        }

        // Avanzar el índice en el archivo correspondiente
        indices[min_index]--;
        if (indices[min_index] == 0) {
            indices[min_index] = leer_bloque(archivos_entrada[min_index], buffers[min_index]);
        }
    }

    // Escribir los restos
    if (io > 0) escribir_bloque(salida, out, io);

    // Cerrar archivos temporales
    for (int i = 0; i < a; i++) {
        fclose(archivos_entrada[i]);
    }

    fclose(salida);
}

// Función principal de MergeSort Externo
void merge_sort(const char* arch_entrada, const char* arch_salida, int a) {
    // Generar los subarreglos ordenados
    generar_subarreglos_y_ordenar(arch_entrada, a);

    // Fusionar los archivos temporales en un archivo ordenado
    merge_k_way(arch_salida, a);
}


int main() {
    // Generar un arreglo aleatorio y guardarlo en un archivo binario
    size_t num_elementos = 20; // Número de elementos (por ejemplo, 1 millón)
    generar_arreglo_binario("archivo_entrada.bin", num_elementos);

    // Leer y mostrar el arreglo desordenado
    std::vector<int64_t> arreglo_entrada;
    leer_arreglo_binario("archivo_entrada.bin", arreglo_entrada);
    printf("Arreglo desordenado (primeros 10 elementos):\n");
    for (size_t i = 0; i < num_elementos && i < arreglo_entrada.size(); i++) {
        printf("%d ", arreglo_entrada[i]);
    }
    printf("\n");

    // Ejecutar MergeSort en el archivo binario generado (sin ordenar realmente)
    merge_sort("archivo_entrada.bin", "archivo_salida.bin", 2);

    // Leer el archivo ordenado y mostrar los primeros 10 elementos
    std::vector<int64_t> arreglo_salida;
    leer_arreglo_binario("archivo_salida.bin", arreglo_salida);

    printf("Arreglo ordenado (primeros 10 elementos):\n");
    for (size_t i = 0; i < num_elementos && i < arreglo_salida.size(); i++) {
        printf("%d ", arreglo_salida[i]);
    }
    printf("\n");

    // Imprimir los accesos a disco
    printf("Accesos a disco: %d\n", accesos_disco);

    // Verificar si el archivo está ordenado (mayor a menor)
    FILE* archivo_salida = fopen("archivo_salida.bin", "rb");
    if (archivo_salida) {
        fseek(archivo_salida, 0, SEEK_END);
        long input_size = ftell(archivo_salida);
        check_sorted(archivo_salida, input_size);
        fclose(archivo_salida);
    } else {
        std::cerr << "Error al abrir el archivo de salida para verificar el orden." << std::endl;
    }
    

    return 0;
}

