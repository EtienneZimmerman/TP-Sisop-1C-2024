#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/servidor/servidor.h>
#include <utils/cliente/cliente.h>
#include <commons/string.h>
#include <commons/log.h>
#include <config/config_cpu.h>
#include "pthread.h"
#include <utils/codec/serializar.h>
#include <utils/codec/deserializar.h>
#include <utils/estructura_de_datos_modulos.h>
#include "readline/readline.h"
#include <commons/temporal.h>
#include "semaphore.h"
#include <utils/instruccion/instruccion.h>
#include <commons/temporal.h>
#include <utils/destroyer/destroyer.h>

// --------------------------------------------------------------------
// -- Variables globales --
//---------------------------------------------------------------------
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_kernel_interrupt;
extern int socket_kernel_dispatch;
extern int socket_memoria;
extern t_log *logger_cpu;
extern t_log *logger_obligatorio_cpu;
extern t_pcb *proceso_ejecutando;
extern sem_t *sem_esperar_respuesta;
extern t_registros *registros;
extern uint32_t *tam_pagina;
extern t_list *tlb;
extern t_config_cpu *config_cpu;
extern t_temporal *clock_;
extern void *rta_memoria;
extern sem_t *mutex_proceso_ejecutando;
extern sem_t *mutex_registros;
extern t_list *interrupciones;
extern pthread_mutex_t m_interrupciones;
extern pthread_mutex_t m_tiempo_inicial;
extern t_temporal *clockcpu; // necesitamos un mutex para el clock????
extern int64_t tiempo_inicial;
// --------------------------------------------------------------------
// -- Registros --
//---------------------------------------------------------------------
void mostrar_registros(t_registros *registros);
void iniciar_registros();
void asignar_registros(t_registros *registros, t_registros *news);

// --------------------------------------------------------------------
// -- Operaciones sincronizadas --
//---------------------------------------------------------------------
void incrementar_pc(t_registros *registros);
void escribir_pcb(t_pcb *pcb, t_registros *registros);

// --------------------------------------------------------------------
// -- Ciclo de instruccion --
//---------------------------------------------------------------------
void fetch(uint32_t *pid, uint32_t *pc);
void *fetch_en_new_thread();
void execute(t_obtener_ret_instruccion *obt_ret_inst);
void ejecutar_mov_in(char *registro_datos, t_direccion_fisica *direccion_fisica);
void ejecutar_mov_out(char *registro_datos, t_direccion_fisica *direccion_fisica);
void ejecutar_copy_string(char *tamanio, t_direccion_fisica *di, t_direccion_fisica *si);
void ejecutar_io_stdin_read(char *interfaz, t_direccion_fisica *direccion_fisica, char *registro_tamaño);
void ejecutar_io_stdout_write(char *interfaz, t_direccion_fisica *direccion_fisica, char *registro_tamaño);
void ejecutar_io_fs_write(char *interfaz, char *nombre_archivo, t_direccion_fisica *dir,
                          char *registro_tamaño, char *registro_puntero_arch);
void ejecutar_io_fs_read(char *interfaz, char *nombre_archivo, t_direccion_fisica *dir,
                         char *registro_tamaño, char *registro_puntero_arch);
void *decode(void *obt_ret_inst_void);
char *traducir_direccion(char *direccion_logica);
int enviar_cde(uint32_t *pid, char *recurso, motivo_desalojo motivo);
int finalizar_proceso(uint32_t *pid);
t_direccion_fisica *traducir_direccion_a_fisica(uint32_t pid, char *registro_direccion);
void *obtener_t_registro(char *registro_datos);
void ejecutar_resize(uint32_t tamanio);

// --------------------------------------------------------------------
// -- Operaciones con sockets --
//---------------------------------------------------------------------
void recibir_pcb_nuevo();
void *abrirSocketKernelDispatch();
void *abrirSocketKernelInterrupt();
void atender_memoria();
void *conectarMemoria(void *arg);
int recibir_acceso_tabla_paginas(int socket_cliente);

// --------------------------------------------------------------------
// -- MMU --
//---------------------------------------------------------------------
void iniciar_tlb();
void limpiar_tlb(uint32_t pid);
#endif