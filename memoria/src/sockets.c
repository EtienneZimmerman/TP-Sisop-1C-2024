#include "main.h"

void *abrirSocketKernel()
{
    int ok = 0;
    while (1)
    {
        log_info(logger_memoria, "Esperando Kernel");
        ok = atender_cliente_kernel();
        if (ok != 0)
        {
            break;
        }
    }
    return "";
}

void *abrirSocketCpu()
{
    int ok = 0;
    while (1)
    {
        log_info(logger_memoria, "Esperando CPU");
        ok = atender_cliente_cpu();
        if (ok != 0)
        {
            log_error(logger_memoria, "Error al atender cliente CPU");
            return "";
        }
    }
    return "";
}

void *abrirSockets()
{
    while (1)
    {
        pthread_t hilo_aux;
        uint32_t respuesta;
        uint32_t resultOk = 0;
        uint32_t resultError = -1;

        int *socket = malloc(sizeof(int));
        *socket = esperar_cliente(*socket_memoria);
        recv(*socket, &respuesta, sizeof(uint32_t), MSG_WAITALL);
        if (respuesta == 1) // Kernel
        {
            log_info(logger_memoria, "Creando el hilo de kernel");
            send(*socket, &resultOk, sizeof(uint32_t), 0);
            socket_kernel_cliente = *socket;
            free(socket);
            pthread_create(&hilo_aux, NULL, abrirSocketKernel, NULL);
            pthread_detach(hilo_aux);
        }
        else if (respuesta == 2) // CPU
        {
            log_info(logger_memoria, "Creando el hilo de cpu");
            send(*socket, &resultOk, sizeof(uint32_t), 0);
            socket_cpu_cliente = *socket;
            free(socket);
            pthread_create(&hilo_aux, NULL, abrirSocketCpu, NULL);
            pthread_detach(hilo_aux);
        }
        else if (respuesta == 3) // Entrada salida
        {
            log_info(logger_memoria, "Creando el hilo de IO");
            send(*socket, &resultOk, sizeof(uint32_t), 0);
            pthread_t thread_atender_io;
            pthread_create(&thread_atender_io, NULL, atender_cliente_io, (void *)socket);
            pthread_detach(thread_atender_io);
        }
        else
        {
            send(*socket, &resultError, sizeof(uint32_t), 0);
            free(socket);
        }
    }
}
