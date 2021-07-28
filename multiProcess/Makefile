# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
ZMQOPTS=-lzmq -lczmq

all:
	g++ multiProcesses.c  $(ZMQOPTS) -o multiProcesses

clean:
	rm multiProcesses
	rm result*.txt
