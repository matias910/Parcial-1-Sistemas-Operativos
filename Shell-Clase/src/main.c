/*
 * main.c — Ciclo interactivo principal del editor
 * Responsable: Samuel (bucle base) / Matías (integración con el shell)
 *
 * Lee comandos de STDIN con fgets, los interpreta (primer caracter no
 * blanco = comando, resto de la línea = argumentos) y despacha a la
 * función correspondiente de editor.h. Corre hasta recibir 'q' o EOF.
 *
 * Integración con el shell (categoría nueva 'edicion', ver cat_edicion.c
 * en la shell y el PDF de sustentación):
 *   Este binario se invoca desde el shell de clase mediante
 *   fork()+execvp() (mismo patrón que usa p_exec para binarios externos).
 *   Para que el flujo sea natural desde el shell -- "e_editor archivo.txt"
 *   debe abrir el archivo de inmediato, como haría 'vi archivo.txt' --
 *   este main() acepta un argumento opcional: si se invoca con
 *   ./editor <archivo>, se llama a cmd_o() automáticamente antes de
 *   iniciar el ciclo interactivo. Sin argumentos, el editor arranca sin
 *   ningún archivo abierto y se debe usar 'o [archivo]' manualmente.
 *
 * Razón por la que 'edicion' es una categoría nueva del shell y no un
 * comando más dentro de una existente: a diferencia de todos los demás
 * comandos (una syscall o un grupo fijo de syscalls, con retorno
 * inmediato al prompt eafitOS>), este programa necesita apropiarse de la
 * entrada estándar en un ciclo propio hasta que el usuario escriba 'q'.
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

int main(int argc, char *argv[]) {
    Editor ed;
    ed.fd = -1;
    ed.ruta[0] = '\0';

    char linea[1024];

    printf("Editor de texto (o/p/a/d/i/s/q). Escribe 'q' para salir.\n");

    /* Si se invoca como './editor archivo.txt' (caso del shell con
     * 'e_editor archivo.txt'), se abre el archivo automáticamente. */
    if (argc == 2) {
        cmd_o(&ed, argv[1]);
    }

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
