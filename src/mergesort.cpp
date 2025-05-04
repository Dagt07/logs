#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t B = 1024; // Tamaño del bloque en bytes (revisar en enuncciado de cuanto era esto)
const size_t M = 50 * 1024 * 1024; // Memoria principal
int accesos_disco = 0;


// cachar que wea debo meter en el sizeof() (no me acuerdo que es lo que tenemos que ordenar)
size_t leer_bloque(FILE *archivo, int *buffer, size_t B){
    size_t elementos = B / sizeof(int); // número de elementos que caben en el bloque
    size_t leidos = fread(buffer, sizeof(int), elementos, archivo); 
    if (leidos > 0) accesos_disco++;
    return leidos; 
}

void escribir_bloque(FILE *archivo, int *buffer, size_t cantidad) {
    fwrite(buffer, sizeof(int), cantidad, archivo);
    if (cantidad > 0) accesos_disco++;
}



// Realiza un ordenamiento externo utilizando el algoritmo MergeSort.
void merge_sort(const char* arch_entrada, const char* arch_salida){
    // 1. Leer en bloques de tamaño B el archivo de entrada .bin
    FILE* entrada = fopen(arch_entrada, "rb");
    if (!entrada) {
        printf("Error abriendo archivo de entrada\n");
        return;
    }

    fseek(entrada,0,SEEK_END);
    size_t tamano_archivo = ftell(entrada);
    fseek(entrada,0,SEEK_SET);

    size_t num_elementos = tamano_archivo / sizeof(int);


    // 2. Dividir en a subarreglos hasta que cada uno de ellos quepa en memoria principal
    // 3. Ordenar en memoria si el tamaño es menor que M
    // 4. Recursivamente aplicar MergeSort externo a cada subarreglo
    // 5. Unir archivos ordenados.

}


