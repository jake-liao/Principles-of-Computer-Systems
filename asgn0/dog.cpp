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

/* copyFile(uint8_t fd) 
Copies file depending on the size, do-while loop
continues while there are still characters to be
read */
void copyFile(char* file)
{
    int16_t fd = open(file, O_RDONLY);
    if (fd == -1){
        char *errMsg = (char*)malloc(200);
        sprintf(errMsg, "cat: %s: No such file or directory", file);
        write(STDOUT_FILENO, errMsg, 100);
    }
    uint16_t fileSize = (uint16_t)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    uint16_t remainingBytes = fileSize;
    do {
        if(remainingBytes > BUFMAX) {
            /* rest of file is larger than BUFMAX
            use BUFMAX as buffer size */
            char *buf = (char *)malloc(BUFMAX);
            read(fd, buf, BUFMAX);
            write(STDOUT_FILENO, buf, BUFMAX);          
            free(buf);

        } else {
            /* remaining file is smaller than BUFMAX
            use remaining byte size as buffer */
            char *buf = (char *)malloc(remainingBytes);
            read(fd, buf, remainingBytes);
            write(STDOUT_FILENO, buf, remainingBytes);
            free(buf);
        }
        remainingBytes = fileSize - (uint16_t)lseek(fd, 0, SEEK_CUR);
    } while(remainingBytes > 0);
    close(fd);
    return;
}
/* copySTDIN() 
Copies file from stdin, do-while loop
allows input until ^D followed by ^D*/
void copySTDIN()
{
    uint16_t size;
    do {
        char *buf = (char *)malloc(BUFMAX);
        size = read(0, buf, BUFMAX);
        write(STDOUT_FILENO, buf, BUFMAX); 

    } while(size > 0);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        // no file, take input from stdin
        copySTDIN();

    } else {
        for(uint8_t i = 1; i < argc; i++) {
            if(strcmp("-", argv[i]) == 0) {
                // take input from stdin
                copySTDIN();
            } else {
                // copy from file
                copyFile(argv[i]);
            }
        }
    }
}