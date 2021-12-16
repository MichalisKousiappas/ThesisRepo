# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
ZMQOPTS=-lzmq -lczmq

all: Graded-VSS

Graded-VSS:
	g++ -Wall globals.c init.c functions.c Graded-VSS.c $(ZMQOPTS) -o Graded-VSS.o

debug:
	g++ -Wall globals.c init.c functions.c Graded-VSS.c -DDEBUG $(ZMQOPTS) -o Graded-VSS.o

clean:
	rm -f Graded-VSS.o
	rm -f result*.dmp
