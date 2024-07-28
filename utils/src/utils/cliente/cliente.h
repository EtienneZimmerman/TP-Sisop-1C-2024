#ifndef CLIENTE_H
#define CLIENTE_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include "../estructura_de_datos.h"
#include <utils/estructura_de_datos_modulos.h>

int crear_conexion(char *ip, char *puerto);
void enviar_paquete(t_paquete *paquete, int socket_cliente, t_log *logger);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void liberar_conexion(int socket_cliente);
uint32_t handshake(int conexion, uint32_t envio, t_log *logger, char *modulo);
void enviar_mensaje(char *mensaje, int socket_cliente);
void eliminar_paquete(t_paquete *paquete);
#endif