#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/tcp.h>

static char buf[1024];

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int create_and_bind_socket(char const * const port){
	int sockfd;

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use this IP

	int rv;
	struct addrinfo *servinfo;
	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	struct addrinfo *p;
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		int yes=1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		// send packets immediatly, don't accumulate.
		if(setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY, &yes, sizeof(int)) == -1) {
			perror("failed to set TCP_NODELAY");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return -2;
	}

	freeaddrinfo(servinfo);
	return sockfd;
}

void clear_read_buf(int fd){
	while(recv(fd,buf,sizeof(buf),MSG_DONTWAIT)>0){};
}

int handle_new_connections(int sockfd){
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size = sizeof their_addr;
	int fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if(fd == -1) {
		return 0;
	}else{
		return fd;
	}
}

std::string sanitized_recvline(int fd){
	assert(fd>0);
	std::string s="";
	char prev;
	char c = '\n';
	int  r;
	while(true){
		prev = c;
		r = read(fd,&c,1);
		if(r!=1) break;
		else if(c=='\n') {if(prev<=' '){s.pop_back();} break;}
	 	else if(c<=' ' && prev<=' ') continue;
		else if( (c>='A'&&c<='Z') || (c>='0'&&c<='9') ) s += c;
		else if( c<=' ' ) s += ' ';
		else continue;
	};
	printf("%d> %s\n", fd, s.c_str());
	return s;
}

// vim: set sw=4 ts=4
