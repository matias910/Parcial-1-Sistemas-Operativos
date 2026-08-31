/*
 * test_edicion.c — Prueba manual de cmd_d y cmd_i, independiente del
 * resto del editor (que aún no está integrado). Útil para probar el
 * módulo de Matías antes de la integración de la Semana 1, día 5-6.
 *
 * Uso:
 *   gcc -Wall -Wextra -o test_edicion edicion.c test_edicion.c
 *   ./test_edicion prueba.txt
 */

#define _POSIX_C_SOURCE 200809L /* necesario para ftruncate() con -std=c11 */

#include "editor.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/* apertura mínima, solo para esta prueba (cmd_o real lo implementa Samuel) */
static int abrir_para_prueba(Editor *ed, const char *ruta) {
    int fd = open(ruta, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    ed->fd = fd;
    strncpy(ed->ruta, ruta, EDITOR_MAX_RUTA - 1);
    ed->ruta[EDITOR_MAX_RUTA - 1] = '\0';
    return 0;
}

static void imprimir_archivo(const Editor *ed) {
    /* lectura simple solo para mostrar el resultado en la prueba */
    lseek(ed->fd, 0, SEEK_SET);
    char buf[4096];
    ssize_t leidos;
    printf("----- contenido actual -----\n");
    while ((leidos = read(ed->fd, buf, sizeof(buf) - 1)) > 0) {
        buf[leidos] = '\0';
        printf("%s", buf);
    }
    printf("\n-----------------------------\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s archivo_de_prueba.txt\n", argv[0]);
        return 1;
    }

    Editor ed = {.fd = -1};
    if (abrir_para_prueba(&ed, argv[1]) < 0) return 1;

    /* archivo de prueba con 3 líneas */
    if (ftruncate(ed.fd, 0) < 0) { perror("ftruncate"); return 1; }
    lseek(ed.fd, 0, SEEK_SET);
    const char *inicial = "linea uno\nlinea dos\nlinea tres\n";
    write(ed.fd, inicial, strlen(inicial));

    printf("Archivo inicial:\n");
    imprimir_archivo(&ed);

    printf("\n>> i 2 \"linea nueva\"\n");
    cmd_i(&ed, 2, "linea nueva");
    imprimir_archivo(&ed);

    printf("\n>> d 1\n");
    cmd_d(&ed, 1);
    imprimir_archivo(&ed);

    printf("\n>> d 99 (caso borde: fuera de rango, debe fallar con perror/mensaje)\n");
    cmd_d(&ed, 99);

    close(ed.fd);
    return 0;
}
