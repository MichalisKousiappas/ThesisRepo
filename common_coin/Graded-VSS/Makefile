# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
LIBS=-lzmq -lczmq -lgsl -lm -lgslcblas
FLAGS=-std=c99
SRC=globals.c gradedshare.c polyfunc.c init.c gradeddecide.c gradecast.c gradedrecover.c vote.c main.c

all: Graded-VSS

Graded-VSS:
	@gcc -Wall $(SRC) $(LIBS) $(FLAGS) -o Graded-VSS.o

debug:
	@gcc -Wall $(SRC) -DDEBUG $(LIBS) $(FLAGS) -o Graded-VSS.o

clean:
	@rm -f Graded-VSS.o
	@rm -f result*.dmp
