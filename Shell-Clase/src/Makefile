CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

# Binario principal: editor de texto completo integrado (o/p/a/d/i/s/q)
FUENTES_EDITOR = main.c io_core.c edicion.c io_utils.c
OBJS_EDITOR = $(FUENTES_EDITOR:.c=.o)

# Prueba independiente del módulo de Matías (d, i), sin pasar por main.c
FUENTES_TEST = test_edicion.c edicion.c io_utils.c
OBJS_TEST = $(FUENTES_TEST:.c=.o)

all: editor test_edicion

editor: $(OBJS_EDITOR)
	$(CC) $(CFLAGS) -o $@ $^

test_edicion: $(OBJS_TEST)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c editor.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o editor test_edicion

.PHONY: all clean
