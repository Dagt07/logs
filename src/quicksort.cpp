//se define M_SIZE <- tamaño de la memoria principal: valor capturado desde los args
//se define B_SIZE <- tamaño del bloque: como tamaño real de bloque en un disco (512B en discos viejos, 4069B en discos modernos) 
//se define N_SIZE <- tamaño del array que contiene a los elementos a ordenar (input), recibido por quicksort
//se define a <- cantidad de particiones que se van a realizar en el algoritmo de quicksort, recibido por quicksort y no cambia

//variables globales
//M_SIZE recibida en el main
//B_SIZE recibida en el main

//quicksort args:
//a: cantidad de particiones a realizar
//N: array a ordenar <- también representa el tamaño del bloque a seleccionar cuando N_SIZE > M_SIZE

//quicksort version 1:
//si N_SIZE <= M_SIZE
// se ordena el bloque completo gratis en memoria principal con sort
//else N_SIZE > M_SIZE
// 1) Se selecciona un bloque aleatorio para ser ordenado, presuntamente N pero seguir pensando en B_A valor más chico que N_SIZE pero > M_SIZE y B
// porque sino se ordenaría en memoria principal gratis
// 2) Se seleccionan aleatoriamente a-1 pivotes dentro del bloque seleccionado en el paso anterior
// 3) en un buffer de tamaño B_SIZE se ordenan los elementos seleccionados en el paso anterior gratis (pensar más porque el tamaño de a permite que sea gratis, ie, a<M_SIZE)
// 4) En otros buffers (a-1 buffers) se almacenan los elementos menores, entre medio y mayores a los pivotes seleccionados en el paso 2 recorriendo N de forma lineal
// 5) Se llama recursivamente a quicksort para cada uno de los buffers generados en el paso 4
// 6) Finalmente se unen los buffers generados de acuerdo a los pivotes, ejemplo:
// el buffer de menores a pivote 1, pivote 1, el buffer de elementos entre pivote 1 y 2, pivote 2 y así sucesivamente

//quicksort args:
//a: cantidad de particiones a realizar
//N: array a ordenar

//quicksort version 2:
//si N_SIZE <= M_SIZE
// se ordena el bloque completo gratis en memoria principal con sort
//else N_SIZE > M_SIZE
// 1) Se selecciona un bloque de tamaño B_SIZE aleatorio
// 2) Se seleccionan aleatoriamente a-1 pivotes dentro del bloque seleccionado en el paso anterior
// 3) en un buffer ordeno gratis los pivotes seleccionados en el paso anterior (ahora si es gratis, en general a es menor que B_SIZE = 4096)
// 4) En otros buffers (a-1 buffers) se almacenan los elementos menores, entre medio y mayores a los pivotes seleccionados en el paso 2 leyendo el input N por bloques de tamaño B_SIZE
// 5) Se llama recursivamente a quicksort para cada uno de los buffers generados en el paso 4
// 6) Finalmente se unen los buffers generados de acuerdo a los pivotes, ejemplo:
// el buffer de menores a pivote 1, pivote 1, el buffer de elementos entre pivote 1 y 2, pivote 2 y así sucesivamente

//quicksort args:
//a: cantidad de particiones a realizar
//N: array a ordenar

//quicksort version 3:
//si N_SIZE <= M_SIZE
// se ordena el bloque completo gratis en memoria principal con sort
//else N_SIZE > M_SIZE
// 1) Se selecciona un bloque de tamaño B_SIZE aleatorio de nuestro input N de N_SIZE (ya que solo podemos leer B_SIZE bytes a la vez)
// 2) Se seleccionan aleatoriamente a-1 pivotes dentro del bloque seleccionado en el paso anterior
// 2.1) en un buffer ordeno gratis los pivotes seleccionados en el paso anterior (es gratis, ya que en general a es menor que B_SIZE = 4096)
// 3) Uso secuencialmente los pivotes ordenados para definir en que (a-1 buffers) se almacenan los elementos menores, entre medio y mayores a los pivotes seleccionados en el paso 2 leyendo el input N por bloques de tamaño B_SIZE
// 4) Se llama recursivamente a quicksort para cada uno de los buffers generados en el paso 3
// 5) Finalmente se unen los buffers generados de acuerdo a los pivotes, ejemplo:
// el buffer de menores a pivote 1, pivote 1, el buffer de elementos entre pivote 1 y 2, pivote 2 y así sucesivamente
//retorna el array ordenado

