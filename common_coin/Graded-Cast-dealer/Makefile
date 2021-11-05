# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
ZMQOPTS=-lzmq -lczmq

all: Grade-Cast dealer

Grade-Cast:
	g++ Grade-Cast.c $(EXTRAS) $(ZMQOPTS) -o Grade-Cast.o

dealer:
	g++ dealerProc.c $(EXTRAS) $(ZMQOPTS) -o dealerProc.o

debug:
	g++ Grade-Cast.c -DDEBUG $(ZMQOPTS) -o Grade-Cast.o
	g++ dealerProc.c -DDEBUG $(ZMQOPTS) -o dealerProc.o


clean:
	rm -f Grade-Cast.o
	rm -f dealerProc.o
	rm -f result*.dmp
