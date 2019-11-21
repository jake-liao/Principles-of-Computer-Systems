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
#include <semaphore.h>
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

sem_t sem;
uint16_t offsetCR = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
uint8_t active_threads = 0;
uint8_t waiting_threads = 0;
int8_t CR[BUFMAX] = { EMPTY };
uint8_t thread_state[BUFMAX] = { WAITING };

struct thread_parameter {
    uint8_t thread_id;
    uint8_t log_flag;
    int8_t log_fd;
    char *logfile;
};

void *worker(void *id);
void handle_client(int8_t soc_fd, void *id);
int8_t socket_setup(uint8_t argc, char *address, char *port);

int main(int argc, char *argv[])
{
    //__ARGUMENT_____________________________________
    if(argc < 2 || argc > 7)
        err(1, "incorrect argument count\n argc: %d", argc);
    int8_t opt, arg_count = argc;
    uint8_t thread_count = 4, lflag = 0;
    char *log_name = 0;
    int8_t log_fd = ERR;
    while((opt = getopt(argc, argv, "N:l:")) != ERR) {
        switch(opt) {
            case 'N':
                --arg_count;
                if(optarg)
                    --arg_count;
                thread_count = atoi(optarg);
                break;
            case 'l':
                ++lflag;
                --arg_count;
                if(optarg) {
                    --arg_count;
                    log_name = (char *)malloc(sizeof(char *) * sizeof(optarg));
                    strcpy(log_name, optarg);
                }
                break;
            default:
                break;
        }
    }
    if(thread_count < 1)
        err(1, "Cannot run with less than 1 thread\n");
    if(arg_count < 1)
        err(1, "invalid argument(s)\n");
    if(lflag) {
        sem_init(&sem, 0, 1);
        log_fd = open(log_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    }
    //__SOCKET_______________________________________
    int8_t socket = socket_setup(arg_count, argv[optind], argv[optind + 1]);
    //__THREADS______________________________________
    pthread_t *thread_pool = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    uint8_t n;
    struct thread_parameter *t_param =
      (struct thread_parameter *)malloc(sizeof(struct thread_parameter) * thread_count);
    for(n = 0; n < thread_count; ++n) {
        t_param[n].thread_id = n;
        t_param[n].log_flag = lflag;
        if(lflag)
            t_param[n].log_fd = log_fd;
        pthread_create(&thread_pool[n], NULL, worker, &t_param[n]);
    }

    //__DISPATCH______________________________________
    while(1) {
        uint8_t i;
        int8_t acc_soc;
        if((acc_soc = accept(socket, NULL, NULL)) == ERR)
            err(1, "accept() failure");
        pthread_mutex_lock(&mutex);
        while(active_threads == thread_count) {
            pthread_cond_wait(&empty, &mutex);
        }
        for(i = 0; i < thread_count; ++i) {
            if(thread_state[i] == WAITING)
            break;
        }
        CR[i] = acc_soc;
        ++active_threads;
        thread_state[i] = WORKING;
        pthread_cond_broadcast(&full);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}

void *worker(void *id)
{
    struct thread_parameter p = *(struct thread_parameter *)id;
    uint8_t i = p.thread_id;
    uint8_t soc;
    while(1) {
        pthread_mutex_lock(&mutex);
        printf("thread #%d locked mutex\n", i);
        while(thread_state[i] == WAITING) {
            ++waiting_threads;
            pthread_cond_wait(&full, &mutex);
            --waiting_threads;
        }
        printf("Worker %d working\n", i);
        soc = CR[i];
        handle_client(soc, id);
        CR[i] = EMPTY;
        thread_state[i] = thread_state[i] - 1;
        --active_threads;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        printf("thread #%d unlocked mutex\n", i);
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

void handle_client(int8_t soc_fd, void *id)
{
    struct thread_parameter p = *(struct thread_parameter *)id;
    uint8_t buffer[BUFMAX] = { 0 };
    uint8_t header[HEADERMAX] = { 0 };
    uint8_t headerSize = 0;
    uint8_t payload[BUFMAX] = { 0 };
    uint8_t err_flag = 0;
    int64_t payloadSize = 0;
    int8_t fd;
    char command[4] = { 0 };
    char filename[27] = { 0 };
    char *substring_start = nullptr;
    char *substring_end = nullptr;
    int size = 0;
    read(soc_fd, (char *)buffer, sizeof(buffer));
    //__GET CONTENT SIZE______________________________
    substring_start = strstr((char *)buffer, "Content-Length: ");
    if(substring_start != nullptr) {
        substring_end = strstr(substring_start, "\r");
        int sub_len = substring_end - substring_start - 16;
        char cont_len_substr[sub_len];
        strncpy(cont_len_substr, substring_start + 16, sub_len);
        size = atoi(cont_len_substr);
        if(size > 0)
            read(soc_fd, (char *)payload, sizeof(payload));
    }
    sscanf((char *)buffer, "%s %s ", command, filename);
    //__PARSING_______________________________________
    if(strcmp(filename, "/") == 0 || strlen(filename) != 27) {
        strcat((char *)header, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
        headerSize = 45;
    } else if(strcmp(command, "PUT") == 0) {
        if(access(filename, W_OK) == 0)
            remove(filename);
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
        printf("Forbidden\n");
        headerSize = 24;
        strcat((char *)header, "HTTP/1.1 403 Forbidden\r\n");
    }

    //__LOGGING_______________________________________
    if(p.log_flag) {
        char buff[BUFMAX] = { 0 };
        uint16_t cursor = 0;
        if(err_flag) {
            sem_wait(&sem);
            offsetCR += (31 + strlen(command) + strlen(filename));
            cursor = offsetCR;
            sem_post(&sem);
            sprintf(buff, "FAIL: %s %s HTTP/1.1 --- response %d\n", command, filename, err);
            pwrite(p.log_fd, buff, strlen(buff), cursor);
        } else {
            sem_wait(&sem);
            cursor = offsetCR;
            printf("cursor before:%hu\n", cursor);
            offsetCR +=
              (40 + (ceil(log10(size)) + 1)  + 9 * (floor(size / 20) + 1)  + 3 * size + (9));
            printf("offsetCR after:%hu\n", offsetCR);
            sem_post(&sem);
            uint16_t count = 0;
            sprintf(buff, "%s %s length %d\n\0", command, filename, size);
            printf("%s", buff);
            pwrite(p.log_fd, buff, strlen(buff), cursor);
            cursor += strlen(buff);
            uint16_t j = 0;
            while(count < size ) {
                sprintf(buff, "%08d", count);
                printf("%s", buff);
                count += 20;
                pwrite(p.log_fd, buff, strlen(buff), cursor);
                cursor += strlen(buff);
                int k = 0;
                for(; k < 20 ; ++k) {
                    if (j ==size) break;
                    if (payload[j] == 0) break;
                    // printf("k:%d", k);
                    sprintf(buff, " %02x", (unsigned int)payload[j]);
                    printf("%s", buff);
                    pwrite(p.log_fd, buff, strlen(buff), cursor);
                    cursor += 3;
                    ++j;
                } 
                pwrite(p.log_fd, "\n", 1, cursor);
                printf("%s", "\n");
                cursor += 1;
            }
            sprintf(buff, "========\n");
            printf("%s", buff);
            pwrite(p.log_fd, buff, strlen(buff), cursor);
            cursor += strlen(buff);
        }
    }
    //__SENDING_______________________________________
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
