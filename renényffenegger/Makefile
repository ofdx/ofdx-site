GPP=g++ -std=c++17

all: rene.o
run: all

clean:
	rm -f rene.o

rene.o: base64.*
	${GPP} -o rene.o -c base64.cpp 
