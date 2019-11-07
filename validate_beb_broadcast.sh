#!/bin/bash

init_time=2
time_to_finish=10
number_of_processes=3


number_of_processes=5
number_of_messages=4

python3 generate_membership_file.py $number_of_processes $number_of_messages

# start 5 processes, each broadcasting 100 messages
for i in `seq 1 $number_of_processes`
do
    ./da_proc $i membership_py &
    da_proc_id[$i]=$!
done

sleep $init_time

for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -USR2 "${da_proc_id[$i]}"
    fi
done

sleep $time_to_finish

# stop all processes (don't know why, but we have to stop them twice)
for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -TERM "${da_proc_id[$i]}"
    fi
done

for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill "${da_proc_id[$i]}"
    fi
done

echo finish

