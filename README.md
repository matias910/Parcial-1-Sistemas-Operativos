# Editor de Texto en Unix — Sistemas Operativos (EAFIT)

Evaluación práctica (primer parcial) de **Sistemas Operativos**: un editor de
texto interactivo en **C**, operado por CLI, construido sobre llamadas al
sistema POSIX de bajo nivel e integrado con el shell visto en clase.

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
src/
  editor.h        # struct Editor + firmas de todos los comandos (contrato compartido)
  edicion.c        # d() e i() — módulo de Matías (buffer dinámico)
  test_edicion.c   # prueba independiente de d/i, sin depender del resto del editor
  Makefile         # targets: all, clean
  (pendiente) io_core.c   # o(), p(), a(), s() + bucle principal — módulo de Samuel
  (pendiente) shell.c     # shell de clase + categoría 'edicion' integrada
```

## 4. Cómo compilar y probar (módulo de Matías)

```bash
cd src
make all           # compila edicion.o + test_edicion.o -> ./test_edicion
./test_edicion prueba.txt
make clean
```

`test_edicion.c` abre un archivo de prueba directamente con `open()` (sin
pasar por `cmd_o`, que todavía no está fusionado) y ejercita `cmd_i` y
`cmd_d`, incluyendo un caso borde (línea fuera de rango).

Una vez fusionado el módulo de Samuel (`o`, `p`, `a`, bucle principal), el
`Makefile` debe actualizarse para compilar todos los `.c` en un único
binario `editor`, como pide el enunciado.

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

## 6. Integración con el shell — categoría `'edicion'` (**pendiente**)

Según el plan de trabajo, el editor debe invocarse desde el shell de clase
(repositorio `SO2026B - Shell`) bajo una categoría nueva, `'edicion'`,
justificada porque el editor requiere un **bucle interactivo persistente**
(un `read`/`fgets` en ciclo hasta `q`), a diferencia del resto de comandos
del shell, que siguen el patrón de una-syscall-por-comando y retornan de
inmediato (`p_exec`).

**Esto todavía no está implementado en este repo** porque requiere el
código real del shell (`p_exec` y cómo despacha categorías de comandos).
Para escribir la integración exacta hace falta:

- El archivo (o los archivos) del shell donde vive `p_exec` y el
  despachador de comandos/categorías.
- Confirmar si el editor se invoca vía `fork`+`execvp` a un binario aparte,
  o como llamada directa a una función dentro del mismo proceso del shell.

## 7. Checklist contra la rúbrica

- [x] `d`, `i` implementados con syscalls de bajo nivel únicamente
- [x] Verificación de retorno + `perror` en cada syscall
- [x] Buffers dinámicos con `malloc`/`realloc`, liberados correctamente (`free`)
- [x] Caso borde probado: línea fuera de rango
- [ ] Integración de `d`/`i` con `o`/`p`/`a` de Samuel en un único `editor`
- [ ] Categoría `'edicion'` en el shell (pendiente código del shell)
- [ ] `valgrind` sobre el binario final integrado
- [ ] Documento PDF de sustentación
- [ ] Video explicativo (5–7 min)
