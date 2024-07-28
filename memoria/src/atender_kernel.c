#include "main.h"

int atender_cliente_kernel()
{
    int cod_op = recibir_operacion(socket_kernel_cliente);
    log_info(logger_memoria, "recibi el cod_op de kernel: %s", cod_op_to_string(cod_op));
    switch (cod_op)
    {
    case MENSAJE:
        recibir_mensaje(socket_kernel_cliente, logger_memoria);
        break;
    case CREAR_PROCESO:
        log_info(logger_memoria, "recibi CREAR PROCESO");
        recibir_crear_proceso(socket_kernel_cliente);
        break;
    case FINALIZAR_PROCESO_MEMORIA:
        recibir_finalizar_proceso(socket_kernel_cliente);
        break;
    case -1:
        return 1;
    default:
        return 1;
    }

    return 0;
}

void recibir_crear_proceso(int socket_cliente)
{
    int size;
    void *buffer;
    int *tam_recibido = malloc(sizeof(int));
    buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger_memoria, "recibiendo buffer");
    t_crear_proceso *new_process = deserializar_nuevo_proceso_memoria(buffer, tam_recibido);
    crear_proceso(new_process->ruta_archivo, new_process->pid);
    avisar_kernel_fin_creacion(new_process->pid, socket_cliente);
    free(tam_recibido);
    free(buffer);
    free(new_process->ruta_archivo);
    free(new_process);
}

void recibir_finalizar_proceso(int socket_cliente)
{
    int size;
    void *buffer;
    int *tam_recibido = malloc(sizeof(int));
    buffer = recibir_buffer(&size, socket_cliente);
    uint32_t *pid = deserealizar_recibir_int_proceso(buffer, tam_recibido);
    eliminar_proceso(*pid);
    avisar_kernel_fin_terminacion_proceso(*pid, socket_cliente);
    free(tam_recibido);
    free(buffer);
    free(pid);
}

void avisar_kernel_fin_creacion(uint32_t pid, int socket_cliente)
{
    t_paquete *paquete = serializar_fin_creacion(&pid);
    enviar_paquete(paquete, socket_cliente, logger_memoria);
    log_info(logger_memoria, "ENVIE FIN CREACION");
}

void avisar_kernel_fin_terminacion_proceso(uint32_t pid, int socket_cliente)
{
    t_paquete *paquete = serializar_fin_terminacion(&pid);
    enviar_paquete(paquete, socket_cliente, logger_memoria);
    log_info(logger_memoria, "ENVIE FIN TERMINACION");
}
