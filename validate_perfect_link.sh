#!/bin/bash

init_time=2
time_to_finish=20

# start 5 processes, each broadcasting 100 messages
for i in `seq 1 5`
do
    ./da_proc $i membership &
    da_proc_id[$i]=$!
done

sleep $init_time

for i in `seq 1 5`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -USR2 "${da_proc_id[$i]}"
    fi
done

sleep $time_to_finish

# stop all processes (don't know why, but we have to stop them twice)
for i in `seq 1 5`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -TERM "${da_proc_id[$i]}"
    fi
done

for i in `seq 1 5`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill "${da_proc_id[$i]}"
    fi
done

echo finish

