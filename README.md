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
│   ├── quicksort.cpp
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
g++ -std=c++17 -O2 main.cpp mergesort.cpp quicksort.cpp -o main 
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