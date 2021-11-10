# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
ZMQOPTS=-lzmq -lczmq

all: Grade-Cast

Grade-Cast:
	g++ -Wall functions.c Grade-Cast.c $(ZMQOPTS) -o Grade-Cast.o

debug:
	g++ -Wall functions.c Grade-Cast.c -DDEBUG $(ZMQOPTS) -o Grade-Cast.o

clean:
	rm -f Grade-Cast.o
	rm -f result*.dmp
