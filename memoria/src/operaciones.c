#include "main.h"

void log_paginas(void *elem)
{
    int *pagina = (int *)elem;
    log_info(logger_memoria, "[%d]", *pagina);
}

int resize_proceso(int pid, int cantidad_de_bytes)
{
    int resto = cantidad_de_bytes % config_memoria->tam_pagina == 0 ? 0 : 1;
    int cantidad_paginas = cantidad_de_bytes / config_memoria->tam_pagina + resto;
    char *pid_str = string_from_format("%d", pid);
    t_list *paginas_del_proceso = dictionary_get(tabla_de_paginas, pid_str);

    int paginas_actuales = list_size(paginas_del_proceso);
    int cantidad_de_bytes_actuales = paginas_actuales * config_memoria->tam_pagina;
    if (cantidad_marcos_libres < cantidad_paginas - paginas_actuales)
    {
        log_error(logger_memoria, "OUT_OF_MEMORY ERROR: No hay suficientes marcos libres para el proceso %d", pid);
        free(pid_str);
        return 1;
    }
    if (paginas_actuales < cantidad_paginas)
    {
        log_info(logger_memoria, "Ampliación de Proceso: PID: %d - Tamaño Actual: %d - Tamaño a Ampliar: %d", pid, cantidad_de_bytes_actuales, cantidad_de_bytes - cantidad_de_bytes_actuales);
        log_info(logger_obligatorio_memoria, "Ampliación de Proceso: PID: %d - Tamaño Actual: %d - Tamaño a Ampliar: %d", pid, cantidad_de_bytes_actuales, cantidad_de_bytes - cantidad_de_bytes_actuales);
        for (int i = paginas_actuales; i < cantidad_paginas; i++)
        {
            int *pagina = malloc(sizeof(int));
            *pagina = obtener_marco_libre();
            list_add(paginas_del_proceso, pagina);
            ocupar_marco(*pagina);
        }
        list_iterate(paginas_del_proceso, log_paginas);
    }
    else if (paginas_actuales > cantidad_paginas)
    {
        log_info(logger_memoria, "Reducción de Proceso: PID: %d - Tamaño Actual: %d - Tamaño a Reducir: %d", pid, cantidad_de_bytes_actuales, cantidad_de_bytes_actuales - cantidad_de_bytes);
        log_info(logger_obligatorio_memoria, "Reducción de Proceso: PID: %d - Tamaño Actual: %d - Tamaño a Reducir: %d", pid, cantidad_de_bytes_actuales, cantidad_de_bytes_actuales - cantidad_de_bytes);
        for (int i = paginas_actuales; i > cantidad_paginas; i--)
        {
            int *pagina = list_get(paginas_del_proceso, i - 1);
            liberar_marco(*pagina);
            free(list_remove(paginas_del_proceso, i - 1));
        }
    }
    free(pid_str);
    return 0;
}

void crear_proceso(char *nombre_archivo_de_instrucciones, uint32_t pid)
{
    // leer instrucciones
    log_info(logger_memoria, "ruta: %s", nombre_archivo_de_instrucciones);
    t_list *instrucciones = leer_archivo(config_memoria->path_instrucciones, nombre_archivo_de_instrucciones);
    char *str_pid = string_from_format("%d", pid);
    dictionary_put(dict_instrucciones, str_pid, instrucciones);

    t_list *paginas_del_proceso = list_create();
    dictionary_put(tabla_de_paginas, str_pid, paginas_del_proceso);

    log_info(logger_memoria, "Creación de Tabla de Páginas: PID: %d - Tamaño: %d", pid, list_size(paginas_del_proceso));
    log_info(logger_obligatorio_memoria, "Creación de Tabla de Páginas: PID: %d - Tamaño: %d", pid, list_size(paginas_del_proceso));
    free(str_pid);
}

void eliminar_proceso(uint32_t pid)
{
    char *pid_str = string_itoa(pid);
    log_info(logger_memoria, "Destrucción de Tabla de Páginas: PID: %d - Tamaño: %d", pid, list_size(dictionary_get(tabla_de_paginas, pid_str)));
    log_info(logger_obligatorio_memoria, "Destrucción de Tabla de Páginas: PID: %d - Tamaño: %d", pid, list_size(dictionary_get(tabla_de_paginas, pid_str)));
    liberar_marcos_de_proceso(pid);

    dictionary_remove_and_destroy(tabla_de_paginas, pid_str, process_destroyer);
    pthread_mutex_lock(&m_dict);
    dictionary_remove_and_destroy(dict_instrucciones, pid_str, list_destroyer);
    pthread_mutex_unlock(&m_dict);
    free(pid_str);
}

void agregar_datos_contiguos_a_memoria(void *datos, int direccion, int tamanio)
{
    memcpy(memoria + direccion, datos, tamanio);
}

void obtener_datos_contiguos_de_memoria(void *buffer, int direccion, int tamanio)
{
    log_info(logger_memoria, "direccion: %d, tamanio: %d", direccion, tamanio);
    memcpy(buffer, memoria + direccion, tamanio);
}

int obtener_marco_de_direccion(int direccion)
{
    return direccion / config_memoria->tam_pagina;
}

int obtener_pagina_de_marco(t_list *paginas_del_proceso, int marco)
{
    for (int i = 0; i < list_size(paginas_del_proceso); i++)
    {
        int *pagina = list_get(paginas_del_proceso, i);
        if (*pagina == marco)
        {
            log_info(logger_memoria, "Página %d encontrada en el marco %d", i, marco);
            return i;
        }
    }
    log_error(logger_memoria, "Página no encontrada en el marco %d", marco);
    return -1;
}

int obtener_tamanio_hasta_llenar_pagina(int direccion)
{
    return config_memoria->tam_pagina - (direccion % config_memoria->tam_pagina);
}

void obtener_datos_de_memoria(void *buffer, int direccion, int tamanio, int pid)
{
    log_info(logger_memoria, "Acceso a espacio de usuario: PID: %d - Accion: LEER - Direccion fisica: %d - Tamaño %d", pid, direccion, tamanio);
    log_info(logger_obligatorio_memoria, "Acceso a espacio de usuario: PID: %d - Accion: LEER - Direccion fisica: %d - Tamaño %d", pid, direccion, tamanio);
    // Obtengo la lista de paginas del proceso
    char *str_pid = string_from_format("%d", pid);
    t_list *paginas_del_proceso = dictionary_get(tabla_de_paginas, str_pid);
    int marco_de_direccion = obtener_marco_de_direccion(direccion);
    int pagina_del_marco = obtener_pagina_de_marco(paginas_del_proceso, marco_de_direccion);
    int tamanio_hasta_llenar_pagina = obtener_tamanio_hasta_llenar_pagina(direccion);

    int *marco;
    int tamanio_restante = tamanio;
    int direccion_de_pagina = direccion;
    int offset = 0;
    int tamanio_a_copiar = tamanio > tamanio_hasta_llenar_pagina ? tamanio_hasta_llenar_pagina : tamanio;

    while (tamanio_restante > 0)
    {
        log_info(logger_memoria, "offset: %d", offset);
        tamanio_a_copiar = tamanio_restante > config_memoria->tam_pagina ? config_memoria->tam_pagina : tamanio_restante;
        obtener_datos_contiguos_de_memoria(buffer + offset, direccion_de_pagina, tamanio_a_copiar);
        if (tamanio_restante > config_memoria->tam_pagina)
            pagina_del_marco++;
        tamanio_restante -= tamanio_a_copiar;
        marco = list_get(paginas_del_proceso, pagina_del_marco);
        direccion_de_pagina = (*marco) * config_memoria->tam_pagina + (direccion_de_pagina % config_memoria->tam_pagina);
        offset += tamanio_a_copiar;
    }
    log_info(logger_memoria, "offset: %d", offset);
    free(str_pid);
}

void agregar_datos_a_memoria(void *datos, int direccion, int tamanio, int pid)
{
    log_info(logger_memoria, "Acceso a espacio de usuario: PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Tamaño %d", pid, direccion, tamanio);
    log_info(logger_obligatorio_memoria, "Acceso a espacio de usuario: PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Tamaño %d", pid, direccion, tamanio);
    // Obtengo la lista de paginas del proceso
    char *str_pid = string_from_format("%d", pid);
    t_list *paginas_del_proceso = dictionary_get(tabla_de_paginas, str_pid);
    int marco_de_direccion = obtener_marco_de_direccion(direccion);
    int pagina_del_marco = obtener_pagina_de_marco(paginas_del_proceso, marco_de_direccion);
    int tamanio_hasta_llenar_pagina = obtener_tamanio_hasta_llenar_pagina(direccion);

    int *marco;
    int tamanio_restante = tamanio;
    int direccion_de_pagina = direccion;
    int offset = 0;
    int tamanio_a_pegar = tamanio > tamanio_hasta_llenar_pagina ? tamanio_hasta_llenar_pagina : tamanio;

    while (tamanio_restante > 0)
    {
        tamanio_a_pegar = tamanio_restante > config_memoria->tam_pagina ? config_memoria->tam_pagina : tamanio_restante;
        agregar_datos_contiguos_a_memoria(datos + offset, direccion_de_pagina, tamanio_a_pegar);
        if (tamanio_restante > config_memoria->tam_pagina)
            pagina_del_marco++;
        tamanio_restante -= tamanio_a_pegar;
        marco = list_get(paginas_del_proceso, pagina_del_marco);
        direccion_de_pagina = (*marco) * config_memoria->tam_pagina + (direccion_de_pagina % config_memoria->tam_pagina);
        offset += tamanio_a_pegar;
    }
    free(str_pid);
}

void copy_string(int di, int si, int tamanio, int pid)
{
    log_info(logger_memoria, "Copiando string de %d a %d con tamanio %d", si, di, tamanio);
    void *string = malloc(tamanio);
    obtener_datos_de_memoria(string, si, tamanio, pid);
    agregar_datos_a_memoria(string, di, tamanio, pid);
    free(string);
}