#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <utils/servidor/servidor.h>
#include <utils/cliente/cliente.h>
#include <config/config_memoria.h>
#include "pthread.h"
#include <utils/codec/deserializar.h>
#include <utils/codec/serializar.h>
#include <utils/lector_archivo/file_reader.h>
#include "readline/readline.h"
#include <utils/instruccion/instruccion.h>
#include <utils/destroyer/destroyer.h>
#include <utils/estructura_de_datos.h>
#include <utils/estructura_de_datos_modulos.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>

// ------------------------------------------------------------------------------------------
// -- Variables globales --
// ------------------------------------------------------------------------------------------
extern t_dictionary *dict_instrucciones;
extern void *memoria;

extern t_bitarray *marcos;
extern int cantidad_marcos_libres;
extern t_dictionary *tabla_de_paginas;
extern int cantidad_de_paginas;

extern int socket_kernel_cliente;
extern int socket_cpu_cliente;
extern int *socket_memoria;

extern t_config_memoria *config_memoria;
extern t_log *logger_memoria;
extern t_log *logger_obligatorio_memoria;
extern pthread_mutex_t m_dict;

// ------------------------------------------------------------------------------------------
// -- Paginación --
// ------------------------------------------------------------------------------------------
int obtener_marco_libre();
void ocupar_marco(int marco);
void liberar_marco(int marco);
void liberar_marcos_de_proceso(int pid);
void crear_estructuras_memoria(char *path);
void destruir_estructuras_memoria();
int acceder_a_tabla_de_paginas(int pid, int pagina);

// ------------------------------------------------------------------------------------------
// -- Operaciones --
// ------------------------------------------------------------------------------------------
int resize_proceso(int pid, int cantidad_de_bytes);

void crear_proceso(char *nombre_archivo_de_instrucciones, uint32_t pid);

void eliminar_proceso(uint32_t pid);

void agregar_datos_contiguos_a_memoria(void *datos, int direccion, int tamanio);
void obtener_datos_contiguos_de_memoria(void *buffer, int direccion, int tamanio);
int obtener_marco_de_direccion(int direccion);
int obtener_pagina_de_marco(t_list *paginas_del_proceso, int marco);
int obtener_tamanio_hasta_llenar_pagina(int direccion);
void obtener_datos_de_memoria(void *buffer, int direccion, int tamanio, int pid);
void agregar_datos_a_memoria(void *datos, int direccion, int tamanio, int pid);
void copy_string(int di, int si, int tamanio, int pid);

// ------------------------------------------------------------------------------------------
// -- Main --
// ------------------------------------------------------------------------------------------
void log_element(void *element);
void proceso_de_prueba();

// ------------------------------------------------------------------------------------------
// -- Sockets --
// ------------------------------------------------------------------------------------------
void *abrirSockets();

// ------------------------------------------------------------------------------------------
// -- CPU --
// ------------------------------------------------------------------------------------------
int atender_cliente_cpu();

// ------------------------------------------------------------------------------------------
// -- IO --
// ------------------------------------------------------------------------------------------
void *atender_cliente_io(void *args);
// ------------------------------------------------------------------------------------------
// -- Kernel --
// ------------------------------------------------------------------------------------------
int atender_cliente_kernel();
void recibir_crear_proceso(int socket_cliente);
void recibir_finalizar_proceso(int socket_cliente);
void avisar_kernel_fin_creacion(uint32_t pid, int socket_cliente);
void avisar_kernel_fin_terminacion_proceso(uint32_t pid, int socket_cliente);

#endif