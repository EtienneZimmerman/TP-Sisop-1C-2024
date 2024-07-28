#ifndef MAIN_H_
#define MAIN_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <config/config_kernel.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/process.h>
#include <semaphore.h>
#include <utils/cliente/cliente.h>
#include <utils/servidor/servidor.h>
#include <utils/estructura_de_datos_modulos.h>
#include "readline/readline.h"
#include <utils/codec/deserializar.h>
#include <utils/codec/serializar.h>
#include <commons/temporal.h>
#include "readline/history.h"
#include <utils/lector_archivo/file_reader.h>
#include <utils/destroyer/destroyer.h>

// ------------------------------------------------------------------------------------------
// -- TADs proceso --
// ------------------------------------------------------------------------------------------
typedef struct
{
    t_pcb *pcb;
    uint32_t udt;
    t_direccion_fisica *dir_fis;
    void *reg;
    char *nombre_archivo;
    uint32_t tamanio_truncar;
    uint32_t tamanio;
    uint32_t posicion_archivo;
    uint32_t tipo_operacion;
} t_io_pcb;

// ------------------------------------------------------------------------------------------
// -- Logger del proceso --
// ------------------------------------------------------------------------------------------

extern t_log *logger_kernel;
extern t_log *logger_obligatorio_kernel;

// ------------------------------------------------------------------------------------------
// -- Config del proceso --
// ------------------------------------------------------------------------------------------

extern char *algoritmo_planificacion;
extern t_config_kernel *config_kernel;
extern uint32_t pid_ejecutando;
extern t_dictionary *dict_interfaces;
// ------------------------------------------------------------------------------------------
// -- Socket del proceso --
// ------------------------------------------------------------------------------------------

extern int socket_kernel;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_memoria;
extern int socket_io_cliente;

extern pthread_t planificador_corto_plazo;

// ------------------------------------------------------------------------------------------
// -- Estructuras Planificacion --
// ------------------------------------------------------------------------------------------

extern t_list *lista_new;
extern t_list *lista_ready;
extern t_list *lista_blocked;
extern t_list *lista_running;
extern t_list *lista_exit;
extern t_list *lista_auxiliar;
extern t_pcb *pcb_ejecutando;
extern t_pcb *pcb_a_ejecutar;

extern t_temporal *clock_global;
extern t_list *estados_procesos;
extern t_list *m_estados_procesos;
// ------------------------------------------------------------------------------------------
// -- Semaforos --
// ------------------------------------------------------------------------------------------

extern sem_t *sem_grado_multiprogamacion;
extern sem_t *sem_process_in_ready;
extern sem_t *sem_cpu_free_to_execute;
extern sem_t *sem_beggin_quantum;
extern sem_t *sem_interruption_quantum;
extern sem_t *sem_planificacion;
extern sem_t *sem_planificacion_largo;
extern sem_t *sem_proceso_se_creo_en_memoria;

extern pthread_mutex_t m_list_NEW;
extern pthread_mutex_t m_list_READY;
extern pthread_mutex_t m_list_RUNNING;
extern pthread_mutex_t m_list_BLOCKED;
extern pthread_mutex_t m_list_EXIT;
extern pthread_mutex_t m_socket_memoria;
extern pthread_mutex_t m_list_auxiliar;
extern pthread_mutex_t m_pid_ejecutando;
extern pthread_mutex_t m_config_kernel_multiprogramacion;

// ------------------------------------------------------------------------------------------
// -- Funciones del proceso --
// -----------------------------------------------------------------------------------------
typedef struct
{
    t_config_kernel *config;
    int64_t *quantum;
} t_parametros_VRR;

t_config_kernel *levantar_configs(char *ruta_archivo);

// ------------------------------------------------------------------------------------------
// -- LOGS --
// -----------------------------------------------------------------------------------------
void loguear_configs(t_config_kernel *config_kernel);
void log_pcb(void *element);
void loggear_pcb(t_log *logger, t_pcb *pcb);
void loggear_registros(t_log *logger, t_registros *registros);
void listar_procesos_en_lista(t_list *list);
void listar_proceso_exec();

// ------------------------------------------------------------------------------------------
// -- SOCKETS --
// -----------------------------------------------------------------------------------------
void *conectarCPUDispatch(void *arg);
void *conectarCPUInterrupt(void *arg);
void *conectarMemoria(void *arg);
void *abrirSocketIO();
void *atender_io(void *args);
void enviar_siguiente_pcb(t_pcb *pcb, bool viene_de_planificacion);
void atender_cpu_dispatch();
int *iniciar_server_kernel(int puerto, t_log *logger);
void destruir_server_kernel(int *server_kernel);
void atender_memoria();

// ------------------------------------------------------------------------------------------
// -- Creadores --
// -----------------------------------------------------------------------------------------
void init_estructuras_planificacion();
void init_planificadores(t_config_kernel *config_kernel);
void init_semaphores(t_config_kernel *config_kernel);
void init_mutex();
void init_semaforos_recursos(int dim);
void init_semaforos_procesos();
void init_clock_global();

// ------------------------------------------------------------------------------------------
// -- IO --
// -----------------------------------------------------------------------------------------
t_interfaz *iniciar_interfaz(char *nombre, int *socket, uint32_t *tipo);
void enviar_io_gen_sleep_a_interfaz(char *nombre_interfaz, uint32_t *unidades_de_trabajo, uint32_t *pid);
void enviar_io_stdin_read_a_interfaz(t_direccion_fisica *dir_fis, char *interfaz, void *tamanio, uint32_t *pid);
void enviar_io_stdout_write_a_interfaz(t_direccion_fisica *dir_fis, char *interfaz, uint32_t *tamanio, uint32_t *pid);

void enviar_io_fs_write_a_interfaz(t_direccion_fisica *dir, char *interfaz, char *nombre_archivo, uint32_t tamanio, uint32_t posicion_archivo, uint32_t *pid);
void enviar_io_fs_read_a_interfaz(t_direccion_fisica *dir, char *interfaz, char *nombre_archivo, uint32_t tamanio, uint32_t posicion_archivo, uint32_t *pid);
void enviar_io_fs_truncate_a_interfaz(char *interfaz, char *nombre_archivo, uint32_t tamanio, uint32_t *pid);
void enviar_io_fs_create_a_interfaz(char *interfaz, char *nombre_archivo, uint32_t *pid);
void enviar_io_fs_delete_a_interfaz(char *interfaz, char *nombre_archivo, uint32_t *pid);
// ------------------------------------------------------------------------------------------
// -- Planificacion Corto --
// -----------------------------------------------------------------------------------------
void *planificadorCortoPlazo(void *arg);
t_pcb *obtener_pcb_con_pid_lista(uint32_t pid, t_list *lista);
int obtener_index(t_list *lista, uint32_t pid);
void interrumpir_proceso(uint32_t pid);
void cambiar_estado_pcb(t_pcb *pcb, estado_code state);
void agregar_a_lista_con_semaforos(t_pcb *pcb, t_list *list, pthread_mutex_t m_sem);
void actualizar_estado_pcb(t_pcb *pcb, estado_code state, t_list *list, pthread_mutex_t mutex);
void prender_clock_RR();
void contador_quantum_RR(void *args);
char *obtener_nombre_estado(int state);
// planificacion.c
void bloquear_proceso(t_pcb *pcb_proceso_a_bloquear, bool es_io);
void desbloquear_proceso(uint32_t pid);
// bool proceso_esta_bloqueado(uint32_t pid);
const char* obtener_string_estado_code(estado_code estado);

// ------------------------------------------------------------------------------------------
// -- Planificacion Largo --
// -----------------------------------------------------------------------------------------
t_pcb *crear_pcb();
char *codigo_estado_string(estado_code codigo);
void log_cambiar_estado(int pid, estado_code viejo, estado_code nuevo);
bool proceso_en_running(t_pcb *pcb);
void *crear_proceso(void *arg);
void nuevo_proceso_a_memoria(uint32_t *pid, char *ruta_archivo);
void *finalizar_proceso(void *pid);
void eliminar_proceso(t_pcb *pcb);
void eliminar_proceso_de_memoria(uint32_t *pid);
void enviar_interrupcion_CPU(uint32_t *pid);
void *mover_a_ready();

// ------------------------------------------------------------------------------------------
// -- Consola --
// -----------------------------------------------------------------------------------------
void crear_hilo_consola();
void inicializar_consola();
void procesar_comando(char *);

// ------------------------------------------------------------------------------------------
// -- Destroyers --
// -----------------------------------------------------------------------------------------
void destruir_listas_planificacion();
void destruir_listas_estados_procesos();
void destroy_semaphores(t_config_kernel *config_kernel);
void pcb_destroyer(void *pcb);
void destruir_clock_global();

// ------------------------------------------------------------------------------------------
// -- Recursos --
// -----------------------------------------------------------------------------------------

void recursos_init(t_config_kernel *config);
bool recurso_existe(char *recurso);
bool recurso_incrementar(char *recurso, int *instancias);
bool recurso_decrementar(char *recurso, int *instancias, t_pcb *aux_pcb_ejecutado);
bool recurso_agregar_bloqueado(char *recurso, t_pcb *pcb);
bool recurso_obtener_bloqueado(char *recurso, uint32_t *pid);
void recursos_deinit();
const char *motivo_desalojo_to_string(motivo_desalojo motivo);
bool criterio_busqueda(void *elemento, void *pid);
int obtener_index_recurso(t_list *lista, char *recurso);
void *recibir_contexto_ejecucion_motivo_signal(t_pcb *aux_pcb_ejecutado, char *recurso);
void *recibir_contexto_ejecucion_motivo_wait(t_pcb *aux_pcb_ejecutado, char *recurso);
void estado_proceso();
void liberar_recursos_tomados(t_pcb *pcb);

#endif
