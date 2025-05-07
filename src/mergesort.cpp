#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>

const size_t B = 1024; // Tamaño del bloque en bytes (revisar en enuncciado de cuanto era esto)
const size_t M = 50 * 1024 * 1024; // Memoria principal
int accesos_disco = 0;


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

// Función para convertir int a string (sin usar std::to_string)
std::string to_string(int num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
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
    // Aquí puedes probar el merge_sort con un archivo de entrada
    merge_sort("archivo_entrada.bin", "archivo_salida.bin", 4);
    printf("Accesos a disco: %d\n", accesos_disco);
    return 0;
}