#include<arpa/inet.h> 
#include<cstdlib>
#include<netdb.h> 
#include<netinet/in.h> 
#include<stdio.h> 
#include<string.h> 
#include<sys/socket.h> 
#include<sys/types.h> 
#include<sys/un.h> 
#include<unistd.h>

#define BUFMAX 32768
#define HOSTNAMESIZE 100
#define ERR -1
#define DEFAULTPORT 80
int main(int argc, char *argv[])
{
	if (argc < 2){
		printf("Missing arguments\n");
		exit(EXIT_FAILURE);
	}
	int16_t s_fd;
	char hostname[HOSTNAMESIZE];
	struct sockaddr_in addr;
	int addrlen = sizeof(addr); 
	char buffer[1024] = {0};

	s_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (s_fd == ERR){
		printf("socket failure\n");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(DEFAULTPORT);
	if (gethostname(hostname, sizeof(hostname)) == ERR){
		printf("gethostname failure\n");
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[1], hostname) == 0 || strcmp(argv[1], "localhost") == 0){
		printf("hostname or localhost detected\n");
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	} else {
		printf("assuming correct IPv4 address\n");
		struct in_addr inAddr;
		addr.sin_addr.s_addr = inet_aton(argv[1], &inAddr);
	}
	if (argc == 3){
		addr.sin_port = htons(atoi(argv[2]));
	}
	if (bind(s_fd, (struct sockaddr*)&addr,sizeof(addr)) == ERR){
		printf("bind failure\n");
		exit(EXIT_FAILURE);
	}
	if (listen(s_fd, 3) == ERR){
		printf("listen failure\n");
		exit(EXIT_FAILURE);
	}
	int16_t acc_soc = accept(s_fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
	if (acc_soc == ERR){
		printf("accept failure\n");
		exit(EXIT_FAILURE);
	}
	read(acc_soc, buffer, 1024);
	printf("%s\n", buffer);
	send(acc_soc, "fuck you", strlen("fuck you"), 0);
	printf("message sent\n");
	return 0;

}