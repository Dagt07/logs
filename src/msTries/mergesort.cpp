#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctime>

const size_t B = 4096; // Tamaño del bloque en bytes (revisar en enuncciado de cuanto era esto)
const size_t M = 50 * 1024 * 1024; // Memoria principal
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

// cachar que wea debo meter en el sizeof() (no me acuerdo que es lo que tenemos que ordenar)
size_t leer_bloque(FILE *archivo, std::vector<int>& buffer){
    size_t elementos = B / sizeof(int);
    buffer.resize(elementos); 
    size_t leidos = fread(buffer.data(), sizeof(int), elementos, archivo);
    if (leidos > 0) accesos_disco++;
    return leidos;
}


void escribir_bloque(FILE* archivo, const std::vector<int>& buffer, size_t cantidad) {
    fwrite(buffer.data(), sizeof(int), cantidad, archivo);
    if (cantidad > 0) accesos_disco++;
}


bool comparar_enteros(int a, int b) {
    return a < b;
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

// Implementación de MergeSort Externo
void merge_sort(const char* arch_entrada, const char* arch_salida, int a) {
    // Abrir el archivo de entrada
    FILE* entrada = fopen(arch_entrada, "rb");
    if (!entrada) {
        printf("Error abriendo archivo de entrada\n");
        return;
    }

    fseek(entrada, 0, SEEK_END);
    size_t tamano_archivo = ftell(entrada);
    fseek(entrada, 0, SEEK_SET);

    size_t num_elementos = tamano_archivo / sizeof(int);
    size_t elementos_por_chunk = num_elementos / a;

    std::vector<int> buffer;
    int contador_chunks = 0;

    // Dividir el archivo en `a` subarreglos y ordenarlos en memoria si caben en M
    while (!feof(entrada) && contador_chunks < a) {
        size_t leidos = leer_bloque(entrada, buffer);
        if (leidos == 0) break;

        std::sort(buffer.begin(), buffer.end());

        // Usamos sprintf para convertir el contador en string y crear el nombre del archivo temporal
        char temp_filename[64];
        sprintf(temp_filename, "temp_%d.bin", contador_chunks++);  // Aquí formamos el nombre sin necesidad de convertir a string
        FILE* temp = fopen(temp_filename, "wb");
        escribir_bloque(temp, buffer, leidos);
        fclose(temp);
    }

    fclose(entrada);

    // Aquí sería donde se realiza el merge de los archivos ordenados
    // Esta parte debe ser generalizada para fusionar `a` archivos
    if (contador_chunks == 1) {
        rename("temp_0.bin", arch_salida);
        return;
    }

    // K-way merge (simplificado a solo 2 archivos por ahora)
    FILE* a1 = fopen("temp_0.bin", "rb");
    FILE* a2 = fopen("temp_1.bin", "rb");
    FILE* salida = fopen(arch_salida, "wb");

    std::vector<int> ba(B / sizeof(int)), bb(B / sizeof(int)), out(B / sizeof(int));
    size_t na = leer_bloque(a1, ba);
    size_t nb = leer_bloque(a2, bb);
    size_t ia = 0, ib = 0, io = 0;

    // Realizar el merge de los archivos
    while (na > 0 || nb > 0) {
        while (ia < na && ib < nb) {
            if (ba[ia] <= bb[ib]) {
                out[io++] = ba[ia++];
            } else {
                out[io++] = bb[ib++];
            }

            if (io == B / sizeof(int)) {
                escribir_bloque(salida, out, io);
                io = 0;
            }
        }

        if (ia == na) {
            na = leer_bloque(a1, ba);
            ia = 0;
        }
        if (ib == nb) {
            nb = leer_bloque(a2, bb);
            ib = 0;
        }
    }

    // Escribir los restos
    while (ia < na) out[io++] = ba[ia++];
    while (ib < nb) out[io++] = bb[ib++];

    if (io > 0) escribir_bloque(salida, out, io);

    fclose(a1);
    fclose(a2);
    fclose(salida);
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

    // Ejecutar MergeSort en el archivo binario generado
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