#include "main.h"

int obtener_marco_libre()
{
    // Recorro marcos (bitarray) hasta que haya una posicion con valor 0 (marco libre) y la retorna
    for (int i = 0; i < marcos->size; i++)
    {
        if (bitarray_test_bit(marcos, i) == 0)
        {
            return i;
        }
    }
    return -1;
}

void ocupar_marco(int marco)
{
    // Seteo el bit de la marco en 1 (marco ocupado)
    bitarray_set_bit(marcos, marco);
    cantidad_marcos_libres--;
}

void liberar_marco(int marco)
{
    // Seteo el bit de la marco en 0 (marco libre)
    bitarray_clean_bit(marcos, marco);
    cantidad_marcos_libres++;
}

void liberar_marcos_de_proceso(int pid)
{
    // Obtengo las paginas del proceso
    char *pid_str = string_from_format("%d", pid);
    t_list *paginas_del_proceso = dictionary_get(tabla_de_paginas, pid_str);

    // Recorro las paginas del proceso
    for (int i = 0; i < list_size(paginas_del_proceso); i++)
    {
        // Obtengo el marco de la pagina
        int *pagina = list_get(paginas_del_proceso, i);

        // Libero el marco
        liberar_marco(*pagina);
    }
    free(pid_str);
}

void crear_estructuras_memoria(char *path)
{
    if (path == NULL)
    {
        iniciar_config("memoria.config");
    }
    else
    {
        iniciar_config(path);
    }
    logger_memoria = log_create("memoria.log", "memoria", 0, LOG_LEVEL_INFO);
    logger_obligatorio_memoria = log_create("memoriaObligatorio.log", "memoria", 1, LOG_LEVEL_INFO);
    config_memoria = leer_configs();

    // para probar el fetch:
    dict_instrucciones = dictionary_create();
    // esto deberia ser parte de la creacion del proceso.

    memoria = malloc(config_memoria->tam_memoria);
    tabla_de_paginas = dictionary_create();
    cantidad_de_paginas = config_memoria->tam_memoria / config_memoria->tam_pagina;
    // Crear bitmap de marcos (divido por 8 porque cantidad de paginas es la cantidad de bits y el size es en bytes)
    void *bitarray_marcos = calloc(cantidad_de_paginas, sizeof(char));
    marcos = bitarray_create_with_mode(bitarray_marcos, cantidad_de_paginas, LSB_FIRST);
    log_info(logger_memoria, "Memoria creada con %d en bitArray", cantidad_de_paginas);
    cantidad_marcos_libres = cantidad_de_paginas;
    log_info(logger_memoria, "Memoria creada con %d marcos libres", cantidad_marcos_libres);
}

void destruir_estructuras_memoria()
{
    log_info(logger_memoria, "Cerrando servidor memoria, liberando memoria y destruyendo estructuras...");
    cerrar_servidor(*socket_memoria);
    free(memoria);
    dictionary_destroy_and_destroy_elements(tabla_de_paginas, process_destroyer);
    free(marcos->bitarray);
    bitarray_destroy(marcos);
    destruir_config(config_memoria);
    log_destroy(logger_memoria);
    log_destroy(logger_obligatorio_memoria);
    free(socket_memoria);
    dictionary_destroy_and_destroy_elements(dict_instrucciones, list_destroyer);
}

int acceder_a_tabla_de_paginas(int pid, int pagina)
{
    char *pid_str = string_from_format("%d", pid);
    log_info(logger_memoria, "Acceso a Tabla de Páginas: PID: %d - Pagina: %d ", pid, pagina);
    t_list *paginas_del_proceso = dictionary_get(tabla_de_paginas, pid_str);
    int marco = *(int *)list_get(paginas_del_proceso, pagina);
    log_info(logger_memoria, "Acceso a Tabla de Páginas: PID: %d - Pagina: %d - Marco: %d", pid, pagina, marco);
    log_info(logger_obligatorio_memoria, "Acceso a Tabla de Páginas: PID: %d - Pagina: %d - Marco: %d", pid, pagina, marco);
    free(pid_str);
    return marco;
}
