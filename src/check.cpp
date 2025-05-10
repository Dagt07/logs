#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>

#define B 1024 * 1024      // Tamaño de bloque B (1MB)

using namespace std;

typedef long long int64_t;

// Función para leer el archivo y verificar si está ordenado
bool checkIfSorted(const char* filename) {
    // Abrir el archivo binario
    FILE* file = fopen(filename, "rb");
    if (!file) {
        cerr << "Error al abrir el archivo." << endl;
        return false;
    }

    vector<int64_t> buffer(B / sizeof(int64_t));  // Buffer para almacenar un bloque de datos
    int64_t previousValue = numeric_limits<int64_t>::min();  // Iniciar con el valor más bajo posible

    // Leer y verificar el archivo en bloques
    while (fread(buffer.data(), sizeof(int64_t), buffer.size(), file) == buffer.size()) {
        // Verificar si el bloque está ordenado
        for (size_t i = 0; i < buffer.size(); ++i) {
            if (buffer[i] < previousValue) {
                // Si encontramos un valor que no está en orden, el archivo no está ordenado
                fclose(file);
                return false;
            }
            previousValue = buffer[i];  // Actualizar el valor anterior
        }
    }

    fclose(file);
    return true;  // Si no encontramos ningún error, el archivo está ordenado
}

int main() {
    const char* outputFileName = "output_4M.bin";

    // Verificar si el archivo está ordenado
    if (checkIfSorted(outputFileName)) {
        cout << "El archivo está ordenado correctamente." << endl;
    } else {
        cout << "El archivo no está ordenado correctamente." << endl;
    }

    return 0;
}
