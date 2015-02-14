FLAGS= -std=c++11 -Wall -Werror 
EXE=1

all:
	g++ -o test -m32 test2.cc libinterrupt.a -ldl
1t.o:1t.cc
	g++ -c 1t.cc
clean:
	rm -f *.o test
