/*
 * edicion.c — Módulo de edición con buffer dinámico
 * Responsable: Matías
 *
 * Implementa d [n] y i [n] [texto].
 *
 * Decisión de diseño (para el PDF de sustentación):
 *   Se opta por cargar el archivo COMPLETO a memoria en cada operación
 *   (en vez de mover bytes en disco con lseek para cada línea) porque:
 *     1. Insertar/borrar una línea en un archivo plano implica, en el
 *        caso general, desplazar TODOS los bytes posteriores a esa
 *        línea. Hacerlo directamente sobre el disco con lseek+write
 *        exige llevar ese desplazamiento byte a byte y es propenso a
 *        corromper el archivo si el equipo no maneja bien los offsets.
 *     2. Trabajar sobre un arreglo de líneas en memoria (malloc/realloc)
 *        permite razonar la operación como "quitar/insertar un elemento
 *        de un arreglo", que es exactamente el reto técnico que pide la
 *        rúbrica para el nivel de Pareja ("manipulación de buffers
 *        dinámicos en memoria para no perder datos al desplazar bytes").
 *     3. El costo (releer y reescribir el archivo completo) es aceptable
 *        para un editor de texto plano de uso interactivo como el que
 *        pide el enunciado (no hay concurrencia ni archivos masivos).
 *
 * Restricción respetada: únicamente open/read/write/lseek/ftruncate/close
 * para I/O de archivo. printf/perror para consola. Sin fopen/fread/fwrite/fclose.
 */

#define _POSIX_C_SOURCE 200809L /* necesario para ftruncate() y strdup() con -std=c11 */

#include "editor.h"
#include "io_utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ------------------------------------------------------------------ */
/* Estructura interna: arreglo dinámico de líneas                      */
/* ------------------------------------------------------------------ */
typedef struct {
    char   **lineas;     /* arreglo de punteros a cadenas (sin '\n')   */
    size_t   cantidad;   /* líneas actualmente usadas                  */
    size_t   capacidad;  /* tamaño reservado del arreglo                */
} ListaLineas;

static int lista_init(ListaLineas *lista) {
    lista->capacidad = 8;
    lista->cantidad = 0;
    lista->lineas = malloc(lista->capacidad * sizeof(char *));
    if (!lista->lineas) {
        perror("malloc");
        return -1;
    }
    return 0;
}

static void lista_liberar(ListaLineas *lista) {
    for (size_t i = 0; i < lista->cantidad; i++) {
        free(lista->lineas[i]);
    }
    free(lista->lineas);
    lista->lineas = NULL;
    lista->cantidad = lista->capacidad = 0;
}

/* Inserta 'texto' (ya duplicado en heap) en la posición 'idx' (0-based),
 * desplazando el resto del arreglo una posición a la derecha. */
static int lista_insertar_en(ListaLineas *lista, size_t idx, char *texto) {
    if (lista->cantidad == lista->capacidad) {
        size_t nueva_cap = lista->capacidad * 2;
        char **tmp = realloc(lista->lineas, nueva_cap * sizeof(char *));
        if (!tmp) {
            perror("realloc");
            return -1;
        }
        lista->lineas = tmp;
        lista->capacidad = nueva_cap;
    }
    for (size_t i = lista->cantidad; i > idx; i--) {
        lista->lineas[i] = lista->lineas[i - 1];
    }
    lista->lineas[idx] = texto;
    lista->cantidad++;
    return 0;
}

/* Elimina la línea en la posición 'idx' (0-based), liberando su memoria
 * y desplazando el resto del arreglo una posición a la izquierda. */
static void lista_borrar_en(ListaLineas *lista, size_t idx) {
    free(lista->lineas[idx]);
    for (size_t i = idx; i + 1 < lista->cantidad; i++) {
        lista->lineas[i] = lista->lineas[i + 1];
    }
    lista->cantidad--;
}

/* ------------------------------------------------------------------ */
/* Partición en líneas (leer_archivo_completo ahora vive en io_utils.c, */
/* compartido con io_core.c)                                           */
/* ------------------------------------------------------------------ */

/* Parte 'contenido' (longitud 'len') en líneas y las agrega a 'lista'.
 * Cada línea queda sin el '\n' final. Si el archivo está vacío,
 * la lista queda con 0 líneas. */
static int separar_en_lineas(const char *contenido, size_t len, ListaLineas *lista) {
    size_t inicio = 0;
    for (size_t i = 0; i < len; i++) {
        if (contenido[i] == '\n') {
            size_t largo = i - inicio;
            char *linea = malloc(largo + 1);
            if (!linea) {
                perror("malloc");
                return -1;
            }
            memcpy(linea, contenido + inicio, largo);
            linea[largo] = '\0';
            if (lista_insertar_en(lista, lista->cantidad, linea) < 0) {
                free(linea);
                return -1;
            }
            inicio = i + 1;
        }
    }
    /* Última línea sin '\n' final (archivo no termina en salto de línea) */
    if (inicio < len) {
        size_t largo = len - inicio;
        char *linea = malloc(largo + 1);
        if (!linea) {
            perror("malloc");
            return -1;
        }
        memcpy(linea, contenido + inicio, largo);
        linea[largo] = '\0';
        if (lista_insertar_en(lista, lista->cantidad, linea) < 0) {
            free(linea);
            return -1;
        }
    }
    return 0;
}

/* Reconstruye el buffer final (cada línea + '\n') y reescribe el archivo
 * completo: lseek a 0, ftruncate a 0, y write del contenido nuevo. */
static int reescribir_archivo(int fd, ListaLineas *lista) {
    size_t total = 0;
    for (size_t i = 0; i < lista->cantidad; i++) {
        total += strlen(lista->lineas[i]) + 1; /* +1 por el '\n' */
    }

    char *buf = malloc(total > 0 ? total : 1);
    if (!buf) {
        perror("malloc");
        return -1;
    }

    size_t offset = 0;
    for (size_t i = 0; i < lista->cantidad; i++) {
        size_t largo = strlen(lista->lineas[i]);
        memcpy(buf + offset, lista->lineas[i], largo);
        offset += largo;
        buf[offset++] = '\n';
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek");
        free(buf);
        return -1;
    }
    if (ftruncate(fd, 0) < 0) {
        perror("ftruncate");
        free(buf);
        return -1;
    }

    size_t total_escritos = 0;
    while (total_escritos < total) {
        ssize_t escritos = write(fd, buf + total_escritos, total - total_escritos);
        if (escritos < 0) {
            perror("write");
            free(buf);
            return -1;
        }
        total_escritos += (size_t)escritos;
    }

    free(buf);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Comandos públicos                                                   */
/* ------------------------------------------------------------------ */

/* d [n] — Borra la línea n (1-indexada) del archivo. */
int cmd_d(Editor *ed, int n) {
    if (ed->fd < 0) {
        fprintf(stderr, "d: no hay ningún archivo abierto (usa o [archivo] primero)\n");
        return -1;
    }
    if (n < 1) {
        fprintf(stderr, "d: número de línea inválido: %d\n", n);
        return -1;
    }

    size_t len;
    char *contenido = leer_archivo_completo(ed->fd, &len);
    if (!contenido) return -1;

    ListaLineas lista;
    if (lista_init(&lista) < 0) {
        free(contenido);
        return -1;
    }
    if (separar_en_lineas(contenido, len, &lista) < 0) {
        free(contenido);
        lista_liberar(&lista);
        return -1;
    }
    free(contenido); /* ya no se necesita: todo vive en 'lista' */

    if ((size_t)n > lista.cantidad) {
        fprintf(stderr, "d: la línea %d está fuera de rango (el archivo tiene %zu líneas)\n",
                n, lista.cantidad);
        lista_liberar(&lista);
        return -1;
    }

    lista_borrar_en(&lista, (size_t)(n - 1)); /* pasar a índice 0-based */

    int r = reescribir_archivo(ed->fd, &lista);
    lista_liberar(&lista);
    return r;
}

/* i [n] [texto] — Inserta 'texto' como nueva línea n, desplazando el
 * resto. Si n == cantidad+1, inserta al final (equivalente a un append
 * de línea, pero pasando por el mismo camino de reescritura completa). */
int cmd_i(Editor *ed, int n, const char *texto) {
    if (ed->fd < 0) {
        fprintf(stderr, "i: no hay ningún archivo abierto (usa o [archivo] primero)\n");
        return -1;
    }
    if (n < 1) {
        fprintf(stderr, "i: número de línea inválido: %d\n", n);
        return -1;
    }

    size_t len;
    char *contenido = leer_archivo_completo(ed->fd, &len);
    if (!contenido) return -1;

    ListaLineas lista;
    if (lista_init(&lista) < 0) {
        free(contenido);
        return -1;
    }
    if (separar_en_lineas(contenido, len, &lista) < 0) {
        free(contenido);
        lista_liberar(&lista);
        return -1;
    }
    free(contenido);

    /* Se permite insertar en cualquier posición entre 1 y cantidad+1 */
    if ((size_t)n > lista.cantidad + 1) {
        fprintf(stderr, "i: la línea %d está fuera de rango (máximo permitido: %zu)\n",
                n, lista.cantidad + 1);
        lista_liberar(&lista);
        return -1;
    }

    char *copia = strdup(texto);
    if (!copia) {
        perror("strdup");
        lista_liberar(&lista);
        return -1;
    }

    if (lista_insertar_en(&lista, (size_t)(n - 1), copia) < 0) {
        free(copia);
        lista_liberar(&lista);
        return -1;
    }

    int r = reescribir_archivo(ed->fd, &lista);
    lista_liberar(&lista);
    return r;
}
