#include "main.h"

void init_estructuras_planificacion()
{
    lista_new = list_create();
    lista_ready = list_create();
    lista_blocked = list_create();
    lista_running = list_create();
    lista_exit = list_create();
    lista_auxiliar = list_create();
    
    estados_procesos = list_create();
    m_estados_procesos = list_create();
}

void init_clock_global()
{
    clock_global = temporal_create();
}

void init_semaphores(t_config_kernel *config_kernel)
{

    sem_grado_multiprogamacion = malloc(sizeof(sem_t));
    sem_process_in_ready = malloc(sizeof(sem_t));
    sem_cpu_free_to_execute = malloc(sizeof(sem_t));
    sem_beggin_quantum = malloc(sizeof(sem_t));
    sem_planificacion = malloc(sizeof(sem_t));
    sem_planificacion_largo = malloc(sizeof(sem_t));
    sem_proceso_se_creo_en_memoria = malloc(sizeof(sem_t));
    sem_init(sem_planificacion, 0, 0);
    sem_init(sem_planificacion_largo, 0, 0);
    sem_init(sem_grado_multiprogamacion, 0, config_kernel->grado_multiprogramacion);
    sem_init(sem_process_in_ready, 0, 0);
    sem_init(sem_cpu_free_to_execute, 0, 1);
    sem_init(sem_beggin_quantum, 0, 0);
    sem_init(sem_proceso_se_creo_en_memoria, 0, 0);

    if (string_equals_ignore_case(config_kernel->algoritmo_planificacion, "RR") || string_equals_ignore_case(config_kernel->algoritmo_planificacion, "VRR"))
    {
        sem_interruption_quantum = malloc(sizeof(sem_t));
        sem_init(sem_interruption_quantum, 0, 1);
        //log_info(logger_kernel, "Inicialice en semaforo de interrupcion por quantum");
    }
}

void init_mutex()
{
    if (pthread_mutex_init(&m_list_NEW, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_list_READY, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_list_RUNNING, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_list_BLOCKED, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_list_EXIT, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_socket_memoria, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_list_auxiliar, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_pid_ejecutando, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&m_config_kernel_multiprogramacion, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
}

void init_planificadores(t_config_kernel *config_kernel)
{

    log_info(logger_kernel, "Inicializando hilos planificadores...");

    pthread_create(&planificador_corto_plazo, NULL, planificadorCortoPlazo, (void *)config_kernel);
    pthread_detach(planificador_corto_plazo);

    if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0)
    {
        log_info(logger_kernel, "Prendiendo el contador de Quantum");
        prender_clock_RR(config_kernel);
    }

}
