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

bool verbose = false;

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    // TODO:
    // - args: -v verbose mode  (default: false)
    //         -p [path] path of directory to store downloaded mod files (default: same as executable)
    //         -d download new random musics (default: true)

    srand(time(NULL));
    int modNumber = rand() % 80000;

    // TODO:
    // - verify if modNumber exists in data file and has status 0 (valid mod file)

    char* url = "api.modarchive.org";
    char message[256] = {'\0'};
    sprintf(message, "GET /downloads.php?moduleid=%d HTTP/1.1\r\nHOST: api.modarchive.org\r\nConnection: close\r\n\r\n", modNumber);
    //sprintf(message, "GET /downloads.php?moduleid=47679 HTTP/1.1\r\nHOST: api.modarchive.org\r\nConnection: close\r\n\r\n");

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    panic(socketfd, "Could not create socket");

    struct hostent* host = gethostbyname(url);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(80),
        .sin_addr = *(struct in_addr*)host->h_addr,
        .sin_zero = {0}
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
    size_t beginBinary = 0;
    const char endHeader[5] = {0x0d,0x0a,0x0d,0x0a,'\0'};   // 2 times \r\n
    while (!found && beginBinary < responseSize)
    {
        char word[5] = {'\0'};
        memcpy(&word[0], &response[beginBinary], 4);
        if (strcmp(word, endHeader) == 0) 
        {
            found = true;
            beginBinary += 3;
        }
        beginBinary += 1;
    }

    // TODO:
    // - verify if file is of type mod
    // - if not, regenerate new number and download new file.
    // - store number and status of file in a data file
    //   - status: 0 - valid mod file
    //             1 - invalid mod file

    // save mod bin file
    char filename[256] = {'\0'};
    sprintf(filename, "%d.mod", modNumber);
    FILE* modFile = fopen(filename, "wb");
    fwrite(&response[beginBinary], 1, responseSize, modFile);
    fclose(modFile);

    char cmd[4 + 256] = {'\0'}; 
    sprintf(cmd, "xmp %s", filename);
    FILE* fo = popen(cmd, "r");
    if (fo == NULL) 
    {
        printf("ERROR: could not open mod file '%s'\n", filename);
        return 1;
    }

    char buff[256];
    while (fgets(buff, sizeof(buff)-1, fo) != NULL) 
    {
        printf("%s", buff);
    }

    printf("\nBye!\n");
    return 0;
}
