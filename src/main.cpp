//main args: 
//argv[1] <- M_SIZE
//argv[2] <- B_SIZE quiza lo podemos dejar fijo en 4096 
//argv[3] <- N_SIZE

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
//llamado a mergesort

//llamado a quicksort

//y más cosas a implementar

