CFLAG = -g -O2 -pthread -fopenmp -Wall --std=c++11

all: da_proc

da_proc: da_proc.cpp  link.o utilities.o ur_broadcast.o be_broadcast.o fifo_broadcast.o lcob_broadcast.o
	g++ $(CFLAG) -o da_proc da_proc.cpp utilities.o link.o ur_broadcast.o be_broadcast.o fifo_broadcast.o lcob_broadcast.o


lcob_broadcast.o: LocalCausalBroadcast.cpp LocalCausalBroadcast.h
	g++ $(CFLAG) -c -o lcob_broadcast.o LocalCausalBroadcast.cpp

fifo_broadcast.o: FifoBroadcast.cpp FifoBroadcast.h
	g++ $(CFLAG) -c -o fifo_broadcast.o FifoBroadcast.cpp

ur_broadcast.o: UrBroadcast.cpp UrBroadcast.h
	g++ $(CFLAG) -c -o ur_broadcast.o UrBroadcast.cpp

be_broadcast.o: BeBroadcast.cpp BeBroadcast.h
	g++ $(CFLAG) -c -o be_broadcast.o BeBroadcast.cpp

link.o: Link.cpp Link.h
	g++ $(CFLAG) -c -o link.o Link.cpp

utilities.o: utilities.cpp utilities.h
	g++ $(CFLAG) -c -o utilities.o utilities.cpp

clean:
	rm da_proc lcob_broadcast.o fifo_broadcast.o ur_broadcast.o be_broadcast.o link.o utilities.o


