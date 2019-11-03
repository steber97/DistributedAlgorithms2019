CFLAG = -g -O2 -pthread -fopenmp -Wall

all: da_proc

da_proc: da_proc.cpp  link.o utilities.o thread_kill.o
	g++ $(CFLAG) -o da_proc da_proc.cpp utilities.o link.o thread_kill.o

utilities.o: utilities.cpp utilities.h link.o
	g++ $(CFLAG) -c -o utilities.o utilities.cpp link.o

thread_kill.o: ThreadKill.h ThreadKill.cpp
	g++ $(CFLAG) -c -o thread_kill.o ThreadKill.cpp

link.o: Link.cpp Link.h
	g++ $(CFLAG) -c -o link.o Link.cpp

clean:
	rm da_proc

