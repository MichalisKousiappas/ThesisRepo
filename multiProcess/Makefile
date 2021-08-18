# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
# EXTRAS can be passed when running make to include extra flags.
# use EXTRAS="-DDEBUG" to add debugging printfs
ZMQOPTS=-lzmq -lczmq

all: multiProcesses

multiProcesses:
	g++ multiProcesses.c $(EXTRAS) $(ZMQOPTS) -o multiProcesses.o

clean:
	rm -f multiProcesses.o
	rm -f result*.txt
