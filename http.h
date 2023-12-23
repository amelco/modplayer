#ifndef HTTP_H
#define HTTP_H

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 1024*1024  // 1 MB

typedef struct {
    int socketfd;
    char* baseUrl;
    int port;
} Config;

Config http_config(char* baseUrl, int port);
unsigned char* http_get(char* endpoint, Config cfg, size_t* responseSize);

#endif // HTTP_H

#ifdef HTTP_IMPLEMENTATION

Config http_config(char* baseUrl, int port)
{
    assert(strlen(baseUrl) <= 255);

    Config cfg = {socket(AF_INET, SOCK_STREAM, 0), baseUrl, port};
    assert(cfg.socketfd != -1);
    
    struct hostent* host = gethostbyname(cfg.baseUrl);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(cfg.port),
        .sin_addr = *(struct in_addr*)host->h_addr,
        .sin_zero = {0}
    };
    int res = connect(cfg.socketfd, (struct sockaddr*)&addr, sizeof(addr));
    assert(res != -1);
    printf("connected to server %s\n", cfg.baseUrl);
    return cfg;
}

unsigned char* http_get(char* endpoint, Config cfg, size_t* responseSize)
{
    char message[BUFFER_SIZE] = {'\0'};
    sprintf(message, "GET %s HTTP/1.1\r\nHOST: %s\r\nConnection: close\r\n\r\n", endpoint, cfg.baseUrl);
    printf("%s\n", message);
    int res = send(cfg.socketfd, message, strlen(message), 0);
    assert(res != -1);

    unsigned char buffer[BUFFER_SIZE];
    static unsigned char response[BUFFER_SIZE*5]; // 5 MB
    int numBytes = 0;
    *responseSize = 0;
    while ((numBytes = recv(cfg.socketfd, buffer, BUFFER_SIZE-1, 0)) > 0)
    {
        memcpy(&response[*responseSize], &buffer[0], numBytes);
        *responseSize += numBytes;
        assert(numBytes != -1);
    }

    return &response[0];
}



#endif // HTTP_IMPLEMENTATION
