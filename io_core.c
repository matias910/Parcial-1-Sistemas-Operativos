/*
 * io_core.c — Núcleo de I/O del editor
 * Responsable: Samuel
 *
 * Implementa o [archivo], p [n], a [texto], s [palabra] y q.
 *
 * Decisión de diseño (para el PDF de sustentación):
 *   - o: abre con O_RDWR | O_CREAT (crea el archivo si no existe) y
 *     permisos 0644. Si ya había un archivo abierto, se cierra primero
 *     para no filtrar descriptores de archivo (fd leak).
 *   - p: lee el archivo completo con leer_archivo_completo() (compartida
 *     con el módulo de edición, en io_utils.c) y luego:
 *       * sin argumento (n == 0): imprime todo el archivo escribiendo
 *         directamente a STDOUT_FILENO (write, FD 1), como pide la
 *         tabla de comandos.
 *       * con n > 0: recorre el buffer contando saltos de línea hasta
 *         encontrar la línea n, y solo esa línea se escribe a salida.
 *   - a: usa lseek(SEEK_END) + write, sin reescribir el archivo — es la
 *     operación más barata porque solo agrega bytes al final.
 *   - s: reutiliza leer_archivo_completo() y localiza coincidencias con
 *     strstr() línea por línea (null-terminando temporalmente cada
 *     salto de línea sobre el mismo buffer, sin copias adicionales).
 *
 * Restricción respetada: únicamente open/read/write/lseek/ftruncate/close
 * para I/O de archivo. printf/perror para consola. Sin fopen/fread/fwrite/fclose.
 */

#define _POSIX_C_SOURCE 200809L

#include "editor.h"
#include "io_utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* o [archivo] — Abre (o crea) un archivo para lectura/escritura. */
int cmd_o(Editor *ed, const char *ruta) {
    if (ed->fd >= 0) {
        /* ya había un archivo abierto: se cierra primero para no
         * filtrar el descriptor anterior */
        if (close(ed->fd) < 0) {
            perror("close");
        }
        ed->fd = -1;
    }

    int fd = open(ruta, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    ed->fd = fd;
    strncpy(ed->ruta, ruta, EDITOR_MAX_RUTA - 1);
    ed->ruta[EDITOR_MAX_RUTA - 1] = '\0';
    printf("Archivo '%s' abierto (fd=%d)\n", ruta, fd);
    return 0;
}

/* p [n] — Imprime la línea n, o todo el archivo si n == 0. */
int cmd_p(Editor *ed, int n) {
    if (ed->fd < 0) {
        fprintf(stderr, "p: no hay ningún archivo abierto (usa o [archivo] primero)\n");
        return -1;
    }

    size_t len;
    char *contenido = leer_archivo_completo(ed->fd, &len);
    if (!contenido) return -1;

    if (n == 0) {
        size_t total_escritos = 0;
        while (total_escritos < len) {
            ssize_t escritos = write(STDOUT_FILENO, contenido + total_escritos, len - total_escritos);
            if (escritos < 0) {
                perror("write");
                free(contenido);
                return -1;
            }
            total_escritos += (size_t)escritos;
        }
        free(contenido);
        return 0;
    }

    if (n < 0) {
        fprintf(stderr, "p: número de línea inválido: %d\n", n);
        free(contenido);
        return -1;
    }

    int linea_actual = 1;
    size_t inicio = 0;
    for (size_t i = 0; i < len; i++) {
        if (contenido[i] == '\n') {
            if (linea_actual == n) {
                write(STDOUT_FILENO, contenido + inicio, i - inicio);
                write(STDOUT_FILENO, "\n", 1);
                free(contenido);
                return 0;
            }
            linea_actual++;
            inicio = i + 1;
        }
    }
    /* última línea sin '\n' final */
    if (linea_actual == n && inicio < len) {
        write(STDOUT_FILENO, contenido + inicio, len - inicio);
        write(STDOUT_FILENO, "\n", 1);
        free(contenido);
        return 0;
    }

    fprintf(stderr, "p: la línea %d está fuera de rango\n", n);
    free(contenido);
    return -1;
}

/* a [texto] — Añade 'texto' como nueva línea al final del archivo. */
int cmd_a(Editor *ed, const char *texto) {
    if (ed->fd < 0) {
        fprintf(stderr, "a: no hay ningún archivo abierto (usa o [archivo] primero)\n");
        return -1;
    }

    if (lseek(ed->fd, 0, SEEK_END) < 0) {
        perror("lseek");
        return -1;
    }

    size_t len = strlen(texto);
    size_t total_escritos = 0;
    while (total_escritos < len) {
        ssize_t escritos = write(ed->fd, texto + total_escritos, len - total_escritos);
        if (escritos < 0) {
            perror("write");
            return -1;
        }
        total_escritos += (size_t)escritos;
    }

    if (write(ed->fd, "\n", 1) < 0) {
        perror("write");
        return -1;
    }

    return 0;
}

/* s [palabra] — Búsqueda simple: imprime "N: <línea>" por cada línea
 * que contiene 'palabra'. */
int cmd_s(Editor *ed, const char *palabra) {
    if (ed->fd < 0) {
        fprintf(stderr, "s: no hay ningún archivo abierto (usa o [archivo] primero)\n");
        return -1;
    }

    size_t len;
    char *contenido = leer_archivo_completo(ed->fd, &len);
    if (!contenido) return -1;

    int linea = 1;
    size_t inicio = 0;
    int encontrados = 0;

    for (size_t i = 0; i <= len; i++) {
        if (i == len || contenido[i] == '\n') {
            char guardado = 0;
            int hay_que_restaurar = 0;
            if (i < len) {
                guardado = contenido[i];
                contenido[i] = '\0';
                hay_que_restaurar = 1;
            }
            if (strstr(contenido + inicio, palabra) != NULL) {
                printf("%d: %s\n", linea, contenido + inicio);
                encontrados++;
            }
            if (hay_que_restaurar) {
                contenido[i] = guardado;
            }
            linea++;
            inicio = i + 1;
        }
    }

    if (!encontrados) {
        printf("s: '%s' no encontrada\n", palabra);
    }

    free(contenido);
    return 0;
}

/* q — Cierra el archivo actual y libera el descriptor. */
int cmd_q(Editor *ed) {
    if (ed->fd >= 0) {
        if (close(ed->fd) < 0) {
            perror("close");
            return -1;
        }
        ed->fd = -1;
    }
    printf("Saliendo del editor.\n");
    return 0;
}
