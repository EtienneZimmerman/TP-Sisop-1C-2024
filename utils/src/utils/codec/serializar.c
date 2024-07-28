#include "serializar.h"

t_buffer *crear_buffer()
{
    t_buffer *buffer;
    buffer = malloc(sizeof(t_buffer));
    buffer->size = 0;
    buffer->stream = NULL;
    return buffer;
}
void agregar_dir_fisica_a_paquete(t_paquete *paquete, t_direccion_fisica *dir)
{
    agregar_var_a_paquete(paquete, &dir->pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &dir->marco, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &dir->desplazamiento_fisico, sizeof(uint32_t));
}

t_buffer *null_buffer()
{
    t_buffer *buffer = malloc(sizeof(t_buffer));
    buffer->size = 0;
    buffer->stream = NULL;
    return buffer;
}

t_paquete *crear_paquete(t_buffer *buffer, int codigo_operacion)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->buffer = buffer;
    return paquete;
}

void agregar_var_a_paquete(t_paquete *paquete, const void *valor, const int tamanio)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio);

    memcpy(paquete->buffer->stream + paquete->buffer->size, valor, tamanio);

    paquete->buffer->size += tamanio;
}

void destruir_buffer(t_buffer *buffer)
{
    free(buffer->stream);
    free(buffer);
}

void serializar_t_registro(t_paquete *paquete, void *reg)
{
    t_registro_abstracto *reg_abs = (t_registro_abstracto *)reg;
    agregar_var_a_paquete(paquete, &reg_abs->tipo, sizeof(uint8_t));
    if (reg_abs->tipo == 1)
    {
        t_registro_8 *reg8 = (t_registro_8 *)reg;
        agregar_var_a_paquete(paquete, &reg8->data, sizeof(uint8_t));
    }
    else
    {
        t_registro_32 *reg32 = (t_registro_32 *)reg;
        agregar_var_a_paquete(paquete, &reg32->data, sizeof(uint32_t));
    }
}

// Capaz el nombre es malo, pero es el mensaje que le enviamos a memoria para que me busque la instruccion siguiente del proceso.
t_paquete *obtener_paquete_instrucciones(uint32_t *pid, uint32_t *pc)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, OBT_INSTRUCCION);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, pc, sizeof(uint32_t));
    return paquete;
}

// Paquete que devuelve memoria al cpu luego de obtener la instruccion.
t_paquete *obtener_paquete_ret_instrucciones(uint32_t *id_inst, char *instrucciones)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, RET_INSTRUCCION);
    uint32_t tam_instruccion = strlen(instrucciones) + 1;
    agregar_var_a_paquete(paquete, id_inst, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &tam_instruccion, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, instrucciones, strlen(instrucciones) + 1);
    free(id_inst);
    return paquete;
}

t_paquete *serializar_interrupcion(uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, INT_PROCESO);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_interrupcion_exit(uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, FINALIZAR_PROCESO_CPU);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

void serializar_registros(t_paquete *paquete, t_registros *registros)
{
    agregar_var_a_paquete(paquete, &registros->ax, sizeof(uint8_t));
    agregar_var_a_paquete(paquete, &registros->bx, sizeof(uint8_t));
    agregar_var_a_paquete(paquete, &registros->cx, sizeof(uint8_t));
    agregar_var_a_paquete(paquete, &registros->dx, sizeof(uint8_t));
    agregar_var_a_paquete(paquete, &registros->eax, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registros->ebx, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registros->ecx, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registros->edx, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registros->si, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registros->di, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registros->pc, sizeof(uint32_t));
}

t_paquete *serializar_paquete_pcb(t_pcb *pcb)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, ENVIAR_PCB);
    agregar_var_a_paquete(paquete, &pcb->pc, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &pcb->pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &pcb->quantum, sizeof(int64_t));
    serializar_registros(paquete, &pcb->registros);

    if (pcb->recurso_esperando != NULL)
    {
        uint32_t recurso_len = strlen(pcb->recurso_esperando) + 1;
        agregar_var_a_paquete(paquete, &recurso_len, sizeof(uint32_t));
        agregar_var_a_paquete(paquete, pcb->recurso_esperando, recurso_len);
    }
    else
    {
        uint32_t recurso_len = 0;
        agregar_var_a_paquete(paquete, &recurso_len, sizeof(uint32_t));
    }

    uint32_t cantidad_recursos = list_size(pcb->recursos_tomados);
    agregar_var_a_paquete(paquete, &cantidad_recursos, sizeof(uint32_t));

    for (uint32_t i = 0; i < cantidad_recursos; i++)
    {
        char *recurso = (char *)list_get(pcb->recursos_tomados, i);
        uint32_t recurso_len = strlen(recurso) + 1;
        agregar_var_a_paquete(paquete, &recurso_len, sizeof(uint32_t));
        agregar_var_a_paquete(paquete, recurso, recurso_len);
    }

    agregar_var_a_paquete(paquete, &pcb->estado, sizeof(estado_code));

    agregar_var_a_paquete(paquete, &pcb->tiempo_inicial, sizeof(int64_t));

    return paquete;
}

t_paquete *serializar_contexto_ejecucion(t_pcb *pcb, motivo_desalojo motivo, char *recurso)
{
    t_paquete *paquete = serializar_paquete_pcb(pcb);
    paquete->codigo_operacion = ENVIAR_CONTEXTO_EJECUCION;
    agregar_var_a_paquete(paquete, (void *)&motivo, sizeof(motivo_desalojo));
    if (recurso != NULL)
    {
        uint32_t recurso_len = strlen(recurso) + 1;
        agregar_var_a_paquete(paquete, &recurso_len, sizeof(uint32_t));
        agregar_var_a_paquete(paquete, recurso, recurso_len);
    }
    else
    {
        uint32_t recurso_len = 0;
        agregar_var_a_paquete(paquete, &recurso_len, sizeof(uint32_t));
    }
    return paquete;
}

t_paquete *obtener_paquete_peticion_io_gen_sleep(char *interfaz, uint32_t *unidades_de_trabajo, t_pcb *pcb, uint32_t *motivo_desalojo)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_GEN_SLEEP;
    uint32_t tam_interfaz = strlen(interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
    agregar_var_a_paquete(paquete, unidades_de_trabajo, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_ejecutar_io_gen_sleep(uint32_t *unidades_de_trabajo, uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_GEN_SLEEP);
    agregar_var_a_paquete(paquete, unidades_de_trabajo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_ejecutar_io_stdin_read(uint32_t *pid, t_direccion_fisica *dir, void *tamanio)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_STDIN_READ);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    serializar_t_registro(paquete, tamanio);
    return paquete;
}

t_paquete *serializar_ejecutar_io_stdout_write(uint32_t *pid, t_direccion_fisica *dir, uint32_t tamanio)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_STDOUT_WRITE);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_error_io(char *nombre_interfaz, uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, ERROR_IO);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_handshake_io(char *nombre_interfaz, uint32_t *tipo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, HANDSHAKE_IO);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, tipo, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_sleep(char *nombre_interfaz, uint32_t *pid, uint32_t *unidades_de_trabajo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_SLEEP);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, unidades_de_trabajo, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_read(char *nombre_interfaz, uint32_t *pid, t_direccion_fisica *direccion_fisica, void *tamanio)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_READ);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, direccion_fisica);
    serializar_t_registro(paquete, tamanio);
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_write(char *nombre_interfaz, uint32_t *pid, t_direccion_fisica *direccion_fisica, uint32_t tamanio)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_WRITE);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, direccion_fisica);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_fs_create(char *nombre_interfaz, uint32_t pid, char *nombre_archivo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_FS_CREATE);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam_nombre_archivo);
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_fs_delete(char *nombre_interfaz, uint32_t pid, char *nombre_archivo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_FS_DELETE);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam_nombre_archivo);
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_fs_truncate(char *nombre_interfaz, uint32_t pid, char *nombre_archivo, uint32_t tamanio)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_FS_TRUNCATE);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam_nombre_archivo);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_fs_write(char *nombre_interfaz, uint32_t pid, char *nombre_archivo, t_direccion_fisica *direccion_fisica, uint32_t tamanio, uint32_t posicion_archivo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_FS_WRITE);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam_nombre_archivo);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &posicion_archivo, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, direccion_fisica);
    return paquete;
}

t_paquete *serializar_interfaz_ocupada_fs_read(char *nombre_interfaz, uint32_t pid, char *nombre_archivo, t_direccion_fisica *direccion_fisica, uint32_t tamanio, uint32_t posicion_archivo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, IO_OCUPADA_FS_READ);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam_nombre_archivo);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &posicion_archivo, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, direccion_fisica);
    return paquete;
}

t_paquete *serializar_fin_io(char *nombre_interfaz, uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, FIN_IO);
    uint32_t tam_interfaz = strlen(nombre_interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_interfaz, tam_interfaz);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_nuevo_proceso_memoria(uint32_t *pid, char *ruta_archivo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, CREAR_PROCESO);
    uint32_t tam_ruta = strlen(ruta_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_ruta, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, ruta_archivo, tam_ruta);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_eliminar_proceso_memoria(uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, FINALIZAR_PROCESO_MEMORIA);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_fin_creacion(uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, FIN_CREA_PRO);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_fin_terminacion(uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, FIN_TERM_PRO);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_handshake_memoria()
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, HANDSHAKE_MEMORIA);
    uint32_t sarasa = 1;
    agregar_var_a_paquete(paquete, &sarasa, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_respuesta_handshake_memoria(int *tam_pagina)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, HANDSHAKE_MEMORIA);
    agregar_var_a_paquete(paquete, tam_pagina, sizeof(int));
    return paquete;
}

t_paquete *serializar_pedir_marco(t_dir_logica *dir)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, PEDIR_MARCO);
    agregar_var_a_paquete(paquete, &dir->pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &dir->pagina, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_respnder_marco(t_registro_tlb *registro_tlb)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, PEDIR_MARCO);
    agregar_var_a_paquete(paquete, &registro_tlb->pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registro_tlb->pagina, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &registro_tlb->marco, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_ejecutar_mov_in(t_mov_in *movin)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_MOVIN);
    agregar_dir_fisica_a_paquete(paquete, movin->dir_fisica);
    serializar_t_registro(paquete, movin->datos);
    return paquete;
}

t_paquete *serializar_ejecutar_mov_out(t_mov_out *movout)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_MOVOUT);
    agregar_dir_fisica_a_paquete(paquete, movout->dir_fisica);
    serializar_t_registro(paquete, movout->datos);
    return paquete;
}

t_paquete *serializar_ejecutar_copy_string(t_copy_string *copy_string)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_COPYSTRING);
    agregar_var_a_paquete(paquete, &copy_string->tamanio, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &copy_string->pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &copy_string->di, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &copy_string->si, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_t_stdin_read(t_paquete *paquete, t_io_stdin_read *stdin_read)
{
    agregar_dir_fisica_a_paquete(paquete, stdin_read->dir_fisica);
    uint32_t tam_interfaz = strlen(stdin_read->interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, stdin_read->interfaz, tam_interfaz);
    serializar_t_registro(paquete, stdin_read->tamanio);
    return paquete;
}

t_paquete *serializar_t_stdout_write(t_paquete *paquete, t_io_stdout_write *stdout_write)
{
    agregar_dir_fisica_a_paquete(paquete, stdout_write->dir_fisica);
    uint32_t tam_interfaz = strlen(stdout_write->interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, stdout_write->interfaz, tam_interfaz);
    return paquete;
}

t_paquete *serializar_rta_movin(void *reg)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, RTA_MOVIN);
    serializar_t_registro(paquete, reg);
    return paquete;
}

t_paquete *serializar_peticion_io_stdin_read(char *interfaz, t_pcb *pcb, uint32_t *motivo_desalojo, t_io_stdin_read *stdin_read)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_STDIN_READ;
    uint32_t tam_interfaz = strlen(interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
    serializar_t_stdin_read(paquete, stdin_read);
    return paquete;
}

t_paquete *serializar_peticion_io_stdout_write(char *interfaz, t_pcb *pcb, uint32_t *motivo_desalojo, t_io_stdout_write *stdout_write, uint32_t *tam)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_STDOUT_WRITE;
    serializar_t_stdout_write(paquete, stdout_write);
    agregar_var_a_paquete(paquete, tam, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_io_stdin(uint32_t *pid, t_direccion_fisica *dir, void *tamanio, char *a_escribir)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_STDIN_READ);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    serializar_t_registro(paquete, tamanio);
    uint32_t tam = strlen(a_escribir) + 1;
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, a_escribir, tam);
    return paquete;
}

t_paquete *serializar_io_stdout(uint32_t *pid, t_direccion_fisica *dir, uint32_t tamanio)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_STDOUT_WRITE);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_ret_io_stdout(uint32_t *pid, uint32_t reg, char *leido)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, RET_IO_STDOUT_WRITE);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &reg, sizeof(uint32_t));
    int size = strlen(leido);
    agregar_var_a_paquete(paquete, &size, sizeof(uint32_t));
    if (size != 0)
    {
        agregar_var_a_paquete(paquete, leido, size + 1);
    }
    return paquete;
}

t_paquete *serializar_ret_io_stdin(uint32_t *pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, RET_IO_STDIN_READ);
    agregar_var_a_paquete(paquete, pid, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_ejecutar_resize(t_resize *resize)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_RESIZE);
    agregar_var_a_paquete(paquete, &resize->pid, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &resize->tamanio, sizeof(uint32_t));
    return paquete;
}

t_paquete *obtener_paquete_peticion_io_fs_create(char *interfaz, char *nombre_archivo, t_pcb *pcb, uint32_t *motivo_desalojo)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_FS_CREATE;
    uint32_t tam_interfaz = strlen(interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
    return paquete;
}

t_paquete *obtener_paquete_peticion_io_fs_delete(char *interfaz, char *nombre_archivo, t_pcb *pcb, uint32_t *motivo_desalojo)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_FS_DELETE;
    uint32_t tam_interfaz = strlen(interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
    return paquete;
}

t_paquete *obtener_paquete_peticion_io_fs_truncate(char *interfaz, char *nombre_archivo, uint32_t tam, t_pcb *pcb, uint32_t *motivo_desalojo)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_FS_TRUNCATE;
    uint32_t tam_interfaz = strlen(interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    return paquete;
}

t_paquete *obtener_paquete_peticion_io_fs_write(char *interfaz, char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, t_pcb *pcb, uint32_t *motivo_desalojo)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_FS_WRITE;
    uint32_t tam_interfaz = strlen(interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    agregar_var_a_paquete(paquete, &posicion_archivo, sizeof(uint32_t));
    return paquete;
}

t_paquete *obtener_paquete_peticion_io_fs_read(char *interfaz, char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, t_pcb *pcb, uint32_t *motivo_desalojo)
{
    t_paquete *paquete = serializar_contexto_ejecucion(pcb, *motivo_desalojo, NULL);
    paquete->codigo_operacion = INSTRUCCION_IO_FS_READ;
    uint32_t tam_interfaz = strlen(interfaz) + 1;
    agregar_var_a_paquete(paquete, &tam_interfaz, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, interfaz, strlen(interfaz) + 1);
    uint32_t tam_nombre_archivo = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam_nombre_archivo, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, strlen(nombre_archivo) + 1);
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    agregar_var_a_paquete(paquete, &posicion_archivo, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_ejecutar_io_fs_create(uint32_t pid, char *nombre_archivo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_FS_CREATE);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam);
    return paquete;
}

t_paquete *serializar_ejecutar_io_fs_delete(uint32_t pid, char *nombre_archivo)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_FS_DELETE);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam);
    return paquete;
}

t_paquete *serializar_ejecutar_io_fs_truncate(uint32_t pid, char *nombre_archivo, uint32_t tamanio)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_FS_TRUNCATE);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tam = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tam);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    return paquete;
}

t_paquete *serializar_ejecutar_io_fs_read(char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, uint32_t pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_FS_READ);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tamanio = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tamanio);
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &posicion_archivo, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    return paquete;
}

t_paquete *serializar_ejecutar_io_fs_write(char *nombre_archivo, uint32_t tam, uint32_t posicion_archivo, t_direccion_fisica *dir, uint32_t pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_FS_WRITE);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    uint32_t tamanio = strlen(nombre_archivo) + 1;
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, nombre_archivo, tamanio);
    agregar_var_a_paquete(paquete, &tam, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, &posicion_archivo, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    return paquete;
}

t_paquete *serializar_io_fs_read(uint32_t pid, t_direccion_fisica *dir, uint32_t tamanio, void *leido)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_FS_READ);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_dir_fisica_a_paquete(paquete, dir);
    agregar_var_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    agregar_var_a_paquete(paquete, leido, tamanio);
    return paquete;
}

t_paquete *serializar_ret_io_fs_read(uint32_t pid)
{
    t_buffer *buff = crear_buffer();
    t_paquete *paquete = crear_paquete(buff, EJECUTAR_IO_FS_READ);
    agregar_var_a_paquete(paquete, &pid, sizeof(uint32_t));
    return paquete;
}

// Agrega la cadena leida al paquete
void serializar_ret_io_fs_write(t_paquete *paquete, uint32_t tamanio, void *leido)
{
    agregar_var_a_paquete(paquete, leido, tamanio);
}
