/*------------------------------------------------------------------------------/
CSE130 	Prof. Miller 		asgn2		httpserver.cpp
hsliao	Jake Hsueh-Yu Liao	1551558
/------------------------------------------------------------------------------*/
#include <arpa/inet.h>
#include <cstdlib>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#define ERR -1
#define EMPTY -1
#define WORKING 1
#define WAITING 0
#define BUFMAX 32768
#define HEADERMAX 4096

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
uint8_t active_threads = 0;
uint8_t waiting_threads = 0;
int8_t CR[BUFMAX] = {EMPTY};
uint8_t thread_state[BUFMAX] = {WAITING};

void *worker(void* id);
void handle_client(int8_t soc_fd);
int8_t socket_setup(uint8_t argc, char *address, char *port);

int main(int argc, char *argv[])
{
    // argument handle
    if(argc < 2 || argc > 7)
        err(1, "incorrect argument count\n argc: %d", argc);
    int8_t opt;
    uint8_t Nflag = 0, lflag = 0, thread_count = 4;
    while((opt = getopt(argc, argv, "N:l")) != ERR) {
        switch(opt) {
            case 'N':
                ++Nflag;
                if(optarg)
                    thread_count = atoi(optarg);
            case 'l':
                ++lflag;
            default:
                break;
        }
    }
    if(thread_count < 1)
        err(1, "Cannot run with less than 1 thread\n");
    if(optind + 2 != argc)
        err(1, "invalid argument(s)\n");

    // socket_setup
    int8_t socket = socket_setup(argc, argv[optind], argv[optind + 1]);
    printf("socket success\n");
    // create threads
    pthread_t* thread_pool = (pthread_t*) malloc(sizeof(pthread_t)* thread_count);
    uint8_t n;
    printf("thread_count:%d\n", thread_count);
    for ( n = 0; n < thread_count; ++n){
      pthread_create(&thread_pool[n], NULL, worker, NULL);
      printf("complete thread %d\n", n);
    }
    printf("prep success\n");
    // dispatcher
    while(1) {
        uint8_t i;
        int8_t acc_soc;
        if((acc_soc = accept(socket, NULL, NULL)) == ERR)
            err(1, "accept() failure");

        pthread_mutex_lock(&mutex);
        while (active_threads == thread_count){
          pthread_cond_wait(&empty, &mutex);
        }
        for ( i = 0 ; i< thread_count; ++ i){
          if (thread_state[i] == WAITING) break;
        }
        CR[i] = acc_soc;
        ++active_threads;
        thread_state[i] = WORKING;
        pthread_cond_broadcast(&full);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}

void *worker(void* id){
  uint8_t i = *(uint8_t*) (&id);
  uint8_t soc;
  while(1){
    pthread_mutex_lock(&mutex);

    while (thread_state[i] == WAITING){
      ++waiting_threads;
      pthread_cond_wait(&full, &mutex);
      --waiting_threads;
    }
    soc = CR[i];
    handle_client(soc);
    CR[i] = EMPTY;
    thread_state[i] = thread_state[i] -1;
    --active_threads;
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&mutex);
  }
}

int8_t socket_setup(uint8_t argc, char *address, char *port)
{
    struct addrinfo *addrs, hints = {};
    int enable = 1;
    int8_t N = 16;
    int8_t main_socket;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(argc == 3) {
        getaddrinfo(address, port, &hints, &addrs);
    } else {
        getaddrinfo(address, "80", &hints, &addrs);
    }
    if((main_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol)) == ERR)
        err(1, "socket() failure");
    if(setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == ERR)
        err(1, "setsockopt() failure");
    if(bind(main_socket, addrs->ai_addr, addrs->ai_addrlen) == ERR)
        err(1, "bind() failure");
    if(listen(main_socket, N) == ERR)
        err(1, "listen() failure");
    return main_socket;
}

void handle_client(int8_t soc_fd)
{
    uint8_t buffer[BUFMAX] = { 0 };
    uint8_t header[HEADERMAX] = { 0 };
    uint8_t headerSize = 0;
    uint8_t payload[BUFMAX] = { 0 };
    int64_t payloadSize = 0;
    int8_t fd;
    char command[4] = { 0 };
    char filename[27] = { 0 };
    char *substring_start = nullptr;
    char *substring_end = nullptr;
    int size = 0;
    read(soc_fd, (char *)buffer, sizeof(buffer));
    // get content-size line
    substring_start = strstr((char *)buffer, "Content-Length: ");
    if(substring_start != nullptr) {
        substring_end = strstr(substring_start, "\r");
        int sub_len = substring_end - substring_start - 16;
        char cont_len_substr[sub_len];
        strncpy(cont_len_substr, substring_start + 16, sub_len);
        size = atoi(cont_len_substr);
        if(size > 0) {
            read(soc_fd, (char *)payload, sizeof(payload));
        }
    }
    sscanf((char *)buffer, "%s %s ", command, filename);

    if(strcmp(filename, "/") == 0 || strlen(filename) != 27) {
        strcat((char *)header, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
        headerSize = 45;
    } else if(strcmp(command, "PUT") == 0) {
        if(access(filename, W_OK) == 0) {
            remove(filename);
        }
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if(fd == ERR) {
            headerSize = 38;
            strcat((char *)header, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        } else {
            if(write(fd, payload, size) == ERR) {
                strcat((char *)header, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
            } else {
                headerSize = 43;
                strcat((char *)header, "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");
                close(fd);
            }
        }

    } else if(strcmp(command, "GET") == 0) {
        fd = open(filename, O_RDONLY);
        if(fd == ERR) {
            headerSize = 26;
            strcat((char *)header, "HTTP/1.1 404 Not Found\r\n\r\n");
        } else {
            struct stat sb;
            stat(filename, &sb);
            payloadSize = sb.st_size;
            headerSize = log10(payloadSize) + 1 + 37;
            sprintf((char *)header, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", sb.st_size);
        }
    } else {
        headerSize = 24;
        strcat((char *)header, "HTTP/1.1 403 Forbidden\r\n");
    }

    if(send(soc_fd, header, headerSize, 0) == ERR)
        err(1, "send() failed");
    while(payloadSize > (int64_t)BUFMAX) {
        read(fd, payload, BUFMAX);
        if(send(soc_fd, payload, BUFMAX, 0) == ERR)
            err(1, "send() failed");
        payloadSize = payloadSize - (int64_t)BUFMAX;
        if(payloadSize == 0)
            close(fd);
    }
    if(payloadSize > 0) {
        read(fd, payload, payloadSize);
        if(send(soc_fd, payload, payloadSize, 0) == ERR)
            err(1, "send() failed");
        close(fd);
        payloadSize = payloadSize - payloadSize;
    }
    return;
}