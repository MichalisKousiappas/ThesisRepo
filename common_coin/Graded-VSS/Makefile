# use this makefile since build requires the file to be in the same directory
# too much of a hasle to figure out how to do it
LIBS=-lzmq -lczmq -lgsl -lm -lgsl
FLAGS=-std=c99
SRC=globals.c gradedshare.c polyfunc.c init.c gradeddecide.c gradecast.c gradedrecover.c main.c
SRC2=globals.c gradedshare.c polyfunc.c init.c gradeddecide.c gradecast.c gradedrecover.c  main.c

all: Graded-VSS

temporary:
	@gcc -Wall $(SRC2) -DDEBUG $(LIBS) $(FLAGS) -o temporary.o

Graded-VSS:
	@gcc -Wall $(SRC) $(LIBS) $(FLAGS) -o Graded-VSS.o

debug:
	@gcc -Wall $(SRC) -DDEBUG $(LIBS) $(FLAGS) -o Graded-VSS.o

clean:
	@rm -f Graded-VSS.o
	@rm -f result*.dmp
	@rm -f temporary.o
	@rm -f temp_result*.dmp
