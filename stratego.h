#pragma once

bool validate_pieces(char* const buf, int size){
	if(size<40) return false;
	signed char _B=6, _F=1, _M=1,
			    _9=1, _8=2, _7=3,
				_6=4, _5=4, _4=4,
				_3=5, _2=8, _1=1;
	int p=0;
	for(int i=0; i<size && p<40; i+=1){
		switch(buf[i]){
		case 'B':_B-=1;buf[p++]=buf[i];break;
		case 'F':_F-=1;buf[p++]=buf[i];break;
		case 'M':_M-=1;buf[p++]=buf[i];break;
		case '9':_9-=1;buf[p++]=buf[i];break;
		case '8':_8-=1;buf[p++]=buf[i];break;
		case '7':_7-=1;buf[p++]=buf[i];break;
		case '6':_6-=1;buf[p++]=buf[i];break;
		case '5':_5-=1;buf[p++]=buf[i];break;
		case '4':_4-=1;buf[p++]=buf[i];break;
		case '3':_3-=1;buf[p++]=buf[i];break;
		case '2':_2-=1;buf[p++]=buf[i];break;
		case '1':_1-=1;buf[p++]=buf[i];break;
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

static char map[100];

void setup_map(char* red_pieces, char* blue_pieces){
	for(int i=0; i<40; i+=1){ map[i] = 0x80|blue_pieces[39-i]; }
	memcpy(map+40,"..  ..  ....  ..  ..",20);
	memcpy(map+60,red_pieces,40);
}

void print_red_map(char* out){
	for(int y=0; y<10; y+=1){
		for(int x=0; x<10; x+=1){
			int i = y*10+x;
			if(map[i]=='.'){
				*out++ = '.';
			}else if(map[i]==' '){
				*out++ = ' ';
			}else if((map[i]&0x80)==0x80){
				*out++ = '?';
			}else{
				*out++ = map[i]&0x7F;
			}
		}
		*out++ = '\n';
	}
}

void print_blue_map(char* out){
	for(int y=9; y>=0; y-=1){
		for(int x=9; x>=0; x-=1){
			int i = y*10+x;
			if(map[i]=='.'){
				*out++ = '.';
			}else if(map[i]==' '){
				*out++ = ' ';
			}else if((map[i]&0x80)==0x80){
				*out++ = map[i]&0x7F;
			}else{
				*out++ = '?';
			}
		}
		*out++ = '\n';
	}
}
