//main args: 
//argv[1] <- M_SIZE
//argv[2] <- B_SIZE quiza lo podemos dejar fijo en 4096 

//se define M_SIZE <- tamaño de la memoria principal: valor capturado desde los args
//se define B_SIZE <- tamaño del bloque: como tamaño real de bloque en un disco (512B en discos viejos, 4069B en discos modernos)
//se define N_SIZE <- tamaño del array que contiene a los elementos a ordenar (input)
//se define a <- valor que se debe determinar acá y define: 1) aridad del mergesort 2) Número de particiones que se van a realizar en el algoritmo de quicksort

//funcion que genera 5  secuencias  de  números  enteros  de  64  bits  de  tamaño  total  𝑁,  con
//𝑁 ∈ {4𝑀, 8𝑀, ...60𝑀} (es decir, 5 secuencias de tamaño 4𝑀, 5 secuencias de tamaño 8𝑀, ...), 
//insertarlos desordenadamente en un arreglo y guardarlo en binario en disco. 
//Para trabajar en disco, se guardará como un archivo binario (.bin) el arreglo de tamaño 𝑁. Este archivo
//se deberá leer y escribir en bloques de tamaño 𝐵. Es incorrecto realizar una lectura o escritura de tamaño
//distinto a 𝐵. Para esto, se recomienda usar las funciones fread, fwrite y fseek de C. Se recomienda
//también usar un buffer para evitar leer y escribir cada vez que se accede a un elemento del arreglo. El
//buffer debe ser de tamaño 𝐵 es decir, tendremos en memoria principal todo el bloque solicitado


//funcion que determina a


//main:
    
    //for(int i = 0; i < 15; i++) {
        //1) llamar a la función que genera los 5 archivos para un único tamaño N <- haciendo todo para no ocupar tanto espacio en disco

        //2) determinar a

        //3) llamado a mergesort sobre cada uno de los 5 archivos de tamaño N. 

        //4) llamado a quicksort sobre cada uno de los 5 archivos de tamaño N. 

        //5) promediar los resultados para ese N

        //6) guardar los resultados en un archivo de salida (csv o txt)

        //7) liberar memoria

    //y más cosas a implementar...

#include "sequence_generator.hpp"
#include <filesystem>

//vector de 15 tamaños N
std::vector<int> v = {4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};

void process_sequence(const std::string& path, std::uint64_t N,
                      std::size_t B_SIZE)
{
    /*  Aquí, en la versión final, abrirás el .bin, ejecutarás
        mergesort externo y quicksort externo, medirás I/O, etc.
        Por ahora solo es un stub:  */
    (void)path; (void)N; (void)B_SIZE;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0]
                  << " <M_SIZE_MB> <B_SIZE_bytes>\n";
        return EXIT_FAILURE;
    }

    const std::uint64_t M_BYTES = std::stoull(argv[1]) * 1024ULL * 1024ULL;
    const std::size_t   B_SIZE  = std::stoull(argv[2]);

    //auto Ns = dataset_sizes(M_BYTES);      // vector de 15 tamaños N

    //for (std::size_t i = 0; i < Ns.size(); ++i)
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        int mult = static_cast<int>((i + 1) * 4);      // 4,8,…,60
        std::uint64_t N = static_cast<std::uint64_t>(v[i]) * M_BYTES / 8;
        //std::uint64_t N = Ns[i];

        std::cout << "\n===  Multiplicador " << mult
                  << " M  →  N = " << N << " elementos  ===\n";

        for (int rep = 1; rep <= 5; ++rep)
        {
            std::ostringstream fn;
            fn << "seq_" << std::setw(2) << std::setfill('0')
               << mult << "M_rep" << rep << ".bin";

            /* 1. Generar                                                         */
            generate_sequence(N, fn.str(), B_SIZE);

            /* 2. Procesar (algoritmos externos, medición, etc.)                  */
            //process_sequence(fn.str(), N, B_SIZE);

            /* 3. Borrar para liberar espacio                                     */
            std::filesystem::remove(fn.str());
        }
    }

    std::cout << "\n✓ Experimento completo sin acumular archivos.\n";
    return EXIT_SUCCESS;
}
