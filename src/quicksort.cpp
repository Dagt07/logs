
//se define M_SIZE <- tamaño de la memoria principal: valor capturado desde los args
//se define B_SIZE <- tamaño del bloque: como tamaño real de bloque en un disco (512B en discos viejos, 4069B en discos modernos) 
//se define N_SIZE <- tamaño del array que contiene a los elementos a ordenar (input)
//se define a <- cantidad de particiones que se van a realizar en el algoritmo de quicksort
//se define b <- se necesitan a - 1 pivotes en cada bloque B_SIZE, por ello b = B_SIZE / 8 ya que cada pivote ocupa 8 bytes
//cada pivote debe estar entre [2,b-1]

//quicksort args: 
//argv[1] <- M_SIZE
//argv[2] <- B_SIZE quiza lo podemos dejar fijo en 4096 en el header quicksort.h 
//argv[3] <- N_SIZE
//argv[4] <- valor de a


