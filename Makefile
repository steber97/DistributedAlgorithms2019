CFLAG = -g -O2 -pthread -fopenmp -Wall

all: da_proc

da_proc: da_proc.cpp  link.o utilities.o timer_killer.o broadcast.o
	g++ $(CFLAG) -o da_proc da_proc.cpp utilities.o link.o timer_killer.o broadcast.o

broadcast.o: Broadcast.cpp Broadcast.h
	g++ $(CFLAG) -c -o broadcast.o Broadcast.cpp

link.o: Link.cpp Link.h timer_killer.o utilities.o
	g++ $(CFLAG) -c Link.cpp timer_killer.o utilities.o -o link.o

utilities.o: utilities.cpp utilities.h
	g++ $(CFLAG) -c  utilities.cpp -o utilities.o

timer_killer.o: TimerKiller.h TimerKiller.cpp
	g++ $(CFLAG) -c TimerKiller.cpp -o timer_killer.o

clean:
	rm da_proc


