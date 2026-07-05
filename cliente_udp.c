// cliente
// gcc cliente_udp.c -o cliente_udp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTA 8080
#define TAM_BUFFER 1024

int main()
{
    int socket_cliente;
    char mensagem[TAM_BUFFER];
    char buffer[TAM_BUFFER];

    struct sockaddr_in endereco_servidor;
    socklen_t tamanho_servidor = sizeof(endereco_servidor);

    // Cria socket UDP
    socket_cliente = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_cliente < 0)
    {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configura endereço do servidor
    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_port = htons(PORTA);

    // IP do servidor
    // Para testar na mesma máquina, use 127.0.0.1
    if (inet_pton(AF_INET, "127.0.0.1", &endereco_servidor.sin_addr) <= 0)
    {
        perror("Endereço IP inválido");
        close(socket_cliente);
        exit(EXIT_FAILURE);
    }

    printf("Digite uma mensagem para enviar ao servidor: ");
    fgets(mensagem, TAM_BUFFER, stdin);

    // Remove o '\n' do final, se existir
    mensagem[strcspn(mensagem, "\n")] = '\0';

    // Envia mensagem ao servidor
    int bytes_enviados = sendto(
        socket_cliente,
        mensagem,
        strlen(mensagem),
        0,
        (struct sockaddr *)&endereco_servidor,
        tamanho_servidor);

    if (bytes_enviados < 0)
    {
        perror("Erro ao enviar mensagem");
        close(socket_cliente);
        exit(EXIT_FAILURE);
    }

    printf("Mensagem enviada ao servidor.\n");

    memset(buffer, 0, TAM_BUFFER);

    // Recebe resposta do servidor
    int bytes_recebidos = recvfrom(
        socket_cliente,
        buffer,
        TAM_BUFFER - 1,
        0,
        (struct sockaddr *)&endereco_servidor,
        &tamanho_servidor);

    if (bytes_recebidos < 0)
    {
        perror("Erro ao receber resposta");
        close(socket_cliente);
        exit(EXIT_FAILURE);
    }

    buffer[bytes_recebidos] = '\0';

    printf("Resposta do servidor: %s\n", buffer);

    close(socket_cliente);

    return 0;
}
