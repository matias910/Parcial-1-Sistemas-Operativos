#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stddef.h>

/* Lee todo el contenido del fd (desde el inicio del archivo) a un buffer
 * malloc'eado y null-terminado. El llamador debe hacer free() del
 * resultado. Devuelve NULL en error (ya reportado con perror). */
char *leer_archivo_completo(int fd, size_t *out_len);

#endif /* IO_UTILS_H */
