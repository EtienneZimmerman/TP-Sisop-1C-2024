#include "main.h"

t_config_memoria *config_memoria;
t_log *logger_memoria;
t_log *logger_obligatorio_memoria;
t_dictionary *dict_instrucciones;
pthread_mutex_t m_dict;
void *memoria;
t_bitarray *marcos;
int cantidad_marcos_libres;
t_dictionary *tabla_de_paginas;
int cantidad_de_paginas;
int socket_kernel_cliente;
int socket_cpu_cliente;
int *socket_memoria;

void log_element(void *element)
{
    log_info(logger_memoria, "%s", (char *)element);
}

int main(int argc, char *argv[])
{
    crear_estructuras_memoria(argv[1]);
    socket_memoria = malloc(sizeof(int));
    char *puerto = string_itoa(config_memoria->puerto_escucha);
    *socket_memoria = iniciar_servidor(puerto, NULL, logger_memoria);
    if (*socket_memoria == -1)
    {
        log_info(logger_memoria, "Error al iniciar servidor con IO");
        return -1;
    }
    free(puerto);
    log_info(logger_memoria, "Servidor memoria levantado");
    pthread_t abrir_sockets;
    pthread_create(&abrir_sockets, NULL, abrirSockets, NULL);
    pthread_detach(abrir_sockets);

    log_info(logger_memoria, "Esperando...");
    readline("bloqueo el main. Enter para continuar.\n");
    destruir_estructuras_memoria();
    return 0;
}