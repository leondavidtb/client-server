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

static const int MAXPENDING = 5;
static const int BUFSIZE = 512;

int SetupTCPServerSocket(const char *service){

    struct addrinfo addrCriteria;                   
    memset(&addrCriteria, 0, sizeof(addrCriteria)); 
    addrCriteria.ai_family = AF_UNSPEC;             
    addrCriteria.ai_flags = AI_PASSIVE;             
    addrCriteria.ai_socktype = SOCK_STREAM;         
    addrCriteria.ai_protocol = IPPROTO_TCP;         

    struct addrinfo *servAddr; 
    int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
    if (rtnVal != 0){
        perror("getaddrinfo() falhou\n");
        exit(EXIT_FAILURE);
    }

    int servSock = -1; 
    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next){

        servSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (servSock < 0)
            continue;

        if ((bind(servSock, addr->ai_addr, addr->ai_addrlen) == 0) && (listen(servSock, MAXPENDING) == 0)){
            break;
        }

        close(servSock);
        servSock = -1;
    }

    freeaddrinfo(servAddr);

    return servSock;
}

int AcceptTCPConnection(int servSock){
    struct sockaddr_storage clntAddr; 

    socklen_t clntAddrLen = sizeof(clntAddr);

    int clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
    if (clntSock < 0){
        perror("accept() falhou\n");
        exit(EXIT_FAILURE);
    }

    return clntSock;
}

void HandleTCPClient(int clntSocket){
    char buffer[BUFSIZE];
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    ssize_t numBytesSent, numBytesRcvd;

    while (true){
        numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
        if (numBytesRcvd < 0){
            perror("recv() falhou\n");
            exit(EXIT_FAILURE);
        }

        fputs("Client: ", stdout);
        fputs(buffer, stdout);
        fputc('\n', stdout);

        fputs("Server: ", stdout);
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
    }
}

int main(int argc, char const *argv[]){
    if (argc != 2){
        perror("ExecuÃ§Ã£o correta: ./chat-server <porta>\n");
        return EXIT_FAILURE;
    }

    int estado_atual = iniciando;
    int servSock, clntSock;

    while (estado_atual != encerrado){
        switch (estado_atual){
        case iniciando:
            servSock = SetupTCPServerSocket(argv[1]);
            estado_atual = comunicando;
            break;
        case comunicando:
            clntSock = AcceptTCPConnection(servSock);
            HandleTCPClient(clntSock);
            estado_atual = finalizando;
            break;
        case finalizando:
            estado_atual = encerrado;
            close(servSock);
            close(clntSock);
            break;
        default:
            break;
        }
    }

    return EXIT_SUCCESS;
}