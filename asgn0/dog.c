/*------------------------------------------------------------------------------/
CSE130 	Prof. Miller 		pa0		dog.c
hsliao	Jake Hsueh-Yu Liao	1551558
/------------------------------------------------------------------------------*/
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define BUFMAX 32768

/*
file desciptor Macro
0 -> stdin -> STDIN_FILENO
1 -> stdout
2 -> stderr
*/

char *getPath(char *filename)
{
    char *path = (char *)malloc(sizeof(filename) + 2);
    strcpy(path, "./");
    strcat(path, filename);
    return path;
}

void copyFile(uint8_t fd)
{
	// read file
    uint16_t fileSize = (uint16_t)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    int EOFFlag = getchar();
    if(EOFFlag == EOF) {
        exit(EXIT_SUCCESS);
    } else {
        uint16_t residualBytes = fileSize;
        do {
            if(residualBytes > BUFMAX) {
                /* rest of file is larger than BUFMAX
                use BUFMAX as buffer size */
                char *buf = (char *)malloc(BUFMAX);
                read(fd, buf, BUFMAX);
                write(STDOUT_FILENO, buf, BUFMAX);
                free(buf);
            } else {
                /* remaining file is smaller than BUFMAX
                use remaining byte size as buffer */
                char *buf = (char *)malloc(residualBytes);
                read(fd, buf, residualBytes);
                write(STDOUT_FILENO, buf, residualBytes);
                free(buf);
            }
            residualBytes = fileSize - (uint16_t)lseek(fd, 0, SEEK_CUR);
            // get rest of file
        } while(residualBytes > 0);
    }
    return;
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        // no file, take input from stdin
        copyFile(STDIN_FILENO);
        return (0);

    } else {
        for(uint8_t i = 1; i < argc; i++) {
            if(strcmp("-", argv[i]) == 0) {
                // take input from stdin
                copyFile(STDIN_FILENO);
            } else {
            	// copy from file
                char *path = getPath(argv[i]);
                uint8_t fd = open(path, O_RDONLY);
                copyFile(fd);
                close(fd);
                free(path);
            }
        }
    }
}