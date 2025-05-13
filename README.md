# Memoria secundaria 
## Mergesort externo vs Quicksort Externo

## Tabla de contenidos
1. [Estructura del repositorio](#estructura-y-archivos-necesarios)
2. [Ejecutar sin ambiente](#ejecutar-sin-ambiente)
3. [Ejecutar en Docker](#ejecutar-en-docker)

### Estructura y archivos necesarios

```
.
├── enunciado/
│   ├── t1_logs.pdf
├── headers/
│   ├── mergesort.hpp
│   ├── quicksort.hpp
│   └── ...
├── src/
│   ├── main.cpp
│   ├── mergesort.cpp
│   ├── quicksort_v3_args.cpp
│   ├── quicksort_v3.cpp
│   ├── sequence_generator.hpp
│   └── ...
└── README.md
```

### Ejecutar sin ambiente

Moverse a la carpeta src
``` 
cd src
```
Compilar de forma conjunta main.cpp, mergesort.cpp y quicksort.cpp usando las siguientes flags y versión de compilación
```
g++ -std=c++17 -O2 main.cpp mergesort.cpp quicksort_v3_args.cpp -o main_docker
```

Ahora con main como binario ejecutable, se deben dar los siguientes argumentos (en caso contrario habrá error):
```
./main 50 4096 30
```
siendo 
- argv[1]: M tamaño de memoria principal
- argv[2]: B tamaño del bloque
- argv[3]: a aridad/particiones a usar


### Ejecutar en Docker

Disclaimer: 
En el computador 2 luego de muchos cambios en el código por la prisa respecto a la hora de entrega dejo de correr en docker, principalmente porque sale el siguiente error:

root@20048341c9e0 /workspace/src (0.046s)
$ ./main 50 4096 30
./main: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.38' not found (required by ./main)
./main: /lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.32' not found (required by ./main)

y es un tema de compilación, por más que se instala las librerias sugeridas con apt no se soluciona.

En computador 1, funcionó

Además en el main.cpp fue comentada la ejecución de quicksort por lo mencionado en el informe, para entorno de sistema operativo host basta descomentarla dentro del mismo archivo en la función process_sequence para que se ejecute y verifiquen sus resultados

**Requisitos:** Tener instalado docker, se puede verificar haciendo ```docker --version```

Descargar el contenedor de docker brindado por el equipo docente, alternativamente ir a [docker.hub](https://hub.docker.com/r/pabloskewes/cc4102-cpp-env)
o hacer ```docker pull pabloskewes/cc4102-cpp-env```

Una vez con la imagen del contenedor descargada
Desde la carpeta del proyecto, abre la terminal y ejecuta:
```
docker run --rm -it -v "$PWD:/workspace" pabloskewes/cpp4102-cpp-env bash

```

**Importante**, para simular un ambiente con memoria controlada se debe ejecutar en lugar del anterior, el siguiente comando:
```
docker run --rm -it -m 500m -v "$PWD":/workspace pabloskewes/cpp4102-cpp-env bash
```

Una vez dentro del bash del ambiente, al hacer ls se debería ver:
```
> README.MD enunciado headers src
```

Se debe ir a la localición de src
```
cd /src
```

Finalmente, compilar y ejecutar la experimentación:
```
g++ -std=c++17 -O2 main.cpp mergesort.cpp quicksort.cpp -o main 
./main 50 4096 30
```
siendo 
- argv[1]: M tamaño de memoria principal
- argv[2]: B tamaño del bloque
- argv[3]: a aridad/particiones a usar