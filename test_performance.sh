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
evaluation_time=20
init_time=2
number_of_messages=10000
number_of_dependencies=5

#sudo tc qdisc add dev lo root netem 2>/dev/null
#sudo tc qdisc change dev lo root netem delay 50ms 200ms loss 10% 25% reorder 25% 50%

python3 generate_membership_file.py $number_of_processes $number_of_dependencies

# make clean    # When doing last tests, it is better to clean before making (but for performance reasons now we don't clean)
make

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

# Check fifo
echo "check fifo"
for i in `seq 1 $number_of_processes`
do
  filename="da_proc_${i}.out"
	python3 check_fifo.py $filename
done


echo "Performance test done."

echo "Check local causal order broadcast"
python3 check_local_causal_broadcast.py membership $number_of_messages

# sudo tc qdisc del dev lo root
echo "Stop messing up with network interface!!"
