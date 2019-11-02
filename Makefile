CFLAG = -g -O2 -pthread -fopenmp -Wall

all: da_proc

da_proc: da_proc.cpp  link.o utilities.o
	g++ $(CFLAG) -o da_proc da_proc.cpp manager.o utilities.o

utilities.o: utilities.cpp utilities.h link.o
	g++ $(CFLAG) -c -o utilities.o utilities.cpp

manager.o: Link.cpp Link.h
	g++ $(CFLAG) -c -o manager.o Manager.cpp

clean:
	rm da_proc

