#include "entradasalida.h"

int socket_kernel;
int socket_memoria;
int socket_io;
t_log *logger_io;
t_log *logger_obligatorio_io;
pthread_t conexion_kernel;
pthread_t conexion_memoria;
pthread_t ejecutar_instruccion;
pthread_mutex_t m_pid_usando;
sem_t *sem_esperar_respuesta;
t_config_io *config_io;
uint32_t tipo_interfaz;
char *nombre_interfaz;
int pid_usando_interfaz;

t_bitarray *bitmap;
void *bloques;
t_dictionary *archivos;

// -----------------------------------------------------------------------------------------
// -- pruebas operaciones --
// -----------------------------------------------------------------------------------------
void prueba_memoria_io_fs_read(char *nombre_archivo)
{
    log_info(logger_io, "Iniciando prueba memoria io fs read");
    t_ejecutar_io_fs_read *fs_read = malloc(sizeof(t_ejecutar_io_fs_read));
    fs_read->pid = 1;
    fs_read->dir_fisica = malloc(sizeof(t_direccion_fisica));
    fs_read->dir_fisica->desplazamiento_fisico = 0;
    fs_read->dir_fisica->marco = 0;
    fs_read->dir_fisica->pid = 1;
    fs_read->nombre_archivo = malloc(strlen(nombre_archivo) + 1);
    strcpy(fs_read->nombre_archivo, nombre_archivo);
    fs_read->posicion_archivo = 0;
    fs_read->tamanio = 10;

    readline("Presione enter para continuar");

    io_fs_read(fs_read);
    log_info(logger_io, "Termine prueba memoria io fs read");
}

void prueba_memoria_io_fs_write(char *nombre_archivo)
{
    log_info(logger_io, "Iniciando prueba memoria io fs write");
    t_ejecutar_io_fs_write *fs_write = malloc(sizeof(t_ejecutar_io_fs_write));
    fs_write->pid = 1;
    fs_write->dir_fisica = malloc(sizeof(t_direccion_fisica));
    fs_write->dir_fisica->desplazamiento_fisico = 0;
    fs_write->dir_fisica->marco = 0;
    fs_write->dir_fisica->pid = 1;
    fs_write->nombre_archivo = malloc(strlen(nombre_archivo) + 1);
    strcpy(fs_write->nombre_archivo, nombre_archivo);
    fs_write->posicion_archivo = 20;
    fs_write->tamanio = 10;

    readline("Presione enter para continuar");

    io_fs_write(fs_write);
    log_info(logger_io, "Termine prueba memoria io fs write");
}

void prueba_io_fs_create(char *nombre_archivo)
{
    log_info(logger_io, "Iniciando prueba io fs create");
    t_ejecutar_io_fs_create *fs_create = malloc(sizeof(t_ejecutar_io_fs_create));
    fs_create->pid = 1;
    fs_create->nombre_archivo = malloc(strlen(nombre_archivo) + 1);
    strcpy(fs_create->nombre_archivo, nombre_archivo);

    readline("Presione enter para continuar");

    io_fs_create(fs_create);
    log_info(logger_io, "Termine prueba memoria io fs create");
}

void prueba_io_fs_delete(char *nombre_archivo)
{
    log_info(logger_io, "Iniciando prueba io fs delete");
    t_ejecutar_io_fs_delete *fs_delete = malloc(sizeof(t_ejecutar_io_fs_delete));
    fs_delete->pid = 1;
    fs_delete->nombre_archivo = malloc(strlen(nombre_archivo) + 1);
    strcpy(fs_delete->nombre_archivo, nombre_archivo);

    readline("Presione enter para continuar");

    io_fs_delete(fs_delete);
    log_info(logger_io, "Termine prueba memoria io fs delete");
}

void prueba_io_fs_truncate(char *nombre_archivo, int tamanio_a_truncar)
{
    log_info(logger_io, "Iniciando prueba io fs truncate");
    t_ejecutar_io_fs_truncate *fs_truncate = malloc(sizeof(t_ejecutar_io_fs_truncate));
    fs_truncate->pid = 1;
    fs_truncate->nombre_archivo = malloc(strlen(nombre_archivo) + 1);
    strcpy(fs_truncate->nombre_archivo, nombre_archivo);
    fs_truncate->tamanio_a_truncar = tamanio_a_truncar;

    readline("Presione enter para continuar");

    io_fs_truncate(fs_truncate);
    log_info(logger_io, "Termine prueba memoria io fs truncate");
}

t_config_io *levantar_configs(char *ruta_archivo)
{
    iniciar_config(ruta_archivo);
    t_config_io *config_io = leer_configs();
    return config_io;
}

void loguear_configs(t_config_io *config_io)
{
    log_info(logger_io, "Tipo interfaz: %s\n"
                        "Tiempo unidad trabajo: %d\n"
                        "IP Kernel: %s\n"
                        "Puerto Kernel: %d\n"
                        "IP Memoria: %s\n"
                        "Puerto Memoria: %d\n"
                        "Path base dialfs: %s\n"
                        "Block size: %d\n"
                        "Block count: %d\n",
             config_io->tipo_interfaz, config_io->tiempo_unidad_trabajo,
             config_io->ip_kernel, config_io->puerto_kernel,
             config_io->ip_memoria, config_io->puerto_memoria,
             config_io->path_base_dialfs, config_io->block_size,
             config_io->block_count);
}

void usarTiempoUnidadesTrabajo()
{
    float tiempo_unidades_trabajo = config_io->tiempo_unidad_trabajo / 1000.0;
    sleep(tiempo_unidades_trabajo);
}

void *io_gen_sleep(void *args)
{
    t_ejecutar_io_gen_sleep *io_sleep = (t_ejecutar_io_gen_sleep *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_GEN_SLEEP\"", io_sleep->pid);
    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_GEN_SLEEP>", io_sleep->pid);
    float tiempo_unidades_trabajo = config_io->tiempo_unidad_trabajo / 1000.0; // viene en MS
    float total = tiempo_unidades_trabajo * io_sleep->unidades_de_trabajo;
    sleep(total);

    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);

    t_paquete *fin_procesamiento = serializar_fin_io(nombre_interfaz, &io_sleep->pid);
    enviar_paquete(fin_procesamiento, socket_kernel, logger_io);

    free(io_sleep);
    return "";
}

void *io_stdin_read(void *args)
{
    t_ejecutar_io_stdin_read *io_read = (t_ejecutar_io_stdin_read *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_STDIN_READ\"", io_read->pid);
    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_STDIN_READ>", io_read->pid);
    char *linea = readline("Ingrese que quiere escribir en memoria: ");
    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);
    t_paquete *paq = serializar_io_stdin(&io_read->pid, io_read->dir_fisica, io_read->tamanio, linea);
    enviar_paquete(paq, socket_memoria, logger_io);

    free(linea);
    free(io_read->dir_fisica);
    free(io_read->tamanio);
    free(io_read);
    return "";
}

void *io_stdout_write(void *args)
{
    t_ejecutar_io_stdout_write *io_write = (t_ejecutar_io_stdout_write *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_STDOUT_WRITE\"", io_write->pid);

    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_STDOUT_WRITE>", io_write->pid);

    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_STDOUT_WRITE>", io_write->pid);

    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);
    t_paquete *paq = serializar_io_stdout(&io_write->pid, io_write->dir_fisica, io_write->tamanio);
    enviar_paquete(paq, socket_memoria, logger_io);

    free(io_write->dir_fisica);
    free(io_write);
    return "";
}

void *io_fs_create(void *args)
{
    t_ejecutar_io_fs_create *fs_create = (t_ejecutar_io_fs_create *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_FS_CREATE\"", fs_create->pid);
    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_FS_CREATE>", fs_create->pid);
    // Demora en realizar la operacion
    usarTiempoUnidadesTrabajo();

    log_info(logger_obligatorio_io, "PID: <%d> - Crear Archivo: <%s>", fs_create->pid, fs_create->nombre_archivo);
    crearArchivo(&fs_create->pid, fs_create->nombre_archivo);

    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);

    t_paquete *paq = serializar_fin_io(nombre_interfaz, &fs_create->pid);
    enviar_paquete(paq, socket_kernel, logger_io);
    free(fs_create->nombre_archivo);
    free(fs_create);
    return "";
}

void *io_fs_delete(void *args)
{
    t_ejecutar_io_fs_delete *fs_delete = (t_ejecutar_io_fs_delete *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_FS_DELETE\"", fs_delete->pid);
    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_FS_DELETE>", fs_delete->pid);
    // Demora en realizar la operacion
    usarTiempoUnidadesTrabajo();

    eliminarArchivo(&fs_delete->pid, fs_delete->nombre_archivo);
    log_info(logger_obligatorio_io, "PID: <%d> - Eliminar Archivo: <%s>", fs_delete->pid, fs_delete->nombre_archivo);

    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);

    t_paquete *paq = serializar_fin_io(nombre_interfaz, &fs_delete->pid);
    enviar_paquete(paq, socket_kernel, logger_io);

    // free(fs_delete->nombre_archivo);
    // free(fs_delete);
    return "";
}

void *io_fs_truncate(void *args)
{
    t_ejecutar_io_fs_truncate *fs_truncate = (t_ejecutar_io_fs_truncate *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_FS_TRUNCATE\"", fs_truncate->pid);
    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_FS_TRUNCATE>", fs_truncate->pid);

    // Demora en realizar la operacion
    usarTiempoUnidadesTrabajo();

    truncarArchivo(&fs_truncate->pid, fs_truncate->nombre_archivo, fs_truncate->tamanio_a_truncar);
    log_info(logger_obligatorio_io, "PID: <%d> - Truncar Archivo: <%s> - Tamaño: <%d>", fs_truncate->pid, fs_truncate->nombre_archivo, fs_truncate->tamanio_a_truncar);

    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);
    t_paquete *paq = serializar_fin_io(nombre_interfaz, &fs_truncate->pid);
    enviar_paquete(paq, socket_kernel, logger_io);

    free(fs_truncate->nombre_archivo);
    free(fs_truncate);
    return "";
}

void *io_fs_read(void *args)
{
    t_ejecutar_io_fs_read *fs_read = (t_ejecutar_io_fs_read *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_FS_READ\"", fs_read->pid);
    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_FS_READ>", fs_read->pid);

    void *a_escribir_en_memoria = calloc(fs_read->tamanio + 1, sizeof(char));
    t_archivo *archivo = dictionary_get(archivos, fs_read->nombre_archivo);
    void *comienzo_archivo = bloques + archivo->bloque_inicial * config_io->block_size;
    memcpy(a_escribir_en_memoria, comienzo_archivo + fs_read->posicion_archivo, fs_read->tamanio);
    log_info(logger_obligatorio_io, "PID: <%d> - Leer Archivo: <%s> - Tamaño: <%d>- Puntero Archivo: <%d>", fs_read->pid, fs_read->nombre_archivo, fs_read->tamanio, fs_read->posicion_archivo);

    t_paquete *paq = serializar_io_fs_read(fs_read->pid, fs_read->dir_fisica, fs_read->tamanio, a_escribir_en_memoria);
    enviar_paquete(paq, socket_memoria, logger_io);

    // Demora en realizar la operacion
    usarTiempoUnidadesTrabajo();

    log_info(logger_io, "Esperando respuesta de memoria");
    sem_wait(sem_esperar_respuesta);
    log_info(logger_io, "Finalizo io_fs_read");

    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);

    // Avisar a kernel que termino la operacion
    t_paquete *paquete_fin_io = serializar_fin_io(nombre_interfaz, &fs_read->pid);
    enviar_paquete(paquete_fin_io, socket_kernel, logger_io);

    free(a_escribir_en_memoria);
    free(fs_read->dir_fisica);
    free(fs_read->nombre_archivo);
    free(fs_read);
    return "";
}

void *io_fs_write(void *args)
{
    t_ejecutar_io_fs_write *fs_write = (t_ejecutar_io_fs_write *)args;
    log_info(logger_io, "Operación: \"PID: %d - Operacion: IO_FS_WRITE\"", fs_write->pid);
    log_info(logger_io, "Tamaño32: %d", fs_write->tamanio);
    log_info(logger_obligatorio_io, "PID: <%d> - Operacion: <IO_FS_WRITE>", fs_write->pid);

    // Acceder a los datos en memoria y traerlos al FS para copiarlos de acuerdo a la posicion_archivo (posicion inicial) y el tamanio

    log_info(logger_obligatorio_io, "PID: <%d> - Escribir Archivo: <%s> - Tamaño a Escribir: <%d> - Puntero Archivo: <%d>", fs_write->pid, fs_write->nombre_archivo, fs_write->tamanio, fs_write->posicion_archivo);
    t_paquete *paquete_fs = serializar_ejecutar_io_fs_write(fs_write->nombre_archivo, fs_write->tamanio, fs_write->posicion_archivo, fs_write->dir_fisica, fs_write->pid);
    enviar_paquete(paquete_fs, socket_memoria, logger_io);

    // Demora en realizar la operacion
    usarTiempoUnidadesTrabajo();

    log_info(logger_io, "Esperando respuesta de memoria");
    sem_wait(sem_esperar_respuesta);
    log_info(logger_io, "Finalizo io_fs_write");

    // Avisar a kernel que termino la operacion
    t_paquete *paquete_fin_io = serializar_fin_io(nombre_interfaz, &fs_write->pid);
    enviar_paquete(paquete_fin_io, socket_kernel, logger_io);

    pthread_mutex_lock(&m_pid_usando);
    pid_usando_interfaz = -1;
    pthread_mutex_unlock(&m_pid_usando);
    free(fs_write->dir_fisica);
    free(fs_write->nombre_archivo);
    free(fs_write);
    return "";
}

void atender_memoria()
{
    while (1)
    {
        int size;
        void *buffer;
        int *tam_recibido = malloc(sizeof(int));
        int cod_op = recibir_operacion(socket_memoria);
        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(socket_memoria, logger_io);
            free(tam_recibido);
            break;
        case RET_IO_STDOUT_WRITE:
            buffer = recibir_buffer(&size, socket_memoria);
            t_ret_io_stdout_write *ret = deserializar_ret_io_stdout_write(buffer, tam_recibido);
            log_info(logger_io, "recibi el pid: %d", ret->pid);
            log_info(logger_io, "Mostrando en pantalla: %s", ret->leido);
            log_info(logger_obligatorio_io, "Mostrando en pantalla: %s", ret->leido);
            t_paquete *paquete = serializar_fin_io(nombre_interfaz, &ret->pid);
            enviar_paquete(paquete, socket_kernel, logger_io);

            free(ret->leido);
            free(ret);
            free(tam_recibido);
            free(buffer);
            break;
        case RET_IO_STDIN_READ:
            buffer = recibir_buffer(&size, socket_memoria);
            uint32_t *pid = deserializar_ret_io_stdin_read(buffer, tam_recibido);
            log_info(logger_io, "recibi el pid: %d", *pid);
            t_paquete *paquete2 = serializar_fin_io(nombre_interfaz, pid);
            enviar_paquete(paquete2, socket_kernel, logger_io);
            free(tam_recibido);
            free(pid);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_READ:
            buffer = recibir_buffer(&size, socket_memoria);
            uint32_t *pid_fs_read = deserializar_ret_io_stdin_read(buffer, tam_recibido);
            log_info(logger_io, "EJECUTAR_IO_FS_READ recibi el pid: %d", *pid_fs_read);
            sem_post(sem_esperar_respuesta);
            free(tam_recibido);
            free(pid_fs_read);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_WRITE:
            buffer = recibir_buffer(&size, socket_memoria);
            t_ret_io_fs_write *fs_write = deserializar_ret_io_fs_write(buffer, tam_recibido);
            log_info(logger_io, "EJECUTAR_IO_FS_WRITE recibi el pid: %d", fs_write->pid);

            t_archivo *archivo = dictionary_get(archivos, fs_write->nombre_archivo);
            void *comienzo_archivo = bloques + archivo->bloque_inicial * config_io->block_size;
            memcpy(comienzo_archivo + fs_write->posicion_archivo, fs_write->leido, fs_write->tamanio);

            actualizar_bloques();
            sem_post(sem_esperar_respuesta);
            free(fs_write->leido);
            free(fs_write->dir_fisica);
            free(fs_write->nombre_archivo);
            free(fs_write);
            free(tam_recibido);
            free(buffer);
            break;
        default:
            free(tam_recibido);
            return;
        }
    }
}

void atender_kernel()
{
    while (1)
    {
        int size;
        void *buffer;
        int *tam_recibido = malloc(sizeof(int));

        int cod_op = recibir_operacion(socket_kernel);
        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(socket_kernel, logger_io);
            free(tam_recibido);
            break;
        case EJECUTAR_IO_GEN_SLEEP:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_gen_sleep *ejecutar_io_gen_sleep = deserializar_ejecutar_io_gen_sleep(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d y las udt: %d", ejecutar_io_gen_sleep->pid, ejecutar_io_gen_sleep->unidades_de_trabajo);
            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 1)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    uint32_t *pid = malloc(sizeof(uint32_t));
                    *pid = ejecutar_io_gen_sleep->pid;
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_sleep(nombre_interfaz, pid, &ejecutar_io_gen_sleep->unidades_de_trabajo);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(ejecutar_io_gen_sleep);
                    free(pid);
                    free(tam_recibido);
                    free(buffer);
                    break;
                }
                else
                {
                    pid_usando_interfaz = ejecutar_io_gen_sleep->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_gen_sleep, (void *)ejecutar_io_gen_sleep);
                    pthread_detach(ejecutar_instruccion);
                    log_info(logger_io, "Recibi %d unidades de trabajo del PID: %d", ejecutar_io_gen_sleep->unidades_de_trabajo, ejecutar_io_gen_sleep->pid);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_GEN_SLEEP");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &ejecutar_io_gen_sleep->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
                free(ejecutar_io_gen_sleep);
            }
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_IO_STDIN_READ:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_stdin_read *ejecutar_io_stdin_read = deserializar_ejecutar_stdin_read(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d", ejecutar_io_stdin_read->pid);
            t_direccion_fisica *dir = ejecutar_io_stdin_read->dir_fisica;
            log_info(logger_io, "Me llego la dir_fis desplazamiento: %d, pid: %d, marco: %d", dir->desplazamiento_fisico, dir->pid, dir->marco);

            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 2)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    uint32_t *pid = malloc(sizeof(uint32_t));
                    *pid = ejecutar_io_stdin_read->pid;
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_read(nombre_interfaz, pid, ejecutar_io_stdin_read->dir_fisica, ejecutar_io_stdin_read->tamanio);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(pid);
                    free(tam_recibido);
                    free(buffer);
                    free(ejecutar_io_stdin_read->dir_fisica);
                    free(ejecutar_io_stdin_read->tamanio);
                    free(ejecutar_io_stdin_read);
                    break;
                }
                else
                {
                    pid_usando_interfaz = ejecutar_io_stdin_read->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_stdin_read, (void *)ejecutar_io_stdin_read);
                    pthread_detach(ejecutar_instruccion);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_STDIN_READ");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &ejecutar_io_stdin_read->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
                free(ejecutar_io_stdin_read->dir_fisica);
                free(ejecutar_io_stdin_read->tamanio);
                free(ejecutar_io_stdin_read);
            }
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_IO_STDOUT_WRITE:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_stdout_write *ejecutar_io_stdout_write = deserializar_ejecutar_stdout_write(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d", ejecutar_io_stdout_write->pid);
            t_direccion_fisica *dir2 = ejecutar_io_stdout_write->dir_fisica;
            log_info(logger_io, "Me llego la dir_fis desplazamiento: %d, pid: %d, marco: %d", dir2->desplazamiento_fisico, dir2->pid, dir2->marco);
            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 3)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    uint32_t *pid = malloc(sizeof(uint32_t));
                    *pid = ejecutar_io_stdout_write->pid;
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_write(nombre_interfaz, pid, ejecutar_io_stdout_write->dir_fisica, ejecutar_io_stdout_write->tamanio);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(pid);
                    free(tam_recibido);
                    free(buffer);
                    free(ejecutar_io_stdout_write->dir_fisica);
                    free(ejecutar_io_stdout_write);
                    break;
                }
                else
                {
                    pid_usando_interfaz = ejecutar_io_stdout_write->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_stdout_write, (void *)ejecutar_io_stdout_write);
                    pthread_detach(ejecutar_instruccion);
                    log_info(logger_io, "Recibi PID: %d", ejecutar_io_stdout_write->pid);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_STDOUT_WRITE");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &ejecutar_io_stdout_write->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
            }
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_CREATE:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_fs_create *fs_create = deserializar_ejecutar_io_fs_create(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d, nombre_archivo: %s", fs_create->pid, fs_create->nombre_archivo);
            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 4)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_fs_create(nombre_interfaz, fs_create->pid, fs_create->nombre_archivo);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(tam_recibido);
                    free(buffer);
                    free(fs_create->nombre_archivo);
                    free(fs_create);
                    break;
                }
                else
                {
                    pid_usando_interfaz = fs_create->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_fs_create, (void *)fs_create);
                    pthread_detach(ejecutar_instruccion);
                    log_info(logger_io, "Recibi PID: %d", fs_create->pid);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_FS_CREATE");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &fs_create->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
                free(fs_create->nombre_archivo);
                free(fs_create);
            }
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_DELETE:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_fs_delete *fs_delete = deserializar_ejecutar_io_fs_delete(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d, nombre_archivo: %s", fs_delete->pid, fs_delete->nombre_archivo);
            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 4)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_fs_delete(nombre_interfaz, fs_delete->pid, fs_delete->nombre_archivo);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(tam_recibido);
                    free(buffer);
                    free(fs_delete->nombre_archivo);
                    free(fs_delete);
                    break;
                }
                else
                {
                    pid_usando_interfaz = fs_delete->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_fs_delete, (void *)fs_delete);
                    pthread_detach(ejecutar_instruccion);
                    log_info(logger_io, "Recibi PID: %d", fs_delete->pid);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_FS_DELETE");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &fs_delete->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
                free(fs_delete->nombre_archivo);
                free(fs_delete);
            }
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_TRUNCATE:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_fs_truncate *fs_truncate = deserializar_ejecutar_io_fs_truncate(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d, nombre_archivo: %s, tamanio_a_truncar: %d", fs_truncate->pid, fs_truncate->nombre_archivo, fs_truncate->tamanio_a_truncar);
            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 4)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_fs_truncate(nombre_interfaz, fs_truncate->pid, fs_truncate->nombre_archivo, fs_truncate->tamanio_a_truncar);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(tam_recibido);
                    free(buffer);
                    free(fs_truncate->nombre_archivo);
                    free(fs_truncate);
                    break;
                }
                else
                {
                    pid_usando_interfaz = fs_truncate->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_fs_truncate, (void *)fs_truncate);
                    pthread_detach(ejecutar_instruccion);
                    log_info(logger_io, "Recibi PID: %d", fs_truncate->pid);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_FS_TRUNCATE");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &fs_truncate->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
                free(fs_truncate->nombre_archivo);
                free(fs_truncate);
            }
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_WRITE:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_fs_write *fs_write = deserializar_ejecutar_io_fs_write(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d, nombre_archivo: %s, tamanio: %d, posicion: %d", fs_write->pid, fs_write->nombre_archivo, fs_write->tamanio, fs_write->posicion_archivo);
            t_direccion_fisica *dir3 = fs_write->dir_fisica;
            log_info(logger_io, "Me llego la dir_fis desplazamiento: %d, pid: %d, marco: %d", dir3->desplazamiento_fisico, dir3->pid, dir3->marco);
            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 4)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_fs_write(nombre_interfaz, fs_write->pid, fs_write->nombre_archivo, fs_write->dir_fisica, fs_write->tamanio, fs_write->posicion_archivo);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(tam_recibido);
                    free(buffer);
                    free(fs_write->nombre_archivo);
                    free(fs_write->dir_fisica);
                    free(fs_write);
                    break;
                }
                else
                {
                    pid_usando_interfaz = fs_write->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_fs_write, (void *)fs_write);
                    pthread_detach(ejecutar_instruccion);
                    log_info(logger_io, "Recibi PID: %d", fs_write->pid);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_FS_WRITE");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &fs_write->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
                free(fs_write->nombre_archivo);
                free(fs_write->dir_fisica);
                free(fs_write);
            }
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_READ:
            buffer = recibir_buffer(&size, socket_kernel);
            t_ejecutar_io_fs_read *fs_read = deserializar_ejecutar_io_fs_read(buffer, tam_recibido);
            log_info(logger_io, "Me llego el pid: %d, nombre_archivo: %s, tamanio: %d, posicion: %d", fs_read->pid, fs_read->nombre_archivo, fs_read->tamanio, fs_read->posicion_archivo);
            t_direccion_fisica *dir4 = fs_read->dir_fisica;
            log_info(logger_io, "Me llego la dir_fis desplazamiento: %d, pid: %d, marco: %d", dir4->desplazamiento_fisico, dir4->pid, dir4->marco);
            // si no es esta interfaz tengo que terminar el proceso que pidio esto.
            if (tipo_interfaz == 4)
            {
                pthread_mutex_lock(&m_pid_usando);

                if (pid_usando_interfaz != -1)
                {
                    pthread_mutex_unlock(&m_pid_usando);
                    log_info(logger_io, "La interfaz esta ocupada");
                    t_paquete *interfaz_ocupada = serializar_interfaz_ocupada_fs_read(nombre_interfaz, fs_read->pid, fs_read->nombre_archivo, fs_read->dir_fisica, fs_read->tamanio, fs_read->posicion_archivo);
                    enviar_paquete(interfaz_ocupada, socket_kernel, logger_io);
                    free(tam_recibido);
                    free(buffer);
                    free(fs_read->nombre_archivo);
                    free(fs_read->dir_fisica);
                    free(fs_read);
                    break;
                }
                else
                {
                    pid_usando_interfaz = fs_read->pid;
                    pthread_mutex_unlock(&m_pid_usando);
                    pthread_create(&ejecutar_instruccion, NULL, io_fs_read, (void *)fs_read);
                    pthread_detach(ejecutar_instruccion);
                    log_info(logger_io, "Recibi PID: %d", fs_read->pid);
                }
            }
            else
            {
                log_info(logger_io, "La interfaz seleccionada no soporta la operacion IO_FS_READ");
                t_paquete *peticion_incorrecta = serializar_error_io(nombre_interfaz, &fs_read->pid);
                enviar_paquete(peticion_incorrecta, socket_kernel, logger_io);
                free(fs_read->nombre_archivo);
                free(fs_read->dir_fisica);
                free(fs_read);
            }
            free(tam_recibido);
            free(buffer);
            break;

        default:
            free(tam_recibido);
            return;
        }
    }
}

void *conectarKernel(void *arg)
{
    t_args_conectar_kernel *args = (t_args_conectar_kernel *)arg;
    char *ip_kernel = args->config->ip_kernel;
    char *puerto_kernel = string_itoa(args->config->puerto_kernel);
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    free(puerto_kernel);
    handshake(socket_kernel, 1, logger_io, "kernel");
    enviar_mensaje("Conectado a Kernel desde IO", socket_kernel);
    uint32_t *tipo = malloc(sizeof(uint32_t));
    *tipo = tipo_interfaz;
    t_paquete *paquete = serializar_handshake_io(args->nombre_interfaz, tipo);
    enviar_paquete(paquete, socket_kernel, logger_io);
    free(tipo);
    free(args);
    atender_kernel();
    return "";
}

void *conectarMemoria(void *arg)
{
    t_args_conectar_memoria *args = (t_args_conectar_memoria *)arg;
    t_config_io *conf = args->config;
    char *ip_memoria = conf->ip_memoria;
    char *puerto_memoria = string_itoa(conf->puerto_memoria);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    handshake(socket_memoria, 3, logger_io, "memoria");
    enviar_mensaje("Conectado a Memoria desde IO", socket_memoria);
    free(args);
    free(puerto_memoria);
    atender_memoria();
    return "";
}

uint32_t asignar_tipo_interfaz(char *tipo_interfaz)
{
    if (strcmp(tipo_interfaz, "GENERICA") == 0)
    {
        return 1;
    }
    else if (strcmp(tipo_interfaz, "STDIN") == 0)
    {
        return 2;
    }
    else if (strcmp(tipo_interfaz, "STDOUT") == 0)
    {
        return 3;
    }
    else if (strcmp(tipo_interfaz, "DIALFS") == 0)
    {
        return 4;
    }
    else
    {
        log_error(logger_io, "NO SE ENCONTRO EL TIPO DE INTERFAZ");
        return -1;
    }
}

void crear_interfaz(char *nombre, char *path)
{
    config_io = levantar_configs(path);
    loguear_configs(config_io);
    tipo_interfaz = asignar_tipo_interfaz(config_io->tipo_interfaz);
    nombre_interfaz = nombre;
    t_args_conectar_kernel *conectar_kernel = malloc(sizeof(t_args_conectar_kernel));
    conectar_kernel->config = config_io;
    conectar_kernel->nombre_interfaz = nombre;
    pid_usando_interfaz = -1;

    if (tipo_interfaz == 4)
    {
        inicializar_bitmap();
        inicializar_bloques();
        inicializar_archivos();
    }

    if (tipo_interfaz == 2 || tipo_interfaz == 3 || tipo_interfaz == 4)
    {
        t_args_conectar_memoria *conectar_memoria = malloc(sizeof(t_args_conectar_memoria));
        conectar_memoria->config = config_io;
        conectar_memoria->nombre_interfaz = nombre;
        pthread_create(&conexion_memoria, NULL, conectarMemoria, (void *)conectar_memoria);
        pthread_detach(conexion_memoria);
    }

    // -----------------------------------------------------------------------------------------
    // -- pruebas DIALFS --
    // -----------------------------------------------------------------------------------------

    // // create
    // prueba_io_fs_create("archivo1");
    // prueba_io_fs_create("archivo2");
    // prueba_io_fs_create("archivo3");
    // prueba_io_fs_create("archivo4");

    // // delete
    // prueba_io_fs_create("archivo5");
    // readline("\nPresione Enter para continuar\n");
    // prueba_io_fs_delete("archivo5");

    // // truncate
    // prueba_io_fs_truncate("archivo1", 500);

    // // read y write
    // prueba_memoria_io_fs_read("archivo1");
    // prueba_memoria_io_fs_write("archivo1");

    // readline("\n------------------------------------------------------\nREADLINE PARA QUE SE DETENGA EL PROGRAMA Y PROBAR DIALFS\n------------------------------------------------------\n");

    pthread_create(&conexion_kernel, NULL, conectarKernel, (void *)conectar_kernel);
    pthread_join(conexion_kernel, NULL);
}

int main(int argc, char *argv[])
{
    logger_io = log_create("io.log", "io", 0, LOG_LEVEL_INFO);
    logger_obligatorio_io = log_create("ioObligatorio.log", "io", 1, LOG_LEVEL_INFO);
    sem_esperar_respuesta = malloc(sizeof(sem_t));
    sem_init(sem_esperar_respuesta, 0, 0);

    crear_interfaz(argv[1], argv[2]);
    // para levantar IO por consola hay que usar configuraciones en el launch.json
    // (desde la pestaña para ejecutar seleccionar io, y clickear sobre el engranaje, asi abrimos el launch.json)
    // pastear:
    // Con esto permitimos iniciar una interfaz dinamicamente segun los parametros de ejecucion.
    /*
        "args" : [
            "Int1",
            "entradasalida.config"
        ],
    */
    if (config_io != NULL)
    {
        destruir_config(config_io);
    }
    if (tipo_interfaz == 4)
    {
        dictionary_destroy_and_destroy_elements(archivos, free);
    }
    log_destroy(logger_io);
    free(sem_esperar_respuesta);
    log_destroy(logger_obligatorio_io);
    return 0;
}