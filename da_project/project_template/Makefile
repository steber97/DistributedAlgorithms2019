CFLAG = -g -O2 -pthread -fopenmp

all: da_proc


da_proc: da_proc.c  manager.o utilities.o sender.o
	g++ -Wall -o da_proc da_proc.c manager.o utilities.o sender.o

utilities.o: utilities.cpp utilities.h manager.o sender.o
	g++ $(CFLAG) -c -o utilities.o utilities.cpp

manager.o: Manager.cpp Manager.h sender.o
	g++ $(CFLAG) -c -o manager.o Manager.cpp sender.o


sender.o: Sender.cpp Sender.h
	g++ $(CFLAG) -c -o sender.o Sender.cpp




clean:
	rm da_proc

