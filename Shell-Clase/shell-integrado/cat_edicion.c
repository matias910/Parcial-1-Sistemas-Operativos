#include "shell.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/**
 * ====================================================================================
 * COMANDO: e_editor [archivo]      —      CATEGORÍA NUEVA: 'edicion'
 * ====================================================================================
 * Responsable: Matías (integración con el shell, ver plan de trabajo).
 *
 * Por qué es una categoría nueva y no un comando dentro de 'datos' o
 * 'monitoreo' (decisión que pide justificar el enunciado, Sección 2):
 *
 * Todos los comandos existentes del shell —sin importar su categoría—
 * comparten el mismo patrón: ejecutan una syscall (o un grupo fijo y
 * acotado de syscalls) y retornan de inmediato al prompt 'eafitOS>'.
 * Ninguno retiene el control de la entrada estándar más allá de sus
 * propios argumentos.
 *
 * El editor de texto del Parcial 1 (o/p/a/d/i/s/q) rompe ese patrón: es
 * un programa interactivo completo que necesita apropiarse de STDIN en
 * un ciclo propio (su propio REPL) hasta que el usuario decida cerrarlo
 * con 'q'. Meterlo dentro de 'datos' (por manipular archivos) o
 * 'monitoreo' (por usar fork/exec) sería engañoso para la tabla de
 * ayuda del shell, ya que no es una operación atómica de una syscall:
 * es una sesión completa. Por eso se crea 'edicion' como categoría
 * propia, con un único comando por ahora: e_editor.
 *
 * Estrategia de integración elegida:
 * Se reutiliza el mismo patrón fork()+execvp() que ya usa p_exec
 * (categoría 'monitoreo') para delegar el control de la terminal a un
 * proceso hijo. La diferencia es que aquí el binario ejecutado no lo
 * elige el usuario: siempre es nuestro propio editor ya compilado
 * (./editor, ver src/ del repositorio), construido a partir de las
 * syscalls de bajo nivel exigidas en el enunciado (open, read, write,
 * lseek, ftruncate, close). Se descartó reescribir o/p/a/d/i/s/q como
 * funciones nativas del shell porque el editor ya es un programa
 * independiente y correctamente probado (test_edicion.c); duplicar su
 * lógica dentro del shell violaría el principio de una sola fuente de
 * verdad para esas syscalls.
 *
 * Syscalls explicadas:
 * 1. fork(2): crea el proceso hijo que se convertirá en el editor.
 * 2. execvp(3): reemplaza la imagen del hijo por el binario ./editor,
 *    heredando los descriptores 0/1/2 (STDIN/STDOUT/STDERR) para que el
 *    usuario interactúe con el editor como si fuera un programa aparte.
 * 3. waitpid(2): el shell padre queda bloqueado mientras el usuario
 *    edita; al recibir 'q' (o EOF) el editor termina y el shell
 *    recupera el prompt 'eafitOS>'.
 */
int cmd_e_editor(int argc, char **argv) {
    /* 1. LLAMADA AL SISTEMA: fork */
    LOG_SYSCALL("fork", "");
    pid_t pid = fork();
    if (pid == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }

    if (pid == 0) {
        /* PROCESO HIJO: se convierte en el editor de texto */
        char *exec_args[3];
        exec_args[0] = (char *)"./editor";
        exec_args[1] = (argc > 1) ? argv[1] : NULL;
        exec_args[2] = NULL;

        /* 2. LLAMADA AL SISTEMA: execvp */
        if (argc > 1) {
            LOG_SYSCALL("execvp", "\"./editor\", \"%s\"", argv[1]);
        } else {
            LOG_SYSCALL("execvp", "\"./editor\"");
        }
        printf("\n" COLOR_INFO "[edicion] Cediendo el control de la terminal al editor de texto...\n" COLOR_RESET);
        printf(COLOR_INFO "[edicion] Escribe 'q' dentro del editor para volver al shell.\n\n" COLOR_RESET);

        execvp(exec_args[0], exec_args);

        /* SI execvp TIENE ÉXITO, ESTA LÍNEA NUNCA SE EJECUTA.
         * Si llegamos aquí, el binario ./editor no se encontró o falló. */
        LOG_SYSCALL_ERROR(strerror(errno));
        fprintf(stderr, COLOR_ERROR
                "Error: no se pudo ejecutar './editor'. Compílalo primero con "
                "'make' dentro de src/ y copia (o enlaza) el binario 'editor' "
                "junto al ejecutable del shell (eafitOS).\n" COLOR_RESET);
        exit(127); /* Código estándar de shell para "comando no encontrado" */
    }

    /* PROCESO PADRE: espera a que el usuario cierre el editor */
    LOG_SYSCALL_RESULT(pid);
    printf(COLOR_PROMPT "[Padre]" COLOR_RESET " Editor lanzado (PID %d). Esperando a que se cierre...\n", pid);

    int status;
    /* 3. LLAMADA AL SISTEMA: waitpid */
    LOG_SYSCALL("waitpid", "%d, &status, 0", pid);
    pid_t waited_pid = waitpid(pid, &status, 0);
    if (waited_pid == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    LOG_SYSCALL_RESULT(waited_pid);

    if (WIFEXITED(status)) {
        printf(COLOR_PROMPT "[Padre]" COLOR_RESET " Editor cerrado. Código de salida: "
               COLOR_RESULT "%d" COLOR_RESET "\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf(COLOR_PROMPT "[Padre]" COLOR_RESET " Editor terminado por señal: "
               COLOR_ERROR "%d" COLOR_RESET "\n", WTERMSIG(status));
    }

    return 0;
}
