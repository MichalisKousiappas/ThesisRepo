# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
ZMQOPTS=-lzmq -lczmq

all: multiProcesses

multiProcesses:
	g++ multiProcesses.c $(EXTRAS) $(ZMQOPTS) -o multiProcesses.o

debug:
	g++ multiProcesses.c -DDEBUG $(ZMQOPTS) -o multiProcesses.o

clean:
	rm -f multiProcesses.o
	rm -f result*.dmp
