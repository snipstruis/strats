all: main.cc stratego.h
	clang++ main.cc -o strats -std=c++14 -g -Wall -Wextra
