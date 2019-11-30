#!/bin/bash

if [ "$1" = "" ] || [ "$2" = "" ]; then
    echo "Validates the submission (i.e., valid input for a simple test-case) for the given excercise"
    echo -e "usage: \n$ $0 <exercice_name (\"FIFO\" or \"LCausal\")> <language (\"C\" or \"JAVA\")>"
    exit 1

fi

# time to wait for correct processes to broadcast all messages (in seconds)
# (should be adapted to the number of messages to send)
time_to_finish=2

init_time=2

number_of_messages=5
number_of_processes=2

# compile (should output: da_proc or Da_proc.class)
make

# prepare input
if [ "$1" = "FIFO" ]; then
echo "writing FIFO input..."

echo "2
1 127.0.0.1 12001
2 127.0.0.1 12002" > membership

else
echo "writing LCausal input..."

echo "2
1 127.0.0.1 12001
2 127.0.0.1 12002
1 2
2 1" > membership
fi

# start 5 processes, each broadcasting 100 messages
for i in `seq 1 $number_of_processes`
do
    if [ "$2" = "C" ]; then
      ./da_proc $i membership $number_of_messages &
    else
      java Da_proc $i membership $number_of_messages &
    fi
    da_proc_id[$i]=$!
done

# leave some time for process initialization
sleep $init_time

# start broadcasting
for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -USR2 "${da_proc_id[$i]}"
    fi
done

# leave some time for the correct processes to broadcast all messages
sleep $time_to_finish

# stop all processes
for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -TERM "${da_proc_id[$i]}"
    fi
done

# wait until all processes stop
for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	    wait "${da_proc_id[$i]}"
    fi
done

#count delivered messages in the logs
for i in `seq 1 $number_of_processes`
do
	python3 count_delivered.py $i
done

echo "Correctness test done."

sudo tc qdisc del dev lo root
echo "Stop messing up with network interface!!"
