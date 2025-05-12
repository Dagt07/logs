#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <cstdio>
#include <cstdlib>
#include <climits> // Para LLONG_MAX
#include <cstring> // Para memcpy
#include <string>  // Para std::string y stoi
#include <cmath>   // Para floor
#include <ctime>   // Para time() en srand

using namespace std;

// --- Constantes y Configuraciones (AJUSTAR ESTOS VALORES) ---
// M: Tamaño de la memoria principal disponible para ordenar un tramo (en bytes)
const long long M_BYTES = 50 * 1024 * 1024; // 50 MB según la tarea

// B: Tamaño del bloque de disco (en bytes)
// Deberías determinar el tamaño real de un bloque en disco o usar uno asignado para la tarea.
// Por ejemplo, 4KB o un valor que te indiquen.
const int B_BYTES = 4 * 1024; // Ejemplo: 4KB

// Calcula cuántos long long caben en M y B
const int RUN_SIZE_ELEMENTS = M_BYTES / sizeof(long long); // Elementos por tramo en memoria
const int BLOCK_SIZE_ELEMENTS = B_BYTES / sizeof(long long); // Elementos por bloque de disco

// Estructura para el MinHeap
struct MinHeapNode {
    long long element; // Elemento a almacenar (64 bits)
    int i;             // Índice del archivo/tramo del cual se tomó el elemento
    FILE* file_ptr;    // Puntero al archivo para leer el siguiente bloque si es necesario
    vector<long long> buffer; // Buffer de entrada para este tramo (tamaño B)
    int buffer_pos;    // Posición actual en el buffer
    long long elements_remaining_in_file; // Para saber si hay más elementos en el archivo de este tramo
    size_t current_buffer_size; // Tamaño real del buffer actual (puede ser < B al final)
};

// Comparador para el MinHeap
struct CompareMinHeapNode {
    bool operator()(MinHeapNode const& a, MinHeapNode const& b) {
        return a.element > b.element;
    }
};

// Prototipos
void mergeSortInMemory(vector<long long>& arr);
FILE* openFile(const char* fileName, const char* mode);
long long countIO = 0; // Contador global de operaciones de E/S

// --- Implementación de Merge Sort en Memoria (sin cambios) ---
void merge(vector<long long>& arr, int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;
    if (n1 <= 0 || n2 <= 0) return; // Evitar asignaciones de tamaño cero o negativo
    vector<long long> L(n1), R(n2);
    memcpy(L.data(), arr.data() + l, n1 * sizeof(long long));
    memcpy(R.data(), arr.data() + m + 1, n2 * sizeof(long long));

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) arr[k++] = L[i++];
        else arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
}

void mergeSortRecursive(vector<long long>& arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortRecursive(arr, l, m);
        mergeSortRecursive(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

void mergeSortInMemory(vector<long long>& arr) {
    if (!arr.empty()) {
        mergeSortRecursive(arr, 0, arr.size() - 1);
    }
}
// --- Fin de Merge Sort en Memoria ---


FILE* openFile(const char* fileName, const char* mode) {
    FILE* fp = fopen(fileName, mode);
    if (fp == NULL) {
        perror(("Error opening file: " + string(fileName)).c_str());
        exit(EXIT_FAILURE);
    }
    return fp;
}

// --- Implementación de createInitialRuns (con mejoras menores y log) ---
int createInitialRuns(const char* input_file, int num_ways, int run_size_elements, int block_size_elements) {
    FILE* in = openFile(input_file, "rb"); // Leer en modo binario

    fseek(in, 0, SEEK_END);
    long long total_file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    long long total_elements_in_file = total_file_size / sizeof(long long);

    if (total_elements_in_file == 0) {
         cout << "Input file is empty. No runs to create." << endl;
         fclose(in);
         return 0; // No hay tramos que crear
    }

    int actual_runs_created = 0;
    vector<FILE*> out_files; // Se crearán dinámicamente según sea necesario

    vector<long long> run_buffer(run_size_elements); // Buffer para un tramo completo en memoria (M)
    vector<long long> block_read_buffer(block_size_elements); // Buffer para leer un bloque (B)

    bool more_input = true;
    long long total_elements_processed = 0;
    int current_run_index = 0; // Índice del tramo actual que se está creando

    cout << "Creating initial runs..." << endl;
    cout << "Total elements to process: " << total_elements_in_file << endl;

    while (more_input && total_elements_processed < total_elements_in_file) {
        int elements_in_current_run = 0;

        // Llenar el buffer de memoria (run_buffer) hasta run_size_elements o EOF
        while (elements_in_current_run < run_size_elements && total_elements_processed < total_elements_in_file) {
            long long elements_to_read_this_block = min((long long)block_size_elements, total_elements_in_file - total_elements_processed);
            elements_to_read_this_block = min(elements_to_read_this_block, (long long)run_size_elements - elements_in_current_run);

            if (elements_to_read_this_block <= 0) {
                 break; // Buffer de memoria lleno o no quedan elementos en el archivo
            }

            size_t read_count = fread(block_read_buffer.data(), sizeof(long long), elements_to_read_this_block, in);
            countIO++; // Contar lectura

            if (read_count == 0) {
                if (ferror(in)) {
                    perror("Error reading input file during run creation");
                    // Limpieza parcial antes de salir
                    fclose(in);
                    for(FILE* f : out_files) if (f) fclose(f);
                    // Podrías querer eliminar los archivos temporales creados hasta ahora
                    exit(EXIT_FAILURE);
                }
                more_input = false; // EOF alcanzado
                break;
            }

            // Copiar del buffer de bloque al buffer de tramo
            memcpy(run_buffer.data() + elements_in_current_run, block_read_buffer.data(), read_count * sizeof(long long));
            elements_in_current_run += read_count;
            total_elements_processed += read_count;

             // Si fread leyó menos de lo esperado (y no hubo error), es EOF
            if (read_count < (size_t)elements_to_read_this_block) {
                more_input = false;
            }
        }


        if (elements_in_current_run > 0) {
            // Redimensionar el vector a los elementos realmente leídos para este tramo
            vector<long long> current_run_data(run_buffer.begin(), run_buffer.begin() + elements_in_current_run);

            // Ordenar el tramo en memoria
            mergeSortInMemory(current_run_data);

            // Abrir el archivo de salida temporal para este tramo
            char fileName[64]; // Aumentado tamaño por si acaso
            snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", current_run_index);
            FILE* current_out_file = openFile(fileName, "wb");
            out_files.push_back(current_out_file); // Guardar para cerrar después

            // Escribir el tramo ordenado al archivo temporal, por bloques B
            int elements_written_in_run = 0;
            vector<long long> block_write_buffer(block_size_elements); // Buffer para escribir bloques B

            while (elements_written_in_run < elements_in_current_run) {
                long long elements_to_write_this_block = min((long long)block_size_elements, (long long)elements_in_current_run - elements_written_in_run);
                if (elements_to_write_this_block <= 0) break;

                // Copiar al buffer de escritura
                memcpy(block_write_buffer.data(), current_run_data.data() + elements_written_in_run, elements_to_write_this_block * sizeof(long long));

                size_t write_count = fwrite(block_write_buffer.data(), sizeof(long long), elements_to_write_this_block, current_out_file);
                countIO++; // Contar escritura

                if (write_count < (size_t)elements_to_write_this_block) {
                     perror("Error writing to temporary file");
                     // Limpieza
                     fclose(in);
                     for(FILE* f : out_files) if (f) fclose(f);
                     // Eliminar archivos temporales
                     exit(EXIT_FAILURE);
                }
                elements_written_in_run += elements_to_write_this_block;
            }
            // fclose(current_out_file); // Se cerrarán todos al final
            actual_runs_created++;
            current_run_index++;
        }
         // Verificar si ya procesamos todo explícitamente
         if (total_elements_processed >= total_elements_in_file) {
            more_input = false;
         }
    }

    // Cerrar todos los archivos abiertos
    fclose(in);
    for (FILE* f : out_files) {
        if (f) fclose(f);
    }

    cout << "Finished creating initial runs. Number of runs = " << actual_runs_created << endl;
    return actual_runs_created; // Devolver el número real de tramos creados
}
// --- Fin de createInitialRuns ---


// --- Implementación de mergeFiles (con mejoras menores y log) ---
void mergeFiles(const char* output_file_name, int num_runs, int block_size_elements) {
    if (num_runs <= 0) {
        cout << "No runs to merge. Creating empty output file." << endl;
        FILE* out = openFile(output_file_name, "wb"); // Crear archivo vacío si no hay tramos
        fclose(out);
        return;
    }
     if (num_runs == 1) {
        cout << "Only one run created. Renaming temporary file to output file." << endl;
        char tempFileName[64];
        snprintf(tempFileName, sizeof(tempFileName), "temp_run_%d.bin", 0);
        // Renombrar o copiar el archivo temporal al archivo final
        // Renombrar es más eficiente pero puede fallar entre diferentes sistemas de archivos.
        // Copiar es más seguro pero implica más I/O. Usaremos rename.
        if (rename(tempFileName, output_file_name) != 0) {
            perror("Error renaming temporary file to output file. Trying copy instead.");
            // Implementación de copia (leer temp, escribir output) como fallback si es necesario
             FILE* in = openFile(tempFileName, "rb");
             FILE* out = openFile(output_file_name, "wb");
             vector<char> buffer(B_BYTES); // Usar B_BYTES como tamaño de buffer de copia
             size_t bytesRead;
             while ((bytesRead = fread(buffer.data(), 1, buffer.size(), in)) > 0) {
                 countIO++; // Cuenta lectura
                 if (fwrite(buffer.data(), 1, bytesRead, out) != bytesRead) {
                     perror("Error writing during file copy");
                     fclose(in); fclose(out); remove(tempFileName); // Limpieza
                     exit(EXIT_FAILURE);
                 }
                 countIO++; // Cuenta escritura
             }
             fclose(in);
             fclose(out);
             remove(tempFileName); // Eliminar el temporal después de copiar
        }
        // Si rename tuvo éxito, el I/O es mínimo (solo metadatos), no lo contamos significativamente.
        // Si se hizo la copia, ya se contó dentro del bucle.
        cout << "Merge phase skipped (only one run)." << endl;
        return;
    }


    cout << "Starting merge phase for " << num_runs << " runs..." << endl;
    FILE* out = openFile(output_file_name, "wb");

    priority_queue<MinHeapNode, vector<MinHeapNode>, CompareMinHeapNode> min_heap;
    vector<FILE*> in_files(num_runs, nullptr); // Inicializar a nullptr
    vector<long long> output_buffer(block_size_elements);
    int output_buffer_pos = 0;

    int valid_runs_found = 0;
    for (int i = 0; i < num_runs; i++) {
        char fileName[64];
        snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
        in_files[i] = fopen(fileName, "rb");

        if (in_files[i] == NULL) {
            // Esto no debería pasar si createInitialRuns funcionó bien y devolvió el número correcto,
            // pero es bueno tener una advertencia.
            cerr << "Warning: Could not open temporary file " << fileName << ". Skipping." << endl;
            continue; // Saltar este archivo
        }

        fseek(in_files[i], 0, SEEK_END);
        long long file_size = ftell(in_files[i]);
        fseek(in_files[i], 0, SEEK_SET);

        if (file_size == 0) {
            // Archivo temporal vacío (no debería ocurrir con la lógica actual de createInitialRuns)
            cout << "Warning: Temporary file " << fileName << " is empty. Skipping." << endl;
            fclose(in_files[i]);
            in_files[i] = nullptr; // Marcar como no usado
            continue; // Saltar este archivo
        }

        MinHeapNode node;
        node.i = i;
        node.file_ptr = in_files[i];
        node.buffer.resize(block_size_elements); // Asignar buffer de entrada
        node.buffer_pos = 0;
        node.elements_remaining_in_file = file_size / sizeof(long long);
        node.current_buffer_size = 0; // Se llenará ahora

        if (node.elements_remaining_in_file > 0) {
            long long elements_to_read_initial = min((long long)block_size_elements, node.elements_remaining_in_file);
            size_t read_count = fread(node.buffer.data(), sizeof(long long), elements_to_read_initial, node.file_ptr);
            countIO++;

            if (read_count > 0) {
                node.element = node.buffer[0];
                node.buffer_pos = 1;
                node.current_buffer_size = read_count; // Guardar cuántos elementos se leyeron realmente
                node.elements_remaining_in_file -= read_count; // Actualizar restantes *después* de leer el buffer
                // No redimensionamos el buffer aquí, reutilizamos el espacio asignado.
                min_heap.push(node);
                valid_runs_found++;
            } else {
                // Error de lectura o archivo inesperadamente vacío en términos de long long
                 if (ferror(node.file_ptr)){
                     perror(("Error reading initial block from " + string(fileName)).c_str());
                 } else {
                     cerr << "Warning: Could not read initial block from " << fileName << " despite non-zero size. Skipping." << endl;
                 }
                fclose(node.file_ptr); // Cerrar si no se pudo leer nada
                in_files[i] = nullptr;
            }
        } else {
            // Archivo existía pero no contenía elementos 'long long' válidos (tamaño < sizeof(long long))
            cerr << "Warning: Temporary file " << fileName << " is too small. Skipping." << endl;
            fclose(node.file_ptr); // Archivo existía pero no tenía elementos 'long long'
            in_files[i] = nullptr;
        }
    }

     if (min_heap.empty()) {
         cout << "No valid runs found to merge. Output file will be empty." << endl;
         fclose(out);
         // Eliminar archivos temporales si existen
         for (int i = 0; i < num_runs; ++i) {
             if (in_files[i]) fclose(in_files[i]); // Asegurarse de cerrar los que se abrieron
             char fileName[64];
             snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
             remove(fileName);
         }
         return;
     }

    cout << "Merging using " << min_heap.size() << " active runs." << endl;

    // Proceso de mezcla principal
    while (!min_heap.empty()) {
        MinHeapNode root = min_heap.top();
        min_heap.pop();

        // Escribir el elemento raíz al buffer de salida
        output_buffer[output_buffer_pos++] = root.element;

        // Si el buffer de salida está lleno, escribirlo al disco
        if (output_buffer_pos == block_size_elements) {
            size_t write_count = fwrite(output_buffer.data(), sizeof(long long), block_size_elements, out);
            countIO++;
             if (write_count < (size_t)block_size_elements) {
                 perror("Error writing to output file during merge");
                 // Limpieza agresiva
                 fclose(out);
                 while (!min_heap.empty()) { // Cerrar archivos restantes en el heap
                     MinHeapNode node = min_heap.top();
                     min_heap.pop();
                     if (node.file_ptr) fclose(node.file_ptr);
                 }
                  for (int i = 0; i < num_runs; ++i) { // Asegurar cierre y eliminar temps
                     if (in_files[i] && in_files[i] != nullptr) fclose(in_files[i]); // Cuidado con doble cierre
                     char fileName[64];
                     snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
                     remove(fileName);
                 }
                 exit(EXIT_FAILURE);
             }
            output_buffer_pos = 0;
        }

        // Avanzar en el buffer del tramo de donde vino el elemento raíz
        if (root.buffer_pos < (int)root.current_buffer_size) {
            // Todavía hay elementos en el buffer actual de este tramo
            root.element = root.buffer[root.buffer_pos++];
            min_heap.push(root); // Reinsertar el nodo con el siguiente elemento
        } else {
            // Buffer vacío, intentar leer el siguiente bloque del archivo correspondiente
            if (root.elements_remaining_in_file > 0 && root.file_ptr) {
                long long elements_to_read_next = min((long long)block_size_elements, root.elements_remaining_in_file);
                size_t read_count = fread(root.buffer.data(), sizeof(long long), elements_to_read_next, root.file_ptr);
                countIO++;

                if (read_count > 0) {
                    // Se leyó un nuevo bloque (o parte de él)
                    root.element = root.buffer[0];
                    root.buffer_pos = 1;
                    root.current_buffer_size = read_count;
                    root.elements_remaining_in_file -= read_count;
                    min_heap.push(root); // Reinsertar con el primer elemento del nuevo buffer
                } else {
                    // Error de lectura o fin de archivo inesperado
                     if (ferror(root.file_ptr)) {
                         perror(("Error reading next block from temp file " + to_string(root.i)).c_str());
                          // Considerar cómo manejar esto, ¿detenerse o continuar sin este archivo? Por ahora, cerramos y continuamos.
                     } else {
                          // Fin normal de este archivo temporal
                     }
                    fclose(root.file_ptr); // Cerrar el archivo de este tramo
                    root.file_ptr = nullptr; // Marcar como cerrado
                    // No reinsertamos este nodo en el heap
                }
            } else {
                // No hay más elementos en el archivo O el puntero ya es null
                if (root.file_ptr) {
                    fclose(root.file_ptr); // Cerrar el archivo si no estaba ya cerrado
                    root.file_ptr = nullptr;
                }
                // No reinsertamos este nodo en el heap
            }
        }
    }

    // Escribir cualquier elemento restante en el buffer de salida
    if (output_buffer_pos > 0) {
         size_t write_count = fwrite(output_buffer.data(), sizeof(long long), output_buffer_pos, out);
         countIO++;
         if (write_count < (size_t)output_buffer_pos) {
             perror("Error writing final block to output file");
             // Limpieza final
             fclose(out);
              for(int i=0; i<num_runs; ++i) {
                 // Los archivos deben estar cerrados por la lógica del heap o aquí
                 if (in_files[i] && in_files[i] != nullptr) {
                    // Esto indica un posible bug si un archivo quedó abierto
                    fclose(in_files[i]);
                 }
                 char fileName[64];
                 snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
                 remove(fileName);
             }
             exit(EXIT_FAILURE);
         }
    }

    fclose(out); // Cerrar el archivo de salida final

    // Limpiar archivos temporales
    cout << "Cleaning up temporary files..." << endl;
    for (int i = 0; i < num_runs; ++i) {
        // Los punteros en in_files podrían ser nullptr si el archivo no se abrió o ya se cerró.
        // El fclose ya debería haber ocurrido cuando el archivo se agotó en el heap.
        // Solo necesitamos eliminar los archivos.
        char fileName[64];
        snprintf(fileName, sizeof(fileName), "temp_run_%d.bin", i);
        if (remove(fileName) != 0) {
            // Podría fallar si el archivo no existía (ej. skipped) o si rename() movió el run 0
             if (num_runs == 1 && i == 0) {
                // No imprimir error si fue el único run y se renombró
             } else {
                // Imprimir advertencia si falla la eliminación de otros archivos temporales
                // Podria ser que el archivo no existio (num_runs era mayor que los archivos validos)
                // No es critico normalmente, pero indica algo a revisar.
                 // cerr << "Warning: could not remove temporary file " << fileName << endl;
             }

        }
    }
     cout << "Merge phase finished." << endl;
}
// --- Fin de mergeFiles ---


// --- Función externalSort (orquestador) ---
void externalSort(const char* input_file, const char* output_file, int arity_k, int run_size_M_elements, int block_size_B_elements) {
    cout << "--- Starting External Sort ---" << endl;
    cout << "Input file: " << input_file << endl;
    cout << "Output file: " << output_file << endl;
    cout << "Memory per run (M): " << run_size_M_elements << " elements (" << (long long)run_size_M_elements * sizeof(long long) / (1024.0*1024.0) << " MiB)" << endl;
    cout << "Disk block size (B): " << block_size_B_elements << " elements (" << (long long)block_size_B_elements * sizeof(long long) / 1024.0 << " KiB)" << endl;
    cout << "Arity (k): " << arity_k << endl;
    countIO = 0; // Reiniciar contador de I/O

    // Fase 1: Crear tramos iniciales ordenados
    // Pasamos la aridad 'arity_k' aquí principalmente para la distribución de archivos, aunque no es estrictamente necesaria
    // para el número de archivos creados, que depende de N/M.
    int actual_num_runs = createInitialRuns(input_file, arity_k, run_size_M_elements, block_size_B_elements);
    cout << "Initial run creation phase complete. Actual runs created: " << actual_num_runs << endl;
    cout << "I/O operations after run creation: " << countIO << endl;

    // Fase 2: Mezclar los tramos
    // Pasamos el número *real* de tramos creados a mergeFiles.
    // La aridad 'arity_k' no se usa directamente en mergeFiles (el número de archivos a mezclar es actual_num_runs),
    // pero la lógica del heap maneja implícitamente la mezcla k-way.
    if (actual_num_runs > 0) {
        mergeFiles(output_file, actual_num_runs, block_size_B_elements);
        cout << "Merge phase complete." << endl;
    } else {
        cout << "No initial runs were created (input file might be empty). Output file created empty." << endl;
        // Asegurar que el archivo de salida exista aunque esté vacío
        FILE* out = fopen(output_file, "wb"); // Abre y cierra para crearlo si no existe
        if (out) fclose(out);
        else {
             perror("Could not create empty output file");
             // No salir necesariamente, pero indicar el problema.
        }
    }

    cout << "--- External Sort Finished ---" << endl;
    cout << "Total I/O operations (reads/writes of blocks): " << countIO << endl;
}
// --- Fin de externalSort ---


// --- Función para generar archivo de prueba (sin cambios) ---
void generateTestInputFile(const char* fileName, long long num_elements) {
    FILE* out = openFile(fileName, "wb");
    srand(time(NULL));
    vector<long long> buffer(BLOCK_SIZE_ELEMENTS);
    int buffer_pos = 0;
    cout << "Generating test input file '" << fileName << "' with " << num_elements << " elements..." << endl;
    long long step = num_elements / 10;
    if (step == 0) step = 1;

    for (long long i = 0; i < num_elements; ++i) {
        // Usar rand() puede ser limitado, considerar <random> para mejor aleatoriedad
        buffer[buffer_pos++] = (static_cast<long long>(rand()) << 32) | rand(); // Generar long long pseudo-aleatorio
        if (buffer_pos == BLOCK_SIZE_ELEMENTS) {
            size_t written = fwrite(buffer.data(), sizeof(long long), BLOCK_SIZE_ELEMENTS, out);
            if(written != BLOCK_SIZE_ELEMENTS) {
                 perror("Error writing during test file generation");
                 fclose(out);
                 exit(EXIT_FAILURE);
            }
            buffer_pos = 0;
        }
        if (i % step == 0 || i == num_elements - 1) {
             cout << "\rGenerated " << i + 1 << "/" << num_elements << " elements..." << flush;
        }
    }
    if (buffer_pos > 0) {
         size_t written = fwrite(buffer.data(), sizeof(long long), buffer_pos, out);
         if(written != (size_t)buffer_pos) {
              perror("Error writing final block during test file generation");
              fclose(out);
              exit(EXIT_FAILURE);
         }
    }
    fclose(out);
    cout << "\nTest input file generated successfully." << endl;
}
// --- Fin de generateTestInputFile ---

// --- Función para verificar si el archivo está ordenado ---
bool checkSorted(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror(("Error opening file for checking: " + string(filename)).c_str());
        return false;
    }
    cout << "Checking if file '" << filename << "' is sorted..." << endl;
    vector<long long> buffer(BLOCK_SIZE_ELEMENTS); // Leer por bloques B
    long long last_element = LLONG_MIN; // Asumiendo que los números no son LLONG_MIN
    bool first_element = true;
    bool sorted = true;
    size_t elements_read_total = 0;

    while (true) {
        size_t read_count = fread(buffer.data(), sizeof(long long), BLOCK_SIZE_ELEMENTS, file);
        countIO++; // Contar lectura para verificación
        elements_read_total += read_count;

        if (read_count == 0) {
            if (ferror(file)) {
                perror(("Error reading file during check: " + string(filename)).c_str());
                sorted = false; // Error de lectura implica que no podemos confirmar
            }
            // Si no hay error, es EOF, salimos del bucle.
            break;
        }

        for (size_t i = 0; i < read_count; ++i) {
            if (!first_element) {
                if (buffer[i] < last_element) {
                    cerr << "\nError: File not sorted! Element " << buffer[i] << " at index ~"
                         << (elements_read_total - read_count + i) << " is less than previous " << last_element << "." << endl;
                    sorted = false;
                    goto end_check; // Salir temprano al encontrar desorden
                }
            }
            last_element = buffer[i];
            first_element = false;
        }

        // Si fread leyó menos de lo esperado, es EOF
         if (read_count < BLOCK_SIZE_ELEMENTS) {
             break;
         }
    }

end_check:
    fclose(file);
    if (sorted) {
        cout << "File '" << filename << "' is sorted correctly." << endl;
    } else {
         cout << "File '" << filename << "' is NOT sorted." << endl;
    }
    return sorted;
}
// --- Fin de checkSorted ---


// --- NUEVA Función para Calcular Aridad Óptima ---
int calculateOptimalArity(long long m_bytes, int b_bytes) {
    if (b_bytes <= 0) {
        cerr << "Error: Block size (B) must be positive." << endl;
        return 2; // Devuelve un mínimo seguro
    }
    // La memoria M debe alojar 'a' buffers de entrada (tamaño B) y 1 buffer de salida (tamaño B).
    // Total memoria para buffers = (a + 1) * B
    // (a + 1) * B <= M  => a + 1 <= M / B => a <= (M / B) - 1
    // Usamos floor para asegurarnos de que quepan completamente.
    double blocks_in_memory = floor((double)m_bytes / b_bytes);

    // Necesitamos al menos 1 bloque de entrada y 1 de salida, por lo que M/B debe ser >= 2.
    if (blocks_in_memory < 2.0) {
        cerr << "Warning: Memory M (" << m_bytes << " bytes) is too small to hold even two blocks B ("
             << b_bytes << " bytes). External sort might be inefficient or impossible. Defaulting arity to 2." << endl;
        return 2; // Aridad mínima
    }

    // Aridad teórica máxima basada en buffers: floor(M/B) - 1
    int max_arity = static_cast<int>(floor(blocks_in_memory)) - 1;


    // Consideraciones adicionales:
    // 1. Sobrecarga del MinHeap: Cada nodo en el heap (habrá 'a' nodos) tiene sobrecarga
    //    (punteros, elemento, índices, etc.), además del buffer de B bytes.
    // 2. Otros usos de memoria: Código del programa, stack, variables locales.
    // Por seguridad, podríamos reducir un poco la aridad calculada.
    // Ejemplo conservador: usar solo el 90% de la memoria para buffers.
    // blocks_in_memory = floor(0.9 * m_bytes / b_bytes);
    // max_arity = static_cast<int>(floor(blocks_in_memory)) - 1;
    // O simplemente asegurar que sea >= 2.

    int optimal_arity = max(2, max_arity); // Asegurar que la aridad sea al menos 2.

    // Podría haber un límite práctico por el número de descriptores de archivo,
    // pero suele ser alto (miles). No lo limitamos explícitamente aquí.

    cout << "Calculated optimal arity based on M/B - 1: " << optimal_arity << endl;
    if (optimal_arity < 2) {
         cout << "Adjusted optimal arity to minimum value: 2" << endl;
         optimal_arity = 2;
    }

    return optimal_arity;
}
// --- Fin de calculateOptimalArity ---

// --- Main Actualizado ---
void printUsage(const char* progName) {
    cerr << "Usage: " << progName << " <input_file> <output_file> [options]" << endl;
    cerr << "Options:" << endl;
    cerr << "  -a <arity>       : Specify the arity (k) for merging (integer >= 2)." << endl;
    cerr << "                     If not specified, an optimal value is calculated." << endl;
    cerr << "  --calculate-arity: Force calculation of optimal arity and print it," << endl;
    cerr << "                     overriding any -a value if both are present." << endl;
    cerr << "  --generate N     : Generate a test input file named <input_file> with N elements" << endl;
    cerr << "                     (long long). Overwrites if <input_file> exists." << endl;
    cerr << "  --check          : After sorting, verify if the output file is sorted." << endl;

}

int main(int argc, char* argv[]) {
    string input_file_str;
    string output_file_str;
    int specified_arity = -1; // -1 indica no especificado
    bool calculate_flag = false;
    bool generate_flag = false;
    bool check_flag = false;
    long long elements_to_generate = 0;

    // Parsear argumentos
    int arg_idx = 1;
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    input_file_str = argv[arg_idx++];
    output_file_str = argv[arg_idx++];

    while (arg_idx < argc) {
        string current_arg = argv[arg_idx];
        if (current_arg == "-a") {
            if (arg_idx + 1 < argc) {
                try {
                    specified_arity = stoi(argv[arg_idx + 1]);
                    if (specified_arity < 2) {
                        cerr << "Error: Arity specified with -a must be >= 2." << endl;
                        return 1;
                    }
                } catch (const std::invalid_argument& ia) {
                    cerr << "Error: Invalid number provided for arity -a." << endl;
                    return 1;
                } catch (const std::out_of_range& oor) {
                     cerr << "Error: Arity number provided for -a is out of range." << endl;
                     return 1;
                }
                arg_idx += 2;
            } else {
                cerr << "Error: -a flag requires an integer argument." << endl;
                printUsage(argv[0]);
                return 1;
            }
        } else if (current_arg == "--calculate-arity") {
            calculate_flag = true;
            arg_idx++;
        } else if (current_arg == "--generate") {
             if (arg_idx + 1 < argc) {
                try {
                    elements_to_generate = stoll(argv[arg_idx + 1]); // Use stoll for long long
                    if (elements_to_generate <= 0) {
                        cerr << "Error: Number of elements to generate must be positive." << endl;
                        return 1;
                    }
                    generate_flag = true;
                } catch (const std::invalid_argument& ia) {
                    cerr << "Error: Invalid number provided for --generate." << endl;
                    return 1;
                } catch (const std::out_of_range& oor) {
                     cerr << "Error: Number for --generate is out of range." << endl;
                     return 1;
                }
                arg_idx += 2;
             } else {
                cerr << "Error: --generate flag requires the number of elements (N)." << endl;
                printUsage(argv[0]);
                return 1;
             }
        } else if (current_arg == "--check") {
            check_flag = true;
            arg_idx++;
        }
        else {
            cerr << "Error: Unknown argument '" << current_arg << "'" << endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // --- Generar archivo de entrada si se solicitó ---
    if (generate_flag) {
         generateTestInputFile(input_file_str.c_str(), elements_to_generate);
         // Después de generar, podríamos querer salir o continuar con el ordenamiento.
         // Por ahora, continuaremos para ordenar el archivo recién generado.
         cout << "Input file '" << input_file_str << "' generated. Proceeding with sort..." << endl;
    } else {
        // Verificar si el archivo de entrada existe si no se generó
        FILE* test_in = fopen(input_file_str.c_str(), "rb");
        if (!test_in) {
            cerr << "Error: Input file '" << input_file_str << "' not found and --generate not specified." << endl;
            printUsage(argv[0]);
            return 1;
        }
        fclose(test_in);
         cout << "Using existing input file: " << input_file_str << endl;
    }


    // --- Determinar la Aridad a Usar ---
    int final_arity;
    int calculated_optimal_arity = calculateOptimalArity(M_BYTES, B_BYTES);

    if (calculate_flag) {
        // Si se pide calcular explícitamente, se usa ese valor y se ignora -a
        final_arity = calculated_optimal_arity;
        cout << "Using explicitly calculated optimal arity: " << final_arity << endl;
    } else if (specified_arity != -1) {
        // Si se especificó -a y no --calculate-arity, se usa el valor especificado
        final_arity = specified_arity;
        cout << "Using user-specified arity: " << final_arity << endl;
    } else {
        // Si no se especificó -a ni --calculate-arity, se usa el valor óptimo calculado como default
        final_arity = calculated_optimal_arity;
        cout << "Using default calculated optimal arity: " << final_arity << endl;
    }


    // --- Ejecutar el Ordenamiento Externo ---
    externalSort(input_file_str.c_str(), output_file_str.c_str(), final_arity, RUN_SIZE_ELEMENTS, BLOCK_SIZE_ELEMENTS);

    // --- Verificar el resultado si se solicitó ---
    if (check_flag) {
         // Reiniciar contador de I/O para la verificación (o usar uno separado)
         long long sort_io = countIO;
         countIO = 0;
         cout << "\n--- Verifying Output File ---" << endl;
         bool is_sorted = checkSorted(output_file_str.c_str());
         cout << "I/O operations during verification: " << countIO << endl;
         cout << "Total I/O operations (sort + verification): " << sort_io + countIO << endl;
         return is_sorted ? 0 : 1; // Devolver 0 si está ordenado, 1 si no.
    }


    return 0;
}