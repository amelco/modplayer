#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <xmp.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#define HTTP_IMPLEMENTATION
#include "http.h"

#define clearscreen() printf("\033[H\033[J")
#define getextension(stringFile) &stringFile[strlen(stringFile)-3] 
#define isextensionmod(stringFile) strcmp(getextension(stringFile), "mod") == 0

bool verbose = false;

char* save(char* absDirectory, int modNumber, unsigned char* modFileContent, size_t responseSize)
{
    static char filename[100] = {'\0'};
    sprintf(filename, "%s/%d.mod", absDirectory, modNumber);
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
    if (memcmp(bytes + startIndex, expectedHeaderValue, size) == 0)
    {
        printf("MOD type: %s\n", expectedHeaderValue);
        return true;
    }
    return false;
}


// TODO:
// * accept mod types via command-line arguments
// * create a function for every part commented in the code 
int main(int argc, char** argv)
{
    // Play file passed as 1st argument
    if (argc > 1)
    {
        play(argv[1]);
        printf("Bye!\n");
        return 0;
    }

    // getting existent mod file list and number of files
    // it's okay to use an unordered array for small number of files
    char fileList[256][256] = {0};
    int num_files = 0;
    char* modFilePath = "/home/andreb/mods";
    DIR* dir = opendir(modFilePath);
    struct dirent* directory;
    if (dir)
    {
        while ((directory = readdir(dir)) != NULL)
        {
            if (isextensionmod(directory->d_name))
            {
                assert(num_files <= 255 && "Too Many files");
                strncpy(fileList[num_files++], directory->d_name, strlen(directory->d_name) - 4);
            }
        }
    }

    // main loop
    for(;;)
    {
        bool accepted = false;
        int modNumber = 0;
        size_t responseSize = 0;
        unsigned char* modFileContent;

        clearscreen();
        printf("#### CTRL+C to exit ####\n\n");

        // loop to generate new mod file number
        while (!accepted)
        {
            // generates random number (music number for API)
            srand(time(NULL));
            modNumber = rand() % 90000;
            char filePath[256] = {'\0'};
            char strModNumber[256] = {'\0'};
            sprintf(filePath, "%s/%d.mod", modFilePath, modNumber);
            sprintf(strModNumber, "%d", modNumber);

            // checking if mod file was (tried to) downloaded before. If so, play file instead
            printf("Checking local files for %d.mod...\n", modNumber);
            bool playedLocal = false;
            for (int i=0; i<num_files; ++i)
            {
                if (modNumber == atoi(fileList[i])) // modFile already exists. Play it and dont download again
                {
                    printf("File %s already exists. Playing...\n", fileList[i]);
                    playedLocal = true;
                    play(filePath);
                }
            }
            strcpy(fileList[num_files++], strModNumber);    // stores even an invalid mod so it would not be downloaded again in the future
            if (playedLocal)
                continue;
            
            // download file
            printf("%d.mod not found locally. Downloading...", modNumber);
            Config cfg = http_config("api.modarchive.org", 80);
            char endpoint[100] = {'\0'};
            sprintf(endpoint, "/downloads.php?moduleid=%d", modNumber);
            unsigned char* response = http_get(endpoint, cfg, &responseSize);

            // remove http header from response
            bool found = false;
            size_t beginBinary = 0;
            const char endHttpHeader[5] = {0x0d,0x0a,0x0d,0x0a,'\0'};   // 2 times \r\n
            while (!found && beginBinary < responseSize)
            {
                char word[5] = {'\0'};
                memcpy(&word[0], &response[beginBinary], 4);
                if (strcmp(word, endHttpHeader) == 0) 
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
                continue;
            }

            // accept AMIGA, IMPM and Extendend Module mod files
            accepted = (
                   is_accepted_header("IMPM", 4, binContent, 0)
                // || is_accepted_header("Extended Module:", 16, binContent, 0)
                || is_accepted_header("M.K.", 4, binContent, 0x438)
            );

            if (!accepted)
            {
                printf("Non-accepted type of mod file (%d.mod).\nDownloading a new one...\n\n", modNumber);
                continue;
            }
            modFileContent = binContent;
        }

        char* filename = save(modFilePath, modNumber, modFileContent, responseSize);
        play(filename);
    }

    printf("\nBye!\n");
    return 0;
}
