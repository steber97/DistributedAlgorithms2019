CFLAG = -g -O2 -pthread -fopenmp -Wall

all: da_proc

da_proc: da_proc.cpp  link.o utilities.o timer_killer.o uniform_broadcast.o
	g++ $(CFLAG) -o da_proc da_proc.cpp utilities.o link.o timer_killer.o uniform_broadcast.o

uniform_broadcast.o: UniformBroadcast.cpp UniformBroadcast.h utilities.o
	g++ $(CFLAG) -o -c UniformBroadcast.cpp utilities.o

utilities.o: utilities.cpp utilities.h link.o
	g++ $(CFLAG) -c -o utilities.o utilities.cpp link.o

timer_killer.o: TimerKiller.h TimerKiller.cpp
	g++ $(CFLAG) -c -o timer_killer.o TimerKiller.cpp

link.o: Link.cpp Link.h
	g++ $(CFLAG) -c -o link.o Link.cpp

clean:
	rm da_proc


