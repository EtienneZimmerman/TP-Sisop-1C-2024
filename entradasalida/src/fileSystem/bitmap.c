#include "entradasalida.h"

int inicializar_bitmap()
{
	char *path_bitmap = string_from_format("%s/bitmap.dat", config_io->path_base_dialfs);
	int cant_bloques = config_io->block_count;
	int size = cant_bloques / 8;
	char *bloque = malloc(size);
	memset(bloque, 0, size);
	bitmap = bitarray_create_with_mode(bloque, size, MSB_FIRST);
	if (bitmap == NULL)
	{
		log_error(logger_io, "No se pudo crear el bitmap");
		return 1;
	}

	FILE *file = fopen(path_bitmap, "rb+");

	if (file == NULL) // Si el archivo no existe, lo creo.
	{
		file = fopen(path_bitmap, "wb+");
		if (file == NULL)
		{
			log_error(logger_io, "No se pudo abrir el archivo bitmap.dat.");
			bitarray_destroy(bitmap);
			return 1;
		}
		fwrite(bitmap->bitarray, 1, cant_bloques / 8, file);
	}
	else // Si el archivo existe, cargo los bloques.
	{
		fread(bitmap->bitarray, 1, cant_bloques / 8, file);
	}

	fclose(file);

	free(path_bitmap);

	return 0;
}

void imprimir_bitmap(int posicion_inicial, int cantidad)
{
	if (posicion_inicial < 0 || cantidad < 0 || posicion_inicial + cantidad > bitmap->size * 8)
	{
		log_error(logger_io, "Posicion inicial o cantidad invalida");
		return;
	}
	for (int i = posicion_inicial; i < cantidad; i++)
	{
		log_info(logger_io, "Acceso a Bitmap - Bloque: %d - Estado: %d", i, bitarray_test_bit(bitmap, i));
	}
}

int bloque_libre()
{
	for (int i = 0; i < bitmap->size * 8; i++)
	{
		log_info(logger_io, "Acceso a Bitmap - Bloque: %d - Estado: %d", i, bitarray_test_bit(bitmap, i));
		if (bitarray_test_bit(bitmap, i) == 0)
		{
			return i;
		}
	}
	return -1;
}

void desasignar_bloque(uint32_t bloque)
{
	log_info(logger_io, "Acceso a Bitmap - Bloque: %d - Estado: %d", bloque, bitarray_test_bit(bitmap, bloque));
	bitarray_clean_bit(bitmap, bloque);
	log_info(logger_io, "Acceso a Bitmap - Bloque: %d - Estado: %d", bloque, bitarray_test_bit(bitmap, bloque));
}

uint32_t asignar_bloque()
{
	uint32_t bloque_a_asignar = bloque_libre();
	bitarray_set_bit(bitmap, bloque_a_asignar);
	log_info(logger_io, "Acceso a Bitmap - Bloque: %d - Estado: %d", bloque_a_asignar, bitarray_test_bit(bitmap, bloque_a_asignar));
	return bloque_a_asignar;
}

int cantidad_de_bloques(int tamanio)
{
	int bloques = tamanio / config_io->block_size;
	if (tamanio % config_io->block_size != 0)
	{
		bloques++;
	}
	return bloques;
}

bool estan_contiguos(int bloque_inicial, int bloques_necesarios)
{
	for (int i = 0; i < bloques_necesarios; i++)
	{
		if (bitarray_test_bit(bitmap, bloque_inicial + i) == 1)
		{
			return false;
		}
	}
	return true;
}