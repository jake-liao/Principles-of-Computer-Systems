/*------------------------------------------------------------------------------/
CSE130 	Prof. Miller 		pa0		dog.c
hsliao	Jake Hsueh-Yu Liao	1551558
/------------------------------------------------------------------------------*/
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BUFMAX 32000

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
    // read stdin ONCE
    uint16_t fileSize = (uint16_t)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (BUFMAX == 0){
    	return;
    } else {
		char *buf = (char *)malloc(fileSize);
	    read(fd, buf, fileSize);
	    write(STDOUT_FILENO, buf, fileSize);
	    free(buf);
    }
    return;
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        // not file, take input from stdin
        copyFile(STDIN_FILENO);
        return(0);

    } else {
        for(uint8_t i = 1; i < argc; i++) {
            if(strcmp("-", argv[i]) == 0) {
                // no file take input from stdin
                copyFile(STDIN_FILENO);
            } else {
                char *path = getPath(argv[i]);
                uint8_t fd = open(path, O_RDWR);
                copyFile(fd);
                close(fd);
                free(path);
            }
        }
    }
}