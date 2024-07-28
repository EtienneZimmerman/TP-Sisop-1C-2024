#ifndef SERIALIZAR_H
#define SERIALIZAR_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <utils/estructura_de_datos.h>
#include <utils/estructura_de_datos_modulos.h>

t_buffer *crear_buffer();
t_buffer *null_buffer();
t_paquete *crear_paquete(t_buffer *buffer, int codigo_operacion);
void agregar_var_a_paquete(t_paquete *paquete, const void *valor, const int tamanio);
void destruir_buffer(t_buffer *buffer);
t_paquete *obtener_paquete_instrucciones(uint32_t *pid, uint32_t *pc);
t_paquete *obtener_paquete_ret_instrucciones(uint32_t *id_inst, char *instrucciones);
t_paquete *serializar_contexto_ejecucion(t_pcb *pcb, motivo_desalojo motivo, char *recurso);
t_paquete *obtener_paquete_peticion_io_gen_sleep(char *interfaz, uint32_t *unidades_de_trabajo, t_pcb *pcb, uint32_t *motivo_desalojo);
t_paquete *serializar_paquete_pcb(t_pcb *pcb);

// --------------------------------------------------------------------
// -- CPU --
// --------------------------------------------------------------------
t_paquete *serializar_pedir_marco(t_dir_logica *dir);
t_paquete *obtener_paquete_ret_instrucciones(uint32_t *id_inst, char *instrucciones);
t_paquete *serializar_ejecutar_mov_in(t_mov_in *dir);
t_paquete *serializar_ejecutar_mov_out(t_mov_out *movout);
t_paquete *serializar_ejecutar_stdin_read(t_io_stdin_read *stdin_read, uint32_t *pid);
t_paquete *serializar_ejecutar_stdout_write(t_io_stdout_write *stdout_write, uint32_t *pid);
void serializar_t_registro(t_paquete *paquete, void *reg);
t_paquete *serializar_ejecutar_copy_string(t_copy_string *copy_string);

t_paquete *serializar_peticion_io_stdout_write(char *interfaz, t_pcb *pcb, uint32_t *motivo_desalojo, t_io_stdout_write *stdout_write, uint32_t *tam);
t_paquete *serializar_peticion_io_stdin_read(char *interfaz, t_pcb *pcb, uint32_t *motivo_desalojo, t_io_stdin_read *stdin_read);
t_paquete *serializar_ejecutar_resize(t_resize *resize);

// --------------------------------------------------------------------
// -- IO --
//---------------------------------------------------------------------
t_paquete *serializar_interrupcion(uint32_t *pid);
t_paquete *serializar_ejecutar_io_gen_sleep(uint32_t *unidades_de_trabajo, uint32_t *pid);
t_paquete *serializar_error_io(char *nombre_interfaz, uint32_t *pid);
t_paquete *serializar_handshake_io(char *nombre_interfaz, uint32_t *tipo_interfaz);
t_paquete *serializar_interfaz_ocupada_sleep(char *nombre_interfaz, uint32_t *pid, uint32_t *unidades_de_trabajo);
t_paquete *serializar_fin_io(char *nombre_interfaz, uint32_t *pid);
t_paquete *serializar_ejecutar_io_stdout_write(uint32_t *pid, t_direccion_fisica *dir, uint32_t tamanio);
t_paquete *serializar_ejecutar_io_stdin_read(uint32_t *pid, t_direccion_fisica *dir, void *tamanio);
t_paquete *serializar_interfaz_ocupada_write(char *nombre_interfaz, uint32_t *pid, t_direccion_fisica *dir, uint32_t tamanio);
t_paquete *serializar_interfaz_ocupada_read(char *nombre_interfaz, uint32_t *pid, t_direccion_fisica *dir, void *tamanio);
t_paquete *serializar_io_stdin(uint32_t *pid, t_direccion_fisica *dir, void *tamanio, char *a_escribir);
t_paquete *serializar_io_stdout(uint32_t *pid, t_direccion_fisica *dir, uint32_t tamanio);
t_paquete *serializar_ret_io_stdout(uint32_t *pid, uint32_t reg, char *leido);
t_paquete *serializar_ret_io_stdin(uint32_t *pid);

// -- DIALFS --

t_paquete *obtener_paquete_peticion_io_fs_create(char *interfaz, char *nombre_archivo, t_pcb *pcb, uint32_t *motivo_desalojo);
t_paquete *obtener_paquete_peticion_io_fs_truncate(char *interfaz, char *nombre_archivo, uint32_t tam, t_pcb *pcb, uint32_t *motivo_desalojo);
t_paquete *obtener_paquete_peticion_io_fs_delete(char *interfaz, char *nombre_archivo, t_pcb *pcb, uint32_t *motivo_desalojo);
t_paquete *obtener_paquete_peticion_io_fs_read(char *interfaz, char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, t_pcb *pcb, uint32_t *motivo_desalojo);
t_paquete *obtener_paquete_peticion_io_fs_write(char *interfaz, char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, t_pcb *pcb, uint32_t *motivo_desalojo);

t_paquete *serializar_ejecutar_io_fs_create(uint32_t pid, char *nombre_archivo);
t_paquete *serializar_ejecutar_io_fs_delete(uint32_t pid, char *nombre_archivo);
t_paquete *serializar_ejecutar_io_fs_truncate(uint32_t pid, char *nombre_archivo, uint32_t tamanio);
t_paquete *serializar_ejecutar_io_fs_read(char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, uint32_t pid);
t_paquete *serializar_ejecutar_io_fs_write(char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, uint32_t pid);

// -- DIALFS (IO - MEMORIA) --
t_paquete *serializar_io_fs_read(uint32_t pid, t_direccion_fisica *dir, uint32_t tamanio, void *leido);
t_paquete *serializar_ret_io_fs_read(uint32_t pid);
void serializar_ret_io_fs_write(t_paquete *paquete, uint32_t tamanio, void *leido);

t_paquete *serializar_interfaz_ocupada_fs_create(char *nombre_interfaz, uint32_t pid, char *nombre_archivo);
t_paquete *serializar_interfaz_ocupada_fs_delete(char *nombre_interfaz, uint32_t pid, char *nombre_archivo);
t_paquete *serializar_interfaz_ocupada_fs_truncate(char *nombre_interfaz, uint32_t pid, char *nombre_archivo, uint32_t tamanio);
t_paquete *serializar_interfaz_ocupada_fs_write(char *nombre_interfaz, uint32_t pid, char *nombre_archivo, t_direccion_fisica *direccion_fisica, uint32_t tamanio, uint32_t posicion_archivo);
t_paquete *serializar_interfaz_ocupada_fs_read(char *nombre_interfaz, uint32_t pid, char *nombre_archivo, t_direccion_fisica *direccion_fisica, uint32_t tamanio, uint32_t posicion_archivo);

// --------------------------------------------------------------------
// -- KERNEL --
//---------------------------------------------------------------------
t_paquete *serializar_interrupcion_exit(uint32_t *pid);
t_paquete *serializar_nuevo_proceso_memoria(uint32_t *pid, char *ruta_archivo);
t_paquete *serializar_eliminar_proceso_memoria(uint32_t *pid);
t_paquete *serializar_handshake_memoria();

// --------------------------------------------------------------------
// -- MEMORIA --
// --------------------------------------------------------------------
t_paquete *serializar_fin_terminacion(uint32_t *pid);
t_paquete *serializar_fin_creacion(uint32_t *pid);
t_paquete *serializar_respuesta_handshake_memoria(int *tam_pagina);
t_paquete *serializar_respnder_marco(t_registro_tlb *reg_tlb);
t_paquete *serializar_rta_movin(void *reg);

#endif