CFLAG = -g -O2 -pthread -fopenmp -Wall

all: da_proc

da_proc: da_proc.cpp  link.o utilities.o
	g++ $(CFLAG) -o da_proc da_proc.cpp utilities.o link.o

utilities.o: utilities.cpp utilities.h link.o
	g++ $(CFLAG) -c -o utilities.o utilities.cpp link.o

link.o: Link.cpp Link.h
	g++ $(CFLAG) -c -o link.o Link.cpp

clean:
	rm da_proc

