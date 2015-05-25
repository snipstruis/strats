all: main.cc libstrat.h
	clang++ main.cc -o strats -std=c++14 -g -Wall -Wextra
