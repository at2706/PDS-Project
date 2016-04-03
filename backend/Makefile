all: main

main: main.cpp SharedFile.cpp main.h SharedFile.h
	g++ -o main -std=c++11 -Wall -O1 -g main.cpp SharedFile.cpp -pthread

clean:
	rm -f main
