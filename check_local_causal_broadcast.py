
import sys
from pprint import pprint


"""
Be careful here:
This program checks if the Local Causal Order broadcast is respected!
If it is not, stops and returns where it is not respected.

To do so, we store a 3D matrix, indexed by process, sequence_number, process_number which stores:
for every process i, for every message j sent by process i, the vector clock for every process k related to message j

It doesn't checks if messages are repeated! May be an issue, but probably by running check fifo before running it 
may solve the issue.

It could be added:
    1) check if no messages are repeated.
    2) check if process delivers message which have not been broadcasted!
    
USAGE
    python3 check_local_causal_broadcast.py membership_file_name number_of_messages 
"""

if __name__ == '__main__':
    membership_file = sys.argv[1]   # this is the membership file, so that the dependencies can be retrieved and the number of processes
    number_of_messages = int(sys.argv[2])   # Number of messages to send (not present in the membership file)

    with open(membership_file, 'r') as f:
        for i, l in enumerate(f):
            if i == 0:
                number_of_processes = int(l.split("\n")[0])
                dependencies = [[] for x in range(number_of_processes+1)]   # We count processes starting from 1
            if i <= number_of_processes:
                continue  # It is reading the ip addresses and ports
            else:
                # we are at the dependencies lines.
                processes = l.split('\n')[0].split(' ')  # The first element in every line is the process itself
                actual = int(processes[0])  # The entire line is the process' dependencies.
                                    # First I did processes = processes[1:], but maybe it is better to keep the process itself
                                    # among its dependencies, so that even the FIFO property is checked

                for p in processes:
                    dependencies[actual].append(int(p))

        print("Number of processes: {}".format(number_of_processes))
        print("Number of messages: {}".format(number_of_messages))
        # pprint(dependencies)


    # Now we can proceed in doing the validation of the local causal order broadcast
    # First we open the da_proc_i.out files, for i in range(1,number_of_processes+1)

    vc_dependencies = [[[] for j in range(number_of_messages+1)] for i in range(number_of_processes+1)]   # This is a 3 dimensional matrix, indexed by process, sequence number,
                            # and process number in the vector clock
                            # It means that for every message sent by every process, it is stored the correct vector clock for that message

    for i in range(1, number_of_processes+1):
        vector_clock = [0 for x in range(0, number_of_processes+1)]   # Initialized to zero.
        with open("da_proc_{}.out".format(i), 'r') as f:
            for l in f:
                if l != "":     # May happen with end of file with newline
                    l = l.split("\n")[0].split(' ')
                    if l[0] == 'b':
                        seq_n = int(l[1])
                        for k, el in enumerate(vector_clock):
                            # Store the current vector clock for every process, or zero if the process is not one of our dependencies
                            # If the sender is ourself, then we store our last previous message (FIFO order)
                            vc_dependencies[i][seq_n].append(0 if k not in dependencies[i] else el if k != i else seq_n - 1 )
                    else:
                        # It is a delivered message, we add it to our vc
                        sender = int(l[1])
                        seq_n = int(l[2])
                        vector_clock[sender] += 1

        # print("Process {}".format(i))

        # for m in vc_dependencies[i]:
        #     print(m)

    """
    Now we have the vector clock for every message broadcasted.
    Therefore it is easy to check if the delivered message respect their vector clock!
    """

    for i in range(1, number_of_processes+1):
        delivered_messages = [0 for x in range(0, number_of_processes+1)]   # Initialized to zero.
        with open("da_proc_{}.out".format(i), 'r') as f:
            for l in f:
                if l != '':  # May happen with end of file with newline
                    l = l.split("\n")[0].split(' ')
                    if l[0] == 'd':
                        # It is a delivered message, we check if the local causal order conditions are fulfilled
                        sender = int(l[1])
                        seq_n = int(l[2])

                        vc = vc_dependencies[sender][seq_n]

                        for dependency, delivered in zip(vc, delivered_messages):
                            if dependency > delivered:
                                print("Something went wrong here! ")
                                print("process {}, sender {}, seq_n {}".format(i, sender, seq_n))
                                exit(0)

                        delivered_messages[sender] += 1



    print("SUCCESS! :)")