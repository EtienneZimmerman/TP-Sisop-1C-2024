#include "main.h"

int socket_kernel;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
int socket_io_cliente;
t_config_kernel *config_kernel;
uint32_t pid_ejecutando;
t_list *lista_ready;
t_list *lista_running;
t_list *lista_blocked;
t_list *lista_new;
t_list *lista_exit;
t_log *logger_kernel;
t_log *logger_obligatorio_kernel;
t_pcb *pcb_a_ejecutar;
t_dictionary *dict_interfaces;
t_list *lista_auxiliar;

sem_t *sem_grado_multiprogamacion;
sem_t *sem_process_in_ready;
sem_t *sem_cpu_free_to_execute;
sem_t *sem_beggin_quantum;
sem_t *sem_interruption_quantum;
sem_t *sem_planificacion;
sem_t *sem_planificacion_largo;
sem_t *sem_proceso_se_creo_en_memoria;

pthread_mutex_t m_list_NEW;
pthread_mutex_t m_list_READY;
pthread_mutex_t m_list_RUNNING;
pthread_mutex_t m_list_BLOCKED;
pthread_mutex_t m_list_EXIT;
pthread_mutex_t m_socket_memoria;
pthread_mutex_t m_list_auxiliar;
pthread_mutex_t m_pid_ejecutando;
pthread_mutex_t m_config_kernel_multiprogramacion;

pthread_t planificador_corto_plazo;

t_list *estados_procesos;
t_list *m_estados_procesos;
t_temporal *clock_global;

t_config_kernel *levantar_configs(char *ruta_archivo)
{
    iniciar_config(ruta_archivo);
    t_config_kernel *config_kernel = leer_configs();
    return config_kernel;
}

void loguear_configs(t_config_kernel *config_kernel)
{

    log_info(logger_kernel, " Algoritmo planificacion: %s\n Grado multiprogramacion: %d\n Ip CPU: %s\n Puerto Dispatch: %d\n Puerto Interrupt: %d\n Puerto Escucha: %d\n Quantum: %d",
             config_kernel->algoritmo_planificacion, config_kernel->grado_multiprogramacion,
             config_kernel->ip_cpu, config_kernel->puerto_cpu_dispatch, config_kernel->puerto_cpu_interrupt,
             config_kernel->puerto_escucha, config_kernel->quantum);

    char **copy_recursos = config_kernel->recursos;
    char **copy_instancias_recursos = config_kernel->instancias_recursos;
    int i = 0;

    // esto deberia funcionar unicamente por que los recursos y las instancias de ellos se asumen de misma longitud

    for (; *copy_recursos != NULL; copy_recursos++)
    {
        log_info(logger_kernel, "Recursos_%d: %s Instancias_recursos_%d: %s", i, *copy_recursos, i, *copy_instancias_recursos);
        copy_instancias_recursos++;
        i++;
    }

    log_info(logger_kernel, "POR EJECUTAR RECURSOS INIT");
    recursos_init(config_kernel);
}

void loggear_registros(t_log *logger, t_registros *registros)
{
    log_info(logger, "AX: %d, BX: %d, CX: %d, DX: %d", registros->ax, registros->bx, registros->cx, registros->dx);
    log_info(logger, "EAX: %d, EBX: %d, ECX: %d, EDX: %d", registros->eax, registros->ebx, registros->ecx, registros->edx);
    log_info(logger, "SI: %d, DI: %d, PC: %d", registros->si, registros->di, registros->pc);
}

void loggear_pcb(t_log *logger, t_pcb *pcb)
{
    log_info(logger, "PID: %d, PC: %d, QUANTUM: %ld", pcb->pid, pcb->pc, pcb->quantum);
    loggear_registros(logger, &pcb->registros);
    log_info(logger, " ");
}

void listar_procesos_en_lista(t_list *list)
{
    for (int i = 0; i < list_size(list); i++)
    {
        loggear_pcb(logger_kernel, (t_pcb *)list_get(list, i));
        loggear_pcb(logger_obligatorio_kernel, (t_pcb *)list_get(list, i));
    }
}

void listar_proceso_exec()
{
    if (list_size(lista_running) > 0)
    {
        loggear_pcb(logger_kernel, list_get(lista_running, 0));
        loggear_pcb(logger_obligatorio_kernel, list_get(lista_running, 0));
    }
    else
    {
        log_info(logger_kernel, "No hay procesos en exec");
    }
}

void log_pcb(void *element)
{
    t_pcb *pcb = (t_pcb *)element;

    loggear_pcb(logger_kernel, pcb);
}

void enviar_io_gen_sleep_a_interfaz(char *nombre_interfaz, uint32_t *unidades_de_trabajo, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_GEN_SLEEP a la interfaz: %s", nombre_interfaz);
    t_interfaz *interfaz = dictionary_get(dict_interfaces, nombre_interfaz);
    t_paquete *paquete_io_gen_sleep = serializar_ejecutar_io_gen_sleep(unidades_de_trabajo, pid);
    pthread_mutex_lock(&interfaz->m_socket);
    enviar_paquete(paquete_io_gen_sleep, *(interfaz->socket_cliente_io), logger_kernel);
    pthread_mutex_unlock(&interfaz->m_socket);
}

void enviar_io_stdin_read_a_interfaz(t_direccion_fisica *dir_fis, char *interfaz, void *tamanio, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_STDIN_READ a la interfaz: %s", interfaz);
    t_interfaz *interfaz_t = dictionary_get(dict_interfaces, interfaz);
    if (interfaz_t == NULL)
    {
        log_error(logger_kernel, "LA INTERFAZ NO FUE ENCONTRADA");
        return;
    }
    t_io_stdin_read *std_read = malloc(sizeof(t_io_stdin_read));
    std_read->dir_fisica = dir_fis;
    std_read->interfaz = interfaz;
    std_read->tamanio = tamanio;
    t_paquete *paq = serializar_ejecutar_io_stdin_read(pid, std_read->dir_fisica, std_read->tamanio);
    pthread_mutex_lock(&interfaz_t->m_socket);
    enviar_paquete(paq, *interfaz_t->socket_cliente_io, logger_kernel);
    pthread_mutex_unlock(&interfaz_t->m_socket);
    free(std_read->dir_fisica);
    free(std_read->interfaz);
    free(std_read->tamanio);
    free(std_read);
}

void enviar_io_stdout_write_a_interfaz(t_direccion_fisica *dir_fis, char *interfaz, uint32_t *tamanio, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_STDOUT_WRITE a la interfaz: %s tamanio: %d", interfaz, *tamanio);
    t_interfaz *interfaz_t = dictionary_get(dict_interfaces, interfaz);
    if (interfaz_t == NULL)
    {
        log_error(logger_kernel, "LA INTERFAZ NO FUE ENCONTRADA");
        return;
    }
    t_io_stdout_write *std_write = malloc(sizeof(t_io_stdout_write));
    std_write->dir_fisica = dir_fis;
    std_write->interfaz = interfaz;
    std_write->tamanio = *tamanio;
    t_paquete *paq;
    paq = serializar_ejecutar_io_stdout_write(pid, std_write->dir_fisica, std_write->tamanio);
    pthread_mutex_lock(&interfaz_t->m_socket);
    enviar_paquete(paq, *interfaz_t->socket_cliente_io, logger_kernel);
    pthread_mutex_unlock(&interfaz_t->m_socket);
    free(std_write->dir_fisica);
    free(std_write->interfaz);
    free(std_write);
}

void enviar_io_fs_create_a_interfaz(char *interfaz, char *nombre_archivo, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_FS_CREATE a la interfaz: %s nombre_archivo: %s", interfaz, nombre_archivo);
    t_interfaz *interfaz_t = dictionary_get(dict_interfaces, interfaz);
    if (interfaz_t == NULL)
    {
        log_error(logger_kernel, "LA INTERFAZ NO FUE ENCONTRADA");
        return;
    }
    t_paquete *paq;
    paq = serializar_ejecutar_io_fs_create(*pid, nombre_archivo);
    pthread_mutex_lock(&interfaz_t->m_socket);
    enviar_paquete(paq, *interfaz_t->socket_cliente_io, logger_kernel);
    pthread_mutex_unlock(&interfaz_t->m_socket);
}

void enviar_io_fs_delete_a_interfaz(char *interfaz, char *nombre_archivo, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_FS_DELETE a la interfaz: %s nombre_archivo: %s", interfaz, nombre_archivo);
    t_interfaz *interfaz_t = dictionary_get(dict_interfaces, interfaz);
    if (interfaz_t == NULL)
    {
        log_error(logger_kernel, "LA INTERFAZ NO FUE ENCONTRADA");
        return;
    }
    t_paquete *paq;
    paq = serializar_ejecutar_io_fs_delete(*pid, nombre_archivo);
    pthread_mutex_lock(&interfaz_t->m_socket);
    enviar_paquete(paq, *interfaz_t->socket_cliente_io, logger_kernel);
    pthread_mutex_unlock(&interfaz_t->m_socket);
}

void enviar_io_fs_truncate_a_interfaz(char *interfaz, char *nombre_archivo, uint32_t tamanio, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_FS_TRUNCATE a la interfaz: %s nombre_archivo: %s, tamanio: %d", interfaz, nombre_archivo, tamanio);
    t_interfaz *interfaz_t = dictionary_get(dict_interfaces, interfaz);
    if (interfaz_t == NULL)
    {
        log_error(logger_kernel, "LA INTERFAZ NO FUE ENCONTRADA");
        return;
    }
    t_paquete *paq;
    paq = serializar_ejecutar_io_fs_truncate(*pid, nombre_archivo, tamanio);
    pthread_mutex_lock(&interfaz_t->m_socket);
    enviar_paquete(paq, *interfaz_t->socket_cliente_io, logger_kernel);
    pthread_mutex_unlock(&interfaz_t->m_socket);
}

void enviar_io_fs_read_a_interfaz(t_direccion_fisica *dir, char *interfaz, char *nombre_archivo, uint32_t tamanio, uint32_t posicion_archivo, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_FS_READ a la interfaz: %s nombre_archivo: %s, tamanio: %d", interfaz, nombre_archivo, tamanio);
    t_interfaz *interfaz_t = dictionary_get(dict_interfaces, interfaz);
    if (interfaz_t == NULL)
    {
        log_error(logger_kernel, "LA INTERFAZ NO FUE ENCONTRADA");
        return;
    }
    t_paquete *paq;
    paq = serializar_ejecutar_io_fs_read(nombre_archivo, tamanio, posicion_archivo, dir, *pid);
    pthread_mutex_lock(&interfaz_t->m_socket);
    enviar_paquete(paq, *interfaz_t->socket_cliente_io, logger_kernel);
    pthread_mutex_unlock(&interfaz_t->m_socket);
}

void enviar_io_fs_write_a_interfaz(t_direccion_fisica *dir, char *interfaz, char *nombre_archivo, uint32_t tamanio, uint32_t posicion_archivo, uint32_t *pid)
{
    log_info(logger_kernel, "por enviar peticion IO_FS_WRITE a la interfaz: %s nombre_archivo: %s, tamanio: %d", interfaz, nombre_archivo, tamanio);
    t_interfaz *interfaz_t = dictionary_get(dict_interfaces, interfaz);
    if (interfaz_t == NULL)
    {
        log_error(logger_kernel, "LA INTERFAZ NO FUE ENCONTRADA");
        return;
    }
    t_paquete *paq;
    paq = serializar_ejecutar_io_fs_write(nombre_archivo, tamanio, posicion_archivo, dir, *pid);
    pthread_mutex_lock(&interfaz_t->m_socket);
    enviar_paquete(paq, *interfaz_t->socket_cliente_io, logger_kernel);
    pthread_mutex_unlock(&interfaz_t->m_socket);
}

t_interfaz *iniciar_interfaz(char *nombre, int *socket, uint32_t *tipo)
{
    t_interfaz *inter = malloc(sizeof(t_interfaz));
    inter->nombre_interfaz = malloc(sizeof(char) * strlen(nombre) + 1);
    memcpy(inter->nombre_interfaz, nombre, strlen(nombre) + 1);
    inter->socket_cliente_io = socket;
    inter->lista_bloqueados = list_create();
    inter->tipo_interfaz = malloc(sizeof(uint32_t));
    pthread_mutex_init(&inter->m_socket, NULL);
    memcpy(inter->tipo_interfaz, tipo, sizeof(uint32_t));
    return inter;
}

int main(int argc, char *argv[])
{

    init_mutex();

    if (argv[1] == NULL)
    {
        config_kernel = levantar_configs("kernel.config");
    }
    else
    {
        config_kernel = levantar_configs(argv[1]);
    }

    init_semaphores(config_kernel);


    // TESTS LOGGERS: CUANDO PROBEMOS NOSOTROS LOGGER_KERNEL = 1 Y LOGGER_OBLIGATORIO_KERNEL = 0
    // TESTS LOGGERS: CUANDO PROBEMOS NOSOTROS LOGGER_KERNEL = 0 Y LOGGER_OBLIGATORIO_KERNEL = 1
    logger_kernel = log_create("kernel.log", "kernel", 0, LOG_LEVEL_INFO);
    logger_obligatorio_kernel = log_create("kernelObligatorio.log", "kernel", 1, LOG_LEVEL_INFO);

    loguear_configs(config_kernel);
    int *sock_copy = iniciar_server_kernel(config_kernel->puerto_escucha, logger_kernel);
    socket_kernel = *sock_copy;
    free(sock_copy);
    log_info(logger_kernel, "Servidor kernel levantado");
    dict_interfaces = dictionary_create();
    pthread_t conexion_cpu_dispatch;
    pthread_t conexion_cpu_interrupt;
    pthread_t conexion_memoria;
    pthread_t atender_io;

    pthread_create(&conexion_cpu_dispatch, NULL, conectarCPUDispatch, (void *)config_kernel);
    pthread_create(&conexion_cpu_interrupt, NULL, conectarCPUInterrupt, (void *)config_kernel);
    pthread_create(&conexion_memoria, NULL, conectarMemoria, (void *)config_kernel);
    pthread_create(&atender_io, NULL, abrirSocketIO, NULL);

    pthread_detach(conexion_memoria);
    pthread_detach(conexion_cpu_dispatch);
    pthread_detach(conexion_cpu_interrupt);
    pthread_detach(atender_io);

    init_estructuras_planificacion();

    init_clock_global();

    init_planificadores(config_kernel);

    pthread_t hilo_mover_a_ready;
    pthread_create(&hilo_mover_a_ready, NULL, mover_a_ready, NULL);
    pthread_detach(hilo_mover_a_ready); 

    crear_hilo_consola();

    readline("Enter para finalizar");

    destruir_server_kernel(&socket_kernel);
    destroy_semaphores(config_kernel);
    destruir_config(config_kernel);
    dictionary_clean_and_destroy_elements(dict_interfaces, interface_destroyer);
    dictionary_destroy(dict_interfaces);
    destruir_listas_planificacion();
    destruir_listas_estados_procesos();
    destruir_clock_global();
    log_destroy(logger_kernel);
    log_destroy(logger_obligatorio_kernel);
    return 0;
}