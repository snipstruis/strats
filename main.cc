#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include "stratego.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

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
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

		int yes=1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
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

constexpr int max3(int const a, int const b, int const c){
	return a>b?(a>c?a:c):(b>c?b:c);
}

int sanitize(char *buf, int size){
// Modifies the buffer, and returns the new size. This is always safe, since the 
// sanitized text is always smaller than the original.
// returns 0 if the buffer contains invalid character.
	int out = 0;
	bool prev_space = true;

	for(int i=0; i<size; i+=1){
		if(buf[i] <= ' '){
			if(prev_space){
				continue;
			}else{
				prev_space = true;
				buf[out++] = buf[i];
			}
		}else if( (buf[i]>='A' && buf[i]<='Z') || (buf[i]>='0' && buf[i]<='9') ){
			prev_space = false;
			buf[out++] = buf[i];
		}else{
			return 0;
		}
	}

	// remove trailing space if any
	if(buf[out-1]<=' ') out-=1;

	return out;
}

char assign_color(char other_color, char *buf, size_t size){
	if( size==3 && memcmp(buf,"RED",3)==0 ){
		if(other_color!='R')
			return 'R';
		else
			return 'B';		
	}else if( size==4 && memcmp(buf,"BLUE",4)==0 ){
		if(other_color!='B')
			return 'B';
		else
			return 'R';
	}else{
		return 0;
	}
}

char buf[110];

void handle_setup_messages(int fd, char* this_color, char* other_color, char* pieces){
	if(*this_color==0){
		// receive preferred color
		int bytes_read = recv(fd, buf, sizeof(buf),0);
		bytes_read = sanitize(buf,bytes_read);
		if(bytes_read == 0){
			send(fd,"INVALID\n",8,0);
			return;
		}

		*this_color = assign_color(*other_color, buf, bytes_read);
		if(*this_color=='B'){
			printf("assigned BLUE to %d\n",fd);
			send(fd,"BLUE\n",5,0);
		}else if(*this_color=='R'){
			printf("assigned RED to %d\n",fd);
			send(fd,"RED\n",4,0);
		}else{
			send(fd,"INVALID\n",8,0);
		}
	}else if(*pieces==0){
		// validate pieces
		int bytes_read = recv(fd, buf, sizeof(buf),0);	
		bytes_read = sanitize(buf, bytes_read);
		if(bytes_read==0){send(fd,"INVALID\n",8,0); return;}

		if(validate_pieces(buf,bytes_read)){
			memcpy(pieces,buf,40);
			printf("received valid piece setup from %d\n",fd);
			send(fd,"OK\n",3,0);
		}else{
			send(fd,"INVALID\n",8,0);
		}
	} else {
		// ignore other input
		recv(fd, buf, sizeof(buf),0);
	}
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

struct setup_info{
	int red_fd, blue_fd;
	char red_pieces[40], blue_pieces[40];
};

setup_info setup_game(int sockfd){
	// set of sockets to listen to
	fd_set master;
	FD_ZERO(&master);
	FD_SET(sockfd,&master);

	int  fd[2]={0,0};
	char color[2]={0,0}; // red='R' blue='B' unset='\0'
	char pieces[2][40];
	memset(pieces,0,80); // zero initialize pieces

	while(true){
		fd_set tmp = master;
		int const maxfd = max3(sockfd,fd[0],fd[1]);
		int sel = select(maxfd+1, &tmp, NULL, NULL, NULL);
		if(sel<=0){perror("select broke D:"); exit(-1);}

		for(int i=0; i<=maxfd; i+=1){
			if( FD_ISSET(i, &tmp) ){
				if(i==sockfd){
					int current = fd[0]==0?0:1;

					// if 0 players are connected
					if(fd[0]==0 && fd[1]==0){
						fd[current] = handle_new_connections(i);
						// if successful
						if(fd[current]!=0){
							printf("first player connected (fd=[%d, %d])\n",fd[0],fd[1]);
							// subscribe to it
							FD_SET(fd[current], &master);
						}
					}
					
					// if 1 player is connected
					else if(fd[0]==0 || fd[1]==0){
						assert(fd[0]==0 || fd[1]==0);
						fd[current] = handle_new_connections(i);
						// if successful
						if(fd[current]!=0){
							printf("second player connected (fd=[%d, %d])\n",fd[0],fd[1]);
							// subscribe to it
							FD_SET(fd[current], &master);
							// stop listening for connections
							FD_CLR(sockfd,&master);
							close(sockfd);
						}
					}else{
						perror("accedentilly accepted third connection, bailing!");
						exit(-1);
					}
				}else{
					int current = fd[0]==i?0:1;
					int other   = fd[0]==i?1:0;
					handle_setup_messages(i,&color[current],&color[other],pieces[current]);

					// if all data is now present
					if(fd[0]!=0        && fd[1]!=0
					&& color[0]!=0     && color[1]!=0
					&& pieces[0][0]!=0 && pieces[1][0]!=0){
						// return it in a more convenient format
						int blue,red;
						if(color[0]=='R'){ red=0; blue=1; }
						else { red=1; blue=0;}
						
						setup_info s;
						s.red_fd  = fd[red];
						s.blue_fd = fd[blue];
						memcpy(s.red_pieces,  pieces[red], 40);
						memcpy(s.blue_pieces, pieces[blue], 40);
						return s;
					} 
				}
			}
		}
	}
}

int main(int argc, char** argv){
	if(argc!=2){
		printf("usage: %s [portnr]\n",argv[0]);
		exit(-1);
	}

	int sockfd = create_and_bind_socket(argv[1]);

    if (listen(sockfd, 3) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

	auto s = setup_game(sockfd);

	write(s.red_fd,"START\n",6);
	write(s.blue_fd,"WAIT\n",5);

	Map::setup_map(s.red_pieces, s.blue_pieces);
	
	puts("\nred map");
	Map::send_red_map(1);

	puts("\nblue map");
	Map::send_blue_map(1);

	return 0;
}

