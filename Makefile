CFLAG = -g -O2 -pthread -fopenmp -Wall --std=c++11

all: da_proc

da_proc: da_proc.cpp  link.o utilities.o ur_broadcast.o be_broadcast.o rco_broadcast.o
	g++ $(CFLAG) -o da_proc da_proc.cpp utilities.o link.o ur_broadcast.o be_broadcast.o rco_broadcast.o

rco_broadcast.o: RcoBroadcast.cpp RcoBroadcast.h ur_broadcast.o
	g++ $(CFLAG) -c -o rco_broadcast.o RcoBroadcast.cpp ur_broadcast.o

ur_broadcast.o: UrBroadcast.cpp UrBroadcast.h be_broadcast.o
	g++ $(CFLAG) -c -o ur_broadcast.o UrBroadcast.cpp be_broadcast.o

be_broadcast.o: BeBroadcast.cpp BeBroadcast.h link.o
	g++ $(CFLAG) -c -o be_broadcast.o BeBroadcast.cpp link.o

link.o: Link.cpp Link.h utilities.o
	g++ $(CFLAG) -c Link.cpp utilities.o -o link.o

utilities.o: utilities.cpp utilities.h
	g++ $(CFLAG) -c  utilities.cpp -o utilities.o

clean:
	rm da_proc


