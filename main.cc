#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <string>

#include "netcode.h"
#include "stratego.h"


inline void send_ok      (int fd) {printf("%d< OK\n",fd);      write(fd,"OK\n",3);}
inline void send_invalid (int fd) {printf("%d< INVALID\n",fd); write(fd,"INVALID\n",8);}
inline void send_red     (int fd) {printf("%d< RED\n",fd);     write(fd,"RED\n",4);}
inline void send_blue    (int fd) {printf("%d< BLUE\n",fd);    write(fd,"BLUE\n",5);}
inline void send_start   (int fd) {printf("%d< START\n",fd);   write(fd,"START\n",6);}
inline void send_wait    (int fd) {printf("%d< WAIT\n",fd);    write(fd,"WAIT\n",5);}
inline void send_win     (int fd) {printf("%d< WIN\n",fd);     write(fd,"WIN\n",4);}
inline void send_lose    (int fd) {printf("%d< LOSE\n",fd);    write(fd,"LOSE\n",5);}
inline void send_empty   (int fd) {printf("%d< EMPTY\n",fd);   write(fd,"EMPTY\n",6);}
inline void send_attack  (int fd, char a, char d, char v){
	printf("%d< ATTACK %c %c %c\n",fd,a,d,v);
	static char b[14];
	sprintf(b,"ATTACK %c %c %c\n",a,d,v);
	write(fd,b,13);
}

constexpr int max3(int const a, int const b, int const c){
	return a>b?(a>c?a:c):(b>c?b:c);
}

char assign_color(char other_color, std::string const & str){
	if( str=="RED" ){
		if(other_color!='R')
			 return 'R';
		else return 'B';		
	}else if( str=="BLUE" ){
		if(other_color!='B')
			 return 'B';
		else return 'R';
	}else{
		return 0;
	}
}

void handle_setup_messages(int const fd, 
		char * const this_color, char * const other_color, char * const pieces){
	std::string input = sanitized_recvline(fd);

	if(*this_color==0){
		*this_color = assign_color(*other_color, input);
		if(*this_color=='B'){
			send_blue(fd);
		}else if(*this_color=='R'){
			send_red(fd);
		}else{
			send_invalid(fd);
		}
	}else if(*pieces==0){
		if(validate_pieces(input,pieces)){
			send_ok(fd);
		}else{
			memset(pieces,0,40);
			send_invalid(fd);
		}
	}
	// clear read buffer
	clear_read_buf(fd);
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

		for(int i=3; i<=maxfd; i+=1){
			if( FD_ISSET(i, &tmp) ){
				if(i==sockfd){
					int current = fd[0]==0?0:1;

					// if 0 players are connected
					if(fd[0]==0 && fd[1]==0){
						fd[current] = handle_new_connections(i);
						// if successful
						if(fd[current]!=0){
							printf("first player connected (fd=%d)\n",fd[current]);
							// subscribe to it
							FD_SET(fd[current], &master);
						}
					}

					// if 1 player is connected
					else if(fd[0]==0 || fd[1]==0){
						fd[current] = handle_new_connections(i);
						// if successful
						if(fd[current]!=0){
							printf("second player connected (fd=%d)\n",fd[current]);
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

bool is_in_lake(int source){
	assert(source<100);
	return Map::map[source] == '~';
}

bool parse_move(std::string input,int* source, int* dest){
	char sx,dx;
	int  sy,dy;
	if(sscanf(&input[0],"MOVE %c%d %c%d",&sx,&sy,&dx,&dy)==4
	&& sx>='A' && sx<='J' && sy>=1 && sy <=10
	&& dx>='A' && dx<='J' && dy>=1 && dy <=10){
		sx-='A';
		dx-='A';
		sy=10-sy;
		dy=10-dy;
		*source = sy*10+sx;
		*dest   = dy*10+dx;
		return true;
	}else{
		return false;
	}
}

void handle_move(int source, int dest, int current_fd, int red_fd, int blue_fd){
	if(Map::is_empty(dest)){
		Map::map[dest]=Map::map[source];
		Map::map[source]='.';
		send_empty(red_fd);
		send_empty(blue_fd);
	}else{
		char other = (current_fd==red_fd)?blue_fd:red_fd;

		char source_piece, dest_piece;
		if(current_fd==red_fd){
			dest_piece   = Map::get_blue_piece(dest);
			source_piece = Map::get_red_piece(source);
		}else{
		   	dest_piece   = Map::get_red_piece(dest);
			source_piece = Map::get_blue_piece(source);
		}

		if(dest_piece == 0){
			send_invalid(current_fd);
			return;
		}else if(dest_piece == 'F'){
			send_win(current_fd);
			send_lose(other);
			exit(0);
		}else{
			char victor = resolve_battle(source_piece,dest_piece);
			if(victor==source_piece){
				Map::map[dest] = Map::map[source];
				Map::map[source] = '.';
			}else if(victor==dest_piece){
				Map::map[source] = '.';
			}else{
				Map::map[source] = '.';
				Map::map[dest]   = '.';
			}
			send_attack(current_fd,    source_piece,dest_piece,victor);
			send_attack(other,source_piece,dest_piece,victor);
		}
	}
}

void handle_turn(int current_fd, int red_fd, int blue_fd, bool &red_turn){
	if( ( red_turn && current_fd==red_fd)
	||  (!red_turn && current_fd==blue_fd) ){
		// the one whose turn it is
		std::string input = sanitized_recvline(current_fd);

		int source, dest;
		bool parsing_valid = parse_move(input, &source, &dest);

		// if blue, mirror coordinates
		if(current_fd==blue_fd){
			source = 99-source;
			dest   = 99-dest;
		}

		if( parsing_valid && !is_in_lake(source) && !is_in_lake(dest)){
			// source and dest are both valid tiles

			char source_piece;
			if(current_fd==red_fd)
				 source_piece = Map::get_red_piece(source);
			else source_piece = Map::get_blue_piece(source);

			if(source_piece==0 || source_piece=='F' || source_piece=='B'){
				send_invalid(current_fd);
				return;
			}else{
				// source is an ordinary, movable unit
				int r = source-dest;
				if((r==  1&&(source%10!=9)) 
				|| (r== -1&&(source%10!=0)) 
				|| (r== 10&&(dest<100)) 
				|| (r==-10&&(dest>=0))){
					handle_move(source,dest,current_fd,red_fd,blue_fd);
				}else{
					send_invalid(current_fd);
					return;
				}
			}

			// send map to other player
			if(current_fd==red_fd) Map::send_blue_map(blue_fd);
			else Map::send_red_map(red_fd);

			red_turn = !red_turn;
		}else{
			send_invalid(current_fd);
		}
	}else{
		clear_read_buf(current_fd);
		send_wait(current_fd);
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

	Map::setup_map(s.red_pieces, s.blue_pieces);

	send_start(s.red_fd);
	Map::send_red_map(s.red_fd);
	send_wait(s.blue_fd);

	fd_set master;
	FD_ZERO(&master);
	FD_SET(s.red_fd,&master);
	FD_SET(s.blue_fd,&master);
	bool red_turn = true;
	while(true){
		fd_set tmp = master;
		int maxfd = s.red_fd>s.blue_fd?s.red_fd:s.blue_fd;
		select(maxfd+1, &tmp, NULL, NULL, NULL);
		for(int i=3; i<=maxfd; i+=1){
			if(FD_ISSET(i,&tmp)){
				handle_turn(i,s.red_fd,s.blue_fd,red_turn);
			}
		}
	}
	return 0;
}

// vim: set sw=4 ts=4
