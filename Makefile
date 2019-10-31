CFLAG = -g -O2 -pthread -fopenmp

all: da_proc


da_proc: da_proc.c  manager.o utilities.o
	g++ $(CFLAG) -Wall -o da_proc da_proc.c manager.o utilities.o

utilities.o: utilities.cpp utilities.h manager.o
	g++ $(CFLAG) -c -o utilities.o utilities.cpp

manager.o: Manager.cpp Manager.h
	g++ $(CFLAG) -c -o manager.o Manager.cpp


clean:
	rm da_proc

