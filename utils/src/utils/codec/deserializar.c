#include "deserializar.h"

uint32_t deserializar_uint32(void *stream, int *desplazamiento)
{
    uint32_t *variable;

    variable = stream + *desplazamiento;
    *desplazamiento += sizeof(uint32_t);

    return *variable;
}

int64_t deserializar_int64(void *stream, int *desplazamiento)
{
    int64_t *variable;

    variable = stream + *desplazamiento;
    *desplazamiento += sizeof(int64_t);

    return *variable;
}

uint8_t deserializar_uint8(void *stream, int *desplazamiento)
{
    uint8_t *variable;

    variable = stream + *desplazamiento;
    *desplazamiento += sizeof(uint8_t);

    return *variable;
}

void *deserializar_t_registro(void *stream, int *desplazamiento)
{
    uint8_t tipo = deserializar_uint8(stream, desplazamiento);
    if (tipo == 1)
    {
        t_registro_8 *reg = malloc(sizeof(t_registro_8));
        reg->tipo = tipo;
        reg->data = deserializar_uint8(stream, desplazamiento);
        return (void *)reg;
    }
    else
    {
        t_registro_32 *reg = malloc(sizeof(t_registro_32));
        reg->tipo = tipo;
        reg->data = deserializar_uint32(stream, desplazamiento);
        return (void *)reg;
    }
}

char *deserializar_char(void *stream, int *desplazamiento, int tamanio)
{
    char *registro = malloc(tamanio);

    memcpy(registro, stream + *desplazamiento, tamanio);
    // registro = stream + *desplazamiento;
    *desplazamiento += tamanio;
    return registro;
}

t_obtener_instruccion *deserializar_obtener_instruccion(void *stream, int *bytes_recibidos)
{
    t_obtener_instruccion *obtener_instruccion = malloc(sizeof(t_obtener_instruccion));
    int desplazamiento = 0;
    obtener_instruccion->pid = deserializar_uint32(stream, &desplazamiento);
    obtener_instruccion->pc = deserializar_uint32(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return obtener_instruccion;
}
t_direccion_fisica *deserializar_direccion_fisica(void *stream, int *des)
{
    t_direccion_fisica *dir = malloc(sizeof(t_direccion_fisica));
    dir->pid = deserializar_uint32(stream, des);
    dir->marco = deserializar_uint32(stream, des);
    dir->desplazamiento_fisico = deserializar_uint32(stream, des);
    return dir;
}

t_obtener_ret_instruccion *deserializar_obtener_ret_instruccion(void *stream, int *bytes_recibidos)
{
    t_obtener_ret_instruccion *obtener_ret_instruccion = malloc(sizeof(t_obtener_ret_instruccion));
    int desplazamiento = 0;
    obtener_ret_instruccion->id_inst = deserializar_uint32(stream, &desplazamiento);
    uint32_t tam_instruccion = deserializar_uint32(stream, &desplazamiento);
    obtener_ret_instruccion->instruccion = deserializar_char(stream, &desplazamiento, tam_instruccion);
    *bytes_recibidos = desplazamiento;
    return obtener_ret_instruccion;
}

uint32_t *deserealizar_recibir_int_proceso(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    uint32_t *pid = malloc(sizeof(uint32_t));
    *pid = deserializar_uint32(stream, &desplazamiento);
    return pid;
}

t_registros deserializar_registros(void *stream, int *desplazamiento)
{
    t_registros registros;
    registros.ax = deserializar_uint8(stream, desplazamiento);
    registros.bx = deserializar_uint8(stream, desplazamiento);
    registros.cx = deserializar_uint8(stream, desplazamiento);
    registros.dx = deserializar_uint8(stream, desplazamiento);
    registros.eax = deserializar_uint32(stream, desplazamiento);
    registros.ebx = deserializar_uint32(stream, desplazamiento);
    registros.ecx = deserializar_uint32(stream, desplazamiento);
    registros.edx = deserializar_uint32(stream, desplazamiento);
    registros.si = deserializar_uint32(stream, desplazamiento);
    registros.di = deserializar_uint32(stream, desplazamiento);
    registros.pc = deserializar_uint32(stream, desplazamiento);
    return registros;
}

t_pcb *deserializar_paquete_pcb(void *stream, int *desplazamiento)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));
    pcb->pc = deserializar_uint32(stream, desplazamiento);
    pcb->pid = deserializar_uint32(stream, desplazamiento);
    pcb->quantum = deserializar_int64(stream, desplazamiento);
    pcb->registros = deserializar_registros(stream, desplazamiento);

    // Deserializar recurso_esperando
    uint32_t recurso_len = deserializar_uint32(stream, desplazamiento);
    if (recurso_len > 0)
    {
        pcb->recurso_esperando = deserializar_char(stream, desplazamiento, recurso_len);
    }
    else
    {
        pcb->recurso_esperando = NULL;
    }

    // Deserializar la lista de recursos_tomados
    uint32_t cantidad_recursos = deserializar_uint32(stream, desplazamiento);
    pcb->recursos_tomados = list_create();
    for (uint32_t i = 0; i < cantidad_recursos; i++)
    {
        uint32_t recurso_len = deserializar_uint32(stream, desplazamiento);
        char *recurso = deserializar_char(stream, desplazamiento, recurso_len);
        list_add(pcb->recursos_tomados, recurso);
    }

    pcb->estado = deserializar_uint32(stream, desplazamiento);

    pcb->tiempo_inicial = deserializar_int64(stream, desplazamiento);

    pthread_mutex_init(&pcb->m_proceso, NULL);

    return pcb;
}

t_contexto_de_ejecucion *deserializar_contexto_ejecucion(void *stream, int *bytes_recibidos)
{
    t_contexto_de_ejecucion *contexto = malloc(sizeof(t_contexto_de_ejecucion));
    int desplazamiento = 0;
    t_pcb *pcb = deserializar_paquete_pcb(stream, &desplazamiento);
    uint32_t motivo_desalojo = deserializar_uint32(stream, &desplazamiento);
    uint32_t recurso_len = deserializar_uint32(stream, &desplazamiento);
    if (recurso_len > 0)
    {
        char *recurso = deserializar_char(stream, &desplazamiento, recurso_len);
        contexto->recurso = recurso;
    }
    else
    {
        contexto->recurso = NULL;
    }

    contexto->pcb_ejecutado = pcb;
    contexto->motivo_desalojo = motivo_desalojo;
    *bytes_recibidos = desplazamiento;
    return contexto;
}

t_peticion_io_gen_sleep *deserializar_obtener_paquete_peticion_io_gen_sleep(void *stream, int *bytes_recibidos)
{
    t_peticion_io_gen_sleep *peticion_io_gen_sleep = malloc(sizeof(t_peticion_io_gen_sleep));
    int desplazamiento = 0;
    peticion_io_gen_sleep->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    peticion_io_gen_sleep->interfaz = interfaz_nombre;
    peticion_io_gen_sleep->unidades_de_trabajo = deserializar_uint32(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return peticion_io_gen_sleep;
}

t_ejecutar_io_gen_sleep *deserializar_ejecutar_io_gen_sleep(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_gen_sleep *ejecutar_io_gen_sleep = malloc(sizeof(t_ejecutar_io_gen_sleep));
    int desplazamiento = 0;
    ejecutar_io_gen_sleep->unidades_de_trabajo = deserializar_uint32(stream, &desplazamiento);
    ejecutar_io_gen_sleep->pid = deserializar_uint32(stream, &desplazamiento);
    return ejecutar_io_gen_sleep;
}

t_handshake_io *deserializar_handshake_io(void *stream, int *bytes_recibidos)
{
    t_handshake_io *handshake = malloc(sizeof(t_handshake_io));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    handshake->nombre = deserializar_char(stream, &desplazamiento, tam_nombre);
    handshake->tipo_interfaz = deserializar_uint32(stream, &desplazamiento);
    return handshake;
}

t_interfaz_mje *deserializar_error_io(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    return interfaz_mje;
}

t_interfaz_mje *deserializar_io_ocupada_sleep(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->unidades_de_trabajo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->direccion_fisica = NULL;
    interfaz_mje->reg = NULL;
    return interfaz_mje;
}

t_interfaz_mje *deserializar_io_ocupada_read(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->unidades_de_trabajo = -1;
    interfaz_mje->direccion_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    interfaz_mje->reg = deserializar_t_registro(stream, &desplazamiento);
    return interfaz_mje;
}

t_interfaz_mje *deserializar_io_ocupada_write(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->unidades_de_trabajo = -1;
    interfaz_mje->direccion_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    interfaz_mje->reg = deserializar_t_registro(stream, &desplazamiento);
    return interfaz_mje;
}

t_interfaz_mje *deserializar_fin_io(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    return interfaz_mje;
}

t_crear_proceso *deserializar_nuevo_proceso_memoria(void *stream, int *bytes_recibidos)
{
    t_crear_proceso *new_process = malloc(sizeof(t_crear_proceso));
    int desplazamiento = 0;
    uint32_t tam_ruta = deserializar_uint32(stream, &desplazamiento);
    new_process->ruta_archivo = deserializar_char(stream, &desplazamiento, tam_ruta);
    new_process->pid = deserializar_uint32(stream, &desplazamiento);
    return new_process;
}

// int deserializar_ret_acceso_tabla_paginas(void *stream, int *bytes_recibidos)
// {
//     int desplazamiento = 0;
//     int marco = deserializar_uint32(stream, &desplazamiento);
//     *bytes_recibidos = desplazamiento;
//     return marco;
// }

// t_acceso_tabla_paginas *deserializar_acceso_tabla_paginas(void *stream, int *bytes_recibidos)
// {
//     t_acceso_tabla_paginas *acceso_tabla_paginas = malloc(sizeof(t_acceso_tabla_paginas));
//     int desplazamiento = 0;
//     acceso_tabla_paginas->pid = deserializar_uint32(stream, &desplazamiento);
//     acceso_tabla_paginas->pagina = deserializar_uint32(stream, &desplazamiento);
//     *bytes_recibidos = desplazamiento;
//     return acceso_tabla_paginas;
// }

uint32_t *deserializar_rta_handhsake_memoria(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    uint32_t *tam_pagina = malloc(sizeof(uint32_t));
    *tam_pagina = deserializar_uint32(stream, &desplazamiento);
    return tam_pagina;
}

t_dir_logica *deserializar_pedir_marco(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_dir_logica *dir_logica = malloc(sizeof(t_dir_logica));
    dir_logica->pid = deserializar_uint32(stream, &desplazamiento);
    dir_logica->pagina = deserializar_uint32(stream, &desplazamiento);
    return dir_logica;
}

t_registro_tlb *deserializar_responder_marco(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_registro_tlb *registro_tlb = malloc(sizeof(t_registro_tlb));
    registro_tlb->pid = deserializar_uint32(stream, &desplazamiento);
    registro_tlb->pagina = deserializar_uint32(stream, &desplazamiento);
    registro_tlb->marco = deserializar_uint32(stream, &desplazamiento);
    registro_tlb->last_access = 0;
    return registro_tlb;
}

t_mov_in *deserializar_ejecutar_mov_in(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_mov_in *movin = malloc(sizeof(t_mov_in));
    movin->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    movin->datos = deserializar_t_registro(stream, &desplazamiento);
    return movin;
}

t_mov_out *deserializar_ejecutar_mov_out(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_mov_out *movout = malloc(sizeof(t_mov_out));
    movout->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    movout->datos = deserializar_t_registro(stream, &desplazamiento);

    return movout;
}

t_copy_string *deserializar_ejecutar_copy_string(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_copy_string *copystring = malloc(sizeof(t_copy_string));
    copystring->tamanio = deserializar_uint32(stream, &desplazamiento);
    copystring->pid = deserializar_uint32(stream, &desplazamiento);
    copystring->di = deserializar_uint32(stream, &desplazamiento);
    copystring->si = deserializar_uint32(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return copystring;
}

t_ejecutar_io_stdin_read *deserializar_ejecutar_stdin_read(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_ejecutar_io_stdin_read *stdin_read = malloc(sizeof(t_ejecutar_io_stdin_read));
    stdin_read->pid = deserializar_uint32(stream, &desplazamiento);
    stdin_read->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    stdin_read->tamanio = deserializar_t_registro(stream, &desplazamiento);

    return stdin_read;
}

t_ejecutar_io_stdout_write *deserializar_ejecutar_stdout_write(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_ejecutar_io_stdout_write *stdout_write = malloc(sizeof(t_ejecutar_io_stdout_write));
    stdout_write->pid = deserializar_uint32(stream, &desplazamiento);
    stdout_write->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    stdout_write->tamanio = deserializar_uint32(stream, &desplazamiento);

    return stdout_write;
}

void *deserializar_rta_movin(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    return deserializar_t_registro(stream, &desplazamiento);
}

t_peticion_io_stdin_read *deserializar_obtener_paquete_peticion_io_stdin_read(void *stream, int *bytes_recibidos)
{
    t_peticion_io_stdin_read *stdin_read = malloc(sizeof(t_peticion_io_stdin_read));
    int desplazamiento = 0;
    stdin_read->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    stdin_read->interfaz = interfaz_nombre;
    stdin_read->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    stdin_read->tamanio = deserializar_t_registro(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return stdin_read;
}

t_peticion_io_stdout_write *deserializar_obtener_paquete_peticion_io_stdout_write(void *stream, int *bytes_recibidos)
{
    t_peticion_io_stdout_write *stdout_write = malloc(sizeof(t_peticion_io_stdout_write));
    int desplazamiento = 0;
    stdout_write->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    stdout_write->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    stdout_write->interfaz = interfaz_nombre;
    stdout_write->tamanio = deserializar_uint32(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return stdout_write;
}

t_resize *deserializar_ejecutar_resize(void *stream, int *bytes_recibidos)
{
    int desplazamiento = 0;
    t_resize *resize = malloc(sizeof(t_resize));
    resize->pid = deserializar_uint32(stream, &desplazamiento);
    resize->tamanio = deserializar_uint32(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return resize;
}
t_ejecutar_io_stdin *deserializar_io_stdin(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_stdin *_stdin = malloc(sizeof(t_ejecutar_io_stdin));
    int desplazamiento = 0;
    _stdin->pid = deserializar_uint32(stream, &desplazamiento);
    _stdin->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    _stdin->tamanio = deserializar_t_registro(stream, &desplazamiento);
    uint32_t tam = deserializar_uint32(stream, &desplazamiento);
    _stdin->a_escribir = deserializar_char(stream, &desplazamiento, tam);
    return _stdin;
}

t_ejecutar_io_stdout_write *deserializar_io_stdout(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_stdout_write *stdout_ = malloc(sizeof(t_ejecutar_io_stdout_write));
    int desplazamiento = 0;
    stdout_->pid = deserializar_uint32(stream, &desplazamiento);
    stdout_->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    stdout_->tamanio = deserializar_uint32(stream, &desplazamiento);
    return stdout_;
}

t_ret_io_stdout_write *deserializar_ret_io_stdout_write(void *stream, int *bytes_recibidos)
{
    t_ret_io_stdout_write *ret = malloc(sizeof(t_ret_io_stdout_write));
    int desplazamiento = 0;
    ret->pid = deserializar_uint32(stream, &desplazamiento);
    ret->tamanio = deserializar_uint32(stream, &desplazamiento);
    int size = 0;
    size = deserializar_uint32(stream, &desplazamiento);
    if (size > 0)
        ret->leido = deserializar_char(stream, &desplazamiento, size + 1);
    else
        ret->leido = NULL;
    return ret;
}

uint32_t *deserializar_ret_io_stdin_read(void *stream, int *bytes_recibidos)
{
    uint32_t *pid = malloc(sizeof(uint32_t));
    int desplazamiento = 0;
    *pid = deserializar_uint32(stream, &desplazamiento);
    return pid;
}

t_ejecutar_io_fs_create *deserializar_ejecutar_io_fs_create(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_fs_create *io_fs_create = malloc(sizeof(t_ejecutar_io_fs_create));
    int desplazamiento = 0;
    io_fs_create->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_create->nombre_archivo = deserializar_char(stream, &desplazamiento, tamanio);
    return io_fs_create;
}

t_ejecutar_io_fs_delete *deserializar_ejecutar_io_fs_delete(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_fs_delete *io_fs_delete = malloc(sizeof(t_ejecutar_io_fs_delete));
    int desplazamiento = 0;
    io_fs_delete->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_delete->nombre_archivo = deserializar_char(stream, &desplazamiento, tamanio);
    return io_fs_delete;
}

t_ejecutar_io_fs_truncate *deserializar_ejecutar_io_fs_truncate(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_fs_truncate *io_fs_truncate = malloc(sizeof(t_ejecutar_io_fs_truncate));
    int desplazamiento = 0;
    io_fs_truncate->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_truncate->nombre_archivo = deserializar_char(stream, &desplazamiento, tamanio);
    io_fs_truncate->tamanio_a_truncar = deserializar_uint32(stream, &desplazamiento);
    return io_fs_truncate;
}

t_ejecutar_io_fs_read *deserializar_ejecutar_io_fs_read(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_fs_read *io_fs_read = malloc(sizeof(t_ejecutar_io_fs_read));
    int desplazamiento = 0;
    io_fs_read->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_read->nombre_archivo = deserializar_char(stream, &desplazamiento, tamanio);
    io_fs_read->tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_read->posicion_archivo = deserializar_uint32(stream, &desplazamiento);
    io_fs_read->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    return io_fs_read;
}

t_ejecutar_io_fs_write *deserializar_ejecutar_io_fs_write(void *stream, int *bytes_recibidos)
{
    t_ejecutar_io_fs_write *io_fs_write = malloc(sizeof(t_ejecutar_io_fs_write));
    int desplazamiento = 0;
    io_fs_write->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_write->nombre_archivo = deserializar_char(stream, &desplazamiento, tamanio);
    io_fs_write->tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_write->posicion_archivo = deserializar_uint32(stream, &desplazamiento);
    io_fs_write->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    return io_fs_write;
}

t_peticion_io_fs_delete *deserializar_peticion_io_fs_delete(void *stream, int *bytes_recibidos)
{
    t_peticion_io_fs_delete *fs_delete = malloc(sizeof(t_peticion_io_fs_delete));
    int desplazamiento = 0;
    fs_delete->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    char *nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    fs_delete->interfaz = interfaz_nombre;
    fs_delete->nombre_archivo = nombre_archivo;
    *bytes_recibidos = desplazamiento;
    return fs_delete;
}

t_peticion_io_fs_truncate *deserializar_peticion_io_fs_truncate(void *stream, int *bytes_recibidos)
{
    t_peticion_io_fs_truncate *fs_truncate = malloc(sizeof(t_peticion_io_fs_truncate));
    int desplazamiento = 0;
    fs_truncate->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    char *nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    uint32_t tam = deserializar_uint32(stream, &desplazamiento);
    fs_truncate->interfaz = interfaz_nombre;
    fs_truncate->nombre_archivo = nombre_archivo;
    fs_truncate->tam = tam;
    *bytes_recibidos = desplazamiento;
    return fs_truncate;
}

t_peticion_io_fs_create *deserializar_peticion_io_fs_create(void *stream, int *bytes_recibidos)
{
    t_peticion_io_fs_create *fs_create = malloc(sizeof(t_peticion_io_fs_create));
    int desplazamiento = 0;
    fs_create->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    char *nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    fs_create->interfaz = interfaz_nombre;
    fs_create->nombre_archivo = nombre_archivo;
    *bytes_recibidos = desplazamiento;
    return fs_create;
}

t_peticion_io_fs_read *deserializar_peticion_io_fs_read(void *stream, int *bytes_recibidos)
{
    t_peticion_io_fs_read *fs_read = malloc(sizeof(t_peticion_io_fs_read));
    int desplazamiento = 0;
    fs_read->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    char *nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    fs_read->interfaz = interfaz_nombre;
    fs_read->nombre_archivo = nombre_archivo;
    fs_read->tam = deserializar_uint32(stream, &desplazamiento);
    fs_read->dir_fis = deserializar_direccion_fisica(stream, &desplazamiento);
    fs_read->posicion_archivo = deserializar_uint32(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return fs_read;
}
t_peticion_io_fs_write *deserializar_peticion_io_fs_write(void *stream, int *bytes_recibidos)
{
    t_peticion_io_fs_write *fs_write = malloc(sizeof(t_peticion_io_fs_write));
    int desplazamiento = 0;
    fs_write->cde = deserializar_contexto_ejecucion(stream, &desplazamiento);
    uint32_t tam_interfaz = deserializar_uint32(stream, &desplazamiento);
    char *interfaz_nombre = deserializar_char(stream, &desplazamiento, tam_interfaz);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    char *nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    fs_write->interfaz = interfaz_nombre;
    fs_write->nombre_archivo = nombre_archivo;
    fs_write->tam = deserializar_uint32(stream, &desplazamiento);
    fs_write->dir_fis = deserializar_direccion_fisica(stream, &desplazamiento);
    fs_write->posicion_archivo = deserializar_uint32(stream, &desplazamiento);
    *bytes_recibidos = desplazamiento;
    return fs_write;
}

t_io_fs_read *deserializar_io_fs_read(void *stream, int *bytes_recibidos)
{
    t_io_fs_read *fs_read = malloc(sizeof(t_io_fs_read));
    int desplazamiento = 0;
    fs_read->pid = deserializar_uint32(stream, &desplazamiento);
    fs_read->dir = deserializar_direccion_fisica(stream, &desplazamiento);
    fs_read->tamanio = deserializar_uint32(stream, &desplazamiento);
    fs_read->leido = deserializar_char(stream, &desplazamiento, fs_read->tamanio);
    *bytes_recibidos = desplazamiento;
    return fs_read;
}

t_ret_io_fs_write *deserializar_ret_io_fs_write(void *stream, int *bytes_recibidos)
{
    t_ret_io_fs_write *io_fs_write = malloc(sizeof(t_ret_io_fs_write));
    int desplazamiento = 0;
    io_fs_write->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_write->nombre_archivo = deserializar_char(stream, &desplazamiento, tamanio);
    io_fs_write->tamanio = deserializar_uint32(stream, &desplazamiento);
    io_fs_write->posicion_archivo = deserializar_uint32(stream, &desplazamiento);
    io_fs_write->dir_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    io_fs_write->leido = deserializar_char(stream, &desplazamiento, io_fs_write->tamanio);
    *bytes_recibidos = desplazamiento;
    return io_fs_write;
}
t_interfaz_mje *deserializar_io_ocupada_fs_read(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    interfaz_mje->tamanio = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->posicion_archivo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->direccion_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    interfaz_mje->reg = NULL;
    return interfaz_mje;
}

t_interfaz_mje *deserializar_io_ocupada_fs_write(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    interfaz_mje->tamanio = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->posicion_archivo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->direccion_fisica = deserializar_direccion_fisica(stream, &desplazamiento);
    interfaz_mje->reg = NULL;
    return interfaz_mje;
}

t_interfaz_mje *deserializar_io_ocupada_fs_create(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    interfaz_mje->direccion_fisica = NULL;
    interfaz_mje->reg = NULL;
    return interfaz_mje;
}

t_interfaz_mje *deserializar_io_ocupada_fs_delete(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    interfaz_mje->direccion_fisica = NULL;
    interfaz_mje->reg = NULL;
    return interfaz_mje;
}

t_interfaz_mje *deserializar_io_ocupada_fs_truncate(void *stream, int *bytes_recibidos)
{
    t_interfaz_mje *interfaz_mje = malloc(sizeof(t_interfaz_mje));
    int desplazamiento = 0;
    uint32_t tam_nombre = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_interfaz = deserializar_char(stream, &desplazamiento, tam_nombre);
    interfaz_mje->pid = deserializar_uint32(stream, &desplazamiento);
    uint32_t tam_nombre_archivo = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->nombre_archivo = deserializar_char(stream, &desplazamiento, tam_nombre_archivo);
    interfaz_mje->tamanio_truncar = deserializar_uint32(stream, &desplazamiento);
    interfaz_mje->direccion_fisica = NULL;
    interfaz_mje->reg = NULL;
    return interfaz_mje;
}
