# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
# EXTRAS can be passed when running make to include extra flags.
# use EXTRAS="-DDEBUG" to add debugging printfs
ZMQOPTS=-lzmq -lczmq

all:
	g++ multipub-sub.c $(EXTRAS) $(ZMQOPTS) -lpthread -o multiPubSub.o

clean:
	rm -f multiPubSub.o
	rm -f result*.txt
