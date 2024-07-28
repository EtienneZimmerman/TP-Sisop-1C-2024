#include "main.h"

pthread_mutex_t m_mover_ready;

int *iniciar_server_kernel(int puerto, t_log *logger)
{
    int *server_kernel = malloc(sizeof(int));
    char *puerto_ = string_itoa(puerto);
    *server_kernel = iniciar_servidor(puerto_, NULL, logger);
    free(puerto_);
    return server_kernel;
}

void io_pcb_destroyer(t_io_pcb *io_pcb)
{
    free(io_pcb);
}

void asignar_pcb_deserializado(t_pcb *pcb_deserializado, t_pcb *pcb_cde)
{
    *pcb_deserializado = *pcb_cde;
    if (pcb_cde->recurso_esperando != NULL)
    {
        pcb_deserializado->recurso_esperando = malloc(string_length(pcb_cde->recurso_esperando) + 1);
        memcpy(pcb_deserializado->recurso_esperando, pcb_cde->recurso_esperando, string_length(pcb_cde->recurso_esperando) + 1);
    }
    if (pcb_cde->recursos_tomados != NULL)
    {
        pcb_deserializado->recursos_tomados = list_create();
        for (int i = 0; i < list_size(pcb_cde->recursos_tomados); i++)
        {
            char *rec = list_get(pcb_cde->recursos_tomados, i);
            char *copy_rec = malloc(sizeof(char) * string_length(rec) + 1);
            memcpy(copy_rec, rec, string_length(rec) + 1);
            list_add(pcb_deserializado->recursos_tomados, copy_rec);
        }
    }
}

void destruir_server_kernel(int *server_kernel)
{
    cerrar_servidor(*server_kernel);
}
void incrementar_grado_multi()
{
    sem_post(sem_grado_multiprogamacion);
}

void atender_cpu_dispatch()
{
    while (1)
    {
        int size;
        t_buffer *buffer;
        int *tam_recibido = malloc(sizeof(int));
        t_pcb *pcb_ejecutado;
        int cod_op = recibir_operacion(socket_cpu_dispatch);
        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(socket_cpu_dispatch, logger_kernel);
            free(tam_recibido);
            break;
        case INSTRUCCION_IO_GEN_SLEEP:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_gen_sleep *peticion_io_gen_sleep = deserializar_obtener_paquete_peticion_io_gen_sleep(buffer, tam_recibido);
            log_info(logger_kernel, "Recibi el contexto de ejecucion motivo: %s", motivo_desalojo_to_string(peticion_io_gen_sleep->cde->motivo_desalojo));
            loggear_pcb(logger_kernel, peticion_io_gen_sleep->cde->pcb_ejecutado);
            log_info(logger_kernel, "Recibi el interfaz: %s y la udt: %i", peticion_io_gen_sleep->interfaz, peticion_io_gen_sleep->unidades_de_trabajo);

            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, peticion_io_gen_sleep->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, peticion_io_gen_sleep->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, peticion_io_gen_sleep->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, peticion_io_gen_sleep->interfaz);
                // pcb_ejecutado->estado = EXITT;
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {
                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, peticion_io_gen_sleep->interfaz);
                bloquear_proceso(pcb_ejecutado, true);
                enviar_io_gen_sleep_a_interfaz(peticion_io_gen_sleep->interfaz, &peticion_io_gen_sleep->unidades_de_trabajo, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);
                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)
                    sem_post(sem_interruption_quantum);
            }
            sem_post(sem_cpu_free_to_execute);
            pcb_destroyer(peticion_io_gen_sleep->cde->pcb_ejecutado);
            free(peticion_io_gen_sleep->interfaz);
            free(peticion_io_gen_sleep->cde);
            free(buffer);
            free(tam_recibido);
            free(peticion_io_gen_sleep);
            break;

        case INSTRUCCION_IO_STDIN_READ:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_stdin_read *stdin_read = deserializar_obtener_paquete_peticion_io_stdin_read(buffer, tam_recibido);
            log_info(logger_kernel, "Recibi el contexto de ejecucion motivo: %s", motivo_desalojo_to_string(stdin_read->cde->motivo_desalojo));
            loggear_pcb(logger_kernel, stdin_read->cde->pcb_ejecutado);
            log_info(logger_kernel, "Recibi la interfaz: %s", stdin_read->interfaz);
            log_info(logger_kernel, "Recibi el desplazamiento: %u, marco: %u, pid: %u", stdin_read->dir_fisica->desplazamiento_fisico, stdin_read->dir_fisica->marco, stdin_read->dir_fisica->pid);
            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, stdin_read->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, stdin_read->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, stdin_read->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, stdin_read->interfaz);
                // pcb_ejecutado->estado = EXIT;
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {
                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, stdin_read->interfaz);

                bloquear_proceso(pcb_ejecutado, true);
                log_info(logger_kernel, "EJECUTE INTERFAZ STDIN");
                enviar_io_stdin_read_a_interfaz(stdin_read->dir_fisica, stdin_read->interfaz, stdin_read->tamanio, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);

                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)
                    sem_post(sem_interruption_quantum);
            }
            sem_post(sem_cpu_free_to_execute);
            free(tam_recibido);
            pcb_destroyer(stdin_read->cde->pcb_ejecutado);
            free(stdin_read->cde);
            free(stdin_read);
            free(buffer);
            break;
        case INSTRUCCION_IO_STDOUT_WRITE:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_stdout_write *stdout_write = deserializar_obtener_paquete_peticion_io_stdout_write(buffer, tam_recibido);
            log_info(logger_kernel, "Recibi el contexto de ejecucion motivo: %s", motivo_desalojo_to_string(stdout_write->cde->motivo_desalojo));
            loggear_pcb(logger_kernel, stdout_write->cde->pcb_ejecutado);
            log_info(logger_kernel, "Recibi la interfaz: %s", stdout_write->interfaz);
            log_info(logger_kernel, "Recibi el desplazamiento: %u, marco: %u, pid: %u", stdout_write->dir_fisica->desplazamiento_fisico, stdout_write->dir_fisica->marco, stdout_write->dir_fisica->pid);
            log_info(logger_kernel, "IO_STDOUT_WRITE tamanio: %u", stdout_write->tamanio);
            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, stdout_write->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, stdout_write->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, stdout_write->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, stdout_write->interfaz);
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {

                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, stdout_write->interfaz);
                bloquear_proceso(pcb_ejecutado, true);
                log_info(logger_kernel, "EJECUTE INTERFAZ STDOUT");
                uint32_t *tam = malloc(sizeof(uint32_t));
                *tam = stdout_write->tamanio;
                enviar_io_stdout_write_a_interfaz(stdout_write->dir_fisica, stdout_write->interfaz, tam, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);
                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)
                    sem_post(sem_interruption_quantum);
                free(tam);
            }
            sem_post(sem_cpu_free_to_execute);
            pcb_destroyer(stdout_write->cde->pcb_ejecutado);
            free(stdout_write->cde);
            free(stdout_write);
            free(buffer);
            free(tam_recibido);
            break;
        case INSTRUCCION_IO_FS_CREATE:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_fs_create *fs_create = deserializar_peticion_io_fs_create(buffer, tam_recibido);
            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, fs_create->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, fs_create->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, fs_create->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, fs_create->interfaz);
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {
                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, fs_create->interfaz);
                bloquear_proceso(pcb_ejecutado, true);
                log_info(logger_kernel, "recibi la interfaz: %s, nombre_archivo: %s", fs_create->interfaz, fs_create->nombre_archivo);
                enviar_io_fs_create_a_interfaz(fs_create->interfaz, fs_create->nombre_archivo, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);
                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)

                    sem_post(sem_interruption_quantum);
            }
            sem_post(sem_cpu_free_to_execute);
            pcb_destroyer(fs_create->cde->pcb_ejecutado);
            free(fs_create->interfaz);
            free(fs_create->nombre_archivo);
            free(fs_create->cde);
            free(fs_create);
            free(buffer);
            free(tam_recibido);
            break;
        case INSTRUCCION_IO_FS_DELETE:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_fs_delete *fs_delete = deserializar_peticion_io_fs_delete(buffer, tam_recibido);
            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, fs_delete->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, fs_delete->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, fs_delete->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, fs_delete->interfaz);
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {

                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, fs_delete->interfaz);
                bloquear_proceso(pcb_ejecutado, true);
                log_info(logger_kernel, "recibi la interfaz: %s, nombre_archivo: %s", fs_delete->interfaz, fs_delete->nombre_archivo);
                enviar_io_fs_delete_a_interfaz(fs_delete->interfaz, fs_delete->nombre_archivo, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);
                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)
                    sem_post(sem_interruption_quantum);
            }

            sem_post(sem_cpu_free_to_execute);
            pcb_destroyer(fs_delete->cde->pcb_ejecutado);
            free(fs_delete->cde);
            free(fs_delete->interfaz);
            free(fs_delete->nombre_archivo);
            free(fs_delete);
            free(buffer);
            free(tam_recibido);
            break;
        case INSTRUCCION_IO_FS_TRUNCATE:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_fs_truncate *fs_truncate = deserializar_peticion_io_fs_truncate(buffer, tam_recibido);
            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, fs_truncate->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, fs_truncate->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, fs_truncate->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, fs_truncate->interfaz);
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {
                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, fs_truncate->interfaz);
                bloquear_proceso(pcb_ejecutado, true);
                log_info(logger_kernel, "recibi la interfaz: %s, nombre_archivo: %s, tamanio: %d", fs_truncate->interfaz, fs_truncate->nombre_archivo, fs_truncate->tam);
                enviar_io_fs_truncate_a_interfaz(fs_truncate->interfaz, fs_truncate->nombre_archivo, fs_truncate->tam, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);
                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)
                    sem_post(sem_interruption_quantum);
            }
            sem_post(sem_cpu_free_to_execute);
            pcb_destroyer(fs_truncate->cde->pcb_ejecutado);
            free(fs_truncate->cde);
            free(fs_truncate->interfaz);
            free(fs_truncate->nombre_archivo);
            free(fs_truncate);
            free(buffer);
            free(tam_recibido);
            break;
        case INSTRUCCION_IO_FS_READ:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_fs_read *fs_read = deserializar_peticion_io_fs_read(buffer, tam_recibido);
            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, fs_read->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, fs_read->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, fs_read->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, fs_read->interfaz);
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {

                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, fs_read->interfaz);
                bloquear_proceso(pcb_ejecutado, true);
                log_info(logger_kernel, "recibi la interfaz: %s, nombre_archivo: %s, tamanio: %d, posicion_archivo: %d", fs_read->interfaz, fs_read->nombre_archivo, fs_read->tam, fs_read->posicion_archivo);
                enviar_io_fs_read_a_interfaz(fs_read->dir_fis, fs_read->interfaz, fs_read->nombre_archivo, fs_read->tam, fs_read->posicion_archivo, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);
                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)
                    sem_post(sem_interruption_quantum);
            }
            sem_post(sem_cpu_free_to_execute);
            pcb_destroyer(fs_read->cde->pcb_ejecutado);
            free(fs_read->dir_fis);
            free(fs_read->cde);
            free(fs_read->interfaz);
            free(fs_read->nombre_archivo);
            free(fs_read);
            free(buffer);
            free(tam_recibido);
            break;
        case INSTRUCCION_IO_FS_WRITE:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_peticion_io_fs_write *fs_write = deserializar_peticion_io_fs_write(buffer, tam_recibido);
            pcb_ejecutado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_ejecutado, fs_write->cde->pcb_ejecutado);

            if (!dictionary_has_key(dict_interfaces, fs_write->interfaz))
            {
                log_info(logger_kernel, "PID: <%d> - Finalizado porque la interfaz <%s> no existe", pcb_ejecutado->pid, fs_write->interfaz);
                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_INTERFACE> - Interface: <%s>", pcb_ejecutado->pid, fs_write->interfaz);
                pthread_mutex_lock(&m_list_RUNNING);
                t_pcb *pcb_old = list_remove(lista_running, 0);
                pcb_destroyer(pcb_old);
                pthread_mutex_unlock(&m_list_RUNNING);
                eliminar_proceso(pcb_ejecutado);
            }
            else
            {

                log_info(logger_obligatorio_kernel, "PID: <%d> - Bloqueado por: <%s>", pcb_ejecutado->pid, fs_write->interfaz);
                bloquear_proceso(pcb_ejecutado, true);
                log_info(logger_kernel, "recibi la interfaz: %s, nombre_archivo: %s, tamanio: %d, posicion_archivo: %d", fs_write->interfaz, fs_write->nombre_archivo, fs_write->tam, fs_write->posicion_archivo);
                enviar_io_fs_write_a_interfaz(fs_write->dir_fis, fs_write->interfaz, fs_write->nombre_archivo, fs_write->tam, fs_write->posicion_archivo, &pcb_ejecutado->pid);
                pthread_mutex_lock(&m_pid_ejecutando);
                pid_ejecutando = 0;
                pthread_mutex_unlock(&m_pid_ejecutando);
                if (strcmp(config_kernel->algoritmo_planificacion, "RR") == 0 || strcmp(config_kernel->algoritmo_planificacion, "VRR") == 0)
                    sem_post(sem_interruption_quantum);
            }
            sem_post(sem_cpu_free_to_execute);
            pcb_destroyer(fs_write->cde->pcb_ejecutado);
            free(fs_write->dir_fis);
            free(fs_write->cde);
            free(fs_write->interfaz);
            free(fs_write->nombre_archivo);
            free(fs_write);
            free(buffer);
            free(tam_recibido);
            break;
        case ENVIAR_CONTEXTO_EJECUCION:
            buffer = recibir_buffer(&size, socket_cpu_dispatch);
            t_contexto_de_ejecucion *cde = deserializar_contexto_ejecucion(buffer, tam_recibido);
            t_pcb *pcb_deserializado = malloc(sizeof(t_pcb));
            asignar_pcb_deserializado(pcb_deserializado, cde->pcb_ejecutado);
            log_info(logger_kernel, "Recibi el contexto de ejecucion motivo: %s", motivo_desalojo_to_string(cde->motivo_desalojo));
            loggear_pcb(logger_kernel, cde->pcb_ejecutado);
            if (cde->motivo_desalojo == INTERRUPCION)
            {
                log_info(logger_kernel, "Desaloje por fin de Quantum");
                log_info(logger_obligatorio_kernel, "PID: <%d> - Desalojado por fin de Quantum", pcb_deserializado->pid);
                sem_wait(sem_planificacion);
                pthread_mutex_lock(&m_list_RUNNING);
                if (lista_running > 0)
                {
                    list_remove_and_destroy_element(lista_running, 0, pcb_destroyer);
                    pthread_mutex_unlock(&m_list_RUNNING);
                    actualizar_estado_pcb(pcb_deserializado, READY, lista_ready, m_list_READY);
                }
                else
                {
                    pthread_mutex_unlock(&m_list_RUNNING);
                    log_error(logger_obligatorio_kernel, "NO HAY ELEMENTO EN RUNNING??? ");
                    estado_proceso();
                }
                sem_post(sem_process_in_ready);
                sem_post(sem_cpu_free_to_execute);
                sem_post(sem_planificacion);
                log_info(logger_kernel, "Le hice un post al semaforo de que el cpu esta libre");
            }
            else if (cde->motivo_desalojo == OUT_OF_MEMORY_ERROR)
            {
                // Obtenemos el pcb del contexto de ejecucion y luego realizamos lo mismo que terminar proceso
                log_info(logger_kernel, "Desaloje por OUT_OF_MEMORY_ERROR");
                sem_wait(sem_planificacion_largo);
                log_info(logger_kernel, "La cantidad de recursos_tomados por pid <%d> ES: <%d>", pcb_deserializado->pid, list_size(pcb_deserializado->recursos_tomados));
                liberar_recursos_tomados(pcb_deserializado);

                pthread_mutex_lock(&m_list_RUNNING);
                list_remove_and_destroy_element(lista_running, 0, pcb_destroyer);
                pthread_mutex_unlock(&m_list_RUNNING);

                actualizar_estado_pcb(pcb_deserializado, EXITT, lista_exit, m_list_EXIT);

                sem_post(sem_planificacion_largo);
                eliminar_proceso_de_memoria(&pcb_deserializado->pid);

                sem_post(sem_cpu_free_to_execute);
                log_info(logger_kernel, "Le hice un post al semaforo de que el cpu esta libre");

                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <OUT_OF_MEMORY>", pcb_deserializado->pid);
            }
            else if (cde->motivo_desalojo == TERMINAR_PROCESO)
            {
                log_info(logger_kernel, "Desaloje para finalizar el proceso.");

                sem_wait(sem_planificacion_largo);
                log_info(logger_kernel, "La cantidad de recursos_tomados por pid <%d> ES: <%d>", pcb_deserializado->pid, list_size(pcb_deserializado->recursos_tomados));
                liberar_recursos_tomados(pcb_deserializado);

                pthread_mutex_lock(&m_list_RUNNING);
                list_remove_and_destroy_element(lista_running, 0, pcb_destroyer);
                pthread_mutex_unlock(&m_list_RUNNING);

                actualizar_estado_pcb(pcb_deserializado, EXITT, lista_exit, m_list_EXIT);

                sem_post(sem_planificacion_largo);
                eliminar_proceso_de_memoria(&pcb_deserializado->pid);

                sem_post(sem_cpu_free_to_execute);
                log_info(logger_kernel, "Le hice un post al semaforo de que el cpu esta libre");

                log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <SUCCESS>", pcb_deserializado->pid);
            }
            else if (cde->motivo_desalojo == MOTIVO_SIGNAL || cde->motivo_desalojo == MOTIVO_WAIT)
            {
                char *recurso = malloc(string_length(cde->recurso) + 1);
                memcpy(recurso, cde->recurso, sizeof(char) * string_length(cde->recurso) + 1);
                log_info(logger_kernel, "recurso: %s", recurso);

                if (!recurso_existe(recurso))
                {
                    log_info(logger_kernel, "PID: <%d> - Finalizado porque el recurso <%s> no existe", pcb_deserializado->pid, recurso);
                    log_info(logger_obligatorio_kernel, "Finaliza el proceso <%d> - Motivo: <INVALID_RESOURCE> - Recurso: <%s>", pcb_deserializado->pid, recurso);
                    // pcb_deserializado->estado = EXIT;
                    pthread_mutex_lock(&m_list_RUNNING);
                    t_pcb *pcb_old = list_remove(lista_running, 0);
                    pcb_destroyer(pcb_old);
                    pthread_mutex_unlock(&m_list_RUNNING);
                    eliminar_proceso(pcb_deserializado);
                    sem_post(sem_cpu_free_to_execute);
                    free(recurso);
                }
                else if (cde->motivo_desalojo == MOTIVO_SIGNAL)
                {
                    recibir_contexto_ejecucion_motivo_signal(pcb_deserializado, recurso);
                }
                else
                {
                    recibir_contexto_ejecucion_motivo_wait(pcb_deserializado, recurso);
                }
                free(cde->recurso);
            }

            pcb_destroyer(cde->pcb_ejecutado);
            free(cde);
            free(buffer);
            free(tam_recibido);
            break;
        case -1:
            free(tam_recibido);
            return;
        default:
            free(tam_recibido);
            break;
        }
    }
}

void *conectarCPUDispatch(void *arg)
{
    t_config_kernel *config_kernel = (t_config_kernel *)arg;
    char *ip_cpu = config_kernel->ip_cpu;
    char *puerto_dispatch = string_itoa(config_kernel->puerto_cpu_dispatch);
    socket_cpu_dispatch = crear_conexion(ip_cpu, puerto_dispatch);
    handshake(socket_cpu_dispatch, 1, logger_kernel, "cpu dispatch");
    enviar_mensaje("conectado al kernel dispatch", socket_cpu_dispatch);
    free(puerto_dispatch);
    atender_cpu_dispatch();
    return "";
}

void *conectarCPUInterrupt(void *arg)
{
    t_config_kernel *config_kernel = (t_config_kernel *)arg;
    char *ip_cpu = config_kernel->ip_cpu;
    char *puerto_interrupt = string_itoa(config_kernel->puerto_cpu_interrupt);
    socket_cpu_interrupt = crear_conexion(ip_cpu, puerto_interrupt);
    handshake(socket_cpu_interrupt, 1, logger_kernel, "cpu interrupt");
    free(puerto_interrupt);
    enviar_mensaje("Conectado al Kernel interrupt", socket_cpu_interrupt);
    return "";
}

void *conectarMemoria(void *arg)
{
    t_config_kernel *config_kernel = (t_config_kernel *)arg;
    char *ip_memoria = config_kernel->ip_memoria;
    char *puerto_memoria = string_itoa(config_kernel->puerto_memoria);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
    handshake(socket_memoria, 1, logger_kernel, "memoria");
    enviar_mensaje("Conectado a kernel", socket_memoria);
    free(puerto_memoria);
    atender_memoria();
    return "";
}

void *atender_io(void *args)
{
    int *socket_cliente_io = (int *)args;

    while (1)
    {
        int size;
        void *buffer;
        int *tam_recibido = malloc(sizeof(int));
        t_interfaz *interfaz;
        t_interfaz_mje *interfaz_mje;
        t_io_pcb *io_pcb;
        t_pcb *pcb;
        int cod_op = recibir_operacion(*socket_cliente_io);
        switch (cod_op)
        {
        case ERROR_IO:
            log_info(logger_kernel, "me llego un error IO");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_error_io(buffer, tam_recibido);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            pthread_mutex_lock(&m_list_BLOCKED);
            int index = obtener_index(lista_blocked, interfaz_mje->pid);
            t_pcb *pcb_blocked = list_remove(lista_blocked, index);
            pthread_mutex_unlock(&m_list_BLOCKED);
            eliminar_proceso(pcb_blocked);
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_SLEEP:
            log_info(logger_kernel, "me llego un IO_SLEEP ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_sleep(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->udt = interfaz_mje->unidades_de_trabajo;
            io_pcb->dir_fis = NULL;
            io_pcb->reg = NULL;
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_READ:
            log_info(logger_kernel, "me llego un IO_READ ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_read(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->dir_fis = malloc(sizeof(t_direccion_fisica));
            io_pcb->dir_fis->desplazamiento_fisico = interfaz_mje->direccion_fisica->desplazamiento_fisico;
            io_pcb->dir_fis->marco = interfaz_mje->direccion_fisica->marco;
            io_pcb->dir_fis->pid = interfaz_mje->direccion_fisica->pid;
            t_registro_abstracto *reg_abs = (t_registro_abstracto *)interfaz_mje->reg;
            if (reg_abs->tipo == 1)
            {
                io_pcb->reg = malloc(sizeof(t_registro_8));
                memcpy(io_pcb->reg, interfaz_mje->reg, sizeof(t_registro_8));
            }
            else
            {
                io_pcb->reg = malloc(sizeof(t_registro_32));
                memcpy(io_pcb->reg, interfaz_mje->reg, sizeof(t_registro_32));
            }
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->reg);
            free(interfaz_mje->direccion_fisica);
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_WRITE:
            log_info(logger_kernel, "me llego un IO_WRITE ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_write(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->dir_fis = interfaz_mje->direccion_fisica;
            io_pcb->reg = interfaz_mje->reg;
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_FS_CREATE:
            log_info(logger_kernel, "me llego un IO_FS_CREATE ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_fs_create(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->nombre_archivo = interfaz_mje->nombre_archivo;
            io_pcb->tipo_operacion = 1;
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_FS_DELETE:
            log_info(logger_kernel, "me llego un IO_FS_DELETE ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_fs_delete(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->nombre_archivo = interfaz_mje->nombre_archivo;
            io_pcb->tipo_operacion = 2;
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_FS_TRUNCATE:
            log_info(logger_kernel, "me llego un IO_FS_TRUNCATE ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_fs_truncate(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->nombre_archivo = interfaz_mje->nombre_archivo;
            io_pcb->tamanio_truncar = interfaz_mje->tamanio_truncar;
            io_pcb->tipo_operacion = 3;
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_FS_WRITE:
            log_info(logger_kernel, "me llego un IO_FS_WRITE ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_fs_write(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->nombre_archivo = interfaz_mje->nombre_archivo;
            io_pcb->dir_fis = interfaz_mje->direccion_fisica;
            io_pcb->tamanio = interfaz_mje->tamanio;
            io_pcb->posicion_archivo = interfaz_mje->posicion_archivo;
            io_pcb->tipo_operacion = 4;
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case IO_OCUPADA_FS_READ:
            log_info(logger_kernel, "me llego un IO_FS_READ ocupada");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_io_ocupada_fs_read(buffer, tam_recibido);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            log_info(logger_kernel, "seleccione la interfaz: %s", interfaz->nombre_interfaz);
            pcb = obtener_pcb_con_pid_lista(interfaz_mje->pid, lista_blocked);
            io_pcb = malloc(sizeof(t_io_pcb));
            io_pcb->pcb = pcb;
            io_pcb->nombre_archivo = interfaz_mje->nombre_archivo;
            io_pcb->dir_fis = interfaz_mje->direccion_fisica;
            io_pcb->tamanio = interfaz_mje->tamanio;
            io_pcb->posicion_archivo = interfaz_mje->posicion_archivo;
            io_pcb->tipo_operacion = 5;
            list_add(interfaz->lista_bloqueados, io_pcb);
            log_info(logger_kernel, "TAMAÑO DE LA LISTA DE BLOQUEADOS EN INTERFAZ: %d", list_size(interfaz->lista_bloqueados));
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case FIN_IO:
            log_info(logger_kernel, "me llego un fin IO");
            buffer = recibir_buffer(&size, *socket_cliente_io);
            interfaz_mje = deserializar_fin_io(buffer, tam_recibido);
            log_info(logger_kernel, "me llego la interfaz: %s, pid: %d ", interfaz_mje->nombre_interfaz, interfaz_mje->pid);
            interfaz = dictionary_get(dict_interfaces, interfaz_mje->nombre_interfaz);

            desbloquear_proceso(interfaz_mje->pid);

            if (list_size(interfaz->lista_bloqueados) > 0)
            {
                t_io_pcb *io_pcb = list_remove(interfaz->lista_bloqueados, 0);
                if (*interfaz->tipo_interfaz == 1)
                {
                    enviar_io_gen_sleep_a_interfaz(interfaz_mje->nombre_interfaz, &io_pcb->udt, &io_pcb->pcb->pid);
                    free(interfaz_mje->nombre_interfaz);
                }
                else if (*interfaz->tipo_interfaz == 2)
                {
                    enviar_io_stdin_read_a_interfaz(io_pcb->dir_fis, interfaz_mje->nombre_interfaz, io_pcb->reg, &io_pcb->pcb->pid);
                }
                else if (*interfaz->tipo_interfaz == 3)
                {
                    enviar_io_stdout_write_a_interfaz(io_pcb->dir_fis, interfaz_mje->nombre_interfaz, io_pcb->reg, &io_pcb->pcb->pid);
                }
                else if (*interfaz->tipo_interfaz == 4)
                {
                    if (io_pcb->tipo_operacion == 1)
                    {
                        enviar_io_fs_create_a_interfaz(interfaz->nombre_interfaz, io_pcb->nombre_archivo, &io_pcb->pcb->pid);
                        free(io_pcb->nombre_archivo);
                        free(interfaz_mje->nombre_interfaz);
                    }
                    else if (io_pcb->tipo_operacion == 2)
                    {
                        enviar_io_fs_delete_a_interfaz(interfaz->nombre_interfaz, io_pcb->nombre_archivo, &io_pcb->pcb->pid);
                        free(io_pcb->nombre_archivo);
                        free(interfaz_mje->nombre_interfaz);
                    }
                    else if (io_pcb->tipo_operacion == 3)
                    {
                        enviar_io_fs_truncate_a_interfaz(interfaz->nombre_interfaz, io_pcb->nombre_archivo, io_pcb->tamanio_truncar, &io_pcb->pcb->pid);
                        free(io_pcb->nombre_archivo);
                        free(interfaz_mje->nombre_interfaz);
                    }
                    else if (io_pcb->tipo_operacion == 4)
                    {
                        enviar_io_fs_write_a_interfaz(io_pcb->dir_fis, interfaz->nombre_interfaz, io_pcb->nombre_archivo, io_pcb->tamanio, io_pcb->posicion_archivo, &io_pcb->pcb->pid);
                        free(io_pcb->nombre_archivo);
                        free(io_pcb->dir_fis);
                        free(interfaz_mje->nombre_interfaz);
                    }
                    else if (io_pcb->tipo_operacion == 5)
                    {
                        enviar_io_fs_read_a_interfaz(io_pcb->dir_fis, interfaz->nombre_interfaz, io_pcb->nombre_archivo, io_pcb->tamanio, io_pcb->posicion_archivo, &io_pcb->pcb->pid);
                        free(io_pcb->nombre_archivo);
                        free(io_pcb->dir_fis);
                        free(interfaz_mje->nombre_interfaz);
                    }
                }
                io_pcb_destroyer(io_pcb);
                free(buffer);
                free(tam_recibido);
                free(interfaz_mje);
                break;
            }
            free(interfaz_mje->nombre_interfaz);
            free(interfaz_mje);
            free(buffer);
            free(tam_recibido);
            break;
        case -1:
            free(tam_recibido);
            return "";
        default:
            free(tam_recibido);
            break;
        }
    }
}

void *mover_a_ready() {
    while(1)
    {
        sem_wait(sem_proceso_se_creo_en_memoria);

        while(1){

            sem_wait(sem_grado_multiprogamacion); //esta linea es la que frena
                pthread_mutex_lock(&m_list_auxiliar);
                pthread_mutex_lock(&m_list_READY);
                pthread_mutex_lock(&m_list_BLOCKED);
                pthread_mutex_lock(&m_list_RUNNING);

                int tam_lista_auxiliar = list_size(lista_auxiliar) > 0 ? list_size(lista_auxiliar) : 0;
                int tamanio_multiprogramacion = list_size(lista_ready) + list_size(lista_running) + list_size(lista_blocked);

                pthread_mutex_lock(&m_config_kernel_multiprogramacion);
                if (tamanio_multiprogramacion + tam_lista_auxiliar >= config_kernel->grado_multiprogramacion)
                {
                    pthread_mutex_unlock(&m_config_kernel_multiprogramacion);
                    pthread_mutex_unlock(&m_list_auxiliar);
                    pthread_mutex_unlock(&m_list_READY);
                    pthread_mutex_unlock(&m_list_BLOCKED);
                    pthread_mutex_unlock(&m_list_RUNNING);
                    //sem_post(sem_proceso_se_creo_en_memoria);
                    continue;
                }
                pthread_mutex_unlock(&m_config_kernel_multiprogramacion);
                pthread_mutex_unlock(&m_list_auxiliar);
                pthread_mutex_unlock(&m_list_READY);
                pthread_mutex_unlock(&m_list_BLOCKED);
                pthread_mutex_unlock(&m_list_RUNNING);
            sem_wait(sem_planificacion_largo);

            pthread_mutex_lock(&m_list_NEW);
            if(list_size(lista_new) > 0)
            {
                t_pcb *pcb = list_remove(lista_new, 0);
                pthread_mutex_unlock(&m_list_NEW); 
                actualizar_estado_pcb(pcb, READY, lista_ready, m_list_READY);
                sem_post(sem_process_in_ready);
            }
            sem_post(sem_planificacion_largo);
            break;
        }    
    }

  return "";
}

void *abrirSocketIO()
{
    while (1)
    {
        int *socket_a = malloc(sizeof(int));
        *socket_a = esperar_cliente(socket_kernel);

        uint32_t resultOk = 0;
        uint32_t resultError = -1;
        uint32_t respuesta;
        pthread_t conexiones_io;
        t_interfaz *interfaz;

        recv(*socket_a, &respuesta, sizeof(uint32_t), MSG_WAITALL);
        if (respuesta == 1)
            send(*socket_a, &resultOk, sizeof(uint32_t), 0);
        else
            send(*socket_a, &resultError, sizeof(uint32_t), 0);

        int cod_op = recibir_operacion(*socket_a);
        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(*socket_a, logger_kernel);
            break;
        }

        cod_op = recibir_operacion(*socket_a);
        switch (cod_op)
        {
        case HANDSHAKE_IO:
            int size;
            void *buffer;
            int *tam_recibido = malloc(sizeof(int));
            buffer = recibir_buffer(&size, *socket_a);
            t_handshake_io *handshake = deserializar_handshake_io(buffer, tam_recibido);
            interfaz = iniciar_interfaz(handshake->nombre, socket_a, &handshake->tipo_interfaz);
            free(buffer);
            free(handshake->nombre);
            free(handshake);
            free(tam_recibido);
            break;
        }

        // Asumo que los nombres de las interfaces son unicos y no deberia escribir 2 veces el mismo nombre.
        // Entiendo que si es asi el dictionary actualiza la key con el ultimo valor.
        // Tiene como warning que no libera la memoria del nodo reemplazado xD
        log_info(logger_kernel, "SE CONECTO LA INTERFAZ DE NOMBRE: %s", interfaz->nombre_interfaz);
        dictionary_put(dict_interfaces, interfaz->nombre_interfaz, interfaz);
        pthread_create(&conexiones_io, NULL, atender_io, (void *)interfaz->socket_cliente_io);
        pthread_detach(conexiones_io);
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
        uint32_t *pid;
        int cod_op = recibir_operacion(socket_memoria);
        switch (cod_op)
        {
        case FIN_CREA_PRO:
            // podriamos hacer esto en otro hilo si hay peligro de bloquear atender a memoria
            buffer = recibir_buffer(&size, socket_memoria);
            pid = deserealizar_recibir_int_proceso(buffer, tam_recibido);
            
            sem_post(sem_proceso_se_creo_en_memoria);

            free(pid);
            free(tam_recibido);
            free(buffer);
            break;
        case FIN_TERM_PRO:
            buffer = recibir_buffer(&size, socket_memoria);
            pid = deserealizar_recibir_int_proceso(buffer, tam_recibido);
            log_info(logger_kernel, "Finalice correctamente el PCB con pid: %d", *pid);
            incrementar_grado_multi();
            free(tam_recibido);
            free(pid);
            free(buffer);
            break;
        default:
            free(tam_recibido);
            break;
        case -1:
            free(tam_recibido);
            return;
        }
    }
}

const char *motivo_desalojo_to_string(motivo_desalojo motivo)
{
    switch (motivo)
    {
    case INTERRUPCION:
        return "INTERRUPCION";
    case TERMINAR_PROCESO:
        return "TERMINAR_PROCESO";
    case MOTIVO_WAIT:
        return "MOTIVO_WAIT";
    case MOTIVO_SIGNAL:
        return "MOTIVO_SIGNAL";
    case IO:
        return "IO";
    case OUT_OF_MEMORY_ERROR:
        return "OUT_OF_MEMORY_ERROR";
    default:
        log_info(logger_kernel, "motivo desconocido: %d", motivo);
        return "UNKNOWN_MOTIVO";
    }
}

void *recibir_contexto_ejecucion_motivo_signal(t_pcb *aux_pcb_ejecutado, char *recurso)
{
    log_info(logger_kernel, "recibir_contexto_ejecucion_motivo_signal(t_pcb *aux_pcb_ejecutado, char * recurso)");
    int instancias_restantes;
    recurso_incrementar(recurso, &instancias_restantes);

    if (list_size(aux_pcb_ejecutado->recursos_tomados) > 0)
    {
        int index = obtener_index_recurso(aux_pcb_ejecutado->recursos_tomados, recurso);
        if (index != -1)
        {
            list_remove_and_destroy_element(aux_pcb_ejecutado->recursos_tomados, index, free);
        }
    }

    free(recurso);
    pthread_mutex_lock(&m_list_RUNNING);
    list_remove_and_destroy_element(lista_running, 0, pcb_destroyer);
    list_add(lista_running, aux_pcb_ejecutado);
    pthread_mutex_unlock(&m_list_RUNNING);
    enviar_siguiente_pcb(aux_pcb_ejecutado, false);
    return "";
}

void *recibir_contexto_ejecucion_motivo_wait(t_pcb *aux_pcb_ejecutado, char *recurso)
{
    log_info(logger_kernel, "recibir_contexto_ejecucion_motivo_wait(t_pcb *aux_pcb_ejecutado, char * recurso)");
    int instancias_restantes;
    bool el_recurso_tenia_instancias_disponibles = recurso_decrementar(recurso, &instancias_restantes, aux_pcb_ejecutado);

    if (el_recurso_tenia_instancias_disponibles)
    {
        log_info(logger_kernel, "antes de agregar %s a la lista de recursos_tomados del proceso %d tiene %d elementos", recurso, aux_pcb_ejecutado->pid, list_size(aux_pcb_ejecutado->recursos_tomados));
        list_add(aux_pcb_ejecutado->recursos_tomados, recurso);
        log_info(logger_kernel, "despues de agregar %s a la lista de recursos_tomados del proceso %d tiene %d elementos", recurso, aux_pcb_ejecutado->pid, list_size(aux_pcb_ejecutado->recursos_tomados));
        pthread_mutex_lock(&m_list_RUNNING);
        list_remove_and_destroy_element(lista_running, 0, pcb_destroyer);
        list_add(lista_running, aux_pcb_ejecutado);
        pthread_mutex_unlock(&m_list_RUNNING);
        enviar_siguiente_pcb(aux_pcb_ejecutado, false);
    }

    return "";
}