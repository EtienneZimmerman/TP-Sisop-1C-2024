#ifndef DESERIALIZAR_H
#define DESERIALIZAR_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <utils/estructura_de_datos_modulos.h>

typedef struct
{
    uint32_t pid;
    uint32_t pc;
} t_obtener_instruccion;

typedef struct
{
    uint32_t id_inst;
    char *instruccion;
} t_obtener_ret_instruccion;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    uint32_t unidades_de_trabajo;
    char *interfaz;
} t_peticion_io_gen_sleep;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    t_direccion_fisica *dir_fisica;
    void *tamanio;
    char *interfaz;
} t_peticion_io_stdin_read;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    t_direccion_fisica *dir_fisica;
    uint32_t tamanio;
    char *interfaz;
} t_peticion_io_stdout_write;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    char *interfaz;
    char *nombre_archivo;

} t_peticion_io_fs_create;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    char *interfaz;
    char *nombre_archivo;

} t_peticion_io_fs_delete;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    char *interfaz;
    char *nombre_archivo;
    uint32_t tam;

} t_peticion_io_fs_truncate;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    char *interfaz;
    char *nombre_archivo;
    uint32_t tam;
    uint32_t posicion_archivo;
    t_direccion_fisica *dir_fis;
} t_peticion_io_fs_read;

typedef struct
{
    t_contexto_de_ejecucion *cde;
    char *interfaz;
    char *nombre_archivo;
    uint32_t tam;
    uint32_t posicion_archivo;
    t_direccion_fisica *dir_fis;
} t_peticion_io_fs_write;

typedef struct
{
    uint32_t unidades_de_trabajo;
    uint32_t pid;
} t_ejecutar_io_gen_sleep;

typedef struct
{
    t_direccion_fisica *dir_fisica;
    void *tamanio;
    uint32_t pid;
} t_ejecutar_io_stdin_read;

typedef struct
{
    t_direccion_fisica *dir_fisica;
    uint32_t tamanio;
    uint32_t pid;
} t_ejecutar_io_stdout_write;

typedef struct
{
    uint32_t tamanio;
    uint32_t pid;
    char *leido;
} t_ret_io_stdout_write;

typedef struct
{
    t_direccion_fisica *dir_fisica;
    void *tamanio;
    uint32_t pid;
    char *a_escribir;
} t_ejecutar_io_stdin;
typedef struct
{
    uint32_t pid;
    char *nombre_archivo;
} t_ejecutar_io_fs_create;

typedef struct
{
    uint32_t pid;
    char *nombre_archivo;
} t_ejecutar_io_fs_delete;

typedef struct
{
    uint32_t pid;
    char *nombre_archivo;
    uint32_t tamanio_a_truncar;
} t_ejecutar_io_fs_truncate;

typedef struct
{
    uint32_t pid;
    uint32_t tamanio;
    t_direccion_fisica *dir_fisica;
    char *nombre_archivo;
    uint32_t posicion_archivo;
} t_ejecutar_io_fs_write;

typedef struct
{
    uint32_t pid;
    uint32_t tamanio;
    t_direccion_fisica *dir_fisica;
    char *nombre_archivo;
    uint32_t posicion_archivo;
} t_ejecutar_io_fs_read;
typedef struct
{
    uint32_t pid;
    char *nombre_interfaz;
    uint32_t unidades_de_trabajo;
    t_direccion_fisica *direccion_fisica;
    void *reg;
    char *nombre_archivo;
    uint32_t tamanio_truncar;
    uint32_t tamanio;
    uint32_t posicion_archivo;
} t_interfaz_mje;
typedef struct
{
    char *nombre;
    uint32_t tipo_interfaz;
} t_handshake_io;

typedef struct
{
    uint32_t pid;
    char *ruta_archivo;
} t_crear_proceso;

typedef struct
{
    uint32_t pid;
    t_direccion_fisica *dir;
    uint32_t tamanio;
    void *leido;
} t_io_fs_read;

typedef struct
{
    uint32_t pid;
    uint32_t tamanio;
    t_direccion_fisica *dir_fisica;
    char *nombre_archivo;
    uint32_t posicion_archivo;
    void *leido;
} t_ret_io_fs_write;

uint32_t deserializar_uint32(void *stream, int *desplazamiento);
char *deserializar_char(void *stream, int *desplazamiento, int tamanio);
t_obtener_instruccion *deserializar_obtener_instruccion(void *stream, int *desplazamiento);
t_obtener_ret_instruccion *deserializar_obtener_ret_instruccion(void *stream, int *bytes_recibidos);
uint32_t *deserealizar_recibir_int_proceso(void *stream, int *bytes_recibidos);
t_pcb *deserializar_paquete_pcb(void *stream, int *bytes_recibidos);
t_contexto_de_ejecucion *deserializar_contexto_ejecucion(void *stream, int *bytes_recibidos);
t_peticion_io_gen_sleep *deserializar_obtener_paquete_peticion_io_gen_sleep(void *stream, int *bytes_recibidos);
t_crear_proceso *deserializar_nuevo_proceso_memoria(void *stream, int *bytes_recibidos);
uint32_t *deserializar_rta_handhsake_memoria(void *stream, int *bytes_recibidos);
t_dir_logica *deserializar_pedir_marco(void *stream, int *bytes_recibidos);

// --------------------------------------------------------------------
// -- IO --
// --------------------------------------------------------------------
t_ejecutar_io_gen_sleep *deserializar_ejecutar_io_gen_sleep(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_error_io(void *stream, int *bytes_recibidos);
t_handshake_io *deserializar_handshake_io(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_io_ocupada_sleep(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_io_ocupada_read(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_io_ocupada_write(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_fin_io(void *stream, int *bytes_recibidos);

t_interfaz_mje *deserializar_io_ocupada_fs_read(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_io_ocupada_fs_write(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_io_ocupada_fs_create(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_io_ocupada_fs_delete(void *stream, int *bytes_recibidos);
t_interfaz_mje *deserializar_io_ocupada_fs_truncate(void *stream, int *bytes_recibidos);

t_ejecutar_io_fs_create *deserializar_ejecutar_io_fs_create(void *stream, int *bytes_recibidos);
t_ejecutar_io_fs_delete *deserializar_ejecutar_io_fs_delete(void *stream, int *bytes_recibidos);
t_ejecutar_io_fs_truncate *deserializar_ejecutar_io_fs_truncate(void *stream, int *bytes_recibidos);
t_ejecutar_io_fs_write *deserializar_ejecutar_io_fs_write(void *stream, int *bytes_recibidos);
t_ejecutar_io_fs_read *deserializar_ejecutar_io_fs_read(void *stream, int *bytes_recibidos);

t_peticion_io_fs_create *deserializar_peticion_io_fs_create(void *stream, int *bytes_recibidos);
t_peticion_io_fs_delete *deserializar_peticion_io_fs_delete(void *stream, int *bytes_recibidos);
t_peticion_io_fs_truncate *deserializar_peticion_io_fs_truncate(void *stream, int *bytes_recibidos);
t_peticion_io_fs_write *deserializar_peticion_io_fs_write(void *stream, int *bytes_recibidos);
t_peticion_io_fs_read *deserializar_peticion_io_fs_read(void *stream, int *bytes_recibidos);

t_io_fs_read *deserializar_io_fs_read(void *stream, int *bytes_recibidos);
t_ret_io_fs_write *deserializar_ret_io_fs_write(void *stream, int *bytes_recibidos);

// --------------------------------------------------------------------
// -- Memoria --
//---------------------------------------------------------------------
t_mov_in *deserializar_ejecutar_mov_in(void *stream, int *bytes_recibidos);
t_mov_out *deserializar_ejecutar_mov_out(void *stream, int *bytes_recibidos);
t_copy_string *deserializar_ejecutar_copy_string(void *stream, int *bytes_recibidos);
t_registro_tlb *deserializar_responder_marco(void *stream, int *bytes_recibidos);
t_resize *deserializar_ejecutar_resize(void *stream, int *bytes_recibidos);
t_ejecutar_io_stdin *deserializar_io_stdin(void *stream, int *bytes_recibidos);
t_ejecutar_io_stdout_write *deserializar_io_stdout(void *stream, int *bytes_recibidos);

t_ejecutar_io_stdin_read *deserializar_ejecutar_stdin_read(void *stream, int *bytes_recibidos);
t_ejecutar_io_stdout_write *deserializar_ejecutar_stdout_write(void *stream, int *bytes_recibidos);

t_peticion_io_stdout_write *deserializar_obtener_paquete_peticion_io_stdout_write(void *stream, int *bytes_recibidos);
t_peticion_io_stdin_read *deserializar_obtener_paquete_peticion_io_stdin_read(void *stream, int *bytes_recibidos);
t_registro_tlb *deserializar_responder_marco(void *stream, int *bytes_recibidos);
t_ret_io_stdout_write *deserializar_ret_io_stdout_write(void *stream, int *bytes_recibidos);
uint32_t *deserializar_ret_io_stdin_read(void *stream, int *bytes_recibidos);

void *deserializar_rta_movin(void *stream, int *bytes_recibidos);

#endif