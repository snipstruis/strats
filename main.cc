#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <string>

#include "netcode.h"
#include "stratego.h"


inline void sendln(int fd, std::string str){
	assert(fd>=1);
	printf("%d< %s\n",fd,str.c_str());
	write(fd,&(str+'\n')[0],str.size()+1);
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
			sendln(fd,"BLUE");
		}else if(*this_color=='R'){
			sendln(fd,"RED");
		}else{
			sendln(fd,"INVALID COLOR");
		}
	}else if(*pieces==0){
		if(validate_pieces(input,pieces)){
			sendln(fd,"OK");
		}else{
			memset(pieces,0,40);
			sendln(fd,"INVALID SETUP");
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
							printf("-- first player connected (fd=%d)\n",fd[current]);
							// subscribe to it
							FD_SET(fd[current], &master);
						}
					}

					// if 1 player is connected
					else if(fd[0]==0 || fd[1]==0){
						fd[current] = handle_new_connections(i);
						// if successful
						if(fd[current]!=0){
							printf("-- second player connected (fd=%d)\n",fd[current]);
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

bool has_valid_moves(bool red){
	for(int y=0; y<10; y+=1){
		for(int x=0; x<10; x+=1){
			if( Map::owner_of(x,y)==(red?'R':'B') 
			 && Map::can_move(x,y)) return true;
		}
	}
	return false;
}

std::string handle_move(std::string input, bool red_turn){
	int source, dest;
	bool parsing_valid = parse_move(input, &source, &dest);

	if(!parsing_valid) 
		return "INVALID MOVE";

	if(!red_turn){
		source = 99-source;
		dest   = 99-dest;
	}

	if(Map::is_in_lake(source) || Map::is_in_lake(dest))
		return "INVALID MOVE";

	char source_piece = red_turn ? Map::get_red_piece(source) : Map::get_blue_piece(source);

	if(source_piece==0 || source_piece=='F' || source_piece=='B')
		return "INVALID MOVE";
	

	if(source_piece=='2'){
		int r  = source-dest;
		int rx = r%10;
		int ry = r/10;

		int update;
		if(rx==0 && ry!=0){
			update = ry>0?-10:10;
		}else if(rx!=0 && ry==0){
			update = rx>0?-1:1;
		}else return "INVALID MOVE";

		for(int i=source+update; i!=dest; i+=update){
			if(!Map::is_empty(i)) return "INVALID MOVE";
		}
	}else{
		int r = source-dest;
		if(!((r==  1&&(dest%10!=9)) 
		  || (r== -1&&(dest%10!=0)) 
		  || (r== 10&&(dest<100)) 
		  || (r==-10&&(dest>=0))))
			return "INVALID MOVE";
	}

	if(Map::is_empty(dest)){
		Map::map[dest]=Map::map[source];
		Map::map[source]='.';
		return "ATTACK NONE";
	}
	
	char dest_piece = red_turn ? Map::get_blue_piece(dest) : Map::get_red_piece(dest);

	if(dest_piece == 0){
		return "INVALID MOVE";
	}
	
	if(dest_piece == 'F'){
		printf("-- %s's flag has been captured, %s wins!\n",
			red_turn?"blue":"red",
			red_turn?"red":"blue");
		return "WIN";
	}
		
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

	if(!has_valid_moves(red_turn)){
		printf("-- %s has no valid moves left, %s wins!\n",red_turn?"red":"blue",red_turn?"blue":"red");
		return "WIN";
	}
	
	if(!has_valid_moves(!red_turn)){
		printf("-- %s has no valid moves left, %s wins!\n",red_turn?"blue":"red",red_turn?"red":"blue");
		return "LOSE";
	}

	std::string output = "ATTACK ";
	output += source_piece;
	output += ' ';
	output += dest_piece;
	output += ' ';
	output += victor;
	return output;
}

int main(int argc, char** argv){
	int sockfd;

	if(argc==2){
		sockfd=create_and_bind_socket(argv[1]);
	}else if(argc==1){
		sockfd=create_and_bind_socket("3720");
	}else{
		printf("usage: %s [portnr]\n",argv[0]);
		exit(-1);
	}

	if (listen(sockfd, 3) == -1) {
		perror("listen");
		exit(1);
	}

	printf("-- server: waiting for connections...\n");

	auto s = setup_game(sockfd);

	Map::setup_map(s.red_pieces, s.blue_pieces);

	int  current_fd = s.red_fd;
	int  other_fd   = s.blue_fd;
	bool red_turn   = true;

	printf("-- turn 1\n");
	sendln(current_fd,"DEFEND NONE");
	bool exit = false;

	std::string last_move = "NONE";
	
	for(size_t turn=2; exit==false; turn+=1){
		// send map
		if(current_fd==s.red_fd) Map::send_red_map(s.red_fd);
		else Map::send_blue_map(s.blue_fd);

		while(true){
			std::string input = sanitized_recvline(current_fd);
			if(input.compare(0,4,"MOVE")==0){
				std::string move = handle_move(input,red_turn);
				last_move = input;
				if(move=="INVALID MOVE"){
					sendln(current_fd,move);
					continue;
				}else if(move.compare(0,6,"ATTACK")==0){
					sendln(current_fd,move);
					printf("-- turn %zu (%s)\n",turn,red_turn?"red":"blue");
					sendln(other_fd,"DEFEND"+move.substr(6));

					clear_read_buf(other_fd);
					std::swap(current_fd, other_fd);
					red_turn=!red_turn;
					break;
				}else if(move=="WIN"){
					sendln(current_fd,"WIN");
					sendln(other_fd,"LOSE");
					exit=true;
					break;
				}else if(move=="LOSE"){
					sendln(current_fd,"LOSE");
					sendln(other_fd,"WIN");
					exit=true;
					break;
				}
			}else if(input=="SURRENDER"){
				sendln(current_fd,"LOSE");
				sendln(other_fd,"WIN");
				exit=true;
				break;

			}else if(input.compare(0,11,"GETLASTMOVE")==0){
				sendln(current_fd,"LASTMOVE "+ inverse_move(last_move));
				continue;
			}else{
				sendln(current_fd,"INVALID COMMAND");
				continue;
			}
		}
	}
	return 0;
}

// vim: set sw=4 ts=4
