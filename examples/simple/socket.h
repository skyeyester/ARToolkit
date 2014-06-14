#ifndef _SOCKET_
#define _SOCKET_

#define MAX_CONN 32

#include <sys/types.h>          // for socket(), fork()
#include <sys/socket.h>         // for socket()
#include <errno.h>              // for perror()
#include <netinet/in.h>         // for sockaddr_in
#include <unistd.h>             // for write(), fork()


int TCPServer(int port){
	// socket
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
                perror("sock error");
                return -1;
        }

	// bind
        struct sockaddr_in local;
        bzero(&local, sizeof(struct sockaddr_in));
        local.sin_family = AF_INET;
        local.sin_port = htons(port); // !! NOTICE: htons()
        local.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr*) &local, sizeof(struct sockaddr_in)) == -1) {
                perror("bind error");
                return -1;
        }

	// listen
        if (listen(sockfd, MAX_CONN) == -1) {
                perror("listen error");
                return -1;
        }
	
	return sockfd;
}

#endif
