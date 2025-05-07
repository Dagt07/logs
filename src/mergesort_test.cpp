#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctime>

const size_t B_SIZE = 4096; // Tamaño del bloque en bytes (revisar en enuncciado de cuanto era esto)
const size_t M_SIZE = 50 * 1024 * 1024; // Memoria principal
int accesos_disco = 0;



void generar_arreglo_binario(const char* nombre_archivo, size_t num_elementos) {
    FILE* archivo = fopen(nombre_archivo, "wb");
    if (!archivo) {
        printf("Error al abrir el archivo para escribir\n");
        return;
    }

    std::vector<int> arreglo(num_elementos);
    
    // Semilla para números aleatorios
    std::srand(static_cast<unsigned int>(std::time(0)));

    // Llenar el arreglo con números aleatorios
    for (size_t i = 0; i < num_elementos; i++) {
        arreglo[i] = std::rand() % 100000;  // Números aleatorios entre 0 y 99999
    }

    // Escribir el arreglo en el archivo binario
    fwrite(arreglo.data(), sizeof(int), num_elementos, archivo);
    fclose(archivo);
}

// Función para leer el archivo binario y cargarlo en un vector
void leer_arreglo_binario(const char* nombre_archivo, std::vector<int>& buffer) {
    FILE* archivo = fopen(nombre_archivo, "rb");
    if (!archivo) {
        printf("Error al abrir el archivo para leer\n");
        return;
    }

    fseek(archivo, 0, SEEK_END);
    size_t tamano_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    size_t num_elementos = tamano_archivo / sizeof(int);
    buffer.resize(num_elementos);

    fread(buffer.data(), sizeof(int), num_elementos, archivo);
    fclose(archivo);
}







// Función para leer un bloque de tamaño B y cargarlo en el buffer
size_t leer_bloque(FILE* archivo, std::vector<int>& buffer) {
    size_t elementos = B_SIZE / sizeof(int);  // Número de enteros que caben en el bloque
    buffer.resize(elementos);
    size_t leidos = fread(buffer.data(), sizeof(int), elementos, archivo);  // Leer el bloque
    return leidos;
}

// Función para escribir un bloque de tamaño B desde el buffer al archivo
void escribir_bloque(FILE* archivo, const std::vector<int>& buffer, size_t cantidad) {
    fwrite(buffer.data(), sizeof(int), cantidad, archivo);
}


void generar_subarreglos_y_ordenar(const char* archivo_entrada, int a){
    FILE* entrada = fopen(archivo_entrada, "rb");
    if (!entrada) {
        printf("Error abriendo archivo de entrada\n");
        return;
    }

    // Obtener el tamaño total del archivo
    fseek(entrada, 0, SEEK_END);
    size_t tamano_archivo = ftell(entrada);
    fseek(entrada, 0, SEEK_SET);

    size_t num_elementos = tamano_archivo / sizeof(int);
    size_t elementos_por_subarreglo = num_elementos / a;

    std::vector<int> buffer;
    int contador_chunks = 0;

    // Leer en bloques, ordenar y escribir subarreglos
    while (!feof(entrada) && contador_chunks < a) {
        size_t leidos = leer_bloque(entrada, buffer);
        if (leidos == 0) break;

        std::sort(buffer.begin(), buffer.begin() + leidos);  // Ordenar en memoria

        // Crear nombre para archivo temporal
        char temp_filename[64];
        sprintf(temp_filename, "temp_%d.bin", contador_chunks++);
        
        FILE* temp = fopen(temp_filename, "wb");
        if (!temp) {
            printf("Error abriendo archivo temporal para escritura\n");
            return;
        }

        escribir_bloque(temp, buffer, leidos);  // Escribir el subarreglo ordenado
        fclose(temp);
    }

    fclose(entrada);
    printf("Generados %d subarreglos.\n", contador_chunks);
}


void merge_k_way(const char* arch_salida, int a){
    FILE* salida = fopen(arch_salida, "wb");
    if (!salida) {
        printf("Error abriendo archivo de salida\n");
        return;
    }

    std::vector<FILE*> archivos_entrada;
    std::vector<std::vector<int>> buffers(a);
    std::vector<size_t> indices(a);

    // Abrir todos los archivos temporales
    for (int i = 0; i < a; i++) {
        char temp_filename[64];
        sprintf(temp_filename, "temp_%d.bin", i);
        FILE* temp = fopen(temp_filename, "rb");
        if (!temp) {
            printf("Error abriendo archivo temporal: %s\n", temp_filename);
            return;
        }
        archivos_entrada.push_back(temp);
        indices[i] = leer_bloque(archivos_entrada[i], buffers[i]);
    }

    std::vector<int> out(B_SIZE / sizeof(int));
    size_t io = 0;

    while (true) {
        // Encuentra el mínimo de todos los archivos abiertos
        int min_val = INT_MAX;
        int min_index = -1;

        for (int i = 0; i < a; i++) {
            if (indices[i] > 0 && buffers[i][indices[i] - 1] < min_val) {
                min_val = buffers[i][indices[i] - 1];
                min_index = i;
            }
        }

        if (min_index == -1) break;  // Si todos los archivos están vacíos, terminamos

        // Escribimos el valor mínimo en el archivo de salida
        out[io++] = min_val;
        if (io == B_SIZE / sizeof(int)) {
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

    // Cerrar archivos
    for (int i = 0; i < a; i++) {
        fclose(archivos_entrada[i]);
    }

    fclose(salida);

}


/* 
Mergesort v2
1. Generación de subarreglos y ordenamiento
    - Leer archivo en bloques de tamaño B
    - Dividir el archivo en a subarreglos que quepan en M.
2. Ordeanar cada subarreglo en memoria
    - Si <= M, ordenar.
    - Si no, ser recursivo (esto creo que nunca pasa?)
3. Fusionar (merge)
    - merge con algoritmo k-way merge
*/
void merge_sort(const char* arch_entrada, const char* arch_salida, int a) {
    // Generar los subarreglos ordenados
    generar_subarreglos_y_ordenar(arch_entrada, a);

    // Fusionar los archivos temporales en un archivo ordenado
    merge_k_way(arch_salida, a);


    /*
    // Copiar los datos del archivo de entrada al archivo de salida sin modificaciones
    FILE* salida = fopen(arch_salida, "wb");
    if (!salida) {
        printf("Error al abrir archivo de salida\n");
        fclose(entrada);
        return;
    }



    // Leer y escribir el contenido del archivo sin modificarlo
    std::vector<int> buffer_(tamano_archivo / sizeof(int));
    fread(buffer_.data(), sizeof(int), buffer_.size(), entrada);
    fwrite(buffer_.data(), sizeof(int), buffer_.size(), salida);

    fclose(entrada);
    fclose(salida);
    */
    
}








int main() {
    // Generar un arreglo aleatorio y guardarlo en un archivo binario
    size_t num_elementos = 10; // Número de elementos (por ejemplo, 1 millón)
    generar_arreglo_binario("archivo_entrada.bin", num_elementos);

    // Leer y mostrar el arreglo desordenado
    std::vector<int> arreglo_entrada;
    leer_arreglo_binario("archivo_entrada.bin", arreglo_entrada);
    printf("Arreglo desordenado (primeros 10 elementos):\n");
    for (size_t i = 0; i < 10 && i < arreglo_entrada.size(); i++) {
        printf("%d ", arreglo_entrada[i]);
    }
    printf("\n");

    // Ejecutar MergeSort en el archivo binario generado (sin ordenar realmente)
    merge_sort("archivo_entrada.bin", "archivo_salida.bin", 2);

    // Leer el archivo ordenado y mostrar los primeros 10 elementos
    std::vector<int> arreglo_salida;
    leer_arreglo_binario("archivo_salida.bin", arreglo_salida);

    printf("Arreglo ordenado (primeros 10 elementos):\n");
    for (size_t i = 0; i < 10 && i < arreglo_salida.size(); i++) {
        printf("%d ", arreglo_salida[i]);
    }
    printf("\n");

    // Imprimir los accesos a disco
    printf("Accesos a disco: %d\n", accesos_disco);

    return 0;
}