# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
ZMQOPTS=-lzmq -lczmq -lgsl -lm
SRC=globals.c polyfunc.c init.c functions.c Graded-VSS.c

all: Graded-VSS

Graded-VSS:
	c99 -Wall $(SRC) $(ZMQOPTS) -o Graded-VSS.o

debug:
	c99 -Wall $(SRC) -DDEBUG $(ZMQOPTS) -o Graded-VSS.o

clean:
	rm -f Graded-VSS.o
	rm -f result*.dmp
