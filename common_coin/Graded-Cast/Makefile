# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
# EXTRAS can be passed when running make to include extra flags.
# use EXTRAS="-DDEBUG" to add debugging printfs
ZMQOPTS=-lzmq -lczmq

all: Graded-Cast dealer

Graded-Cast:
	g++ Graded-Cast.c $(EXTRAS) $(ZMQOPTS) -o Graded-Cast.o

dealer:
	g++ dealerProc.c $(EXTRAS) $(ZMQOPTS) -o dealerProc.o

clean:
	rm -f Graded-Cast.o
	rm -f dealerProc.o
	rm -f result*.txt
