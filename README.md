# DistributedAlgorithms2019

To build the program use the command

```$ make```

in the same folder of the Makefile.



Then you can run a single process using:

```$ da_proc n membership m```

where  *n* is the number of the process you want to run and *m* is the number of messages that you want your program to broadcast to the other processes. These must be previously specified by number, ip address and port number on the *membership* file.



As an alternative you can use the *validate_fifo_broadcast.sh* script to automatically run a determined number of processes broadcasting a determined number of messages. You can modify these two parameters directly in the script. Eventually the script verifies the correctness of the written output by using the script *check_fifo.py*. 

Notice that to be able to run processes with the script provided by us is necessary to have a directory named *output* in the same folder of the script and the *da_proc*.

