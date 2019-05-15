all: simulator.o writeOutput.o
	gcc -o simulator simulator.o writeOutput.o -lpthread

simulator: simulator.c writeOutput.c writeOutput.h threadFunctions.h syncFunctions.h
	gcc -o simulator simulator.c writeOutput.c

run: simulator
	./simulator

clean:
	rm -f *.o