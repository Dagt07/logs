# Memoria secundaria 
## Mergesort externo vs Quicksort Externo

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