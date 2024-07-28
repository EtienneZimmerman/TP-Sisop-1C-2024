#include "file_reader.h"

t_list *leer_archivo(char *path, char *nombre_de_archivo)
{
    FILE *arch;
    char **split = string_split(nombre_de_archivo, "/");
    if (strcmp(*split, nombre_de_archivo) != 0)
    {

        char **split2 = split;
        if (*split != NULL)
        {
            free(*split);
            split++;
        }
        char *path_append = string_new();
        char *last_read;
        while (*split != NULL)
        {
            last_read = malloc(strlen(*split) + 1);
            memcpy(last_read, *split, strlen(*split) + 1);
            string_append(&path_append, last_read);
            free(*split);
            free(last_read);
            split++;
            last_read = malloc(strlen(*split) + 1);
            memcpy(last_read, *split, strlen(*split) + 1);
            free(*split);
            split++;
        }
        string_append(&path_append, "/");
        chdir(path_append);
        free(path_append);
        arch = fopen(last_read, "r");
        free(last_read);
        free(split2);
    }
    else
    {
        chdir(path);
        arch = fopen(nombre_de_archivo, "r");
        free(*split);
        free(split);
    }

    if (arch == NULL)
    {
        printf("Error al abrir el archivo\n");
        return NULL;
    }
    t_list *lista_instrucciones = list_create();
    while (!feof(arch))
    {
        char str[200];
        char **split3 = string_n_split(fgets(str, 200, arch), 2, "\n");
        char *msg = string_from_format("%s", *split3);
        list_add_in_index(lista_instrucciones, lista_instrucciones->elements_count, msg);
        char **split_copy = split3;
        free(*split3);
        split3++;
        free(*split3);
        free(split_copy);
    }
    fclose(arch);
    chdir("../");
    return lista_instrucciones;
}

t_list *listar_archivos_en_directorio(char *path)
{
    struct dirent *entry;
    DIR *dir = opendir(path);

    t_list *lista_archivos = list_create();

    if (dir == NULL)
    {
        printf("Error al abrir el directorio\n");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type != DT_REG) // Si no es un archivo regular
        {
            continue;
        }
        // Ignorar archivos que no sean .txt
        if (strcmp(entry->d_name + strlen(entry->d_name) - 4, ".txt") != 0)
        {
            continue;
        }
        char *filename_copy = strdup(entry->d_name);
        list_add(lista_archivos, filename_copy);
    }
    closedir(dir);
    return lista_archivos;
}