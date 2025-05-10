#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm> // For std::sort
#include "sequence_generator.hpp"
#include <queue>
#include <math.h>

#include <filesystem>
using namespace std;

#define B_SIZE 4096  // Tamaño del bloque de disco
#define M_SIZE (50 * 1024 * 1024) // Memoria principal de 50 MB

int disk_access = 0; // Contador de accesos al disco


// Funciones auxiliares que nos ayudaran más adelante.
// !!!!!! cachar que wea debo meter en el sizeof() (no me acuerdo que es lo que tenemos que ordenar)
size_t leer_bloque(FILE *archivo, std::vector<std::uint64_t>& buffer, size_t elementos_por_subarreglo) {
    buffer.resize(elementos_por_subarreglo);  // Ajustamos el tamaño del buffer según los elementos que necesitamos
    size_t leidos = fread(buffer.data(), sizeof(std::uint64_t), elementos_por_subarreglo, archivo);  // Leemos los elementos necesarios
    return leidos;
}
void escribir_bloque(FILE* archivo, const std::vector<std::uint64_t>& buffer, long offset) {
    fseek(archivo, offset * B_SIZE, SEEK_SET);
    fwrite(buffer.data(), sizeof(std::uint64_t), buffer.size(), archivo);  
}





/* 
Mergesort_v3 (este si q si)
    Te dedico a ti ayudante que revisará esta obra de arte salida de 40 horas 
    sin dormir bien y un dibujo en una servilleta.abort
    merge_sort args:
        - arch_entrada: nombre del archivo de entrada (debe ser binario)
        - arch_salida: nombre del archivo de salida (debe ser binario)
        - N: número de elementos en el archivo de entrada
        - a: número de subarreglos a crear y ordenar en memoria principal

    ideas de mejora:
        - Revisar en un principio si de una cabe en memoria principal y no divido nada.
*/

// Estructura para manejar los elementos de la min-heap
struct Element {
    std::uint64_t value;  // El valor del elemento
    int file_index;       // Índice del archivo de donde proviene el valor
    size_t position;      // Posición del elemento dentro del archivo
    
    bool operator>(const Element& other) const {
        return value > other.value;  // Para que la min-heap saque siempre el valor mínimo
    }
};

void merge_k_way() {
    std::vector<FILE*> files;  // Vector para almacenar los punteros a los archivos temporales
    std::vector<std::vector<std::uint64_t>> buffers;  // Buffers para almacenar los bloques leídos de cada archivo
    std::priority_queue<Element, std::vector<Element>, std::greater<Element>> min_heap;  // Min-heap para el merge

    // Abrir todos los archivos temporales
    size_t a = 3;  // Asumimos que hay 3 archivos temporales (esto debe ser ajustado según el número de archivos)
    for (int i = 0; i < a; ++i) {
        std::string temp_file_name = "temp_" + std::to_string(i) + ".bin";
        FILE* temp_file = fopen(temp_file_name.c_str(), "rb");
        if (!temp_file) {
            std::cerr << "Error abriendo archivo temporal " << temp_file_name << std::endl;
            return;
        }
        files.push_back(temp_file);
        buffers.push_back(std::vector<std::uint64_t>());

        // Leer el primer bloque de cada archivo
        size_t leidos = leer_bloque(temp_file, buffers[i], buffers[i].size());
        if (leidos > 0) {
            // Insertar el primer elemento del bloque en la min-heap
            min_heap.push({buffers[i][0], i, 0});
        }
    }

    // Abrir el archivo de salida
    FILE* salida = fopen("sorted_output.bin", "wb");
    if (!salida) {
        std::cerr << "Error abriendo archivo de salida" << std::endl;
        return;
    }

    // Realizar el merge k-way
    while (!min_heap.empty()) {
        // Extraer el mínimo de la min-heap
        Element min_elem = min_heap.top();
        min_heap.pop();

        // Escribir el valor mínimo en el archivo de salida
        fwrite(&min_elem.value, sizeof(std::uint64_t), 1, salida);

        // Si aún hay más elementos en el archivo de donde provino el valor mínimo, leer el siguiente elemento
        size_t next_pos = min_elem.position + 1;
        int file_index = min_elem.file_index;

        if (next_pos < buffers[file_index].size()) {
            min_heap.push({buffers[file_index][next_pos], file_index, next_pos});
        } else {
            // Si el buffer se agotó, cargar el siguiente bloque de ese archivo
            size_t leidos = leer_bloque(files[file_index], buffers[file_index], buffers[file_index].size());
            if (leidos > 0) {
                // Insertar el primer elemento del bloque recién leído en la min-heap
                min_heap.push({buffers[file_index][0], file_index, 0});
            }
        }
    }

    // Cerrar los archivos de entrada y salida
    fclose(salida);
    for (FILE* f : files) {
        fclose(f);
    }

    // Eliminar los archivos temporales
    for (int i = 0; i < a; ++i) {
        std::string temp_file_name = "temp_" + std::to_string(i) + ".bin";
        remove(temp_file_name.c_str());
    }

    std::cout << "Merge completado y archivos temporales eliminados." << std::endl;
}
// Función recursiva para generar subarreglos y ordenarlos
void generar_subarreglos_y_ordenar_recursivo(FILE* entrada, size_t tamano_archivo, size_t elementos_por_subarreglo) {
    // Condición base: si el tamaño de los elementos es pequeño y cabe en memoria, ordenar
    if (tamano_archivo <= M_SIZE) {
        std::vector<int64_t> buffer(tamano_archivo / sizeof(int64_t));
        fread(buffer.data(), sizeof(int64_t), buffer.size(), entrada);  // Leer el archivo en bloques

        std::sort(buffer.begin(), buffer.end());  // Ordenar en memoria

        // Mostrar el subarreglo ordenado
        // Mostrar el subarreglo ordenado
        std::cout << "Subarreglo ordenado: ";
        for (const auto& val : buffer) {
            std::cout << val << " ";
        }
        std::cout << std::endl;  // Imprime todo el subarreglo ordenado


        // Escribir el subarreglo ordenado en un archivo temporal
        char temp_filename[64];
        sprintf(temp_filename, "temp_sorted_%ld.bin", ftell(entrada));  // Nombre único para cada archivo temporal
        FILE* temp = fopen(temp_filename, "wb");
        if (temp) {
            fwrite(buffer.data(), sizeof(int64_t), buffer.size(), temp);  // Escribir los datos en el archivo temporal
            fclose(temp);
        }
        return;  // Fin de la recursión, ya que el archivo es lo suficientemente pequeño
    }

    // Si el tamaño es demasiado grande, dividirlo en partes más pequeñas y aplicar recursión
    std::vector<int64_t> buffer(elementos_por_subarreglo);  // Buffer para leer los subarreglos
    size_t leidos;
    
    // Mientras haya datos que leer, seguir dividiendo el archivo
    while ((leidos = fread(buffer.data(), sizeof(int64_t), elementos_por_subarreglo, entrada)) > 0) {
        std::sort(buffer.begin(), buffer.begin() + leidos);  // Ordenar en memoria

        // Crear el nombre del archivo temporal de forma única
        char temp_filename[64];
        sprintf(temp_filename, "temp_%ld.bin", ftell(entrada));  // Nombre único basado en la posición del archivo
        FILE* temp = fopen(temp_filename, "wb");
        if (temp) {
            fwrite(buffer.data(), sizeof(int64_t), leidos, temp);  // Escribir los subarreglos ordenados
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

    // Determinar el número total de elementos en el archivo
    size_t num_elementos = tamano_archivo / sizeof(int64_t);  // Tamaño del archivo dividido entre el tamaño de cada número

    // Definir cuántos elementos irán en cada subarreglo
    size_t elementos_por_subarreglo = num_elementos / a;  // Dividir los elementos en `a` subarreglos

    // Llamar a la función recursiva para dividir y ordenar los subarreglos
    generar_subarreglos_y_ordenar_recursivo(entrada, tamano_archivo, elementos_por_subarreglo);

    fclose(entrada);
    std::cout << "Subarreglos generados y ordenados.\n";
}

void split_and_fun(const char* arch_entrada, const char* arch_salida,size_t N, int a) {

    FILE* entrada = fopen(arch_entrada, "rb");
    if (!entrada) {
        printf("Error abriendo archivo de entrada\n");
        return;
    }

    fseek(entrada, 0, SEEK_END);
    size_t tamano_archivo = ftell(entrada);
    fseek(entrada, 0, SEEK_SET);

    
    // Calculamos cuántos elementos leemos por subarreglo, asegurándonos de que cada subarreglo cabe en memoria
    size_t num_elementos = N;  // La cantidad total de elementos generados
    size_t elementos_por_subarreglo = N;  // Dividimos en a subarreglos
    size_t num_subarreglos = 0;
    size_t total_leidos = 0;

    int i = 1;
    while (elementos_por_subarreglo * sizeof(std::uint64_t) > M_SIZE) {
        elementos_por_subarreglo /= a;  // Reducimos el tamaño del subarreglo hasta que quepa en memoria
        i++;
    }

    num_subarreglos = pow(a, i);  

    std::vector<std::uint64_t> buffer;  // Buffer para almacenar los valores leídos del archivo
    std::vector<std::string> temp_files(a);  // Vector para almacenar los nombres de los archivos temporales

    
        
    while (num_subarreglos < a && total_leidos < N) {
        size_t leidos = leer_bloque(entrada, buffer, elementos_por_subarreglo);  // Leemos el tamaño adecuado para cada subarreglo
        if (leidos == 0) break;  // Fin del archivo o leídos

        // Ordenar el subarreglo en memoria principal
        std::sort(buffer.begin(), buffer.begin() + leidos);

        /*
        // Mostrar el subarreglo ordenado
        std::cout << "Subarreglo " << num_subarreglos << " ordenado: ";
        for (size_t i = 0; i < leidos && i < N; ++i) {  // Solo imprimimos N elementos
            std::cout << buffer[i] << " ";
        }
        std::cout << std::endl;
        */
        

        // Guardar el subarreglo ordenado en un archivo temporal
        std::string temp_file_name = "temp_" + std::to_string(num_subarreglos) + ".bin";
        std::cout << "Guardando subarreglo " << num_subarreglos << " en " << temp_file_name << std::endl;
        FILE* temp_file = fopen(temp_file_name.c_str(), "wb+");
        if (!temp_file) {
            printf("Error abriendo archivo temporal\n");
            return;
        }
        fwrite(buffer.data(), sizeof(std::uint64_t), leidos, temp_file);  // Usamos uint64_t (8 bytes)
        fclose(temp_file);

        temp_files[num_subarreglos] = temp_file_name;
        num_subarreglos++;
        total_leidos += leidos;  // Actualizar el total de elementos leídos
    }

    std::cout << "Subarreglos generados y ordenados en memoria principal." << std::endl;
    fclose(entrada);
}

void merge_sort(const char* arch_entrada, const char* arch_salida, size_t N, int a) {
    // Generar los subarreglos ordenados
    generar_subarreglos_y_ordenar(arch_entrada, a);

    // Fusionar los archivos temporales en un archivo ordenado
    //merge_k_way();
}




bool check_sorted(FILE* file, long input_size) {
    // Calculamos cuántos enteros caben en input_size bytes
    long num_elements = input_size / sizeof(int);
    
    if (num_elements <= 0) {
        cerr << "El tamaño del archivo no es suficiente para contener al menos un entero." << endl;
        return false;
    }

    vector<int> v(num_elements);
    
    // Nos aseguramos de empezar desde el inicio del archivo
    fseek(file, 0, SEEK_SET);
    
    // Leemos la cantidad de bytes especificada
    size_t bytesRead = fread(v.data(), sizeof(int), num_elements, file);
    
    /*
    // Verificamos si se leyeron la cantidad correcta de bytes
    cout << "Esperado leer: " << input_size << " bytes." << endl;
    cout << "Leídos: " << bytesRead * sizeof(int) << " bytes." << endl;
    */

    
    if (bytesRead * sizeof(int) != input_size) {
        cerr << "Error leyendo el archivo. Se esperaban " << input_size << " bytes, pero solo se leyeron " << bytesRead * sizeof(int) << " bytes." << endl;
        return false;  // Si no se leen todos los datos correctamente, devolvemos false
    }

    // Verificamos si está ordenado
    if (is_sorted(v.begin(), v.end())) {
        cout << "El archivo está ordenado." << endl;
        return true;
    } else {
        cout << "El archivo NO está ordenado." << endl;
        return false;
    }
}

// Función para imprimir los primeros N elementos de un archivo binario
void printFirstElements(FILE* file, long elements) {//, long input_size) {
    vector<uint64_t> buffer(elements);
    fseek(file, 0, SEEK_SET);
    size_t bytesRead = fread(buffer.data(), sizeof(uint64_t), elements, file);
    
    if (bytesRead != elements) {
        cerr << "Error al leer los primeros elementos del archivo." << endl;
        exit(1);
    }

    cout << "Primeros " << elements << " elementos del archivo:" << endl;
    for (long i = 0; i < elements; ++i) {
        cout << buffer[i] << " ";
    }

    cout << endl;
      // Checking if vector v is sorted or not
    if (is_sorted(buffer.begin(), buffer.end()))
      cout << "Sorted";
    else
      cout << "Not Sorted";
    cout << endl;
    
    
}

int main(int argc, char* argv[]) {

    long N_SIZE = 20; // Este valor debe ser determinado por el archivo de entrada
    int a = 2; // Número de particiones (se puede ajustar dependiendo de M y B)
    
    // Generar la secuencia aleatoria y guardarla en "input.bin"
    generate_sequence(N_SIZE, "input.bin", B_SIZE);

    // Abrir archivo binario con el arreglo
    FILE* file = fopen("input.bin", "rb+"); // Abre el archivo en modo lectura/escritura binaria

    if (!file) {
        cerr << "Error al abrir el archivo" << endl;
        return 1;
    }

    // Imprimir los primeros X elementos antes de ordenar
    //printFirstElements(file, N_SIZE);
    //cout << "Sorted? " << check_sorted(file, N_SIZE) << endl;

    // Llamar al merge_sort externo
    merge_sort("input.bin", "sorted_output.bin", N_SIZE, a); 

    cout << "Accesos al disco: " << disk_access << endl;

    // Imprimir los primeros X elementos después de ordenar
    fseek(file, 0, SEEK_SET);  // Volver al principio del archivo para leer de nuevo
    //printFirstElements(file,N_SIZE);// N_SIZE);
    //cout << "Sorted? " << check_sorted(file, N_SIZE) << endl;

    fclose(file);
    return 0;
}