#!/bin/bash
# ============================================================================
# pruebas/test_editor.sh — Script de pruebas del editor de texto
# ============================================================================
# Entregable "Código de Demostración" (rúbrica, Sección 6.2): ejecuta y
# valida los comandos requeridos (o/p/a/d/i/s/q) contra el binario ya
# compilado en ../src/editor, incluyendo los casos borde exigidos por el
# enunciado (Día 11 del plan de trabajo):
#   - archivo inexistente (o debe crearlo, ya que usa O_CREAT)
#   - línea fuera de rango (p, d, i)
#   - insertar en línea 0 (inválida)
#   - borrar en archivo vacío
# Si valgrind está instalado, además valida 0 fugas de memoria y 0 errores.
#
# Uso:
#   cd pruebas
#   ./test_editor.sh
#
# Código de salida: 0 si todo pasó, 1 si algo falló (útil para CI).
# ============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/../src"
WORKDIR=$(mktemp -d)
FALLOS=0
TOTAL=0

VERDE='\033[0;32m'
ROJO='\033[0;31m'
AMARILLO='\033[1;33m'
RESET='\033[0m'

cleanup() {
    rm -rf "$WORKDIR"
}
trap cleanup EXIT

check() {
    # check "descripción" "archivo_salida" "patrón_esperado"
    local desc="$1" salida="$2" patron="$3"
    TOTAL=$((TOTAL + 1))
    if grep -qF "$patron" "$salida"; then
        echo -e "  ${VERDE}[OK]${RESET} $desc"
    else
        echo -e "  ${ROJO}[FALLO]${RESET} $desc (no se encontró: \"$patron\")"
        FALLOS=$((FALLOS + 1))
    fi
}

check_valgrind_limpio() {
    # check_valgrind_limpio "descripción" "archivo_stderr_valgrind"
    local desc="$1" archivo="$2"
    TOTAL=$((TOTAL + 1))
    if grep -q "All heap blocks were freed -- no leaks are possible" "$archivo" \
       && grep -q "ERROR SUMMARY: 0 errors" "$archivo"; then
        echo -e "  ${VERDE}[OK]${RESET} $desc (0 fugas, 0 errores)"
    else
        echo -e "  ${ROJO}[FALLO]${RESET} $desc"
        FALLOS=$((FALLOS + 1))
    fi
}

echo "============================================================"
echo " Compilando el editor (make -C src)"
echo "============================================================"
make -C "$SRC_DIR" clean >/dev/null 2>&1
if ! make -C "$SRC_DIR" all > "$WORKDIR/build.log" 2>&1; then
    echo -e "${ROJO}La compilación falló. Ver $WORKDIR/build.log${RESET}"
    cat "$WORKDIR/build.log"
    exit 1
fi
echo -e "${VERDE}Compilación exitosa (0 warnings esperados con -Wall -Wextra).${RESET}"
if grep -qi "warning" "$WORKDIR/build.log"; then
    echo -e "${AMARILLO}[AVISO] Se detectaron warnings en la compilación:${RESET}"
    grep -i "warning" "$WORKDIR/build.log"
fi

EDITOR="$SRC_DIR/editor"
TIENE_VALGRIND=0
if command -v valgrind >/dev/null 2>&1; then
    TIENE_VALGRIND=1
else
    echo -e "${AMARILLO}[AVISO] valgrind no está instalado; se omite la verificación de fugas.${RESET}"
fi

correr() {
    # correr <archivo_comandos> <archivo_salida_combinada> [archivo_stderr_valgrind]
    # 'salida' siempre contiene stdout+stderr DEL PROGRAMA (para revisar
    # mensajes de error con perror/fprintf). Si hay valgrind, sus propios
    # diagnósticos van aparte en 'verr' (comparten el mismo fd físico que
    # el stderr del programa, así que se separan por post-procesamiento).
    local comandos="$1" salida="$2" verr="${3:-}"
    if [ "$TIENE_VALGRIND" -eq 1 ] && [ -n "$verr" ]; then
        valgrind --leak-check=full --error-exitcode=99 \
            "$EDITOR" < "$comandos" > "$salida.stdout" 2> "$verr"
        # Todo lo que no es línea de diagnóstico de valgrind ('==PID==')
        # es salida real del programa (stdout ya capturado aparte, más
        # los mensajes de error que el programa mandó a stderr).
        grep -v '^==[0-9]*==' "$verr" > "$salida.progerr" || true
        cat "$salida.stdout" "$salida.progerr" > "$salida"
    else
        "$EDITOR" < "$comandos" > "$salida" 2>&1
    fi
}

echo ""
echo "============================================================"
echo " Escenario 1: comandos base completos (o/a/i/p/d/s/q)"
echo "============================================================"
cd "$WORKDIR"
printf 'o notas.txt\na primera linea\na segunda linea\na tercera linea\ni 2 linea insertada\np\nd 1\np\ns linea\nq\n' > cmds1.txt
correr cmds1.txt out1.txt vg1.txt

check "o crea el archivo" out1.txt "abierto (fd="
check "a agrega líneas (verificado vía p)" out1.txt "primera linea"
check "i inserta en la posición correcta" out1.txt "linea insertada"
check "p tras d ya no muestra la línea borrada" out1.txt $'linea insertada\nsegunda linea\ntercera linea'
check "s encuentra coincidencias por número de línea" out1.txt "1: linea insertada"
[ "$TIENE_VALGRIND" -eq 1 ] && check_valgrind_limpio "Escenario 1 sin fugas de memoria" vg1.txt

echo ""
echo "============================================================"
echo " Escenario 2: archivo inexistente (o debe crearlo con O_CREAT)"
echo "============================================================"
rm -f nuevo.txt
printf 'o nuevo.txt\na contenido nuevo\np\nq\n' > cmds2.txt
correr cmds2.txt out2.txt vg2.txt
check "o crea un archivo que no existía" out2.txt "abierto (fd="
[ -f nuevo.txt ] && check "el archivo quedó realmente en disco" <(echo "existe: si") "existe: si" \
    || { echo -e "  ${ROJO}[FALLO]${RESET} el archivo no se creó en disco"; FALLOS=$((FALLOS+1)); TOTAL=$((TOTAL+1)); }
[ "$TIENE_VALGRIND" -eq 1 ] && check_valgrind_limpio "Escenario 2 sin fugas de memoria" vg2.txt

echo ""
echo "============================================================"
echo " Escenario 3: casos borde (línea 0, fuera de rango, archivo vacío)"
echo "============================================================"
touch vacio.txt
printf 'o vacio.txt\nd 1\ni 0 texto invalido\ni 5 fuera de rango\np 3\nq\n' > cmds3.txt
correr cmds3.txt out3.txt vg3.txt
check "d en archivo vacío reporta error controlado" out3.txt "está fuera de rango"
check "i en línea 0 se rechaza (número de línea inválido)" out3.txt "número de línea inválido: 0"
check "i más allá del máximo permitido se rechaza" out3.txt "máximo permitido"
check "p de línea inexistente reporta error controlado" out3.txt "la línea 3 está fuera de rango"
[ "$TIENE_VALGRIND" -eq 1 ] && check_valgrind_limpio "Escenario 3 sin fugas de memoria (casos borde)" vg3.txt

echo ""
echo "============================================================"
echo " Escenario 4: prueba unitaria de d/i (test_edicion)"
echo "============================================================"
rm -f prueba_unitaria.txt
if [ "$TIENE_VALGRIND" -eq 1 ]; then
    valgrind --leak-check=full --error-exitcode=99 \
        "$SRC_DIR/test_edicion" prueba_unitaria.txt > out4.txt.stdout 2> vg4.txt
    grep -v '^==[0-9]*==' vg4.txt > out4.txt.progerr || true
    cat out4.txt.stdout out4.txt.progerr > out4.txt
else
    "$SRC_DIR/test_edicion" prueba_unitaria.txt > out4.txt 2>&1
fi
check "insertar y borrar en test_edicion funciona" out4.txt "linea nueva"
check "caso borde línea 99 se rechaza" out4.txt "línea 99 está fuera de rango"
[ "$TIENE_VALGRIND" -eq 1 ] && check_valgrind_limpio "test_edicion sin fugas de memoria" vg4.txt

echo ""
echo "============================================================"
echo " RESUMEN: $((TOTAL - FALLOS))/$TOTAL verificaciones pasaron"
echo "============================================================"
if [ "$FALLOS" -eq 0 ]; then
    echo -e "${VERDE}TODAS LAS PRUEBAS PASARON${RESET}"
    exit 0
else
    echo -e "${ROJO}$FALLOS VERIFICACIÓN(ES) FALLARON${RESET}"
    exit 1
fi
