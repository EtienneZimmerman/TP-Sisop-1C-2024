#include "entradasalida.h"

void crearArchivo(uint32_t *pid, char *nombreArchivo)
{
    t_archivo *archivo = malloc(sizeof(t_archivo));
    archivo->bloque_inicial = asignar_bloque();
    archivo->tam_archivo = 0;
    dictionary_put(archivos, nombreArchivo, archivo);
    actualizar_bitmap();

    actualizar_archivo(nombreArchivo);

    log_info(logger_io, "Archivo creado: %s", nombreArchivo);
}

void eliminarArchivo(uint32_t *pid, char *nombreArchivo)
{
    t_archivo *archivo = dictionary_get(archivos, nombreArchivo);
    // int tamanioArchivo = archivo->tam_archivo;
    if (archivo == NULL)
    {
        log_error(logger_io, "El archivo %s no existe.", nombreArchivo);
        return;
    }

    int cantidad_de_bloques_a_desasignar = cantidad_de_bloques(archivo->tam_archivo);
    int bloque_actual = archivo->bloque_inicial;
    for (int i = 0; i < cantidad_de_bloques_a_desasignar; i++)
    {
        desasignar_bloque(bloque_actual + i);
    }
    actualizar_bitmap();
    eliminar_archivo_txt(nombreArchivo);
    dictionary_remove_and_destroy(archivos, nombreArchivo, free);
    log_info(logger_io, "Archivo borrado: %s", nombreArchivo);
}

void truncarArchivo(uint32_t *pid, char *nombreArchivo, uint32_t tamanio)
{
    t_archivo *archivo = dictionary_get(archivos, nombreArchivo);
    if (archivo == NULL)
    {
        log_error(logger_io, "El archivo %s no existe.", nombreArchivo);
        return;
    }
    log_info(logger_io, "Encontré el archivo %s.", nombreArchivo);

    int bloques_necesarios = cantidad_de_bloques(tamanio);
    int bloques_en_uso = cantidad_de_bloques(archivo->tam_archivo);

    bool hay_suficientes_bloques_contiguos = estan_contiguos(archivo->bloque_inicial, bloques_necesarios);

    // Si se quiere agrandar el archivo
    if (bloques_necesarios > bloques_en_uso)
    {
        log_info(logger_io, "Agrandar archivo.");

        // Si no hay bloques contiguos
        if (!hay_suficientes_bloques_contiguos)
        {
            log_info(logger_io, "No hay bloques contiguos, se procede a compactar.");
            compactar(nombreArchivo);
        }

        int bloques_a_asignar = bloques_necesarios - bloques_en_uso;

        log_info(logger_io, "Se procede a asignar bloques. Bloque inicial: %d - Bloques en uso: %d - Bloques a asignar: %d", archivo->bloque_inicial, bloques_en_uso, bloques_a_asignar);

        for (int i = 0; i < bloques_a_asignar; i++)
        {
            bitarray_set_bit(bitmap, archivo->bloque_inicial + bloques_en_uso + i);
        }

        archivo->tam_archivo = tamanio;

        actualizar_bitmap();
        actualizar_archivo(nombreArchivo);

        log_info(logger_io, "Archivo truncado (agrandado): %s - Tamaño: %d.", nombreArchivo, tamanio);
    }
    // Si se quiere achicar
    else if (bloques_necesarios < bloques_en_uso)
    {
        log_info(logger_io, "Achicar archivo.");
        int bloques_a_liberar = bloques_en_uso - bloques_necesarios;

        log_info(logger_io, "Se procede a liberar bloques. Bloque inicial: %d - Bloques en uso: %d - Bloques a liberar: %d", archivo->bloque_inicial, bloques_en_uso, bloques_a_liberar);

        for (int i = 0; i < bloques_a_liberar; i++)
        {
            desasignar_bloque(archivo->bloque_inicial + bloques_en_uso - i);
        }

        archivo->tam_archivo = tamanio;

        actualizar_bitmap();
        actualizar_archivo(nombreArchivo);

        log_info(logger_io, "Archivo truncado (achicado): %s - Tamaño: %d.", nombreArchivo, tamanio);
    }
    else
    {
        log_error(logger_io, "No hay nada que truncar, el archivo ya tiene el tamaño deseado.");
    }
}
