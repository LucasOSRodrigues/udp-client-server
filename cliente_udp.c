// cliente
// gcc cliente_udp.c -o cliente_udp
// Uso: ./cliente_udp <arquivo1> [arquivo2] ...

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTA 8080
#define TAM_BUFFER 1024

// Espaço reservado no início do pacote para o cabeçalho de texto:
// <nome> <extensao> <segmento_atual> <total_de_segmentos><espaço>
#define TAM_CABECALHO 256
#define TAM_DADOS (TAM_BUFFER - TAM_CABECALHO)

// Extrai o nome base (sem diretórios e sem extensão) e a extensão do caminho.
// Se o arquivo não tiver extensão, usa "-".
void extrair_nome_extensao(const char *caminho,
                           char *nome, size_t tam_nome,
                           char *extensao, size_t tam_extensao)
{
    const char *base = strrchr(caminho, '/');
    base = (base != NULL) ? base + 1 : caminho;

    const char *ponto = strrchr(base, '.');

    if (ponto != NULL && ponto != base && *(ponto + 1) != '\0')
    {
        size_t len = (size_t)(ponto - base);
        if (len >= tam_nome)
            len = tam_nome - 1;

        memcpy(nome, base, len);
        nome[len] = '\0';

        snprintf(extensao, tam_extensao, "%s", ponto + 1);
    }
    else
    {
        snprintf(nome, tam_nome, "%s", base);
        snprintf(extensao, tam_extensao, "-");
    }
}

// Envia um arquivo em segmentos e espera o "OK" do servidor a cada um.
// Retorna 0 em sucesso, -1 em erro.
int enviar_arquivo(int socket_cliente,
                   struct sockaddr_in *endereco_servidor,
                   const char *caminho)
{
    char nome[128];
    char extensao[32];
    char pacote[TAM_BUFFER];
    char resposta[TAM_BUFFER];

    socklen_t tamanho_servidor = sizeof(*endereco_servidor);

    extrair_nome_extensao(caminho, nome, sizeof(nome),
                          extensao, sizeof(extensao));

    FILE *arquivo = fopen(caminho, "rb");

    if (arquivo == NULL)
    {
        fprintf(stderr, "Erro ao abrir arquivo: %s\n", caminho);
        return -1;
    }

    // Descobre o tamanho do arquivo para calcular o total de segmentos
    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);

    // Arquivo vazio ainda gera 1 segmento (sem dados)
    int total_segmentos = (int)((tamanho_arquivo + TAM_DADOS - 1) / TAM_DADOS);
    if (total_segmentos == 0)
        total_segmentos = 1;

    printf("Enviando %s (nome: %s, extensao: %s, %ld bytes, %d segmentos)\n",
           caminho, nome, extensao, tamanho_arquivo, total_segmentos);

    for (int segmento = 1; segmento <= total_segmentos; segmento++)
    {
        char dados[TAM_DADOS];
        size_t bytes_lidos = fread(dados, 1, TAM_DADOS, arquivo);

        // Monta o cabeçalho de texto no início do pacote
        int tam_cabecalho = snprintf(pacote, sizeof(pacote),
                                     "%s %s %d %d ",
                                     nome, extensao,
                                     segmento, total_segmentos);

        // Copia os dados binários logo após o cabeçalho
        memcpy(pacote + tam_cabecalho, dados, bytes_lidos);

        int tam_pacote = tam_cabecalho + (int)bytes_lidos;

        int bytes_enviados = sendto(
            socket_cliente,
            pacote,
            tam_pacote,
            0,
            (struct sockaddr *)endereco_servidor,
            tamanho_servidor);

        if (bytes_enviados < 0)
        {
            perror("Erro ao enviar segmento");
            fclose(arquivo);
            return -1;
        }

        // Espera o ACK do servidor antes do próximo segmento
        int bytes_recebidos = recvfrom(
            socket_cliente,
            resposta,
            sizeof(resposta) - 1,
            0,
            (struct sockaddr *)endereco_servidor,
            &tamanho_servidor);

        if (bytes_recebidos < 0)
        {
            perror("Erro ao receber confirmação");
            fclose(arquivo);
            return -1;
        }

        printf("Segmento %d/%d enviado (%d bytes de dados)\n",
               segmento, total_segmentos, (int)bytes_lidos);
    }

    fclose(arquivo);

    printf("Arquivo %s enviado com sucesso.\n\n", caminho);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <arquivo1> [arquivo2] ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Valida que todos os arquivos existem antes de iniciar o envio
    for (int i = 1; i < argc; i++)
    {
        FILE *arquivo = fopen(argv[i], "rb");

        if (arquivo == NULL)
        {
            fprintf(stderr, "Erro ao abrir arquivo: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        fclose(arquivo);
    }

    int socket_cliente;
    struct sockaddr_in endereco_servidor;

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

    for (int i = 1; i < argc; i++)
    {
        if (enviar_arquivo(socket_cliente, &endereco_servidor, argv[i]) < 0)
        {
            close(socket_cliente);
            exit(EXIT_FAILURE);
        }
    }

    close(socket_cliente);

    return 0;
}
