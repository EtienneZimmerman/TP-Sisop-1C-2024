#include "main.h"

uint32_t convertir_direccion(uint32_t marco, uint32_t direccion)
{
    return marco * config_memoria->tam_pagina + direccion;
}

void *atender_cliente_io(void *args)
{
    int *socket_io_cliente = (int *)args;
    while (1)
    {
        int size;
        void *buffer;
        int tam = 0;
        int cod_op = recibir_operacion(*socket_io_cliente);
        log_info(logger_memoria, "recibi el cod_op de IO: %s", cod_op_to_string(cod_op));
        switch (cod_op)
        {
        case MENSAJE:
            recibir_mensaje(*socket_io_cliente, logger_memoria);
            break;
        case EJECUTAR_IO_STDIN_READ:
            log_info(logger_memoria, "LLEGUE A EJECUTUAR_IO_STD_READ");
            buffer = recibir_buffer(&size, *socket_io_cliente);
            t_ejecutar_io_stdin *stdin_ = deserializar_io_stdin(buffer, &tam);
            log_info(logger_memoria, "Recibi: PID: %d, desplazamiento: %d, marco: %d, a escribir: %s", stdin_->pid, stdin_->dir_fisica->desplazamiento_fisico, stdin_->dir_fisica->marco, stdin_->a_escribir);
            // falta validar el funcionamiento con memoria!
            agregar_datos_a_memoria(stdin_->a_escribir, convertir_direccion(stdin_->dir_fisica->marco, stdin_->dir_fisica->desplazamiento_fisico), strlen(stdin_->a_escribir), stdin_->pid);
            // devolver a IO el OK
            t_paquete *paq = serializar_ret_io_stdin(&stdin_->pid);
            enviar_paquete(paq, *socket_io_cliente, logger_memoria);
            free(stdin_->a_escribir);
            free(stdin_->dir_fisica);
            free(stdin_->tamanio);
            free(stdin_);
            free(buffer);
            break;
        case EJECUTAR_IO_STDOUT_WRITE:
            log_info(logger_memoria, "LLEGUE A EJECUTUAR_IO_STD_WRITE");
            buffer = recibir_buffer(&size, *socket_io_cliente);
            t_ejecutar_io_stdout_write *stdout_ = deserializar_io_stdout(buffer, &tam);
            log_info(logger_memoria, "Recibi: PID: %d, desplazamiento: %d, marco: %d, tamanio: %u", stdout_->pid, stdout_->dir_fisica->desplazamiento_fisico, stdout_->dir_fisica->marco, stdout_->tamanio);
            // falta validar el funcionamiento con memoria!
            char *leido;
            leido = calloc(stdout_->tamanio + 1, sizeof(char));
            obtener_datos_de_memoria(leido, convertir_direccion(stdout_->dir_fisica->marco, stdout_->dir_fisica->desplazamiento_fisico), stdout_->tamanio, stdout_->pid);
            log_info(logger_memoria, "Lei el dato : %s", leido);
            t_paquete *paque = serializar_ret_io_stdout(&stdout_->pid, stdout_->tamanio, leido);
            enviar_paquete(paque, *socket_io_cliente, logger_memoria);
            free(stdout_->dir_fisica);
            free(leido);
            free(stdout_);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_READ:
            log_info(logger_memoria, "LLEGUE A EJECUTUAR_IO_FS_READ");
            buffer = recibir_buffer(&size, *socket_io_cliente);
            t_io_fs_read *fs_read = deserializar_io_fs_read(buffer, &tam);
            // falta validar el funcionamiento con memoria!
            agregar_datos_a_memoria(fs_read->leido, convertir_direccion(fs_read->dir->marco, fs_read->dir->desplazamiento_fisico), fs_read->tamanio, fs_read->pid);
            t_paquete *paquete = serializar_ret_io_fs_read(fs_read->pid);
            enviar_paquete(paquete, *socket_io_cliente, logger_memoria);
            free(fs_read->dir);
            free(fs_read->leido);
            free(fs_read);
            free(buffer);
            break;
        case EJECUTAR_IO_FS_WRITE:
            log_info(logger_memoria, "LLEGUE A EJECUTUAR_IO_FS_WRITE");
            buffer = recibir_buffer(&size, *socket_io_cliente);
            t_ejecutar_io_fs_write *fs_write = deserializar_ejecutar_io_fs_write(buffer, &tam);
            log_info(logger_memoria, "Recibi: PID: %d, desplazamiento: %d, marco: %d, tamanio: %u", fs_write->pid, fs_write->dir_fisica->desplazamiento_fisico, fs_write->dir_fisica->marco, fs_write->tamanio);
            // falta validar el funcionamiento con memoria!
            void *leido_fs;
            leido_fs = calloc(fs_write->tamanio + 1, sizeof(char));
            obtener_datos_de_memoria(leido_fs, convertir_direccion(fs_write->dir_fisica->marco, fs_write->dir_fisica->desplazamiento_fisico), fs_write->tamanio, fs_write->pid);
            log_info(logger_memoria, "Lei el dato : %s", (char *)leido_fs);
            // Armo el nuevo paquete para enviar a IO
            t_paquete *paquete_fs = serializar_ejecutar_io_fs_write(fs_write->nombre_archivo, fs_write->tamanio, fs_write->posicion_archivo, fs_write->dir_fisica, fs_write->pid);
            serializar_ret_io_fs_write(paquete_fs, fs_write->tamanio, leido_fs);
            enviar_paquete(paquete_fs, *socket_io_cliente, logger_memoria);
            free(fs_write->dir_fisica);
            free(fs_write->nombre_archivo);
            free(leido_fs);
            free(fs_write);
            free(buffer);
            break;
        default:
            log_info(logger_memoria, "Llego un cod op no reconocido");
            free(socket_io_cliente);
            return "";
            break;
        }
    }
    free(socket_io_cliente);
    return "";
}
