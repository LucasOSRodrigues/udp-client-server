// servidor
// gcc servidor_udp.c -o servidor_udp
// Uso: ./servidor_udp <diretorio_de_saida>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORTA 8080
#define TAM_BUFFER 1024

// Localiza o início dos dados binários no pacote: o byte seguinte ao
// quarto espaço do cabeçalho <nome> <ext> <seg> <total> <dados...>
// Retorna -1 se o cabeçalho estiver malformado.
int inicio_dos_dados(const char *pacote, int tam_pacote)
{
    int espacos = 0;

    for (int i = 0; i < tam_pacote; i++)
    {
        if (pacote[i] == ' ')
        {
            espacos++;

            if (espacos == 4)
                return i + 1;
        }
    }

    return -1;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <diretorio_de_saida>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *diretorio_saida = argv[1];

    struct stat info_diretorio;
    if (stat(diretorio_saida, &info_diretorio) < 0 ||
        !S_ISDIR(info_diretorio.st_mode))
    {
        fprintf(stderr, "Diretório de saída inválido: %s\n", diretorio_saida);
        exit(EXIT_FAILURE);
    }

    int socket_servidor;
    char buffer[TAM_BUFFER];
    char resposta[64];

    struct sockaddr_in endereco_servidor;
    struct sockaddr_in endereco_cliente;

    socklen_t tamanho_cliente = sizeof(endereco_cliente);

    // Arquivo em recepção (um por vez)
    FILE *arquivo_saida = NULL;
    char caminho_saida[512];

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

    printf("Servidor UDP aguardando arquivos na porta %d...\n", PORTA);
    printf("Arquivos recebidos serão salvos em: %s\n", diretorio_saida);

    while (1)
    {
        memset(buffer, 0, TAM_BUFFER);

        // Recebe pacote do cliente
        int bytes_recebidos = recvfrom(
            socket_servidor,
            buffer,
            TAM_BUFFER,
            0,
            (struct sockaddr *)&endereco_cliente,
            &tamanho_cliente);

        if (bytes_recebidos < 0)
        {
            perror("Erro ao receber pacote");
            continue;
        }

        // Parseia o cabeçalho de texto
        char nome[128];
        char extensao[32];
        int segmento;
        int total_segmentos;

        int pos_dados = inicio_dos_dados(buffer, bytes_recebidos);

        if (pos_dados < 0 ||
            sscanf(buffer, "%127s %31s %d %d",
                   nome, extensao, &segmento, &total_segmentos) != 4)
        {
            fprintf(stderr, "Pacote com cabeçalho inválido, ignorado.\n");
            continue;
        }

        int tam_dados = bytes_recebidos - pos_dados;

        printf("Recebido de %s:%d -> arquivo %s (ext: %s) segmento %d/%d (%d bytes)\n",
               inet_ntoa(endereco_cliente.sin_addr),
               ntohs(endereco_cliente.sin_port),
               nome, extensao, segmento, total_segmentos, tam_dados);

        // Primeiro segmento: abre o arquivo de saída
        if (segmento == 1)
        {
            if (arquivo_saida != NULL)
                fclose(arquivo_saida);

            // Extensão "-" indica arquivo sem extensão
            if (strcmp(extensao, "-") == 0)
            {
                snprintf(caminho_saida, sizeof(caminho_saida),
                         "%s/%s", diretorio_saida, nome);
            }
            else
            {
                snprintf(caminho_saida, sizeof(caminho_saida),
                         "%s/%s.%s", diretorio_saida, nome, extensao);
            }

            arquivo_saida = fopen(caminho_saida, "wb");

            if (arquivo_saida == NULL)
            {
                perror("Erro ao criar arquivo de saída");
                continue;
            }
        }

        if (arquivo_saida == NULL)
        {
            fprintf(stderr, "Segmento %d recebido sem arquivo aberto, ignorado.\n",
                    segmento);
            continue;
        }

        // Escreve os dados binários do segmento
        if (tam_dados > 0)
            fwrite(buffer + pos_dados, 1, tam_dados, arquivo_saida);

        // Último segmento: fecha e anuncia o arquivo salvo
        if (segmento == total_segmentos)
        {
            fclose(arquivo_saida);
            arquivo_saida = NULL;

            printf("Arquivo salvo: %s\n", caminho_saida);
        }

        // Envia ACK do segmento ao cliente
        snprintf(resposta, sizeof(resposta), "OK %d", segmento);

        int bytes_enviados = sendto(
            socket_servidor,
            resposta,
            strlen(resposta),
            0,
            (struct sockaddr *)&endereco_cliente,
            tamanho_cliente);

        if (bytes_enviados < 0)
        {
            perror("Erro ao enviar confirmação");
        }
    }

    close(socket_servidor);

    return 0;
}
