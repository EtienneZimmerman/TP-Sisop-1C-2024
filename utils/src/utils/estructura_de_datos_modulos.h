#ifndef ESTRUCTURA_DE_DATOS_MODULOS_H
#define ESTRUCTURA_DE_DATOS_MODULOS_H
#include "stdint.h"
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "pthread.h"
// ------------------------------------------------------------------------------------------
// -- Structs --
// ------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------
// -- CPU --
// ------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t pc;
    uint8_t ax;
    uint8_t bx;
    uint8_t cx;
    uint8_t dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;
    uint32_t di;
} t_registros;

// ------------------------------------------------------------------------------------------
// -- KERNEL --
// ------------------------------------------------------------------------------------------
typedef enum
{
    NEW,
    READY,
    EXEC,
    BLOCKED,
    EXITT,
    ERROR,
    READY_AUX,
} estado_code;
typedef struct pcb
{
    uint32_t pid;
    uint32_t pc;
    int64_t quantum;
    t_registros registros;
    t_list *recursos_tomados;
    char *recurso_esperando;
    estado_code estado;
    int64_t tiempo_inicial;
    pthread_mutex_t m_proceso;
} t_pcb;

#define ID_RECURSO_LONG_MAX 10
typedef struct recurso
{
    char id[ID_RECURSO_LONG_MAX + 1];
    int instancias;
    int indice_semaforo_asociado_a_recurso;
    t_queue *bloqueados;
    pthread_mutex_t m_recurso;
} t_recurso;

typedef enum
{
    SET,
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,
    WAIT,
    SIGNAL,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT
} id_instruccion;

typedef enum
{
    INTERRUPCION,
    TERMINAR_PROCESO,
    MOTIVO_WAIT,
    MOTIVO_SIGNAL,
    IO,
    OUT_OF_MEMORY_ERROR
} motivo_desalojo;

typedef struct
{
    t_pcb *pcb_ejecutado;
    motivo_desalojo motivo_desalojo;
    char *recurso;
} t_contexto_de_ejecucion;

typedef struct
{
    char *nombre_interfaz;
    int *socket_cliente_io;
    uint32_t *tipo_interfaz;
    t_list *lista_bloqueados;
    pthread_mutex_t m_socket;
} t_interfaz;

typedef struct
{
    uint32_t pid;
    uint32_t marco;
    uint32_t desplazamiento_fisico;
} t_direccion_fisica;

typedef struct
{
    uint32_t pid;
    uint32_t pagina;
} t_dir_logica;

typedef struct
{
    uint32_t pid;
    uint32_t pagina;
    uint32_t marco;
    uint64_t last_access;
} t_registro_tlb;

typedef struct
{
    char *interfaz;
    t_direccion_fisica *dir_fisica;
    void *tamanio;
} t_io_stdin_read;

typedef struct
{
    uint32_t tamanio;
    uint32_t pid;
    uint32_t di;
    uint32_t si;
} t_copy_string;

typedef struct
{
    char *interfaz;
    t_direccion_fisica *dir_fisica;
    uint32_t tamanio;
} t_io_stdout_write;

typedef struct
{
    t_direccion_fisica *dir_fisica;
    void *datos;
} t_mov_in;

typedef struct
{
    t_direccion_fisica *dir_fisica;
    void *datos;
} t_mov_out;

typedef struct
{
    uint8_t tipo;
} t_registro_abstracto;

typedef struct
{
    uint8_t tipo;
    uint8_t data;
} t_registro_8;

typedef struct
{
    uint8_t tipo;
    uint32_t data;
} t_registro_32;
typedef struct
{
    uint32_t *pid;
    motivo_desalojo motivo;
} t_interrupcion;

typedef struct
{
    uint32_t pid;
    uint32_t tamanio;
} t_resize;

#endif
