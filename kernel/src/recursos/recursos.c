#include "main.h"

static t_dictionary *recursos;

/**
 * @brief       Inicializa el diccionario de recursos a partir de la estructura
 *              de configuración del Kernel.
 *
 * @details     Crea un diccionario que mantiene el módulo internamente, que es
 *              destruido con recursos_deinit. Guarda, para cada recurso:
 *              - su nombre
 *              - la cantidad de instancias disponibles
 *              - una cola de bloqueados por ese recurso
 */
void recursos_init(t_config_kernel *config)
{
    recursos = dictionary_create();
    for (int i = 0; i < string_array_size(config->recursos); i++)
    {
        t_recurso *recurso = malloc(sizeof(t_recurso));
        memset(recurso->id, 0, ID_RECURSO_LONG_MAX + 1);
        strncpy(recurso->id, config->recursos[i], ID_RECURSO_LONG_MAX);
        recurso->bloqueados = queue_create();
        recurso->instancias = atoi(config->instancias_recursos[i]);
        recurso->indice_semaforo_asociado_a_recurso = i;
        pthread_mutex_init(&(recurso->m_recurso), NULL);
        dictionary_put(recursos, config->recursos[i], recurso);
    }
}

/**
 * @brief       Consulta si un recurso existe o no.
 *
 * @param{in} recurso   El recurso en cuestión.
 *
 * @return      true si el recurso existe, false si no existe.
 */
bool recurso_existe(char *recurso)
{
    return dictionary_has_key(recursos, recurso);
}

static bool recurso_modificar_instancias(char *recurso, int *instancias, int delta)
{
    t_recurso *aux = (t_recurso *)dictionary_get(recursos, recurso);
    if (aux != NULL)
    {
        log_info(logger_kernel, "Instancias del recurso antes %s : %d", recurso, aux->instancias);
        pthread_mutex_lock(&aux->m_recurso);

        aux->instancias += delta;
        if (instancias != NULL)
        {
            *instancias = aux->instancias;
        }

        pthread_mutex_unlock(&aux->m_recurso);
        log_info(logger_kernel, "Instancias del recurso despues %s : %d", recurso, aux->instancias);

        return true;
    }
    else
    {
        log_error(logger_kernel, "No se encontró el recurso %s", recurso);
    }
    return false;
}


static bool recurso_modificar_instancias2(char *recurso, int *instancias, int delta, int pid)
{
    t_recurso *aux = (t_recurso *)dictionary_get(recursos, recurso);
    if (aux != NULL)
    {
        log_info(logger_kernel, "Instancias del recurso antes %s : %d", recurso, aux->instancias);
        pthread_mutex_lock(&aux->m_recurso);

        aux->instancias += delta;
        if (instancias != NULL)
        {
            *instancias = aux->instancias;
        }

        log_info(logger_kernel, "Instancias del recurso despues %s : %d", recurso, aux->instancias);

        log_info(logger_kernel, "ANTES Recurso <%s> - queue_size(recurso->bloqueados) = %d", recurso, queue_size(aux->bloqueados));
        int index = obtener_index(aux->bloqueados->elements, pid);
        if (index != -1) 
        {
            // Eliminar el elemento en el índice encontrado
            list_remove(aux->bloqueados->elements, index);
        } 
        log_info(logger_kernel, "DESPUES Recurso <%s> - queue_size(recurso->bloqueados) = %d", recurso, queue_size(aux->bloqueados));
        pthread_mutex_unlock(&aux->m_recurso);

        return true;
    }
    else
    {
        log_error(logger_kernel, "No se encontró el recurso %s", recurso);
    }
    return false;
}

/**
 * @brief       Incrementa el número de instancias disponibles de un recurso.
 *
 * @param{in}   recurso     El recurso en cuestión.
 * @param{out}  instancias  El número de instancias del recurso luego de incrementar. NULL safe.
 *
 * @return      false si el recurso no existe, o true si existe.
 */
bool recurso_incrementar(char *recurso, int *instancias)
{
    bool result = recurso_modificar_instancias(recurso, instancias, 1);

    if (result && *instancias <= 0)
    {
        uint32_t pid;
        recurso_obtener_bloqueado(recurso, &pid);
        t_pcb *pcb_proceso_a_desbloquear = obtener_pcb_con_pid_lista(pid, lista_blocked);

        if (pcb_proceso_a_desbloquear != NULL)
        {
            pthread_mutex_lock(&pcb_proceso_a_desbloquear->m_proceso);
            char *rec = malloc(sizeof(char) * strlen(recurso) + 1);
            memcpy(rec, recurso, strlen(recurso) + 1);
            list_add(pcb_proceso_a_desbloquear->recursos_tomados, rec);

            if (pcb_proceso_a_desbloquear->recurso_esperando != NULL) 
            {
                free(pcb_proceso_a_desbloquear->recurso_esperando);
                pcb_proceso_a_desbloquear->recurso_esperando = NULL;
            }
            
            pthread_mutex_unlock(&pcb_proceso_a_desbloquear->m_proceso);
            desbloquear_proceso(pcb_proceso_a_desbloquear->pid);
        }
    }

    return result;
}

/**
 * @brief       Decrementa el número de instancias disponibles de un recurso.
 *
 * @param{in}   recurso     El recurso en cuestión.
 * @param{out}  instancias  El número de instancias del recurso luego de decrementar. NULL safe.
 *
 * @return      false si el recurso no existe, o true si existe.
 */
bool recurso_decrementar(char *recurso, int *instancias, t_pcb *aux_pcb_ejecutado)
{
    bool result = recurso_modificar_instancias(recurso, instancias, -1);

    if (result && *instancias < 0)
    {
        
        log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", aux_pcb_ejecutado->pid, recurso);

        bloquear_proceso(aux_pcb_ejecutado, false);
        recurso_agregar_bloqueado(recurso, aux_pcb_ejecutado);

        sem_post(sem_cpu_free_to_execute);
        log_info(logger_kernel, "Le hice un post al semaforo de que el cpu esta libre");

        return false;
    }

    return result;
}

bool criterio_busqueda(void *elemento, void *pid)
{
    return ((t_pcb *)elemento)->pid == *(uint32_t *)pid;
}

/**
 * @brief     Agrega una referencia al PCB de un proceso a la cola de bloqueados por un recurso.
 *
 * @param{in} recurso  El recurso en cuestión.
 * @param{in} pcb      Referencia al PCB del proceso a agregar.
 *
 * @return    false si el recurso no existe, o true si existe.
 */
bool recurso_agregar_bloqueado(char *recurso, t_pcb *pcb)
{
    t_recurso *aux;
    if ((aux = (t_recurso *)dictionary_get(recursos, recurso)) != NULL)
    {
        pthread_mutex_lock(&aux->m_recurso);
        queue_push(aux->bloqueados, &(pcb->pid));
        pthread_mutex_unlock(&aux->m_recurso);

        pcb->recurso_esperando = recurso;
        return true;
    }
    return false;
}

/**
 * @brief      Obtiene el PCB del primer proceso bloqueado esperando por un recurso.
 *
 * @param{in}  recurso  El recurso en cuestión.
 * @param{out} pcb      Dirección de memoria donde se almacenará el puntero al PCB obtenido.
 *
 * @return     false si el recurso no existe, o true si existe.
 */
bool recurso_obtener_bloqueado(char *recurso, uint32_t *pid)
{
    t_recurso *aux;
    if ((aux = (t_recurso *)dictionary_get(recursos, recurso)) != NULL)
    {
        // queue_pop me devuelve un puntero al pcb del primer proceso bloqueado por ese recurso
        pthread_mutex_lock(&aux->m_recurso);
        uint32_t *pid_ptr = (uint32_t *)queue_pop(aux->bloqueados);
        pthread_mutex_unlock(&aux->m_recurso);

        if (pid_ptr != NULL)
        {
            *pid = *pid_ptr;
        }
        else
        {
            return false; // No hay procesos bloqueados
        }
        return true;
    }
    return false;
}

static void destruir_recurso(void *recurso)
{
    t_recurso *rec = (t_recurso *)recurso;
    queue_destroy(rec->bloqueados);
}

/**
 * @brief       Destruye el diccionario de recursos, y todos los recursos en él.
 */
void recursos_deinit()
{
    dictionary_destroy_and_destroy_elements(recursos, destruir_recurso);
}

int obtener_index_recurso(t_list *lista, char *recurso)
{
    char *recurso_seleccionado;
    for (int i = 0; i < list_size(lista); i++)
    {
        recurso_seleccionado = list_get(lista, i);

        if (strcmp(recurso_seleccionado, recurso) == 0)
        {
            return i;
        }
    }
    return -1;
}

void liberar_recursos_tomados(t_pcb *pcb_liberar)
{
    for (int i = 0; i < list_size(pcb_liberar->recursos_tomados); i++)
    {
        char *recurso_tomado = list_get(pcb_liberar->recursos_tomados, i);
        t_recurso *aux_recurso_tomado = (t_recurso *)dictionary_get(recursos, recurso_tomado);
        int instancias_restantes;
        recurso_incrementar(aux_recurso_tomado->id, &instancias_restantes);
        log_info(logger_kernel, "Libero el recurso: <%s> - tomado por el proceso pid: <%d>", aux_recurso_tomado->id, pcb_liberar->pid);
    }

    if (pcb_liberar->recurso_esperando != NULL)
    {
        int instancias;
        recurso_modificar_instancias2(pcb_liberar->recurso_esperando, &instancias, 1, pcb_liberar->pid);

        if (pcb_liberar->recurso_esperando != NULL) 
        {
            free(pcb_liberar->recurso_esperando);
            pcb_liberar->recurso_esperando = NULL;
        }  
    }
}