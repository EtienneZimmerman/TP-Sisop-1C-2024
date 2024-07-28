#include "servidor.h"

int iniciar_servidor(char *puerto, char *ip, t_log *logger)
{

    int socket_servidor = -1; // Si hay error retornará

    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(ip, puerto, &hints, &servinfo);
    if (err != 0)
    {
        printf("ERROR EN EL GET ADD INFO");
        abort();
    }

    // Creamos el socket de escucha del servidor
    socket_servidor = socket(servinfo->ai_family,
                             servinfo->ai_socktype,
                             servinfo->ai_protocol);

    // Asociamos el socket a un puerto

    err = bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

    if (err != 0)
    {
        printf("ERROR EN EL bind");
        abort();
    }

    // Escuchamos las conexiones entrantes

    err = listen(socket_servidor, SOMAXCONN);

    if (err != 0)
    {
        printf("ERROR EN EL LISTEN");
        abort();
    }

    log_trace(logger, "Listo para escuchar a mi cliente");

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{

    int socket_cliente;

    socket_cliente = accept(socket_servidor, NULL, NULL);

    return socket_cliente;
}

t_paquete *recibir_paquete(int socket, t_log *logger)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->stream = NULL;
    paquete->buffer->size = 0;
    paquete->codigo_operacion = 0;

    // Primero recibimos el codigo de operacion
    recv(socket, &(paquete->codigo_operacion), sizeof(uint8_t), 0);
    // log_info(logger, "Recibido codigo de operacion: %d", paquete->codigo_operacion);
    //  Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
    recv(socket, &(paquete->buffer->size), sizeof(uint32_t), 0);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket, paquete->buffer->stream, paquete->buffer->size, 0);

    return paquete;
}

void recibir_mensaje(int socket_cliente, t_log *logger)
{
    int size = 0;
    char *buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Me llego el mensaje %s", buffer);
    free(buffer);
}

int recibir_operacion(int socket_cliente)
{
    int cod_op;
    if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
        return cod_op;
    else
    {
        close(socket_cliente);
        return -1;
    }
}

void *recibir_buffer(int *size, int socket_cliente)
{
    void *buffer;

    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);

    return buffer;
}

void cerrar_servidor(int socket)
{
    close(socket);
}

const char *cod_op_to_string(op_code cod_op)
{
    switch (cod_op)
    {
    case MENSAJE:
        return "MENSAJE";
    case OBT_INSTRUCCION:
        return "OBT_INSTRUCCION";
    case RET_INSTRUCCION:
        return "RET_INSTRUCCION";
    case INT_PROCESO:
        return "INT_PROCESO";
    case FINALIZAR_PROCESO_CPU:
        return "FINALIZAR_PROCESO_CPU";
    case ENVIAR_PCB:
        return "ENVIAR_PCB";
    case ENVIAR_CONTEXTO_EJECUCION:
        return "ENVIAR_CONTEXTO_EJECUCION";
    case INSTRUCCION_IO_STDIN_READ:
        return "INSTRUCCION_IO_STDIN_READ";
    case INSTRUCCION_IO_STDOUT_WRITE:
        return "INSTRUCCION_IO_STDOUT_WRITE";
    case INSTRUCCION_IO_GEN_SLEEP:
        return "INSTRUCCION_IO_GEN_SLEEP";
    case EJECUTAR_IO_GEN_SLEEP:
        return "EJECUTAR_IO_GEN_SLEEP";
    case ERROR_IO:
        return "ERROR_IO";
    case HANDSHAKE_IO:
        return "HANDSHAKE_IO";
    case HANDSHAKE_MEMORIA:
        return "HANDSHAKE_MEMORIA";
    case EJECUTAR_MOVIN:
        return "EJECUTAR_MOVIN";
    case EJECUTAR_MOVOUT:
        return "EJECUTAR_MOVOUT";
    case EJECUTAR_COPYSTRING:
        return "EJECUTAR_COPYSTRING";
    case EJECUTAR_IO_STDIN_READ:
        return "EJECUTAR_IO_STDIN_READ";
    case RTA_MOVIN:
        return "RTA_MOVIN";
    case EJECUTAR_IO_STDOUT_WRITE:
        return "EJECUTAR_IO_STDOUT_WRITE";
    case RET_IO_STDIN_READ:
        return "RET_IO_STDIN_READ";
    case RET_IO_STDOUT_WRITE:
        return "RET_IO_STDOUT_WRITE";
    case PEDIR_MARCO:
        return "PEDIR_MARCO";
    case IO_OCUPADA_SLEEP:
        return "IO_OCUPADA_SLEEP";
    case IO_OCUPADA_READ:
        return "IO_OCUPADA_READ";
    case IO_OCUPADA_WRITE:
        return "IO_OCUPADA_WRITE";
    case FIN_IO:
        return "FIN_IO";
    case PAQUETE:
        return "PAQUETE";
    case CREAR_PROCESO:
        return "CREAR_PROCESO";
    case FINALIZAR_PROCESO_MEMORIA:
        return "FINALIZAR_PROCESO_MEMORIA";
    case FIN_TERM_PRO:
        return "FIN_TERM_PRO";
    case FIN_CREA_PRO:
        return "FIN_CREA_PRO";
    case EJECUTAR_RESIZE:
        return "EJECUTAR_RESIZE";
    case EJECUTAR_RESIZE_OUT_OF_MEMORY_ERROR:
        return "EJECUTAR_RESIZE_OUT_OF_MEMORY_ERROR";
    case EJECUTAR_IO_FS_CREATE:
        return "EJECUTAR_IO_FS_CREATE";
    case EJECUTAR_IO_FS_DELETE:
        return "EJECUTAR_IO_FS_DELETE";
    case EJECUTAR_IO_FS_TRUNCATE:
        return "EJECUTAR_IO_FS_TRUNCATE";
    case EJECUTAR_IO_FS_WRITE:
        return "EJECUTAR_IO_FS_WRITE";
    case EJECUTAR_IO_FS_READ:
        return "EJECUTAR_IO_FS_READ";
    default:
        return "UNKNOWN_COD_OP";
    }
}