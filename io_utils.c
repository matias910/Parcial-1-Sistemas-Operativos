#define _POSIX_C_SOURCE 200809L

#include "io_utils.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

char *leer_archivo_completo(int fd, size_t *out_len) {
    off_t tam = lseek(fd, 0, SEEK_END);
    if (tam < 0) {
        perror("lseek");
        return NULL;
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek");
        return NULL;
    }

    char *buf = malloc((size_t)tam + 1);
    if (!buf) {
        perror("malloc");
        return NULL;
    }

    size_t total_leidos = 0;
    while (total_leidos < (size_t)tam) {
        ssize_t leidos = read(fd, buf + total_leidos, (size_t)tam - total_leidos);
        if (leidos < 0) {
            perror("read");
            free(buf);
            return NULL;
        }
        if (leidos == 0) break; /* EOF antes de lo esperado */
        total_leidos += (size_t)leidos;
    }
    buf[total_leidos] = '\0';
    if (out_len) *out_len = total_leidos;
    return buf;
}
