// servidor
// gcc servidor_udp.c -o servidor_udp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTA 8080
#define TAM_BUFFER 1024

int main()
{
    int socket_servidor;
    char buffer[TAM_BUFFER];
    char resposta[TAM_BUFFER + 50];

    struct sockaddr_in endereco_servidor;
    struct sockaddr_in endereco_cliente;

    socklen_t tamanho_cliente = sizeof(endereco_cliente);

    // Cria socket UDP
    socket_servidor = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_servidor < 0)
    {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configura endereço do servidor
    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_addr.s_addr = INADDR_ANY;
    endereco_servidor.sin_port = htons(PORTA);

    // Associa o socket à porta
    if (bind(socket_servidor,
             (struct sockaddr *)&endereco_servidor,
             sizeof(endereco_servidor)) < 0)
    {
        perror("Erro no bind");
        close(socket_servidor);
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP aguardando mensagens na porta %d...\n", PORTA);

    while (1)
    {
        memset(buffer, 0, TAM_BUFFER);
        memset(resposta, 0, sizeof(resposta));

        // Recebe mensagem do cliente
        int bytes_recebidos = recvfrom(
            socket_servidor,
            buffer,
            TAM_BUFFER - 1,
            0,
            (struct sockaddr *)&endereco_cliente,
            &tamanho_cliente);

        if (bytes_recebidos < 0)
        {
            perror("Erro ao receber mensagem");
            continue;
        }

        buffer[bytes_recebidos] = '\0';

        printf("Mensagem recebida de %s:%d -> %s\n",
               inet_ntoa(endereco_cliente.sin_addr),
               ntohs(endereco_cliente.sin_port),
               buffer);

        // Monta resposta
        snprintf(resposta, sizeof(resposta),
                 "OK. Mensagem recebida: %s", buffer);

        // Envia resposta ao cliente
        int bytes_enviados = sendto(
            socket_servidor,
            resposta,
            strlen(resposta),
            0,
            (struct sockaddr *)&endereco_cliente,
            tamanho_cliente);

        if (bytes_enviados < 0)
        {
            perror("Erro ao enviar resposta");
        }
    }

    close(socket_servidor);

    return 0;
}
