#pragma once
#include <unistd.h>

bool validate_pieces(std::string const & input, char* output){
	if(input.size()<40) return false;
	signed char _B=6, _F=1, _M=1,
				_9=1, _8=2, _7=3,
				_6=4, _5=4, _4=4,
				_3=5, _2=8, _1=1;
	int p=0;
	for(size_t i=0; i<input.size(); i+=1){
		if(p>=40) return false;
		switch(input[i]){
			case 'B':_B-=1;output[p++]=input[i];break;
			case 'F':_F-=1;output[p++]=input[i];break;
			case 'M':_M-=1;output[p++]=input[i];break;
			case '9':_9-=1;output[p++]=input[i];break;
			case '8':_8-=1;output[p++]=input[i];break;
			case '7':_7-=1;output[p++]=input[i];break;
			case '6':_6-=1;output[p++]=input[i];break;
			case '5':_5-=1;output[p++]=input[i];break;
			case '4':_4-=1;output[p++]=input[i];break;
			case '3':_3-=1;output[p++]=input[i];break;
			case '2':_2-=1;output[p++]=input[i];break;
			case '1':_1-=1;output[p++]=input[i];break;
			default: if(input[i]>' ') return false;
		}
	}
	if(_B==0&&_F==0&&_M==0
	 &&_9==0&&_8==0&&_7==0
	 &&_6==0&&_5==0&&_4==0
	 &&_3==0&&_2==0&&_1==0){
		return true;
	}else{
		return false;
	}
}

char resolve_battle(char attacker,char defender){
	assert(attacker=='M' || (attacker>='1'&&attacker<='9'));
	assert(defender=='M' || defender=='F' || defender=='B' || (defender>='1'&&defender<='9'));

	if(defender=='B'){
		if(attacker=='3')
			 return attacker; 
		else return defender;
	}

	if(defender=='M'){
		if(attacker=='1')
			 return attacker;
		else return defender;

	}else if(attacker=='M'){
		return attacker;

	}else if(attacker==defender) return '0';
	else return attacker>defender?attacker:defender;
}

std::string inverse_move(std::string input){
	char sx,dx;
	int  sy,dy;
	sscanf(&input[0],"MOVE %c%d %c%d",&sx,&sy,&dx,&dy);
	std::string output = ""; 
	output += ('J'-(sx-'A'));
	if((11-sy)==10) output+="10";
	else output += (10-sy+'1');
	output += " ";
	output += ('J'-(dx-'A'));
	if((11-dy)==10) output+="10";
	else output += (10-dy+'1');
	return output;
}

namespace Map{

	static char map[100];
	static char buf[110];

	void setup_map(char* red_pieces, char* blue_pieces){
		for(int i=0; i<40; i+=1){ map[i] = 0x80|blue_pieces[39-i]; }
		memcpy(map+40,"..~~..~~....~~..~~..",20);
		memcpy(map+60,red_pieces,40);
	}

	void send_red_map(int fd){
		char *out = buf;
		for(int y=0; y<10; y+=1){
			for(int x=0; x<10; x+=1){
				int i = y*10+x;
				if(map[i]=='.'){
					*out++ = '.';
				}else if(map[i]=='~'){
					*out++ = '~';
				}else if((map[i]&0x80)==0x80){
					*out++ = '#';
				}else{
					*out++ = map[i]&0x7F;
				}
			}
			*out++ = '\n';
			printf("%d< %.*s",fd,11,out-11);
		}
		write(fd,buf,110);
	}

	void send_blue_map(int fd){
		char *out = buf;
		for(int y=9; y>=0; y-=1){
			for(int x=9; x>=0; x-=1){
				int i = y*10+x;
				if(map[i]=='.'){
					*out++ = '.';
				}else if(map[i]=='~'){
					*out++ = '~';
				}else if((map[i]&0x80)==0x80){
					*out++ = map[i]&0x7F;
				}else{
					*out++ = '#';
				}
			}
			*out++ = '\n';
			printf("%d< %.*s",fd,11,out-11);
		}
		write(fd,buf,110);
	}

	char get_red_piece(size_t index){
		assert(index>=0);
		assert(index<100);
		if( (0x80&map[index])==0x80 || map[index]=='.' || map[index]=='~')
			return 0;
		else return map[index];
	}

	char get_blue_piece(size_t index){
		assert(index>=0);
		assert(index<100);
		if( (0x80&map[index])==0 || map[index]=='.' || map[index]=='~')
			return 0;
		else return map[index]&0x7F;
	}

	bool is_in_lake(int index){
		assert(index>=0);
		assert(index<100);
		return Map::map[index] == '~';
	}

	bool is_empty(int index){
		assert(index>=0);
		assert(index<100);
		return map[index]=='.';
	}

	char owner_of(int x, int y){
		char type = map[y*10+x]&0x7F;
		unsigned char owner= map[y*10+x]&0x80;
		if(type=='.' || type=='~') return 0;
		else if(owner==0x80) return 'B';
		else return 'R';
	}

	bool can_move(int x, int y){
		assert(x>=0); assert(x<=9);
		assert(y>=0); assert(y<=9);
		unsigned char type  = map[y*10+x]&0x7F;
		char owner = ((map[y*10+x]&0x80)==0x80)?'B':'R';
		return (((type>='1'&&type<='9')||type=='M') &&
		 ( (y!=0 && owner_of(x,y-1)!=owner && type!='~')
		|| (y!=9 && owner_of(x,y+1)!=owner && type!='~')
		|| (x!=0 && owner_of(x-1,y)!=owner && type!='~')
		|| (x!=9 && owner_of(x+1,y)!=owner && type!='~') ));
	}
}

// vim: set sw=4 ts=4
