#include "main.h"

void enviar_rta_movin(void *reg)
{
    // tambien esta operacion la supongo siempre exitosa.
    t_paquete *paq = serializar_rta_movin(reg);
    enviar_paquete(paq, socket_cpu_cliente, logger_memoria);
    log_info(logger_memoria, "envie el dato a movin cpu");
}

int traducir_direccion(uint32_t marco, uint32_t desplazamiento)
{
    return config_memoria->tam_pagina * marco + desplazamiento;
}

int atender_cliente_cpu()
{

    int cod_op = recibir_operacion(socket_cpu_cliente);
    if (cod_op == -1)
        return 1;
    int size;
    t_buffer *buffer;
    t_buffer *buff;
    t_paquete *paquete;
    t_direccion_fisica *dir_fis;
    int *tam_recibido = malloc(sizeof(int));
    t_registro_abstracto *reg;
    log_info(logger_memoria, "recibi el cod_op de cpu: %s", cod_op_to_string(cod_op));
    switch (cod_op)
    {
    case MENSAJE:
        recibir_mensaje(socket_cpu_cliente, logger_memoria);
        free(tam_recibido);
        break;
    case OBT_INSTRUCCION:
        buffer = recibir_buffer(&size, socket_cpu_cliente);
        t_obtener_instruccion *obt_instruc = deserializar_obtener_instruccion(buffer, tam_recibido);
        log_info(logger_memoria, "buscando pid: %d, pc: %d", obt_instruc->pid, obt_instruc->pc);
        char *instruccion = obtener_instruccion(dict_instrucciones, &obt_instruc->pid, &obt_instruc->pc, m_dict);
        if (instruccion == NULL)
        {
            log_error(logger_memoria, "instruccion null????");
            free(obt_instruc);
            free(tam_recibido);
            free(buffer);
            break;
        }
        log_info(logger_memoria, "INSTRCCION: %s", instruccion);
        char *sentencia = obtener_sentencia(instruccion);
        log_info(logger_memoria, "Sentencia: %s", sentencia);
        id_instruccion *id_inst = malloc(sizeof(id_instruccion));
        *id_inst = obtener_id_instruccion(sentencia);
        paquete = obtener_paquete_ret_instrucciones(id_inst, instruccion);
        usleep(config_memoria->retardo_respuesta * 1000);

        enviar_paquete(paquete, socket_cpu_cliente, logger_memoria);
        free(sentencia);
        free(obt_instruc);
        free(tam_recibido);
        free(buffer);
        break;
    case HANDSHAKE_MEMORIA:
        buffer = recibir_buffer(&size, socket_cpu_cliente);
        free(buffer);
        log_info(logger_memoria, "por enviar el tamaño de pagina: %d", config_memoria->tam_pagina);
        int tam_pagina = config_memoria->tam_pagina;
        paquete = serializar_respuesta_handshake_memoria(&tam_pagina);
        enviar_paquete(paquete, socket_cpu_cliente, logger_memoria);
        log_info(logger_memoria, "envie el tamaño de pagina: %d", config_memoria->tam_pagina);
        free(tam_recibido);
        break;
    case PEDIR_MARCO:
        buffer = recibir_buffer(&size, socket_cpu_cliente);
        t_dir_logica *dir_logica = deserializar_pedir_marco(buffer, tam_recibido);
        t_registro_tlb *registro_tlb = malloc(sizeof(t_registro_tlb));
        registro_tlb->pid = dir_logica->pid;
        registro_tlb->pagina = dir_logica->pagina;
        registro_tlb->marco = acceder_a_tabla_de_paginas(dir_logica->pid, dir_logica->pagina);
        // se descomenta para la prueba de vrr y se comenta la linea de arriba.
        // registro_tlb->marco = 1;
        t_paquete *paq = serializar_respnder_marco(registro_tlb);
        enviar_paquete(paq, socket_cpu_cliente, logger_memoria);
        free(registro_tlb);
        free(dir_logica);
        free(tam_recibido);
        free(buffer);
        break;
    case EJECUTAR_MOVIN:
        buffer = recibir_buffer(&size, socket_cpu_cliente);
        t_mov_in *movin = deserializar_ejecutar_mov_in(buffer, &size);
        t_direccion_fisica *dir = movin->dir_fisica;
        log_info(logger_memoria, "Recibi la direccion fisica pid: %d, marco: %d, desplazamiento: %d", dir->pid, dir->marco, dir->desplazamiento_fisico);
        reg = (t_registro_abstracto *)movin->datos;
        if (reg->tipo == 1)
        {
            t_registro_8 *reg8 = (t_registro_8 *)movin->datos;
            obtener_datos_de_memoria(&reg8->data, traducir_direccion(movin->dir_fisica->marco, movin->dir_fisica->desplazamiento_fisico), sizeof(uint8_t), movin->dir_fisica->pid);
            enviar_rta_movin(reg8);
            log_info(logger_memoria, "Recibi el dato: %d", reg8->data);
        }
        else
        {
            t_registro_32 *reg32 = (t_registro_32 *)movin->datos;
            obtener_datos_de_memoria(&reg32->data, traducir_direccion(movin->dir_fisica->marco, movin->dir_fisica->desplazamiento_fisico), sizeof(uint32_t), movin->dir_fisica->pid);
            enviar_rta_movin(reg32);
            log_info(logger_memoria, "Recibi el dato: %u", reg32->data);
        }
        free(movin->dir_fisica);
        free(movin->datos);
        free(movin);
        free(buffer);
        free(tam_recibido);
        break;
    case EJECUTAR_MOVOUT:
        buffer = recibir_buffer(&size, socket_cpu_cliente);
        t_mov_out *movout = deserializar_ejecutar_mov_out(buffer, &size);
        dir_fis = movout->dir_fisica;
        log_info(logger_memoria, "Recibi la direccion fisica pid: %d, Marco: %d, desplazamiento: %d", dir_fis->pid, dir_fis->marco, dir_fis->desplazamiento_fisico);

        reg = (t_registro_abstracto *)movout->datos;
        if (reg->tipo == 1)
        {
            t_registro_8 *reg8 = (t_registro_8 *)movout->datos;
            log_info(logger_memoria, "Recibi el dato: %u", reg8->data); // hay un warn aca abajo
            agregar_datos_a_memoria(&reg8->data, traducir_direccion(movout->dir_fisica->marco, movout->dir_fisica->desplazamiento_fisico), sizeof(uint8_t), movout->dir_fisica->pid);
        }
        else
        {
            t_registro_32 *reg32 = (t_registro_32 *)movout->datos;
            log_info(logger_memoria, "Recibi el dato: %u", reg32->data);
            agregar_datos_a_memoria(&reg32->data, traducir_direccion(movout->dir_fisica->marco, movout->dir_fisica->desplazamiento_fisico), sizeof(uint32_t), movout->dir_fisica->pid);
        }
        buff = crear_buffer();
        paquete = crear_paquete(buff, EJECUTAR_MOVOUT);
        agregar_var_a_paquete(paquete, &movout->dir_fisica->pid, sizeof(uint32_t));
        enviar_paquete(paquete, socket_cpu_cliente, logger_memoria);
        free(movout->dir_fisica);
        free(movout->datos);
        free(movout);
        free(tam_recibido);
        free(buffer);
        break;

    case EJECUTAR_COPYSTRING:
        buffer = recibir_buffer(&size, socket_cpu_cliente);
        t_copy_string *copystring = deserializar_ejecutar_copy_string(buffer, &size);
        log_info(logger_memoria, "Recibi la el tamanio: %d, si: %d, di: %d, pid: %d", copystring->tamanio, copystring->si, copystring->di, copystring->pid);
        copy_string(copystring->di, copystring->si, copystring->tamanio, copystring->pid);
        buff = crear_buffer();
        paquete = crear_paquete(buff, EJECUTAR_COPYSTRING);
        agregar_var_a_paquete(paquete, &copystring->pid, sizeof(uint32_t));
        enviar_paquete(paquete, socket_cpu_cliente, logger_memoria);
        free(tam_recibido);
        free(copystring);
        free(buffer);
        break;
    case EJECUTAR_RESIZE:
        buffer = recibir_buffer(&size, socket_cpu_cliente);
        t_resize *resize = deserializar_ejecutar_resize(buffer, &size);
        int resultado = resize_proceso(resize->pid, resize->tamanio);
        op_code codigo = resultado == 1 ? EJECUTAR_RESIZE_OUT_OF_MEMORY_ERROR : EJECUTAR_RESIZE;
        buff = crear_buffer();
        paquete = crear_paquete(buff, codigo);
        agregar_var_a_paquete(paquete, &resize->pid, sizeof(uint32_t));
        enviar_paquete(paquete, socket_cpu_cliente, logger_memoria);
        free(tam_recibido);
        free(resize);
        free(buffer);
        break;
    case -1:
        free(tam_recibido);
        return 1;
    default:
        free(tam_recibido);
        break;
    }
    return 0;
}