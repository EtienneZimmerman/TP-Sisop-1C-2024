#include "cpu.h"

int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_kernel_interrupt;
int socket_kernel_dispatch;
int socket_memoria;

t_log *logger_cpu;
t_log *logger_obligatorio_cpu;
t_pcb *proceso_ejecutando;
sem_t *sem_esperar_respuesta;
uint32_t *tam_pagina;
t_config_cpu *config_cpu;
t_list *tlb;
t_temporal *clock_;
void *rta_memoria;

sem_t *mutex_proceso_ejecutando;
sem_t *mutex_registros;
t_registros *registros;
t_list *interrupciones;
pthread_mutex_t m_interrupciones;
pthread_mutex_t m_tiempo_inicial;

t_temporal *clockcpu;
int64_t tiempo_inicial;

void handshake_memoria()
{
    t_paquete *paquete = serializar_handshake_memoria();
    enviar_paquete(paquete, socket_memoria, logger_cpu);
}

void incrementar_pc(t_registros *registros)
{
    sem_wait(mutex_registros);
    registros->pc += 1;
    sem_post(mutex_registros);
}

void escribir_pcb(t_pcb *pcb, t_registros *registros)
{
    sem_wait(mutex_proceso_ejecutando);
    pcb->registros = *registros;
    pcb->pc = registros->pc;
    sem_post(mutex_proceso_ejecutando);
}

void recibir_pcb_nuevo()
{
    int cod_op = recibir_operacion(socket_kernel_dispatch);
    log_info(logger_cpu, "esperando cod op en recibir_pcb codop: %s", cod_op_to_string(cod_op));
    switch (cod_op)
    {
    case ENVIAR_PCB: // pcb proveniente del kernel.
        int size;
        void *buffer;
        int tam_recibido = 0;

        buffer = recibir_buffer(&size, socket_kernel_dispatch);
        sem_wait(mutex_proceso_ejecutando);
        t_pcb *proceso_recibido = deserializar_paquete_pcb(buffer, &tam_recibido);
        if (proceso_ejecutando->recurso_esperando != NULL)
            free(proceso_ejecutando->recurso_esperando);
        if (proceso_ejecutando->recursos_tomados != NULL && list_size(proceso_ejecutando->recursos_tomados) > 0)
            list_destroy_and_destroy_elements(proceso_ejecutando->recursos_tomados, element_destroyer);
        else if (proceso_ejecutando->recursos_tomados != NULL && list_size(proceso_ejecutando->recursos_tomados) == 0)
            list_destroy(proceso_ejecutando->recursos_tomados);
        free(proceso_ejecutando);
        proceso_ejecutando = proceso_recibido;
        pthread_mutex_lock(&m_tiempo_inicial);
        tiempo_inicial = temporal_gettime(clockcpu);
        pthread_mutex_unlock(&m_tiempo_inicial);
        sem_wait(mutex_registros);
        asignar_registros(registros, &proceso_ejecutando->registros);
        sem_post(mutex_registros);
        sem_post(mutex_proceso_ejecutando);
        log_info(logger_cpu, "Recibi el PCB, pid: %d, pc: %d, quantum: %ld", proceso_ejecutando->pid, proceso_ejecutando->pc, proceso_ejecutando->quantum);
        mostrar_registros(&proceso_ejecutando->registros);
        log_info(logger_cpu, "La lista de recursos_tomados del proceso %d tiene %d elementos", proceso_ejecutando->pid, list_size(proceso_ejecutando->recursos_tomados));
        log_info(logger_cpu, "El recurso_esperando del proceso %d es %s elementos", proceso_ejecutando->pid, proceso_ejecutando->recurso_esperando);
        free(buffer);
        break;
    case -1:
        return;
    default:
        return;
    }
}

void *abrirSocketKernelDispatch()
{

    socket_kernel_dispatch = esperar_cliente(socket_cpu_dispatch);

    uint32_t resultOk = 0;
    uint32_t resultError = -1;
    uint32_t respuesta;
    recv(socket_kernel_dispatch, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    if (respuesta == 1)
        send(socket_kernel_dispatch, &resultOk, sizeof(uint32_t), 0);
    else
    {
        send(socket_kernel_dispatch, &resultError, sizeof(uint32_t), 0);
        log_error(logger_cpu, "OCURRIO UN ERROR CONECTANDO A DISPATCH");
    }

    int cod_op = recibir_operacion(socket_kernel_dispatch);

    switch (cod_op)
    {
    case MENSAJE:
        recibir_mensaje(socket_kernel_dispatch, logger_cpu);
        break;
    }

    recibir_pcb_nuevo();
    pthread_t fetch_thread;
    pthread_create(&fetch_thread, NULL, fetch_en_new_thread, NULL);
    pthread_detach(fetch_thread);
    return "";
}

void *abrirSocketKernelInterrupt()
{
    socket_kernel_interrupt = esperar_cliente(socket_cpu_interrupt);

    uint32_t resultOk = 0;
    uint32_t resultError = -1;
    uint32_t respuesta;
    recv(socket_kernel_interrupt, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    if (respuesta == 1)
        send(socket_kernel_interrupt, &resultOk, sizeof(uint32_t), 0);
    else
        send(socket_kernel_interrupt, &resultError, sizeof(uint32_t), 0);

    while (1)
    {
        int cod_op = recibir_operacion(socket_kernel_interrupt);
        void *buffer;
        int size;
        int *tam_recibido = malloc(sizeof(int));
        uint32_t *pid;

        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(socket_kernel_interrupt, logger_cpu);
            free(tam_recibido);
            break;
        case INT_PROCESO:
            buffer = recibir_buffer(&size, socket_kernel_interrupt);
            pid = deserealizar_recibir_int_proceso(buffer, tam_recibido);
            log_info(logger_cpu, "Recibi el pid: %d para interrumpir", *pid);
            t_interrupcion *inter = malloc(sizeof(t_interrupcion));
            inter->pid = malloc(sizeof(uint32_t));
            *inter->pid = *pid;
            inter->motivo = INTERRUPCION;
            pthread_mutex_lock(&m_interrupciones);
            list_add(interrupciones, inter);
            pthread_mutex_unlock(&m_interrupciones);
            free(buffer);
            free(tam_recibido);
            free(pid);
            break;
        case FINALIZAR_PROCESO_CPU:
            buffer = recibir_buffer(&size, socket_kernel_interrupt);
            pid = deserealizar_recibir_int_proceso(buffer, tam_recibido);
            log_info(logger_cpu, "Recibi el pid: %d para finalizar", *pid);
            t_interrupcion *interr = malloc(sizeof(t_interrupcion));
            interr->pid = malloc(sizeof(uint32_t));
            *interr->pid = *pid;
            interr->motivo = TERMINAR_PROCESO;
            pthread_mutex_lock(&m_interrupciones);
            list_add(interrupciones, interr);
            pthread_mutex_unlock(&m_interrupciones);
            free(buffer);
            free(tam_recibido);
            free(pid);
            break;
        case -1:
            free(tam_recibido);
            return "";
            break;
        default:
            free(tam_recibido);
            break;
        }
    }

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
        int dspl = 0;
        uint32_t pid;
        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(socket_memoria, logger_cpu);
            log_info(logger_cpu, "Recibi un mensaje de memoria");
            free(tam_recibido);
            break;
        case HANDSHAKE_MEMORIA:
            buffer = recibir_buffer(&size, socket_memoria);
            tam_pagina = deserializar_rta_handhsake_memoria(buffer, tam_recibido);
            log_info(logger_cpu, "Recibi el tamaño de pagina: %d", *tam_pagina);
            free(tam_recibido);
            free(buffer);
            break;
        case RET_INSTRUCCION:
            buffer = recibir_buffer(&size, socket_memoria);
            t_obtener_ret_instruccion *obt_ret_instruc = deserializar_obtener_ret_instruccion(buffer, tam_recibido);
            log_info(logger_cpu, "Recibi el id_instr: %d y la instruccion: %s", obt_ret_instruc->id_inst, obt_ret_instruc->instruccion);
            pthread_t decode_thread;
            pthread_create(&decode_thread, NULL, decode, (void *)obt_ret_instruc);
            pthread_detach(decode_thread);
            free(buffer);
            free(tam_recibido);
            break;
        case PEDIR_MARCO:
            buffer = recibir_buffer(&size, socket_memoria);
            t_registro_tlb *registro_tlb = deserializar_responder_marco(buffer, tam_recibido);
            rta_memoria = registro_tlb;
            sem_post(sem_esperar_respuesta);
            free(buffer);
            free(tam_recibido);
            break;
        case RTA_MOVIN:
            buffer = recibir_buffer(&size, socket_memoria);
            void *reg = deserializar_rta_movin(buffer, tam_recibido);
            t_registro_abstracto *reg_abs = (t_registro_abstracto *)reg;
            if (reg_abs->tipo == 1)
            {
                rta_memoria = malloc(sizeof(uint8_t));
                memcpy(rta_memoria, &((t_registro_8 *)reg)->data, sizeof(uint8_t));
            }
            else
            {
                rta_memoria = malloc(sizeof(uint32_t));
                t_registro_32 *reg32 = (t_registro_32 *)reg;
                memcpy(rta_memoria, &reg32->data, sizeof(uint32_t));
            }
            free(reg);
            sem_post(sem_esperar_respuesta);
            free(buffer);
            free(tam_recibido);
            break;
        case EJECUTAR_MOVOUT:
            buffer = recibir_buffer(&size, socket_memoria);
            pid = deserializar_uint32(buffer, &dspl);
            log_info(logger_cpu, "Se realizo un MOVOUT exitoso del pid: %d", pid);
            free(tam_recibido);
            free(buffer);
            sem_post(sem_esperar_respuesta);
            break;
        case EJECUTAR_COPYSTRING:
            buffer = recibir_buffer(&size, socket_memoria);
            pid = deserializar_uint32(buffer, &dspl);
            log_info(logger_cpu, "Se realizo un COPY_STRING exitoso del pid: %d", pid);
            free(buffer);
            free(tam_recibido);
            sem_post(sem_esperar_respuesta);
            break;
        case EJECUTAR_RESIZE:
            buffer = recibir_buffer(&size, socket_memoria);
            pid = deserializar_uint32(buffer, &dspl);
            log_info(logger_cpu, "Se realizo un resize exitoso del pid: %d", pid);
            int *resultado_satisfactorio = malloc(sizeof(int));
            *resultado_satisfactorio = 0;
            rta_memoria = resultado_satisfactorio;
            sem_post(sem_esperar_respuesta);
            free(tam_recibido);
            free(buffer);
            break;
        case EJECUTAR_RESIZE_OUT_OF_MEMORY_ERROR:
            log_error(logger_cpu, "OUT_OF_MEMORY ERROR: No hay suficientes marcos libres para el proceso %d", proceso_ejecutando->pid);
            buffer = recibir_buffer(&size, socket_memoria);
            // enviar mensaje al kernel para que finalice el proceso.
            uint32_t *motivo = malloc(sizeof(uint32_t));
            *motivo = OUT_OF_MEMORY_ERROR;
            int *resultado_out_of_memory = malloc(sizeof(int));
            *resultado_out_of_memory = 1;
            rta_memoria = resultado_out_of_memory;
            sem_wait(mutex_proceso_ejecutando);
            t_paquete *paquete_contexto = serializar_contexto_ejecucion(proceso_ejecutando, *motivo, NULL);
            enviar_paquete(paquete_contexto, socket_kernel_dispatch, logger_cpu);
            sem_post(mutex_proceso_ejecutando);
            sem_post(sem_esperar_respuesta);
            free(tam_recibido);
            free(motivo);
            free(buffer);
            break;
        case -1:
            free(tam_recibido);
            return;
            break;
        default:
            free(tam_recibido);
            break;
        }
    }
}

void *conectarMemoria(void *arg)
{
    t_config_cpu *config_cpu = (t_config_cpu *)arg;
    char *ip_memoria = config_cpu->ip_memoria;
    char *puerto_memoria = string_itoa(config_cpu->puerto_memoria);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    free(puerto_memoria);
    handshake(socket_memoria, 2, logger_cpu, "memoria");
    handshake_memoria();
    log_info(logger_cpu, "envie peticion de tam_pagina");
    atender_memoria();
    return "";
}

int finalizar_proceso(uint32_t *pid)
{
    sem_wait(mutex_proceso_ejecutando);
    if (proceso_ejecutando->pid == *pid)
    {
        uint32_t *motivo = malloc(sizeof(uint32_t));
        *motivo = TERMINAR_PROCESO;
        t_paquete *paquete_contexto = serializar_contexto_ejecucion(proceso_ejecutando, *motivo, NULL);
        enviar_paquete(paquete_contexto, socket_kernel_dispatch, logger_cpu);
        sem_post(mutex_proceso_ejecutando);
        free(motivo);
        return 1;
    }
    else
    {
        sem_post(mutex_proceso_ejecutando);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        iniciar_config("cpu.config");
    }
    else
    {
        iniciar_config(argv[1]);
    }
    logger_cpu = log_create("cpu.log", "cpu", 0, LOG_LEVEL_INFO);
    logger_obligatorio_cpu = log_create("cpuObligatorio.log", "cpu", 1, LOG_LEVEL_INFO);
    registros = malloc(sizeof(t_registros));
    iniciar_registros();
    config_cpu = leer_configs();
    proceso_ejecutando = (t_pcb *)malloc(sizeof(t_pcb));
    proceso_ejecutando->recursos_tomados = NULL;
    proceso_ejecutando->recurso_esperando = NULL;
    proceso_ejecutando->pid = 0;
    interrupciones = list_create();
    log_info(logger_cpu, "SE SETEA  proceso_ejecutando->pid = %d", proceso_ejecutando->pid);
    t_config_cpu *config_cpu = leer_configs();
    char *puerto_dispatch = string_itoa(config_cpu->puerto_escucha_dispatch);
    socket_cpu_dispatch = iniciar_servidor(puerto_dispatch, NULL, logger_cpu);
    free(puerto_dispatch);
    log_info(logger_cpu, "Servidor cpu dispatch levantado");
    char *puerto_interrupt = string_itoa(config_cpu->puerto_escucha_interrupt);
    socket_cpu_interrupt = iniciar_servidor(puerto_interrupt, NULL, logger_cpu);
    free(puerto_interrupt);
    log_info(logger_cpu, "Servidor cpu interrupt levantado");
    pthread_t atender_kernel_dispatch;
    pthread_t atender_kernel_interrupt;
    pthread_t conexion_memoria;
    sem_esperar_respuesta = malloc(sizeof(sem_t));
    mutex_proceso_ejecutando = malloc(sizeof(sem_t));
    mutex_registros = malloc(sizeof(sem_t));
    clockcpu = temporal_create();
    // inicio el semaforo en 0 por si llega una interrupcion que espere a la fase de interrupcion.
    // una vez en la fase de interrupcion se hace un sem_post para escuchar si llego algo. Cuando finaliza la recepcion termina
    // haciendo un post al semaforo
    sem_init(sem_esperar_respuesta, 0, 0);
    sem_init(mutex_proceso_ejecutando, 0, 1);
    sem_init(mutex_registros, 0, 1);

    pthread_create(&conexion_memoria, NULL, conectarMemoria, (void *)config_cpu);
    sem_init(mutex_proceso_ejecutando, 0, 1);
    sem_init(mutex_registros, 0, 1);
    pthread_create(&atender_kernel_dispatch, NULL, abrirSocketKernelDispatch, NULL);
    pthread_create(&atender_kernel_interrupt, NULL, abrirSocketKernelInterrupt, NULL);
    iniciar_tlb();

    log_info(logger_cpu, "Esperando...");
    pthread_detach(conexion_memoria);
    pthread_detach(atender_kernel_dispatch);
    pthread_detach(atender_kernel_interrupt);

    readline("Bloqueo el main, enter para finalizar");

    cerrar_servidor(socket_cpu_dispatch);
    cerrar_servidor(socket_cpu_interrupt);
    destruir_config(config_cpu);
    if (list_size(interrupciones) > 0)
    {
        list_destroy_and_destroy_elements(interrupciones, interruption_destroyer);
    }
    else
    {
        list_destroy(interrupciones);
    }
    free(registros);
    if (proceso_ejecutando->recurso_esperando != NULL)
        free(proceso_ejecutando->recurso_esperando);
    if (proceso_ejecutando->recursos_tomados != NULL)
        list_destroy_and_destroy_elements(proceso_ejecutando->recursos_tomados, element_destroyer);
    free(proceso_ejecutando);
    log_destroy(logger_cpu);
    log_destroy(logger_obligatorio_cpu);
    sem_close(sem_esperar_respuesta);
    free(sem_esperar_respuesta);
    sem_close(mutex_proceso_ejecutando);
    free(mutex_proceso_ejecutando);
    sem_close(mutex_registros);
    free(mutex_registros);
    return 0;
}