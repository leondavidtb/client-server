#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

enum estados{
    iniciando,
    comunicando,
    finalizando,
    encerrado
};

static const int BUFSIZE = 512;

int SetupTCPClientSocket(const char *host, const char *service){

    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;

    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(host, service, &addrCriteria, &servAddr);
    if (rtnVal != 0){
        {
            perror("getaddrinfo() falhou\n");
            exit(EXIT_FAILURE);
        }
    }

    int sock = -1;
    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next){
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sock < 0)
            continue;

        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0)
            break;

        close(sock);
        sock = -1;
    }
    freeaddrinfo(servAddr);
    return sock;
}

void HandleTCPServer(int clntSocket){
    char buffer[BUFSIZE];
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    ssize_t numBytesSent, numBytesRcvd;

    while (true){
        fputs("Client: ", stdout);
        read = getline(&line, &len, stdin);
        numBytesSent = send(clntSocket, line, len, 0);

        if (numBytesSent < 0){
            perror("send() falhou\n");
            exit(EXIT_FAILURE);
        }
        if (strcmp(line, "exit\n") == 0){
            free(line);
            break;
        }

        numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
        if (numBytesRcvd < 0){
            perror("recv() falhou\n");
            exit(EXIT_FAILURE);
        }

        fputs("Server: ", stdout);
        fputs(buffer, stdout);
        fputc('\n', stdout);
    }
}

int main(int argc, char const *argv[]){
    if (argc < 2 || argc > 3){
        perror("ExecuÃ§Ã£o correta: ./chat-client <ip> [<porta>]\n");
        return EXIT_FAILURE;
    }

    const char *server = argv[1];
    const char *service = (argc == 3) ? argv[2] : "5001";

    int estado_atual = iniciando;
    int sock;

    while (estado_atual != encerrado){
        switch (estado_atual){
        case iniciando:
            sock = SetupTCPClientSocket(server, service);
            if (sock < 0){
                perror("SetupTCPClientSocket() falhou.\n");
                return EXIT_FAILURE;
            }
            estado_atual = comunicando;
            break;
        case comunicando:
            HandleTCPServer(sock);
            estado_atual = finalizando;
            break;
        case finalizando:
            close(sock);
            estado_atual = encerrado;
            break;
        default:
            break;
        }
    }

    return EXIT_SUCCESS;
}