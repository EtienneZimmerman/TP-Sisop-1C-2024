#include "main.h"
void contador_quantum_RR(void *args)
{
    // RR
    t_config_kernel *cfg_kernel = (t_config_kernel *)args;
    t_temporal *clock;
    int64_t tiempo_transcurrido;
    int64_t tiempo_inicial;
    while (1)
    {
        sem_wait(sem_beggin_quantum);
        clock = temporal_create();
        tiempo_inicial = temporal_gettime(clock);
        float time_to_sleep = cfg_kernel->quantum * 1000.0;
        uint32_t pid_inicial = pid_ejecutando;
        while (1)
        {
            tiempo_transcurrido = temporal_gettime(clock);
            if (tiempo_transcurrido - tiempo_inicial >= cfg_kernel->quantum)
            {
                if (pid_inicial == pid_ejecutando)
                {
                    interrumpir_proceso(pid_ejecutando);
                }
                temporal_destroy(clock);
                break;
            }
            else
            {
                usleep(time_to_sleep / 3.0);
            }
        }
    }
}

estado_code obtener_estado_proceso(int pid)
{
    pthread_mutex_t *mutex = list_get(m_estados_procesos, pid - 1);
    pthread_mutex_lock(mutex);

    estado_code *estado = list_get(estados_procesos, pid - 1);
    estado_code estado_valor = *estado;

    pthread_mutex_unlock(mutex);
    return estado_valor;
}

void contador_quantum_VRR(void *args)
{
    t_parametros_VRR *parametrosVRR = (t_parametros_VRR *)args;
    int64_t tiempo_transcurrido;
    int64_t tiempo_inicial;
    tiempo_inicial = temporal_gettime(clock_global);
    log_info(logger_kernel, "Logueando Quantums config: %d, param: %li", config_kernel->quantum, *parametrosVRR->quantum);
    int64_t time_to_sleep = (*parametrosVRR->quantum) * 1000;
    pthread_mutex_lock(&m_pid_ejecutando);
    uint32_t pid_inicial = pid_ejecutando;
    pthread_mutex_unlock(&m_pid_ejecutando);

    while (1)
    {
        estado_code estado_actual = obtener_estado_proceso(pid_inicial);

        switch (estado_actual)
        {
        case BLOCKED:
        {
            log_info(logger_kernel, "aaaaaaaaaaaaaaaaaaaaaa BLOCKED aaaaaaaaaaaaaaaaaaaaaaaaaaaa %d", pid_inicial);
            goto salir_while;
        }

        case EXITT:
        {
            log_info(logger_kernel, "bbbbbbbbbbbbbbbbbbbbb EXITT bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb %d", pid_inicial);
            goto salir_while;
        }

        case EXEC:
        {
            log_info(logger_kernel, "cccccccccccccccccccccc EXEC ccccccccccccccccccccccccccccccc %d", pid_inicial);
        }

        default:
        {
            break;
        }
        }

        pthread_mutex_lock(&m_pid_ejecutando);
        if (pid_ejecutando == 0)
        {
            pthread_mutex_unlock(&m_pid_ejecutando);
            break;
        }
        pthread_mutex_unlock(&m_pid_ejecutando);

        tiempo_transcurrido = temporal_gettime(clock_global);
        log_info(logger_kernel, " tiempo_transcurrido - tiempo_inicial > config->quantum - *parametrosVRR->quantum --------------------> %ld > %ld", (tiempo_transcurrido - tiempo_inicial), (*parametrosVRR->quantum));
        if (tiempo_transcurrido - tiempo_inicial >= *parametrosVRR->quantum)
        {
            if (pid_inicial == pid_ejecutando)
            {
                interrumpir_proceso(pid_ejecutando);
            }
            break;
        }
        else
        {
            usleep(time_to_sleep / 3.0);
        }
    }
salir_while:;
    //}
    free(parametrosVRR);
}

void prender_clock_RR(t_config_kernel *config_kernel)
{
    log_info(logger_kernel, "El quantum sacado de la config es:  %d", config_kernel->quantum);

    pthread_t switcher_clock_RR;
    pthread_create(&switcher_clock_RR, NULL, (void *)contador_quantum_RR, (void *)config_kernel);
    pthread_detach(switcher_clock_RR);
}

void cambiar_estado_pcb(t_pcb *pcb, estado_code nuevoEstado)
{

    log_info(logger_obligatorio_kernel, "PID: <%d> - Estado Anterior: %s - Estado Actual: %s", pcb->pid, obtener_string_estado_code(pcb->estado), obtener_string_estado_code(nuevoEstado));

    pthread_mutex_lock(&pcb->m_proceso);
    pcb->estado = nuevoEstado;
    pthread_mutex_unlock(&pcb->m_proceso);

    // Actualizar la lista de estados
    pthread_mutex_t *mutex1 = list_get(m_estados_procesos, pcb->pid - 1);
    pthread_mutex_lock(mutex1);
    estado_code *estado_actual = list_get(estados_procesos, pcb->pid - 1);
    *estado_actual = nuevoEstado;
    pthread_mutex_unlock(mutex1);
}

void agregar_a_lista_con_semaforos(t_pcb *pcb, t_list *list, pthread_mutex_t m_sem)
{
    pthread_mutex_lock(&m_sem);
    pthread_mutex_lock(&pcb->m_proceso);
    list_add(list, pcb);
    
    if(list == lista_ready)
    {
        int size = list_size(list);
        char buffer[1024]; // Asumiendo que 1024 es suficiente para todos los PIDs
        snprintf(buffer, sizeof(buffer), "%s: [", "Cola READY");
        
        for (int i = 0; i < size; i++) {
            t_pcb *pcb = (t_pcb *)list_get(list, i);
            char pid_str[12]; // Asumiendo que el PID no superará los 11 caracteres
            snprintf(pid_str, sizeof(pid_str), "%d", pcb->pid);
            strncat(buffer, pid_str, sizeof(buffer) - strlen(buffer) - 1);
            if (i < size - 1) {
                strncat(buffer, ", ", sizeof(buffer) - strlen(buffer) - 1);
            }
        }
        
        strncat(buffer, "]", sizeof(buffer) - strlen(buffer) - 1);
        log_info(logger_obligatorio_kernel, "%s", buffer);

    }else if(list == lista_auxiliar)
    {
        int size = list_size(list);
        char buffer[1024]; // Asumiendo que 1024 es suficiente para todos los PIDs
        snprintf(buffer, sizeof(buffer), "%s: [", "Cola PRIORIDAD");
        
        for (int i = 0; i < size; i++) {
            t_pcb *pcb = (t_pcb *)list_get(list, i);
            char pid_str[12]; // Asumiendo que el PID no superará los 11 caracteres
            snprintf(pid_str, sizeof(pid_str), "%d", pcb->pid);
            strncat(buffer, pid_str, sizeof(buffer) - strlen(buffer) - 1);
            if (i < size - 1) {
                strncat(buffer, ", ", sizeof(buffer) - strlen(buffer) - 1);
            }
        }
        
        strncat(buffer, "]", sizeof(buffer) - strlen(buffer) - 1);
        log_info(logger_obligatorio_kernel, "%s", buffer);
    } 

    pthread_mutex_unlock(&m_sem);
    pthread_mutex_unlock(&pcb->m_proceso);
}

t_pcb *remover_de_lista_auxiliar()
{
    pthread_mutex_lock(&m_list_auxiliar);
    t_pcb *pcb = list_remove(lista_auxiliar, 0);
    pthread_mutex_unlock(&m_list_auxiliar);
    return pcb;
}

void *planificadorCortoPlazo(void *arg)
{
    t_config_kernel *config_kernel = (t_config_kernel *)arg;
    int64_t *q = malloc(sizeof(int64_t));

    while (1)
    {

        sem_wait(sem_cpu_free_to_execute);
        sem_wait(sem_process_in_ready);
        sem_wait(sem_planificacion);

        if (!(strcmp(config_kernel->algoritmo_planificacion, "FIFO")))
        {
            log_info(logger_kernel, "Entre por FIFO");

            if (list_size(lista_ready) != 0)
            {
                pthread_mutex_lock(&m_list_READY);
                pcb_a_ejecutar = list_remove(lista_ready, 0);
                pthread_mutex_unlock(&m_list_READY);

                actualizar_estado_pcb(pcb_a_ejecutar, EXEC, lista_running, m_list_RUNNING);

                log_info(logger_kernel, "PID: %d - Estado Anterior: READY - Estado Actual: RUNNING", pcb_a_ejecutar->pid);
                //log_info(logger_obligatorio_kernel, "PID: <%d> - Estado Anterior: READY - Estado Actual: RUNNING", pcb_a_ejecutar->pid);

                enviar_siguiente_pcb(pcb_a_ejecutar, true);
            }
            sem_post(sem_planificacion);
        }
        else if (!(strcmp(config_kernel->algoritmo_planificacion, "RR")))
        {
            sem_wait(sem_interruption_quantum);
            log_info(logger_kernel, "Entre por RR");

            pthread_mutex_lock(&m_list_READY);
            pcb_a_ejecutar = list_remove(lista_ready, 0);
            pthread_mutex_unlock(&m_list_READY);

            actualizar_estado_pcb(pcb_a_ejecutar, EXEC, lista_running, m_list_RUNNING);

            log_info(logger_kernel, "El proceso %d cambio su estado a RUNNING", pcb_a_ejecutar->pid);
            log_info(logger_kernel, "PID: %d - Estado Anterior: READY - Estado Actual: RUNNING", pcb_a_ejecutar->pid);
            //log_info(logger_obligatorio_kernel, "PID: <%d> - Estado Anterior: READY - Estado Actual: RUNNING", pcb_a_ejecutar->pid);

            enviar_siguiente_pcb(pcb_a_ejecutar, true);

            sem_post(sem_beggin_quantum);
            sem_post(sem_planificacion);
        }
        else if (!(strcmp(config_kernel->algoritmo_planificacion, "VRR")))
        {
            log_info(logger_kernel, "Entre por VRR");

            t_pcb *pcb_a_ejecutar;
            t_parametros_VRR *parametrosVRR = malloc(sizeof(t_parametros_VRR));
            parametrosVRR->config = config_kernel;
            if (list_size(lista_auxiliar) != 0)
            {

                pcb_a_ejecutar = remover_de_lista_auxiliar();
                *q = pcb_a_ejecutar->quantum;
                parametrosVRR->quantum = q;
            }
            else if (list_size(lista_ready) != 0)
            {
                log_info(logger_kernel, "Entre por lista ready");

                pthread_mutex_lock(&m_list_READY);
                pcb_a_ejecutar = list_remove(lista_ready, 0);
                pthread_mutex_unlock(&m_list_READY);
                *q = parametrosVRR->config->quantum;
                parametrosVRR->quantum = q;

                pthread_mutex_lock(&pcb_a_ejecutar->m_proceso);
                pcb_a_ejecutar->quantum = parametrosVRR->config->quantum;
                pthread_mutex_unlock(&pcb_a_ejecutar->m_proceso);
            }
            else
            {
                sem_post(sem_planificacion);
                continue;
            }

            actualizar_estado_pcb(pcb_a_ejecutar, EXEC, lista_running, m_list_RUNNING);

            log_info(logger_kernel, "El proceso %d cambio su estado a RUNNING", pcb_a_ejecutar->pid);
            loggear_pcb(logger_kernel, pcb_a_ejecutar);
            log_info(logger_kernel, "PID: %d - Estado Anterior: READY - Estado Actual: RUNNING", pcb_a_ejecutar->pid);
            //log_info(logger_obligatorio_kernel, "PID: <%d> - Estado Anterior: READY - Estado Actual: RUNNING", pcb_a_ejecutar->pid);
            enviar_siguiente_pcb(pcb_a_ejecutar, true);

            sem_post(sem_planificacion);
            contador_quantum_VRR((void *)parametrosVRR);
        }
        else
        {
            log_error(logger_kernel, "Algoritmo invalido");
            sem_post(sem_planificacion);
        }
    }
    return "";
}

void agregar_a_lista_auxiliar(t_pcb *pcb)
{
    pthread_mutex_lock(&m_list_auxiliar);
    list_add(lista_auxiliar, pcb);
    pthread_mutex_unlock(&m_list_auxiliar);
}

void interrumpir_proceso(uint32_t pid)
{
    uint32_t *copy_pid = malloc(sizeof(uint32_t));
    *copy_pid = pid;
    t_paquete *paquete = serializar_interrupcion(copy_pid);
    log_info(logger_kernel, "Interrumpiendo pid: %d", *copy_pid);
    enviar_paquete(paquete, socket_cpu_interrupt, logger_kernel);
    sem_post(sem_interruption_quantum);
    free(copy_pid);
}

char *obtener_nombre_estado(int state)
{
    char *string_state;
    switch (state)
    {
    case NEW:
        string_state = "NEW";
        break;
    case READY:
        string_state = "READY";
        break;
    case BLOCKED:
        string_state = "BLOCKED";
        break;
    case EXEC:
        string_state = "RUNNING";
        break;
    case EXITT:
        string_state = "EXIT";
        break;
    case READY_AUX:
        string_state = "READY++";
        break;
    default:
        string_state = "ESTADO NO REGISTRADO";
        break;
    }
    return string_state;
}

void actualizar_estado_pcb(t_pcb *pcb, estado_code state, t_list *list, pthread_mutex_t mutex)
{
    cambiar_estado_pcb(pcb, state);
    agregar_a_lista_con_semaforos(pcb, list, mutex);
    log_info(logger_kernel, "El pcb <%d> entro en la cola de <%s> con un QUANTUM DE %li", pcb->pid, obtener_nombre_estado(state), pcb->quantum);
}

int obtener_index(t_list *lista, uint32_t pid)
{
    t_pcb *pcb_seleccionado;
    for (int i = 0; i < list_size(lista); i++)
    {
        pcb_seleccionado = list_get(lista, i);
        if (pcb_seleccionado->pid == pid)
        {
            return i;
        }
    }
    return -1;
}

t_pcb *obtener_pcb_con_pid_lista(uint32_t pid, t_list *lista)
{
    t_pcb *pcb_seleccionado;
    for (int i = 0; i < list_size(lista); i++)
    {
        pcb_seleccionado = list_get(lista, i);
        if (pcb_seleccionado->pid == pid)
        {
            log_info(logger_kernel, "Encontre un PCB");
            return pcb_seleccionado;
        }
    }
    log_info(logger_kernel, "No encontre un PCB");

    return NULL;
}

void enviar_siguiente_pcb(t_pcb *pcb, bool viene_de_planificacion)
{
    if (viene_de_planificacion & !(strcmp(config_kernel->algoritmo_planificacion, "VRR"))) // SI VIENE DE WAIT O SIGNAL NO SE ACTUALIZA EL TIEMPO INICIAL
    {
        pcb->tiempo_inicial = temporal_gettime(clock_global);
    }

    t_paquete *paquete = serializar_paquete_pcb(pcb);
    pthread_mutex_lock(&m_pid_ejecutando);
    pid_ejecutando = pcb->pid;
    pthread_mutex_unlock(&m_pid_ejecutando);
    enviar_paquete(paquete, socket_cpu_dispatch, logger_kernel);
}

void bloquear_proceso(t_pcb *pcb_proceso_a_bloquear, bool es_io)
{
    pthread_mutex_lock(&m_pid_ejecutando);
    pid_ejecutando = 0;
    pthread_mutex_unlock(&m_pid_ejecutando);
    sem_wait(sem_planificacion);
    pthread_mutex_lock(&m_list_RUNNING);
    pthread_mutex_lock(&pcb_proceso_a_bloquear->m_proceso);
    int index = obtener_index(lista_running, pcb_proceso_a_bloquear->pid);
    t_pcb *pcb_exec = list_remove(lista_running, index);

    if (es_io & !(strcmp(config_kernel->algoritmo_planificacion, "VRR")))
    {
        int64_t tiempo_final2 = temporal_gettime(clock_global);
        log_info(logger_kernel, "tiempo_final2 - pcb_proceso_a_bloquear->tiempo_inicial %li", (tiempo_final2 - pcb_proceso_a_bloquear->tiempo_inicial));
        pcb_proceso_a_bloquear->quantum = pcb_proceso_a_bloquear->quantum - (tiempo_final2 - pcb_proceso_a_bloquear->tiempo_inicial);
    }

    pthread_mutex_unlock(&m_list_RUNNING);
    pthread_mutex_unlock(&pcb_proceso_a_bloquear->m_proceso);
    pcb_destroyer(pcb_exec);
    actualizar_estado_pcb(pcb_proceso_a_bloquear, BLOCKED, lista_blocked, m_list_BLOCKED);
    sem_post(sem_planificacion);
}

void desbloquear_proceso(uint32_t pid)
{
    sem_wait(sem_planificacion);
    pthread_mutex_lock(&m_list_BLOCKED);
    int index = obtener_index(lista_blocked, pid);
    t_pcb *pcb_proceso_a_desbloquear = (t_pcb *)list_remove(lista_blocked, index);
    pthread_mutex_unlock(&m_list_BLOCKED);

    if (!(strcmp(config_kernel->algoritmo_planificacion, "VRR")) && pcb_proceso_a_desbloquear->quantum < config_kernel->quantum && pcb_proceso_a_desbloquear->quantum > 0)
    {
        actualizar_estado_pcb(pcb_proceso_a_desbloquear, READY_AUX, lista_auxiliar, m_list_auxiliar);
    }
    else
    {
        actualizar_estado_pcb(pcb_proceso_a_desbloquear, READY, lista_ready, m_list_READY);
    }

    sem_post(sem_process_in_ready);
    sem_post(sem_planificacion);
}

const char* obtener_string_estado_code(estado_code estado) {
    switch (estado) {
        case NEW:
            return "NEW";
        case READY:
            return "READY";
        case EXEC:
            return "EXEC";
        case BLOCKED:
            return "BLOCKED";
        case EXITT:
            return "EXIT";
        case ERROR:
            return "ERROR";
        case READY_AUX:
            return "READY_AUX";
        default:
            return "UNKNOWN";
    }
}