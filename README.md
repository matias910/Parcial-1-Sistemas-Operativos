# Editor de Texto en Unix — Sistemas Operativos (EAFIT)

Evaluación práctica (primer parcial) de **Sistemas Operativos**: un editor de
texto interactivo en **C**, operado por CLI, construido sobre llamadas al
sistema POSIX de bajo nivel e integrado con el shell visto en clase
(carpeta `Shell-Clase/shell/`, sin modificar).

**Equipo (Parejas):** Samuel & Matías
**Nivel de equipo:** 2 (Parejas) — comandos base + inserción arbitraria (`i`) + búsqueda (`s`)

## 1. Objetivo

Construir un editor de texto (inspirado en `ed`/`vi`) que lea, modifique y
guarde archivos planos usando exclusivamente las siguientes syscalls:
`open`, `read`, `write`, `lseek`, `ftruncate`, `close`. Para consola se usa
únicamente `printf`/`scanf`/`fgets`. **Prohibido** `fopen`/`fread`/`fwrite`/`fclose`.

## 2. Comandos soportados

| Comando | Descripción | Responsable |
|---|---|---|
| `o [archivo]` | Abre (o crea) un archivo | Samuel |
| `p [n]` | Imprime la línea `n`, o todo el archivo sin argumento | Samuel |
| `a [texto]` | Añade `texto` como nueva línea al final | Samuel |
| `d [n]` | Borra la línea `n` | **Matías** |
| `i [n] [texto]` | Inserta `texto` como línea `n`, desplazando el resto | **Matías** |
| `s [palabra]` | Búsqueda simple de `palabra` en el archivo | Samuel |
| `q` | Cierra el archivo y sale | Samuel |

## 3. Estructura del repositorio

```
README.md
Shell-Clase/
  Banderas_CLI/       # material de clase, sin relación con este parcial
  callsys/            # material de clase, sin relación con este parcial
  shell/              # shell de clase (SO2026B), SIN MODIFICAR — referencia
    shell.h
    main.c
    cat_datos.c
    cat_memoria.c
    cat_monitoreo.c
    cat_util.c
    Makefile
  src/                # editor de texto standalone (o/p/a/d/i/s/q)
    editor.h          # struct Editor + firmas de todos los comandos (contrato compartido)
    io_utils.h/.c      # leer_archivo_completo() — compartida entre io_core.c y edicion.c
    io_core.c          # o(), p(), a(), s() — módulo de Samuel
    edicion.c          # d() e i() — módulo de Matías (buffer dinámico en memoria)
    main.c             # bucle interactivo principal del editor standalone
    test_edicion.c     # prueba independiente de d/i (sin pasar por o/p/a)
    Makefile           # compila ./editor y ./test_edicion
  shell-integrado/    # COPIA de shell/ + la integración del editor
    shell.h            # + prototipo de e_editor (categoría nueva 'edicion')
    main.c             # + registro de e_editor en la tabla de comandos y en 'help'
    cat_datos.c        # sin cambios respecto a shell/
    cat_memoria.c      # sin cambios respecto a shell/
    cat_monitoreo.c    # sin cambios respecto a shell/
    cat_util.c         # sin cambios respecto a shell/
    cat_edicion.c      # NUEVO — comando e_editor, módulo de Matías
    Makefile           # + compila cat_edicion.c y construye/copia ./editor desde ../src
  pruebas/
    test_editor.sh     # script de pruebas automatizado (17/17 verificaciones)
```

**Por qué hay dos carpetas de shell:** `shell/` es el shell de la clase tal
cual, sin ningún cambio (para dejar constancia del punto de partida).
`shell-integrado/` es una copia de esa misma carpeta con la integración del
editor ya aplicada (`cat_edicion.c` + los cambios en `shell.h` y `main.c`).
Así queda claro qué se modificó y qué no.

## 4. Cómo compilar y probar

### 4.1 Editor standalone (sin el shell)

```bash
cd Shell-Clase/src
make all            # compila ./editor y ./test_edicion
./test_edicion prueba.txt   # prueba d/i de forma aislada
./editor                    # editor interactivo: o, p, a, d, i, s, q
./editor archivo.txt        # o directamente abriendo un archivo
make clean
```

### 4.2 Shell integrado (con `e_editor`)

```bash
cd Shell-Clase/shell-integrado
make all            # compila eafitOS, compila ./editor desde ../src y lo copia aquí
```

Dentro del shell:

```
eafitOS> e_editor notas.txt
Editor de texto (o/p/a/d/i/s/q). Escribe 'q' para salir.
Archivo 'notas.txt' abierto (fd=3)
> a primera linea
> p
primera linea
> q
Saliendo del editor.
eafitOS> d_read notas.txt
primera linea
```

`make clean` en `shell-integrado/` también limpia `src/`
(`$(MAKE) -C ../src clean`), así que si vas a correr `src/test_edicion`
justo después, recuerda volver a compilar con `make -C ../src all`.

### 4.3 Script de pruebas

```bash
cd Shell-Clase/pruebas
./test_editor.sh
```

Compila el editor, corre 4 escenarios (comandos base, archivo inexistente,
casos borde, prueba unitaria de `d`/`i`) y valida cada uno con `valgrind`
si está instalado. Resultado actual: **17/17 verificaciones pasan, 0 fugas
de memoria y 0 errores** en los 4 escenarios.

## 5. Decisión de diseño: buffer dinámico (`d`, `i`)

Para `d` e `i` se optó por **cargar el archivo completo en memoria**
(arreglo dinámico de líneas con `malloc`/`realloc`) en lugar de desplazar
bytes directamente en disco con `lseek`, porque:

1. Insertar o borrar una línea implica en el caso general desplazar todos
   los bytes posteriores; hacerlo directamente en disco es propenso a
   corromper el archivo si el desplazamiento de offsets no es exacto.
2. Modelar el archivo como un arreglo de líneas en memoria permite razonar
   la operación como "insertar/quitar un elemento de un arreglo", que es
   el reto técnico que pide la rúbrica para el nivel de Pareja.
3. El costo de releer/reescribir el archivo completo es aceptable para un
   editor interactivo de texto plano sin concurrencia, como el que exige
   el enunciado.

Verificación de errores: cada syscall revisa su valor de retorno y reporta
con `perror` (o un mensaje de rango con `fprintf(stderr, ...)` para errores
de lógica como "línea fuera de rango").

## 6. Integración con el shell — categoría nueva `'edicion'`

**Decisión:** se crea `'edicion'` como categoría completamente nueva (no se
mete `e_editor` dentro de `'datos'` ni `'monitoreo'`), con un único comando
por ahora: `e_editor [archivo]`.

**Justificación:** todos los comandos existentes del shell (`datos`,
`memoria`, `monitoreo`, `utilidades`) siguen el mismo patrón: ejecutan una
syscall (o un grupo fijo de syscalls) y retornan de inmediato al prompt
`eafitOS>`. El editor, en cambio, necesita apropiarse de la entrada
estándar en un **bucle interactivo persistente** propio (o/p/a/d/i/s/q)
hasta que el usuario escriba `q`. No es una operación atómica de una
syscall, es una sesión completa — por eso amerita su propia categoría.

**Estrategia de integración:** se reutiliza el mismo patrón `fork()` +
`execvp()` que ya usa `p_exec` (categoría `monitoreo`) para delegar el
control de la terminal a un proceso hijo. La diferencia es que aquí el
binario ejecutado no lo elige el usuario: siempre es nuestro propio
`./editor` ya compilado desde `src/`. Se descartó reescribir
`o/p/a/d/i/s/q` como funciones nativas del shell porque el editor ya es un
programa independiente, correctamente probado (`test_edicion.c`), y
duplicar su lógica dentro del shell violaría el principio de una sola
fuente de verdad para esas syscalls.

**Limitación conocida (para el script de pruebas):** como `e_editor` hace
`fork+execvp`, si se prueba con entrada *pipeada* (`echo "..." | ./eafitOS`)
en vez de un terminal interactivo real, el buffer interno de `stdio` del
shell padre puede "adelantarse" y leer líneas del pipe que en realidad
iban dirigidas al editor hijo, antes de que este arranque. Esto es un
efecto conocido de mezclar `fgets` bufferizado con `fork+exec` sobre un
pipe (le pasa a cualquier shell simple, no solo a este). **No ocurre con
uso interactivo normal desde terminal.** El script `pruebas/test_editor.sh`
evita este problema porque prueba el binario `./editor` directamente, sin
pasar por el shell.

## 7. Checklist contra la rúbrica

- [x] `d`, `i` implementados con syscalls de bajo nivel únicamente
- [x] Verificación de retorno + `perror` en cada syscall
- [x] Buffers dinámicos con `malloc`/`realloc`, liberados correctamente (`free`)
- [x] Caso borde probado: línea fuera de rango
- [x] Integración de `d`/`i` con `o`/`p`/`a`/`s` de Samuel en un único `editor`
- [x] Categoría `'edicion'` en el shell (`e_editor`, patrón `fork+execvp` de `p_exec`)
- [x] `valgrind` sobre el binario final integrado — 0 fugas, 0 errores
- [x] Script de pruebas (`pruebas/test_editor.sh`) — 17/17 verificaciones pasan
- [ ] Documento PDF de sustentación
- [ ] Video explicativo (5–7 min)
