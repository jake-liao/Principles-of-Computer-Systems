/*------------------------------------------------------------------------------/
CSE130 	Prof. Miller 		asgn1		httpserver.cpp
hsliao	Jake Hsueh-Yu Liao	1551558
/------------------------------------------------------------------------------*/
#include <arpa/inet.h>
#include <cstdlib>
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#define ERR -1
#define BUFMAX 32768
#define HEADERMAX 4096

void handle_client(int8_t soc_fd) {
  uint8_t buffer[BUFMAX] = {0};
  uint8_t header[HEADERMAX] = {0};
  uint8_t headerSize = 0;
  char command[4] = {0};
  char filename[27] = {0};
  char data[HEADERMAX] = {0};
  int size = 0;
  read(soc_fd, (char *)buffer, sizeof(buffer));
  sscanf((char *)buffer, "%s %s %*s %*s %*s %*s %*s %*s %*s %d %s", command,
         filename, &size, data);
  memset(buffer, 0, BUFMAX);
  if (strcmp(filename, "/") == 0 || strlen(filename) != 27) {
    strcat((char *)header, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
    headerSize = 45;
  } else if (strcmp(command, "PUT") == 0) {
    if (access(filename, W_OK) == 0) {
      remove(filename);
    }
    int8_t fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);
    if (fd == ERR) {
      headerSize = 38;
      strcat((char *)header, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
    } else {
      write(fd, data, sizeof(data));
      headerSize = 43;
      strcat((char *)header, "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");
    }

  } else if (strcmp(command, "GET") == 0) {
    int8_t fd = open(filename, O_RDONLY);
    if (fd == ERR) {
      headerSize = 26;
      strcat((char *)header, "HTTP/1.1 404 Not Found\r\n\r\n");
    } else {
      uint8_t fileSize = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, 0);
      char fileData[fileSize];
      read(fd, fileData, fileSize);
      close(fd);
      headerSize = log10 (fileSize) + 1 + 37 + fileSize;
      int8_t dig = log10 (fileSize) + 1;
      printf("headersize:%d\nlog+1:%d\n", headerSize, dig);
      sprintf((char *)buffer, "HTTP/1.1 200 OK\r\nContent-Length: %hhu\r\n\r\n%s\r\n", fileSize,
              fileData);
      strcat((char *)header, (char *)buffer);
    }
  } else {
  	headerSize = 24;
    strcat((char *)header, "HTTP/1.1 403 Forbidden\r\n");
  }
  if (send(soc_fd, (char *)header, headerSize, 0) == ERR)
    err(1, "send() failed");
  return;
}

int main(int argc, char *argv[]) {
  if (argc < 2 || argc >= 4)
    err(1, "missing argument(s)\n argc: %d", argc);
  struct addrinfo *addrs, hints = {};
  int enable = 1;
  int8_t N = 16;
  int8_t main_socket;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (argc == 3) {
    getaddrinfo(argv[1], argv[2], &hints, &addrs);
  } else {
    getaddrinfo(argv[1], "80", &hints, &addrs);
  }
  if ((main_socket = socket(addrs->ai_family, addrs->ai_socktype,
                            addrs->ai_protocol)) == ERR) {
    err(1, "socket() failure");
  }
  if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable,
                 sizeof(enable)) == ERR) {
    err(1, "setsockopt() failure");
  }
  if (bind(main_socket, addrs->ai_addr, addrs->ai_addrlen) == ERR) {
    err(1, "bind() failure");
  }
  if (listen(main_socket, N) == ERR) {
    err(1, "listen() failure");
  }
  while (1) {
    int16_t acc_soc;
    if ((acc_soc = accept(main_socket, NULL, NULL)) == ERR) {
      err(1, "accept() failure");
    }
    handle_client(acc_soc);
  }
  return 0;
}