#include "entradasalida.h"
// Cuando no hay suficientes bloques contiguos para truncar (ampliar) un archivo, se debe compactar el FS.
int compactar(char *nombre_archivo)
{
    log_info(logger_io, "DialFS - Inicio Compactación: PID: %d - Inicio Compactación.", pid_usando_interfaz);
    
    log_info(logger_obligatorio_io, "PID: <%d> - Inicio Compactación", pid_usando_interfaz);
    
    // 0. Verificar que el archivo exista.
    t_archivo *archivo_a_ampliar = dictionary_get(archivos, nombre_archivo);
    if (archivo_a_ampliar == NULL)
    {
        log_error(logger_io, "El archivo %s no existe.", nombre_archivo);
        return 1;
    }

    // 1. Guardar los datos del archivo (obteniendolos del void *bloques) que se quiere ampliar en un buffer.
    void *buffer_archivo_a_ampliar = malloc(archivo_a_ampliar->tam_archivo);
    int posicion_inicial_archivo_a_ampliar = archivo_a_ampliar->bloque_inicial * config_io->block_size;
    memcpy(buffer_archivo_a_ampliar, bloques + posicion_inicial_archivo_a_ampliar, archivo_a_ampliar->tam_archivo);

    // 2. Compactar todos los otros archivos en bloques contiguos.
    int bloque_actual = 0;
    void *bloques_compactados = calloc(config_io->block_count * config_io->block_size, sizeof(char));

    t_list *archivos_a_compactar = dictionary_keys(archivos);

    // Se filtran los archivos que no sean el archivo a ampliar
    bool es_archivo_a_ampliar(void *param_nombre_archivo)
    {
        return strcmp(nombre_archivo, param_nombre_archivo) == 0;
    }
    list_remove_by_condition(archivos_a_compactar, es_archivo_a_ampliar);

    for (int i = 0; i < list_size(archivos_a_compactar); i++)
    {
        char *nombre_archivo_a_compactar = list_get(archivos_a_compactar, i);
        t_archivo *archivo = dictionary_get(archivos, nombre_archivo_a_compactar);

        // Se copian los datos del archivo a compactar en los bloques compactados, contiguo al archivo anterior
        int posicion_inicial_compactada = bloque_actual * config_io->block_size;
        int posicion_inicial_archivo = archivo->bloque_inicial * config_io->block_size;
        memcpy(bloques_compactados + posicion_inicial_compactada, bloques + posicion_inicial_archivo, archivo->tam_archivo);

        archivo->bloque_inicial = bloque_actual;
        // Se actualiza el bloque actual
        bloque_actual += archivo->tam_archivo / config_io->block_size;
        // Si lo que queda del archivo no entra en un bloque, se agrega un bloque más
        if (archivo->tam_archivo % config_io->block_size != 0 || archivo->tam_archivo == 0)
        {
            bloque_actual++;
        }
    }

    // Se actualiza el void *bloques con los bloques compactados.
    memcpy(bloques, bloques_compactados, bloque_actual * config_io->block_size);

    // 3. Agregar los datos del archivo que se quiere ampliar al final del FS.
    int posicion_final_archivo_a_ampliar = bloque_actual * config_io->block_size;
    memcpy(bloques + posicion_final_archivo_a_ampliar, buffer_archivo_a_ampliar, archivo_a_ampliar->tam_archivo);

    archivo_a_ampliar->bloque_inicial = bloque_actual;
    bloque_actual += archivo_a_ampliar->tam_archivo / config_io->block_size;
    if (archivo_a_ampliar->tam_archivo % config_io->block_size != 0)
    {
        bloque_actual++;
    }

    // 4. Modificar bitmap.
    for (int i = 0; i < bloque_actual; i++)
    {
        bitarray_set_bit(bitmap, i);
    }

    for (int i = bloque_actual; i < config_io->block_count; i++)
    {
        bitarray_clean_bit(bitmap, i);
    }

    // 5. Actualizar los archivos con los cambios realizados en el FS.
    actualizar_bitmap();
    actualizar_bloques();
    actualizar_archivos();

    // 6. Liberar memoria.
    free(buffer_archivo_a_ampliar);
    free(bloques_compactados);
    list_destroy(archivos_a_compactar);

    // 7. Retardo de compactación.
    usleep(config_io->retraso_compactacion * 1000);

    log_info(logger_io, "DialFS - Fin Compactación: PID: %d - Fin Compactación.", pid_usando_interfaz);

    log_info(logger_obligatorio_io, "PID: <%d> - Fin Compactación", pid_usando_interfaz);

    return 0;
    
}