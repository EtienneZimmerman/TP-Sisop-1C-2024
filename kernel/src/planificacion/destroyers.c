#include "main.h"

void destruir_listas_estados_procesos() 
{
    for (int i = 0; i < list_size(m_estados_procesos); i++) {
        pthread_mutex_t *mutex = list_get(m_estados_procesos, i);
        pthread_mutex_destroy(mutex);
        free(mutex); 

        estado_code *estado = list_get(estados_procesos, i);
        free(estado);
    }
    list_destroy(m_estados_procesos); 
    list_destroy(estados_procesos); 
}

void destruir_listas_planificacion()
{
    if (list_size(lista_new) > 0)
        list_destroy_and_destroy_elements(lista_new, pcb_destroyer);
    else
        list_destroy(lista_new);
    if (list_size(lista_ready) > 0)
        list_destroy_and_destroy_elements(lista_ready, pcb_destroyer);
    else
        list_destroy(lista_ready);
    if (list_size(lista_blocked) > 0)
        list_destroy_and_destroy_elements(lista_blocked, pcb_destroyer);
    else
        list_destroy(lista_blocked);
    if (list_size(lista_running) > 0)
        list_destroy_and_destroy_elements(lista_running, pcb_destroyer);
    else
        list_destroy(lista_running);
    if (list_size(lista_exit) > 0)
        list_destroy_and_destroy_elements(lista_exit, pcb_destroyer);
    else
        list_destroy(lista_exit);
}

void destroy_semaphores(t_config_kernel *config_kernel)
{
    sem_destroy(sem_grado_multiprogamacion);
    sem_destroy(sem_process_in_ready);
    sem_destroy(sem_cpu_free_to_execute);
    sem_destroy(sem_beggin_quantum);
    sem_destroy(sem_planificacion);
    sem_destroy(sem_planificacion_largo);
    sem_destroy(sem_proceso_se_creo_en_memoria);
    if (string_equals_ignore_case(config_kernel->algoritmo_planificacion, "RR") || string_equals_ignore_case(config_kernel->algoritmo_planificacion, "VRR"))
    {
        sem_destroy(sem_interruption_quantum);
        free(sem_interruption_quantum);
    }
    free(sem_grado_multiprogamacion);
    free(sem_process_in_ready);
    free(sem_cpu_free_to_execute);
    free(sem_beggin_quantum);
    free(sem_planificacion);
    free(sem_planificacion_largo);
    free(sem_proceso_se_creo_en_memoria);
}

void pcb_destroyer(void *pcb_void)
{
    t_pcb *pcb = (t_pcb *)pcb_void;
    if (pcb->recurso_esperando != NULL)
    {
        free(pcb->recurso_esperando);
    }
    if (pcb->recursos_tomados != NULL && list_size(pcb->recursos_tomados) > 0)
    {
        list_destroy_and_destroy_elements(pcb->recursos_tomados, element_destroyer);
    }
    else if (pcb->recursos_tomados != NULL && list_size(pcb->recursos_tomados) == 0)
    {
        list_destroy(pcb->recursos_tomados);
    }
    free(pcb);
}

void destruir_clock_global()
{
    temporal_destroy(clock_global);
}
