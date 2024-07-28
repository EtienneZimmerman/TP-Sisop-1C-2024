#include "instruccion.h"

char *obtener_instruccion(t_dictionary *dict_instrucciones, uint32_t *pid, uint32_t *pc, pthread_mutex_t m_dict)
{
    pthread_mutex_lock(&m_dict);
    char *pid_str = string_itoa(*pid);
    t_list *instrucciones = dictionary_get(dict_instrucciones, pid_str);
    pthread_mutex_unlock(&m_dict);
    free(pid_str);
    if (instrucciones != NULL)
    {
        return list_get(instrucciones, *pc);
    }
    return NULL;
}

char *obtener_sentencia(char *instruccion)
{
    char **instruccion_split = string_n_split(instruccion, 10, " ");
    char *ret = malloc(sizeof(char) * string_length(*instruccion_split) + 1);
    memcpy(ret, *instruccion_split + '\0', string_length(*instruccion_split) + 1);
    char **split_copy = instruccion_split;
    for (; *instruccion_split != NULL; instruccion_split++)
    {
        free(*instruccion_split);
    }
    free(split_copy);
    return ret;
}

id_instruccion obtener_id_instruccion(char *sentencia)
{

    if (!strcmp(sentencia, "SET"))
    {
        return SET;
    }
    else if (!strcmp(sentencia, "MOV_IN"))
    {
        return MOV_IN;
    }
    else if (!strcmp(sentencia, "MOV_OUT"))
    {
        return MOV_OUT;
    }
    else if (!strcmp(sentencia, "SUM"))
    {
        return SUM;
    }
    else if (!strcmp(sentencia, "SUB"))
    {
        return SUB;
    }
    else if (!strcmp(sentencia, "JNZ"))
    {
        return JNZ;
    }
    else if (!strcmp(sentencia, "RESIZE"))
    {
        return RESIZE;
    }
    else if (!strcmp(sentencia, "COPY_STRING"))
    {
        return COPY_STRING;
    }
    else if (!strcmp(sentencia, "WAIT"))
    {
        return WAIT;
    }
    else if (!strcmp(sentencia, "SIGNAL"))
    {
        return SIGNAL;
    }
    else if (!strcmp(sentencia, "IO_GEN_SLEEP"))
    {
        return IO_GEN_SLEEP;
    }
    else if (!strcmp(sentencia, "IO_STDIN_READ"))
    {
        return IO_STDIN_READ;
    }
    else if (!strcmp(sentencia, "IO_STDOUT_WRITE"))
    {
        return IO_STDOUT_WRITE;
    }
    else if (!strcmp(sentencia, "IO_FS_CREATE"))
    {
        return IO_FS_CREATE;
    }
    else if (!strcmp(sentencia, "IO_FS_DELETE"))
    {
        return IO_FS_DELETE;
    }
    else if (!strcmp(sentencia, "IO_FS_TRUNCATE"))
    {
        return IO_FS_TRUNCATE;
    }
    else if (!strcmp(sentencia, "IO_FS_WRITE"))
    {
        return IO_FS_WRITE;
    }
    else if (!strcmp(sentencia, "IO_FS_READ"))
    {
        return IO_FS_READ;
    }
    else
    {
        return EXIT;
    }
}

char **obtener_parametros(char *instruccion)
{
    char **instruccion_split = string_n_split(instruccion, 10, " ");
    return instruccion_split;
}

void *obtener_registro(t_registros *registros, char *registro)
{
    if (!strcmp(registro, "AX"))
    {
        return &registros->ax;
    }
    else if (!strcmp(registro, "BX"))
    {
        return &registros->bx;
    }
    else if (!strcmp(registro, "CX"))
    {
        return &registros->cx;
    }
    else if (!strcmp(registro, "DX"))
    {
        return &registros->dx;
    }
    else if (!strcmp(registro, "EAX"))
    {
        return &registros->eax;
    }
    else if (!strcmp(registro, "EBX"))
    {
        return &registros->ebx;
    }
    else if (!strcmp(registro, "ECX"))
    {
        return &registros->ecx;
    }
    else if (!strcmp(registro, "EDX"))
    {
        return &registros->edx;
    }
    else if (!strcmp(registro, "SI"))
    {
        return &registros->si;
    }
    else if (!strcmp(registro, "DI"))
    {
        return &registros->di;
    }
    else if (!strcmp(registro, "PC"))
    {
        return &registros->pc;
    }
    return 0;
}

void setear_registro(t_registros *registros, char *registro_destino, char *valor)
{
    if (!strcmp(registro_destino, "AX") || !strcmp(registro_destino, "BX") || !strcmp(registro_destino, "CX") || !strcmp(registro_destino, "DX"))
    {
        uint8_t *registro = obtener_registro(registros, registro_destino);
        *registro = atoi(valor);
    }
    else
    {
        uint32_t *registro = obtener_registro(registros, registro_destino);
        *registro = atoi(valor);
    }
}

void sumar_registro(t_registros *registros, char *registro_destino, char *registro_origen)
{
    if (!strcmp(registro_destino, "AX") || !strcmp(registro_destino, "BX") || !strcmp(registro_destino, "CX") || !strcmp(registro_destino, "DX"))
    {
        uint8_t *reg = obtener_registro(registros, registro_destino);
        if (!strcmp(registro_origen, "AX") || !strcmp(registro_origen, "BX") || !strcmp(registro_origen, "CX") || !strcmp(registro_origen, "DX"))
        {
            uint8_t *reg_orig = obtener_registro(registros, registro_origen);
            *reg = *reg + (*reg_orig);
        }
        else
        {
            uint32_t *reg_orig32 = obtener_registro(registros, registro_origen);
            uint32_t valor = *reg_orig32;
            uint8_t a_sumar = 0;
            while (valor > 0)
            {
                a_sumar++;
                valor--;
            }
            *reg += a_sumar;
        }
    }
    else
    {
        uint32_t *reg_dest = obtener_registro(registros, registro_destino);
        if (!strcmp(registro_origen, "AX") || !strcmp(registro_origen, "BX") || !strcmp(registro_origen, "CX") || !strcmp(registro_origen, "DX"))
        {
            uint8_t *reg_orig8 = obtener_registro(registros, registro_origen);
            uint8_t valor = *reg_orig8;
            uint32_t a_sumar = 0;
            while (valor > 0)
            {
                a_sumar++;
                valor--;
            }
            *reg_dest += a_sumar;
        }
        else
        {
            uint32_t *reg_orig32 = obtener_registro(registros, registro_origen);
            *reg_dest += *(reg_orig32);
        }
    }
}

void restar_registro(t_registros *registros, char *registro_destino, char *registro_origen)
{
    if (!strcmp(registro_destino, "AX") || !strcmp(registro_destino, "BX") || !strcmp(registro_destino, "CX") || !strcmp(registro_destino, "DX"))
    {
        uint8_t *reg = obtener_registro(registros, registro_destino);
        if (!strcmp(registro_origen, "AX") || !strcmp(registro_origen, "BX") || !strcmp(registro_origen, "CX") || !strcmp(registro_origen, "DX"))
        {
            uint8_t *reg_orig = obtener_registro(registros, registro_origen);
            *reg = *reg - (*reg_orig);
        }
        else
        {
            uint32_t *reg_orig32 = obtener_registro(registros, registro_origen);
            uint32_t valor = *reg_orig32;
            uint8_t a_sumar = 0;
            while (valor > 0)
            {
                a_sumar++;
                valor--;
            }
            *reg -= a_sumar;
        }
    }
    else
    {
        uint32_t *reg_dest = obtener_registro(registros, registro_destino);
        if (!strcmp(registro_origen, "AX") || !strcmp(registro_origen, "BX") || !strcmp(registro_origen, "CX") || !strcmp(registro_origen, "DX"))
        {
            uint8_t *reg_orig8 = obtener_registro(registros, registro_origen);
            uint8_t valor = *reg_orig8;
            uint32_t a_sumar = 0;
            while (valor > 0)
            {
                a_sumar++;
                valor--;
            }
            *reg_dest -= a_sumar;
        }
        else
        {
            uint32_t *reg_orig32 = obtener_registro(registros, registro_origen);
            *reg_dest -= *(reg_orig32);
        }
    }
}

void saltar_si_no_es_cero(t_registros *registros, char *registro, char *instruccion)
{
    uint32_t *registro_val = obtener_registro(registros, registro);
    if (*registro_val != 0)
    {
        registros->pc = atoi(instruccion);
    }
}
