CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g -D_GNU_SOURCE
ARCHSALIDA = eafitOS
SRCS = main.c cat_datos.c cat_memoria.c cat_monitoreo.c cat_util.c cat_edicion.c
OBJS = $(SRCS:.c=.o)

# Ruta al módulo del editor (categoría nueva 'edicion', Parcial 1).
# Se asume la estructura de este repositorio: shell/ y src/ como
# carpetas hermanas dentro de la raíz del proyecto.
EDITOR_DIR = ../src
EDITOR_BIN = editor

all: $(ARCHSALIDA) $(EDITOR_BIN)


$(ARCHSALIDA): $(OBJS)
	$(CC) $(CFLAGS) -o $(ARCHSALIDA) $(OBJS)

# Compila el editor en src/ y copia el binario aquí, junto a eafitOS,
# para que 'e_editor' (fork+execvp de "./editor") lo encuentre en el
# directorio de trabajo actual.
$(EDITOR_BIN):
	$(MAKE) -C $(EDITOR_DIR) editor
	cp $(EDITOR_DIR)/editor ./$(EDITOR_BIN)

%.o: %.c shell.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(ARCHSALIDA) $(EDITOR_BIN)
	$(MAKE) -C $(EDITOR_DIR) clean

.PHONY: all clean
