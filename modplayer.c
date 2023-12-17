#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024*1024  // 1Kb
#define panic(x,msg) if (x == -1) { printf("ERROR: %s\n", msg); return 1; }
#define h_addr h_addr_list[0]

int main(void)
{
    srand(time(NULL));
    int modNumber = rand() % 80000;
    char* url = "api.modarchive.org";
    char message[256] = {'\0'};
    sprintf(message, "GET /downloads.php?moduleid=%d HTTP/1.1\r\nHOST: api.modarchive.org\r\nConnection: close\r\n\r\n", modNumber);
    //sprintf(message, "GET /downloads.php?moduleid=47679 HTTP/1.1\r\nHOST: api.modarchive.org\r\nConnection: close\r\n\r\n");

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    panic(socketfd, "Could not create socket");

    struct hostent* host = gethostbyname(url);
    struct sockaddr_in addr = {
        AF_INET,
        htons(80),
        *(struct in_addr*)host->h_addr
    };
    printf("Connecting to %s ... ", host->h_name);
    int res = connect(socketfd, (struct sockaddr*)&addr, sizeof(addr));
    panic(res,"Could not connect");
    printf("connected to server\n");

    res = send(socketfd, message, strlen(message), 0);
    panic(res,"Could send data request");
    printf("Done!\n %s\n", message);

    unsigned char buffer[BUFFER_SIZE];
    unsigned char response[BUFFER_SIZE];
    int numBytes = 0;
    size_t responseSize = 0;
    while ((numBytes = recv(socketfd, buffer, BUFFER_SIZE-1, 0)) > 0)
    {
        memcpy(&response[responseSize], &buffer[0], numBytes);
        responseSize += numBytes;
        panic(numBytes,"Could get response from server");
    }

    // remove http header from response
    bool found = false;
    size_t index = 0;
    const char end[5] = {0x0d,0x0a,0x0d,0x0a,'\0'};   // 2 times \r\n
    while (!found && index < responseSize)
    {
        char word[5] = {'\0'};
        memcpy(&word[0], &response[index], 4);
        if (strcmp(word, end) == 0) 
        {
            found = true;
            index += 3; // points right to the beggining of binary data
        }
        index += 1;
    }

    // save mod bin file
    FILE* modFile = fopen("test.mod", "wb");
    fwrite(&response[index], 1, responseSize, modFile);
    fclose(modFile);

    return 0;
}
