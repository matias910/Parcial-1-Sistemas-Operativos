#ifndef EDITOR_H
#define EDITOR_H

#include <stddef.h>

#define EDITOR_MAX_RUTA 256

/*
 * Contrato compartido del equipo (Día 1 del plan de trabajo).
 * Todas las funciones reciben un puntero a este struct para
 * operar sobre el archivo actualmente abierto con 'o'.
 */
typedef struct {
    int  fd;                     /* descriptor de archivo abierto con o(); -1 si no hay archivo abierto */
    char ruta[EDITOR_MAX_RUTA];  /* ruta del archivo actualmente abierto */
} Editor;

/* --- Comandos base (Sección 3 del enunciado) --- SAMUEL --- */
int cmd_o(Editor *ed, const char *ruta);
int cmd_p(Editor *ed, int n);              /* n == 0 -> imprime todo el archivo */
int cmd_a(Editor *ed, const char *texto);
int cmd_q(Editor *ed);

/* --- Comandos de nivel Pareja (Sección 4) --- MATÍAS --- */
int cmd_d(Editor *ed, int n);
int cmd_i(Editor *ed, int n, const char *texto);

/* --- Comando de búsqueda (nivel Pareja) --- SAMUEL --- */
int cmd_s(Editor *ed, const char *palabra);

#endif /* EDITOR_H */
