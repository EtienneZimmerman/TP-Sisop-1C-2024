#include "cpu.h"

bool wait_o_signal = false;

void fetch(uint32_t *pid, uint32_t *pc)
{
    if (*pid != 0)
    {
        t_paquete *paquete = obtener_paquete_instrucciones(pid, pc);
        enviar_paquete(paquete, socket_memoria, logger_cpu);
        log_info(logger_cpu, "Peticion de instruccion a memoria");
        log_info(logger_obligatorio_cpu, "PID: <%u> - FETCH - Program Counter: <%u>", *pid, *pc);
    }
}

void *fetch_en_new_thread()
{
    fetch(&proceso_ejecutando->pid, &proceso_ejecutando->pc);
    return "";
}

bool registro_es_8(char *registro)
{
    return strcmp("AX", registro) == 0 || strcmp("BX", registro) == 0 || strcmp("CX", registro) == 0 || strcmp("DX", registro) == 0;
}

int enviar_cde(uint32_t *pid, char *recurso, motivo_desalojo motivo)
{
    log_info(logger_cpu, "PID_exec: %d PID_desalojar: %d", proceso_ejecutando->pid, *pid);
    sem_wait(mutex_proceso_ejecutando);
    if (proceso_ejecutando->pid == *pid)
    {
        log_info(logger_cpu, "POR SERIALIZAR RECURSO:  %s", recurso);
        t_paquete *paquete_contexto = serializar_contexto_ejecucion(proceso_ejecutando, motivo, recurso);
        enviar_paquete(paquete_contexto, socket_kernel_dispatch, logger_cpu);
        sem_post(mutex_proceso_ejecutando);
        return 1;
    }
    else
    {
        sem_post(mutex_proceso_ejecutando);
    }
    return 0;
}

void execute(t_obtener_ret_instruccion *obt_ret_inst)
{
    char **params = obtener_parametros(obt_ret_inst->instruccion);
    char **copy_params = params;
    params++;
    char *registro_destino;
    char *valor;
    char *registro_origen;
    char *registro;
    char *instruccion;
    char *interfaz;
    char *unidades_de_trabajo;
    char *nombre_archivo;
    char *registro_tamaño;
    switch (obt_ret_inst->id_inst)
    {
    case SET:
        registro_destino = *params;
        params++;
        valor = *params;
        log_info(logger_cpu, "por ejecutar set del registro: %s, valor: %s", registro_destino, valor);
        sem_wait(mutex_registros);
        setear_registro(registros, registro_destino, valor);
        sem_post(mutex_registros);
        mostrar_registros(registros);
        if (strcmp(registro_destino, "PC") != 0)
        {
            incrementar_pc(registros);
        }
        escribir_pcb(proceso_ejecutando, registros);
        break;
    case SUM:
        registro_destino = *params;
        params++;
        registro_origen = *params;
        log_info(logger_cpu, "por ejecutar sum del registro: %s, origen: %s", registro_destino, registro_origen);
        sem_wait(mutex_registros);
        sumar_registro(registros, registro_destino, registro_origen);
        sem_post(mutex_registros);
        mostrar_registros(registros);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        break;
    case SUB:
        registro_destino = *params;
        params++;
        registro_origen = *params;
        log_info(logger_cpu, "por ejecutar sub del registro: %s, origen: %s", registro_destino, registro_origen);
        sem_wait(mutex_registros);
        restar_registro(registros, registro_destino, registro_origen);
        sem_post(mutex_registros);
        mostrar_registros(registros);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        break;
    case JNZ:
        registro = *params;
        params++;
        instruccion = *params;
        sem_wait(mutex_registros);
        saltar_si_no_es_cero(registros, registro, instruccion);
        sem_post(mutex_registros);
        mostrar_registros(registros);
        escribir_pcb(proceso_ejecutando, registros);
        break;
    case IO_GEN_SLEEP:
        interfaz = *params;
        params++;
        unidades_de_trabajo = *params;
        uint32_t *udt = malloc(sizeof(uint32_t));
        *udt = 0;
        int udt_int = atoi(unidades_de_trabajo);
        log_info(logger_cpu, "udt_int: %d", udt_int);
        while (udt_int > 0)
        {
            *udt += 1;
            udt_int--;
        }
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        uint32_t *motivo = malloc(sizeof(uint32_t));
        *motivo = IO;
        t_paquete *paquete = obtener_paquete_peticion_io_gen_sleep(interfaz, udt, proceso_ejecutando, motivo);
        enviar_paquete(paquete, socket_kernel_dispatch, logger_cpu);
        sem_wait(mutex_proceso_ejecutando);
        proceso_ejecutando->pid = 0;
        sem_post(mutex_proceso_ejecutando);
        free(motivo);
        log_info(logger_cpu, "Peticion de instruccion IO_GEN_SLEEP a kernel, interfaz: %s, unidades de trabajo: %s, udt: %i", interfaz, unidades_de_trabajo, *udt);
        free(udt);
        break;
    case WAIT:
        char *recurso_wait = *params;
        log_info(logger_cpu, "Ejecutando WAIT para el recurso: %s", recurso_wait);

        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);

        // Desalojar el proceso con el motivo MOTIVO_WAIT
        if (enviar_cde(&proceso_ejecutando->pid, recurso_wait, MOTIVO_WAIT))
        {
            log_info(logger_cpu, "Proceso desalojado después de ejecutar WAIT");
        }
        else
        {
            log_error(logger_cpu, "Error al desalojar el proceso después de ejecutar WAIT");
        }

        wait_o_signal = true;
        break;
    case SIGNAL:
        char *recurso_signal = *params;
        log_info(logger_cpu, "Ejecutando SIGNAL para el recurso: %s", recurso_signal);

        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        // Desalojar el proceso con el motivo MOTIVO_SIGNAL
        if (enviar_cde(&proceso_ejecutando->pid, recurso_signal, MOTIVO_SIGNAL))
        {
            log_info(logger_cpu, "Proceso desalojado después de ejecutar SIGNAL");
        }
        else
        {
            log_error(logger_cpu, "Error al desalojar el proceso después de ejecutar SIGNAL");
        }

        wait_o_signal = true;
        break;
    case IO_FS_DELETE:
        interfaz = *params;
        params++;
        nombre_archivo = *params;
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        uint32_t *motivo1;
        motivo1 = malloc(sizeof(uint32_t));
        *motivo1 = IO;
        t_paquete *paq;
        paq = obtener_paquete_peticion_io_fs_delete(interfaz, nombre_archivo, proceso_ejecutando, motivo1);
        enviar_paquete(paq, socket_kernel_dispatch, logger_cpu);
        sem_wait(mutex_proceso_ejecutando);
        proceso_ejecutando->pid = 0;
        sem_post(mutex_proceso_ejecutando);
        free(motivo1);
        log_info(logger_cpu, "Peticion de instruccion IO_FS_DELETE a kernel, interfaz: %s, nombre_archivo: %s", interfaz, nombre_archivo);
        break;
    case IO_FS_CREATE:
        interfaz = *params;
        params++;
        nombre_archivo = *params;
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        uint32_t *motivo2;
        motivo2 = malloc(sizeof(uint32_t));
        *motivo2 = IO;
        t_paquete *paquete1;
        paquete1 = obtener_paquete_peticion_io_fs_create(interfaz, nombre_archivo, proceso_ejecutando, motivo2);
        enviar_paquete(paquete1, socket_kernel_dispatch, logger_cpu);
        sem_wait(mutex_proceso_ejecutando);
        proceso_ejecutando->pid = 0;
        sem_post(mutex_proceso_ejecutando);
        free(motivo2);
        log_info(logger_cpu, "Peticion de instruccion IO_FS_CREATE a kernel, interfaz: %s, nombre_archivo: %s", interfaz, nombre_archivo);
        break;
    case IO_FS_TRUNCATE:
        interfaz = *params;
        params++;
        nombre_archivo = *params;
        params++;
        registro_tamaño = *params;
        void *reg2 = obtener_t_registro(registro_tamaño);
        uint32_t data2 = 0;
        uint32_t dat2 = 0;
        if (((t_registro_abstracto *)reg2)->tipo == 1)
        {
            t_registro_8 *reg8_ = (t_registro_8 *)reg2;
            while (reg8_->data > 0)
            {
                dat2++;
                reg8_->data--;
            }
            data2 = dat2;
            log_info(logger_cpu, "8! + data: %d", data2);
        }
        else
        {
            t_registro_32 *reg32_ = (t_registro_32 *)reg2;
            log_info(logger_cpu, "32! + data: %d", reg32_->data);
            data2 = reg32_->data;
        }
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        uint32_t *motivo3;
        motivo3 = malloc(sizeof(uint32_t));
        *motivo3 = IO;
        t_paquete *paquete2;
        paquete2 = obtener_paquete_peticion_io_fs_truncate(interfaz, nombre_archivo, data2, proceso_ejecutando, motivo3);
        enviar_paquete(paquete2, socket_kernel_dispatch, logger_cpu);
        sem_wait(mutex_proceso_ejecutando);
        proceso_ejecutando->pid = 0;
        sem_post(mutex_proceso_ejecutando);
        free(reg2);
        free(motivo3);
        log_info(logger_cpu, "Peticion de instruccion IO_FS_TRUNCATE a kernel, interfaz: %s, nombre_archivo: %s, registro_tamaño: %s, registro_valor: %d", interfaz, nombre_archivo, registro_tamaño, data2);
        break;
    case EXIT:
        log_info(logger_cpu, "Ejecutando EXIT para el proceso: %d", proceso_ejecutando->pid);
        int esperar_recepcion = finalizar_proceso(&proceso_ejecutando->pid);
        if (esperar_recepcion == 1)
        {
            sem_wait(mutex_proceso_ejecutando);
            proceso_ejecutando->pid = 0;
            sem_post(mutex_proceso_ejecutando);
        }
        break;

    default:
        break;
    }
    params = copy_params;
    for (; *params != NULL; params++)
    {
        free(*params);
    }
    free(copy_params);
}

void ejecutar_resize(uint32_t tamanio)
{
    t_resize *resize = malloc(sizeof(t_resize));
    resize->pid = proceso_ejecutando->pid;
    if (tamanio == 0)
    {
        limpiar_tlb(resize->pid);
    }
    resize->tamanio = tamanio;
    t_paquete *paquete = serializar_ejecutar_resize(resize);
    enviar_paquete(paquete, socket_memoria, logger_cpu);
    free(resize);
    sem_wait(sem_esperar_respuesta);
    int resultado = *((int *)rta_memoria);

    if (resultado == 1)
    {
        log_info(logger_cpu, "No se pudo realizar el resize");
        sem_wait(mutex_proceso_ejecutando);
        proceso_ejecutando->pid = 0;
        sem_post(mutex_proceso_ejecutando);
    }
    else
    {
        log_info(logger_cpu, "Se realizo el resize");
    }
    free(rta_memoria);
}

void ejecutar_mov_in(char *registro_datos, t_direccion_fisica *direccion_fisica)
{
    t_mov_in *movin = malloc(sizeof(t_mov_in));
    movin->datos = obtener_t_registro(registro_datos);
    movin->dir_fisica = direccion_fisica;
    t_paquete *paquete = serializar_ejecutar_mov_in(movin);
    enviar_paquete(paquete, socket_memoria, logger_cpu);
    sem_wait(sem_esperar_respuesta);

    if (registro_es_8(registro_datos))
    {
        uint8_t *reg = obtener_registro(registros, registro_datos);
        *reg = *((uint8_t *)rta_memoria);
        log_info(logger_obligatorio_cpu, "PID: <%d> - Acción: <LEER> - Dirección Física: <%u | %u> - Valor: <%u>", proceso_ejecutando->pid, direccion_fisica->marco, direccion_fisica->desplazamiento_fisico, *reg);
    }
    else
    {
        uint32_t *reg = obtener_registro(registros, registro_datos);
        *reg = *((uint32_t *)rta_memoria);
        log_info(logger_obligatorio_cpu, "PID: <%d> - Acción: <LEER> - Dirección Física: <%u | %u> - Valor: <%u>", proceso_ejecutando->pid, direccion_fisica->marco, direccion_fisica->desplazamiento_fisico, *reg);
    }

    free(movin->datos);
    free(movin);
    free(rta_memoria);
}

void ejecutar_mov_out(char *registro_datos, t_direccion_fisica *direccion_fisica)
{
    t_mov_out *mov_out = malloc(sizeof(t_mov_out));
    mov_out->dir_fisica = direccion_fisica;
    mov_out->datos = obtener_t_registro(registro_datos);
    t_paquete *paquete = serializar_ejecutar_mov_out(mov_out);
    enviar_paquete(paquete, socket_memoria, logger_cpu);
    log_info(logger_cpu, "Ejecute mov_out");

    if (registro_es_8(registro_datos))
    {
        t_registro_8 *valor = ((t_registro_8 *)mov_out->datos);
        log_info(logger_obligatorio_cpu, "PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%u | %u> - Valor: <%u>", proceso_ejecutando->pid, direccion_fisica->marco, direccion_fisica->desplazamiento_fisico, valor->data);
    }
    else
    {
        t_registro_32 *valor = ((t_registro_32 *)mov_out->datos);
        log_info(logger_obligatorio_cpu, "PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%u | %u> - Valor: <%u>", proceso_ejecutando->pid, direccion_fisica->marco, direccion_fisica->desplazamiento_fisico, valor->data);
    }
    free(mov_out->datos);
    free(mov_out);
    sem_wait(sem_esperar_respuesta);
}

void ejecutar_copy_string(char *tamanio, t_direccion_fisica *di, t_direccion_fisica *si)
{
    uint32_t tamanio_int = 0;
    sscanf(tamanio, "%u", &tamanio_int);
    t_copy_string *copy_string = malloc(sizeof(t_copy_string));
    copy_string->pid = di->pid;
    copy_string->di = di->marco * (*tam_pagina) + di->desplazamiento_fisico;
    copy_string->si = si->marco * (*tam_pagina) + si->desplazamiento_fisico;
    copy_string->tamanio = tamanio_int;
    t_paquete *paquete = serializar_ejecutar_copy_string(copy_string);
    enviar_paquete(paquete, socket_memoria, logger_cpu);
    log_info(logger_cpu, "Ejecute copy_string");
    free(copy_string);
    sem_wait(sem_esperar_respuesta);
}

void ejecutar_io_stdin_read(char *interfaz, t_direccion_fisica *direccion_fisica, char *registro_tamaño)
{

    t_io_stdin_read *stdin_read = malloc(sizeof(t_io_stdin_read));
    stdin_read->interfaz = interfaz;
    stdin_read->dir_fisica = direccion_fisica;
    stdin_read->tamanio = obtener_t_registro(registro_tamaño);
    incrementar_pc(registros);
    escribir_pcb(proceso_ejecutando, registros);
    uint32_t *motivo = malloc(sizeof(uint32_t));
    *motivo = IO;
    t_paquete *paquete = serializar_peticion_io_stdin_read(interfaz, proceso_ejecutando, motivo, stdin_read);
    enviar_paquete(paquete, socket_kernel_dispatch, logger_cpu);
    sem_wait(mutex_proceso_ejecutando);
    proceso_ejecutando->pid = 0;
    sem_post(mutex_proceso_ejecutando);
    free(stdin_read->tamanio);
    free(stdin_read);
    free(motivo);
    log_info(logger_cpu, "Peticion de instruccion IO_STDIN_READ a kernel, interfaz: %s", interfaz);
    log_info(logger_cpu, "Ejecute stdin");
}

void ejecutar_io_stdout_write(char *interfaz, t_direccion_fisica *direccion_fisica, char *registro_tamaño)
{
    t_io_stdout_write *io_stdout = malloc(sizeof(t_io_stdout_write));
    io_stdout->interfaz = interfaz;
    io_stdout->dir_fisica = direccion_fisica;
    void *reg = obtener_t_registro(registro_tamaño);
    uint32_t *data = malloc(sizeof(uint32_t));
    *data = 0;
    uint32_t dat = 0;
    if (((t_registro_abstracto *)reg)->tipo == 1)
    {
        t_registro_8 *reg8_ = (t_registro_8 *)reg;
        while (reg8_->data > 0)
        {
            dat++;
            reg8_->data--;
        }
        *data = dat;
        io_stdout->tamanio = *data;
        log_info(logger_cpu, "8! + data: %d", *data);
    }
    else
    {
        t_registro_32 *reg32_ = (t_registro_32 *)reg;
        log_info(logger_cpu, "32! + data: %d", reg32_->data);
        *data = reg32_->data;
        io_stdout->tamanio = *data;
    }
    log_info(logger_cpu, "Mostrando el tamaño elegido del registro %s: %d", registro_tamaño, io_stdout->tamanio);

    incrementar_pc(registros);
    escribir_pcb(proceso_ejecutando, registros);
    uint32_t *motivo = malloc(sizeof(uint32_t));
    *motivo = IO;
    t_paquete *paquete = serializar_peticion_io_stdout_write(interfaz, proceso_ejecutando, motivo, io_stdout, data);
    enviar_paquete(paquete, socket_kernel_dispatch, logger_cpu);
    sem_wait(mutex_proceso_ejecutando);
    proceso_ejecutando->pid = 0;
    sem_post(mutex_proceso_ejecutando);
    free(motivo);
    free(data);
    free(reg);
    free(io_stdout);
    log_info(logger_cpu, "Peticion de instruccion IO_STDOUT_WRITE a kernel, interfaz: %s", interfaz);
    log_info(logger_cpu, "Ejecute stdout");
}

void ejecutar_io_fs_write(char *interfaz, char *nombre_archivo, t_direccion_fisica *dir, char *registro_tamaño, char *registro_puntero_arch)
{

    incrementar_pc(registros);
    escribir_pcb(proceso_ejecutando, registros);
    uint32_t *motivo = malloc(sizeof(uint32_t));
    *motivo = IO;
    void *reg = obtener_t_registro(registro_tamaño);
    uint32_t tamanio = 0;
    uint32_t dat = 0;
    if (((t_registro_abstracto *)reg)->tipo == 1)
    {
        t_registro_8 *reg8_ = (t_registro_8 *)reg;
        while (reg8_->data > 0)
        {
            dat++;
            reg8_->data--;
        }
        tamanio = dat;
        log_info(logger_cpu, "8! + data: %d", tamanio);
    }
    else
    {
        t_registro_32 *reg32_ = (t_registro_32 *)reg;
        tamanio = reg32_->data;
        log_info(logger_cpu, "32! + data: %d", tamanio);
    }

    void *reg2 = obtener_t_registro(registro_puntero_arch);
    uint32_t posicion_archivo = 0;
    uint32_t dat2 = 0;
    if (((t_registro_abstracto *)reg2)->tipo == 1)
    {
        t_registro_8 *reg8_ = (t_registro_8 *)reg2;
        while (reg8_->data > 0)
        {
            dat2++;
            reg8_->data--;
        }
        posicion_archivo = dat2;
        log_info(logger_cpu, "8! + data: %d", posicion_archivo);
    }
    else
    {
        t_registro_32 *reg32_ = (t_registro_32 *)reg2;
        posicion_archivo = reg32_->data;
        log_info(logger_cpu, "32! + data: %d", posicion_archivo);
    }

    t_paquete *paquete = obtener_paquete_peticion_io_fs_write(interfaz, nombre_archivo, tamanio, posicion_archivo, dir, proceso_ejecutando, motivo);
    free(reg);
    free(reg2);
    enviar_paquete(paquete, socket_kernel_dispatch, logger_cpu);
    sem_wait(mutex_proceso_ejecutando);
    proceso_ejecutando->pid = 0;
    sem_post(mutex_proceso_ejecutando);
    free(motivo);
    log_info(logger_cpu, "Peticion de instruccion IO_FS_WRITE a kernel, interfaz: %s", interfaz);
}

void ejecutar_io_fs_read(char *interfaz, char *nombre_archivo, t_direccion_fisica *dir, char *registro_tamaño, char *registro_puntero_arch)
{

    incrementar_pc(registros);
    escribir_pcb(proceso_ejecutando, registros);
    uint32_t *motivo = malloc(sizeof(uint32_t));
    *motivo = IO;
    void *reg = obtener_t_registro(registro_tamaño);
    uint32_t tamanio = 0;
    uint32_t dat = 0;
    if (((t_registro_abstracto *)reg)->tipo == 1)
    {
        t_registro_8 *reg8_ = (t_registro_8 *)reg;
        while (reg8_->data > 0)
        {
            dat++;
            reg8_->data--;
        }
        tamanio = dat;
        log_info(logger_cpu, "8! + data: %d", tamanio);
    }
    else
    {
        t_registro_32 *reg32_ = (t_registro_32 *)reg;
        tamanio = reg32_->data;
        log_info(logger_cpu, "32! + data: %d", tamanio);
    }

    void *reg2 = obtener_t_registro(registro_puntero_arch);
    uint32_t posicion_archivo = 0;
    uint32_t dat2 = 0;
    if (((t_registro_abstracto *)reg2)->tipo == 1)
    {
        t_registro_8 *reg8_ = (t_registro_8 *)reg2;
        while (reg8_->data > 0)
        {
            dat2++;
            reg8_->data--;
        }
        posicion_archivo = dat2;
        log_info(logger_cpu, "8! + data: %d", posicion_archivo);
    }
    else
    {
        t_registro_32 *reg32_ = (t_registro_32 *)reg2;
        posicion_archivo = reg32_->data;
        log_info(logger_cpu, "32! + data: %d", posicion_archivo);
    }

    t_paquete *paquete = obtener_paquete_peticion_io_fs_read(interfaz, nombre_archivo, tamanio, posicion_archivo, dir, proceso_ejecutando, motivo);
    enviar_paquete(paquete, socket_kernel_dispatch, logger_cpu);
    sem_wait(mutex_proceso_ejecutando);
    proceso_ejecutando->pid = 0;
    sem_post(mutex_proceso_ejecutando);
    free(motivo);
    free(reg);
    free(reg2);
    log_info(logger_cpu, "Peticion de instruccion IO_FS_READ a kernel, interfaz: %s", interfaz);
}

char *traducir_direccion(char *direccion_logica)
{
    return "";
}

void *decode(void *obt_ret_inst_void)
{
    t_obtener_ret_instruccion *obt_ret_inst = (t_obtener_ret_instruccion *)obt_ret_inst_void;
    log_info(logger_cpu, "Decodificando la instruccion de id: %d, instruccion: %s", obt_ret_inst->id_inst, obt_ret_inst->instruccion);
    log_info(logger_obligatorio_cpu, "PID: <%d> - Ejecutando: <%s>", proceso_ejecutando->pid, obt_ret_inst->instruccion);
    char **params;
    params = obtener_parametros(obt_ret_inst->instruccion);
    char **copy_params = params;
    params++;
    char *registro_datos;
    char *registro_direccion;
    char *tamanio;
    char *interfaz;
    char *registro_tamaño;
    char *nombre_archivo;
    char *registro_puntero_arch;
    t_direccion_fisica *direccion_fisica;
    switch (obt_ret_inst->id_inst)
    {
    case RESIZE:
        tamanio = *params;
        log_info(logger_cpu, "Por ejecutar resize con tamanio: %s", tamanio);
        uint32_t tamanio_int = atoi(tamanio);
        ejecutar_resize(tamanio_int);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        break;
    case MOV_IN:
        registro_datos = *params;
        params++;
        registro_direccion = *params;
        direccion_fisica = traducir_direccion_a_fisica(proceso_ejecutando->pid, registro_direccion);
        ejecutar_mov_in(registro_datos, direccion_fisica);
        mostrar_registros(registros);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        free(direccion_fisica);
        break;
    case MOV_OUT:
        registro_direccion = *params;
        params++;
        registro_datos = *params;
        direccion_fisica = traducir_direccion_a_fisica(proceso_ejecutando->pid, registro_direccion);
        ejecutar_mov_out(registro_datos, direccion_fisica);
        mostrar_registros(registros);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        free(direccion_fisica);
        break;
    case COPY_STRING:
        tamanio = *params;
        direccion_fisica = traducir_direccion_a_fisica(proceso_ejecutando->pid, "DI");
        t_direccion_fisica *direccion_fisica_si = traducir_direccion_a_fisica(proceso_ejecutando->pid, "SI");
        ejecutar_copy_string(tamanio, direccion_fisica, direccion_fisica_si);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        free(direccion_fisica);
        free(direccion_fisica_si);
        break;
    case IO_STDIN_READ:
        interfaz = *params;
        params++;
        registro_direccion = *params;
        params++;
        registro_tamaño = *params;
        direccion_fisica = traducir_direccion_a_fisica(proceso_ejecutando->pid, registro_direccion);
        ejecutar_io_stdin_read(interfaz, direccion_fisica, registro_tamaño);
        free(direccion_fisica);
        break;
    case IO_STDOUT_WRITE:
        interfaz = *params;
        params++;
        registro_direccion = *params;
        params++;
        registro_tamaño = *params;
        direccion_fisica = traducir_direccion_a_fisica(proceso_ejecutando->pid, registro_direccion);
        ejecutar_io_stdout_write(interfaz, direccion_fisica, registro_tamaño);
        free(direccion_fisica);
        break;
    case IO_FS_READ:
        interfaz = *params;
        params++;
        nombre_archivo = *params;
        params++;
        registro_direccion = *params;
        params++;
        registro_tamaño = *params;
        params++;
        registro_puntero_arch = *params;
        direccion_fisica = traducir_direccion_a_fisica(proceso_ejecutando->pid, registro_direccion);
        ejecutar_io_fs_read(interfaz, nombre_archivo, direccion_fisica, registro_tamaño, registro_puntero_arch);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        free(direccion_fisica);
        break;
    case IO_FS_WRITE:
        interfaz = *params;
        params++;
        nombre_archivo = *params;
        params++;
        registro_direccion = *params;
        params++;
        registro_tamaño = *params;
        params++;
        registro_puntero_arch = *params;
        direccion_fisica = traducir_direccion_a_fisica(proceso_ejecutando->pid, registro_direccion);
        ejecutar_io_fs_write(interfaz, nombre_archivo, direccion_fisica, registro_tamaño, registro_puntero_arch);
        incrementar_pc(registros);
        escribir_pcb(proceso_ejecutando, registros);
        free(direccion_fisica);
        break;
    default:
        log_info(logger_cpu, "Procedo con el execute de la instruccion");
        execute(obt_ret_inst);
        break;
    }

    // Permito interrumpir
    if (wait_o_signal)
    {
        recibir_pcb_nuevo();
    }

    wait_o_signal = false;
    pthread_mutex_lock(&m_interrupciones);
    if (list_size(interrupciones) > 0)
    {
        t_interrupcion *inter = list_remove(interrupciones, 0);
        sem_wait(mutex_proceso_ejecutando);
        if (proceso_ejecutando->pid == *inter->pid && proceso_ejecutando->pid != 0)
        {
            t_paquete *paquete_contexto = serializar_contexto_ejecucion(proceso_ejecutando, inter->motivo, NULL);
            enviar_paquete(paquete_contexto, socket_kernel_dispatch, logger_cpu);
            free(inter->pid);
            free(inter);
            proceso_ejecutando->pid = 0;
        }
        else
        {
            free(inter->pid);
            free(inter);
        }
        sem_post(mutex_proceso_ejecutando);
    }
    pthread_mutex_unlock(&m_interrupciones);

    sem_wait(mutex_proceso_ejecutando);
    if (proceso_ejecutando->pid == 0)
    {
        sem_post(mutex_proceso_ejecutando);
        recibir_pcb_nuevo();
    }
    else
    {
        sem_post(mutex_proceso_ejecutando);
    }

    fetch(&proceso_ejecutando->pid, &proceso_ejecutando->pc);

    free(obt_ret_inst->instruccion);
    params = copy_params;
    for (; *params != NULL; params++)
    {
        free(*params);
    }
    free(copy_params);
    free(obt_ret_inst);
    return "";
}
