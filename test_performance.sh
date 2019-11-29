#!/bin/bash
#
# Tests the performance of the Uniform Reliable Broadcast application.
#
# usage: ./test_performance evaluation_time
#
# evaluation_time: Specifies the number of seconds the application
#                  should run.
#

number_of_processes=10
evaluation_time=50
init_time=2
number_of_messages=10000

python3 generate_membership_file.py $number_of_processes

#start 5 processes
for i in `seq 1 $number_of_processes`
do
    ./da_proc $i membership $number_of_messages &
    da_proc_id[$i]=$!
done

#leave some time for process initialization
sleep $init_time

#start broadcasting
echo "Evaluating application for ${evaluation_time} seconds."
for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -USR2 "${da_proc_id[$i]}"
    fi
done

#let the processes do the work for some time
sleep $evaluation_time

#stop all processes
for i in `seq 1 $number_of_processes`
do
    if [ -n "${da_proc_id[$i]}" ]; then
	kill -TERM "${da_proc_id[$i]}"
    fi
done

#wait until all processes stop
for i in `seq 1 $number_of_processes`
do
	wait "${da_proc_id[$i]}"
done

#count delivered messages in the logs
for i in `seq 1 $number_of_processes`
do
	python3 count_delivered.py $i
done


echo "Performance test done."
