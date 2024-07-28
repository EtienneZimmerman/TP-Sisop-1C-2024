
#include "main.h"

pthread_t hilo_consola;
pthread_mutex_t m_multiprogramacion;

bool pausar = false;

void crear_hilo_consola()
{
    pthread_create(&hilo_consola, NULL, (void *)inicializar_consola, NULL);
    pthread_join(hilo_consola, NULL);
}

void ejecutar_script(char *nombre_archivo)
{
    t_list *instrucciones = leer_archivo("", nombre_archivo);

    if(instrucciones == NULL)
    {
        /* pthread_mutex_lock(&m_logs);
        log_info(logger_obligatorio_kernel, "ERRO al leer el archivo: %s", nombre_archivo);
        pthread_mutex_unlock(&m_logs); */
        return;
    }

    char *instruccion;
    while (list_size(instrucciones) > 0)
    {
        instruccion = (char *)list_remove(instrucciones, 0);
        procesar_comando(instruccion);
        free(instruccion);
    }
    list_destroy(instrucciones);
}

void detener_planificacion()
{
    if (pausar)
    {
        sem_wait(sem_planificacion);
        sem_wait(sem_planificacion_largo);
        pausar = false;
    }
}

void iniciar_planificacion()
{
    if (!pausar)
    {
        sem_post(sem_planificacion);
        sem_post(sem_planificacion_largo);
        pausar = true;
    }
}

void *cambiar_multiprogramacion(void *valor_param)
{
    pthread_mutex_lock(&m_multiprogramacion);
    int valor = *(int *)valor_param;
    
    pthread_mutex_lock(&m_config_kernel_multiprogramacion);
    if(config_kernel->grado_multiprogramacion < valor)//subo grado de multiprogramacion
    {
        int diferencia_grados_multiprogramacion = valor - config_kernel->grado_multiprogramacion;
        for (int e = 0; e < diferencia_grados_multiprogramacion; e++)
        {
            sem_post(sem_grado_multiprogamacion);
        }    
    }

    
    config_kernel->grado_multiprogramacion = valor;
    pthread_mutex_unlock(&m_config_kernel_multiprogramacion);

    free(valor_param);
    pthread_mutex_unlock(&m_multiprogramacion);
    return "";
}

void estado_proceso()
{
    log_info(logger_kernel, "Logueando PCBs en NEW");
    log_info(logger_obligatorio_kernel, "Logueando PCBs en NEW");
    log_info(logger_obligatorio_kernel, " ");
    listar_procesos_en_lista(lista_new);

    log_info(logger_kernel, "Logueando PCBs en READY");
    log_info(logger_obligatorio_kernel, "Logueando PCBs en READY");
    log_info(logger_obligatorio_kernel, " ");
    listar_procesos_en_lista(lista_ready);

    log_info(logger_kernel, "Logueando PCB en EXEC");
    log_info(logger_obligatorio_kernel, "Logueando PCB en EXEC");
    log_info(logger_obligatorio_kernel, " ");
    listar_proceso_exec();

    log_info(logger_kernel, "Logueando PCBs en BLOCKED");
    log_info(logger_obligatorio_kernel, "Logueando PCBs en BLOCKED");
    log_info(logger_obligatorio_kernel, " ");
    listar_procesos_en_lista(lista_blocked);

    log_info(logger_kernel, "Logueando PCBs en EXIT");
    log_info(logger_obligatorio_kernel, "Logueando PCBs en EXIT");
    log_info(logger_obligatorio_kernel, " ");
    listar_procesos_en_lista(lista_exit);
}

void inicializar_consola()
{
    while (1)
    {
        char *linea = readline(">Ingrese una instruccion: ");
        if (linea)
        {
            add_history(linea);
            if (strcmp(linea, "exit") == 0)
            {
                printf("Saliendo de la consola\n");
                free(linea);
                break;
            }
            // Call your function with the obtained line here
            procesar_comando(linea);
            free(linea);
        }
    }
    return;
}

void procesar_comando(char *linea)
{
    char path[256];
    uint32_t *pid = malloc(sizeof(uint32_t));
    int *valor = malloc(sizeof(int));
    pthread_t hilo_aux;

    if (sscanf(linea, "EJECUTAR_SCRIPT %255s", path) == 1)
    {
        printf("Ejecutar script con path: %s\n", path);
        free(pid);
        free(valor);
        ejecutar_script(path);
    }
    else if (sscanf(linea, "INICIAR_PROCESO %255s", path) == 1)
    {
        printf("Iniciar proceso con path: %s\n", path);
        char *path_param = malloc(strlen(path) + 1);
        strcpy(path_param, path);
        crear_proceso((void *)path_param);
        free(valor);
        free(pid);
    }
    else if (sscanf(linea, "FINALIZAR_PROCESO %u", pid) == 1)
    {
        printf("Finalizar proceso con PID: %d\n", *pid);
        log_info(logger_obligatorio_kernel, "Finaliza el proceso <%u> - Motivo: <INTERRUPTED_BY_USER>", *pid);
        pthread_create(&hilo_aux, NULL, finalizar_proceso, (void *)pid);
        pthread_detach(hilo_aux);
        free(valor);
    }
    else if (strcmp(linea, "DETENER_PLANIFICACION") == 0)
    {
        printf("Detener planificación\n");
        detener_planificacion();
        free(valor);
        free(pid);
    }
    else if (strcmp(linea, "INICIAR_PLANIFICACION") == 0)
    {
        printf("Iniciar planificación\n");
        iniciar_planificacion();
        free(valor);
        free(pid);
    }
    else if (sscanf(linea, "MULTIPROGRAMACION %d", valor) == 1)
    {
        printf("Multiprogramación con valor: %d\n", *valor);
        pthread_create(&hilo_aux, NULL, cambiar_multiprogramacion, (void *)valor);
        pthread_detach(hilo_aux);
        free(pid);
    }
    else if (strcmp(linea, "PROCESO_ESTADO") == 0)
    {
        printf("Estado del proceso\n");
        free(valor);
        free(pid);
        estado_proceso();
    }
    else
    {
        printf("El comando no existe\n");
        free(valor);
        free(pid);
    }
}
