#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <xmp.h>
#include <errno.h>
#include <unistd.h>

#define HTTP_IMPLEMENTATION
#include "http.h"


bool verbose = false;

char* save(int modNumber, unsigned char* modFileContent, size_t responseSize)
{
    static char filename[100] = {'\0'};
    sprintf(filename, "%d.mod", modNumber);
    FILE* modFile = fopen(filename, "wb");
    fwrite(modFileContent, 1, responseSize, modFile);
    fclose(modFile);
    return filename;
}

int play(char* filename)
{
    char cmd[4 + 256] = {'\0'}; 
    sprintf(cmd, "xmp %s", filename);
    FILE* fo = popen(cmd, "r");
    if (fo == NULL) 
    {
        printf("ERROR: could not open mod file '%s'\n", filename);
        return 1;
    }
    if (errno != 0)
    {
        printf("ERROR: %s", strerror(errno));
        return  errno;
    }
    // prints output of command to stdout
    char buff[256];
    while (fgets(buff, sizeof(buff)-1, fo) != NULL) 
    {
        printf("%s", buff);
    }
    pclose(fo);
    return 0;
}

bool is_accepted_header(char* expectedHeaderValue, int size, unsigned char* bytes, int startIndex)
{
    if (memcmp(bytes + startIndex, expectedHeaderValue, size) == 0) return true;
    return false;
}

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        play(argv[1]);
        printf("Bye!\n");
        return 0;
    }

    bool accepted = false;
    int modNumber = 0;
    size_t responseSize = 0;
    unsigned char* modFileContent;
    while (!accepted)
    {
        srand(time(NULL));
        modNumber = rand() % 90000;

        // TODO:
        // - verify if modNumber exists in data file and has status 0 (valid mod file)

        Config cfg = http_config("api.modarchive.org", 80);
        char endpoint[100] = {'\0'};
        sprintf(endpoint, "/downloads.php?moduleid=%d", modNumber);
        unsigned char* response = http_get(endpoint, cfg, &responseSize);

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
        unsigned char* binContent = &response[beginBinary];

        if (responseSize <= 427)
        {
            printf("Music non-existent in the website.\nDownloading a new one...\n\n");
            sleep(1);
            continue;
        }

        // accept AMIGA, IMPM and Extendend Module mod files
        accepted = (
               is_accepted_header("M.K.", 4, binContent, 0x438)
            || is_accepted_header("IMPM", 4, binContent, 0)
            || is_accepted_header("Extended Module:", 16, binContent, 0)
        );

        if (!accepted)
        {
            printf("Non-accepted type of mod file (%d.mod).\nDownloading a new one...\n\n", modNumber);
            sleep(1);
            continue;
        }
        modFileContent = binContent;
    }

    char* filename = save(modNumber, modFileContent, responseSize);
    play(filename);

    printf("\nBye!\n");
    return 0;
}
