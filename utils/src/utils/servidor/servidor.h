#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include "../estructura_de_datos.h"

int iniciar_servidor(char *puerto, char *ip, t_log *logger);
int esperar_cliente(int socket_servidor);
t_paquete *recibir_paquete(int socket, t_log *logger);
void recibir_mensaje(int socket_cliente, t_log *logger);
int recibir_operacion(int socket_cliente);
void *recibir_buffer(int *size, int socket_cliente);
void cerrar_servidor(int socket);
const char *cod_op_to_string(op_code cod_op);

#endif