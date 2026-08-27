/*
 * main.c — Ciclo interactivo principal del editor
 * Responsable: Samuel
 *
 * Lee comandos de STDIN con fgets, los interpreta (primer caracter no
 * blanco = comando, resto de la línea = argumentos) y despacha a la
 * función correspondiente de editor.h. Corre hasta recibir 'q' o EOF.
 *
 * NOTA de integración con el shell (Sección 2 del enunciado, pendiente):
 * este mismo bucle es la razón por la que 'edicion' debe ser una
 * categoría nueva en el shell de clase: a diferencia del resto de
 * comandos del shell (una syscall, retorno inmediato), este programa
 * necesita mantener el control de la terminal en un ciclo propio hasta
 * que el usuario escriba 'q'. Falta el código del shell (SO2026B) para
 * escribir la integración exacta (fork+execvp de este binario, o
 * llamada directa a esta función desde el shell).
 */

#include "editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void quitar_salto_linea(char *s) {
    size_t l = strlen(s);
    if (l > 0 && s[l - 1] == '\n') {
        s[l - 1] = '\0';
    }
}

int main(void) {
    Editor ed;
    ed.fd = -1;
    ed.ruta[0] = '\0';

    char linea[1024];

    printf("Editor de texto (o/p/a/d/i/s/q). Escribe 'q' para salir.\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(linea, sizeof(linea), stdin)) {
            break; /* EOF (Ctrl+D) */
        }
        quitar_salto_linea(linea);
        if (linea[0] == '\0') continue;

        char comando;
        int consumidos = 0;
        if (sscanf(linea, " %c %n", &comando, &consumidos) < 1) {
            continue;
        }

        char *resto = linea + consumidos;
        while (*resto == ' ') resto++; /* saltar espacios extra */

        switch (comando) {
            case 'o':
                if (*resto == '\0') {
                    fprintf(stderr, "uso: o [archivo]\n");
                    break;
                }
                cmd_o(&ed, resto);
                break;

            case 'p': {
                int n = (*resto != '\0') ? atoi(resto) : 0;
                cmd_p(&ed, n);
                break;
            }

            case 'a':
                if (*resto == '\0') {
                    fprintf(stderr, "uso: a [texto]\n");
                    break;
                }
                cmd_a(&ed, resto);
                break;

            case 'd':
                if (*resto == '\0') {
                    fprintf(stderr, "uso: d [n]\n");
                    break;
                }
                cmd_d(&ed, atoi(resto));
                break;

            case 'i': {
                int n;
                int desplazamiento = 0;
                if (sscanf(resto, "%d %n", &n, &desplazamiento) < 1) {
                    fprintf(stderr, "uso: i [n] [texto]\n");
                    break;
                }
                cmd_i(&ed, n, resto + desplazamiento);
                break;
            }

            case 's':
                if (*resto == '\0') {
                    fprintf(stderr, "uso: s [palabra]\n");
                    break;
                }
                cmd_s(&ed, resto);
                break;

            case 'q':
                cmd_q(&ed);
                return 0;

            default:
                fprintf(stderr, "Comando desconocido: %c\n", comando);
        }
    }

    /* EOF sin 'q' explícito: cerrar el archivo si seguía abierto */
    if (ed.fd >= 0) {
        cmd_q(&ed);
    }
    return 0;
}
