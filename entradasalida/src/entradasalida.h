#ifndef ENTRADASALIDA_H_
#define ENTRADASALIDA_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>
#include <utils/cliente/cliente.h>
#include "pthread.h"
#include "readline/readline.h"
#include <unistd.h>
#include <utils/codec/deserializar.h>
#include <utils/codec/serializar.h>
#include <utils/servidor/servidor.h>
#include <utils/lector_archivo/file_reader.h>
#include <utils/estructura_de_datos_modulos.h>
#include <utils/config/config.h>
#include "semaphore.h"

extern int socket_kernel;
extern int socket_memoria;
extern int socket_io;
extern t_log *logger_io;
extern t_log *logger_obligatorio_io;
extern pthread_t conexion_kernel;
extern pthread_t conexion_memoria;
extern pthread_t ejecutar_instruccion;
extern pthread_mutex_t m_pid_usando;
extern sem_t *sem_esperar_respuesta;
extern uint32_t tipo_interfaz;
extern char *nombre_interfaz;
extern int pid_usando_interfaz;

extern t_bitarray *bitmap;
extern void *bloques;
extern t_dictionary *archivos;

typedef struct
{
    char *tipo_interfaz;
    int tiempo_unidad_trabajo;
    char *ip_kernel;
    int puerto_kernel;
    char *ip_memoria;
    int puerto_memoria;
    char *path_base_dialfs;
    int block_size;
    int block_count;
    int retraso_compactacion;
} t_config_io;

extern t_config_io *config_io;

typedef struct
{
    t_config_io *config;
    char *nombre_interfaz;

} t_args_conectar_kernel;

typedef struct
{
    t_config_io *config;
    char *nombre_interfaz;

} t_args_conectar_memoria;

typedef struct
{
    int bloque_inicial;
    int tam_archivo;
} t_archivo;

// -----------------------------------------------------------------------------------------
// -- commit --
// -----------------------------------------------------------------------------------------
int actualizar_bitmap();
int actualizar_bloques();
int actualizar_archivo(char *nombre_archivo);
void formatear_datos_archivo(char *nombre_archivo, char *datos, int *size);
int actualizar_archivos();
void eliminar_archivo_txt(char *nombre_archivo);

// -----------------------------------------------------------------------------------------
// -- bitmap --
// -----------------------------------------------------------------------------------------
int inicializar_bitmap();
void imprimir_bitmap(int posicion_inicial, int cantidad);
uint32_t asignar_bloque();
int bloque_libre();
void desasignar_bloque(uint32_t bloque);
int cantidad_de_bloques(int tamanio);
bool estan_contiguos(int bloque_inicial, int bloques_necesarios);

// -----------------------------------------------------------------------------------------
// -- bloques --
// -----------------------------------------------------------------------------------------
int inicializar_bloques();
int inicializar_archivos();

// -----------------------------------------------------------------------------------------
// -- compactacion --
// -----------------------------------------------------------------------------------------
int compactar(char *nombre_archivo);

// -----------------------------------------------------------------------------------------
// -- configs --
// -----------------------------------------------------------------------------------------
void iniciar_config(char *ruta_archivo);
void destruir_config(t_config_io *config_io);
t_config_io *leer_configs(void);
t_config_io *levantar_configs(char *ruta_archivo);
void loguear_configs(t_config_io *config_io);

void usarTiempoUnidadesTrabajo();

// -----------------------------------------------------------------------------------------
// -- manejo de archivos --
// -----------------------------------------------------------------------------------------
t_archivo *leer_config_archivo(char *path_archivo);

// -----------------------------------------------------------------------------------------
// -- operaciones FS--
// -----------------------------------------------------------------------------------------
void crearArchivo(uint32_t *pid, char *nombreArchivo);
void eliminarArchivo(uint32_t *pid, char *nombreArchivo);
void truncarArchivo(uint32_t *pid, char *nombreArchivo, uint32_t tamanio);

// -----------------------------------------------------------------------------------------
// -- instrucciones IO --
// -----------------------------------------------------------------------------------------
void *io_gen_sleep(void *args);
void *io_stdin_read(void *args);
void *io_stdout_write(void *args);
void *io_fs_create(void *args);
void *io_fs_delete(void *args);
void *io_fs_truncate(void *args);
void *io_fs_read(void *args);
void *io_fs_write(void *args);

#endif