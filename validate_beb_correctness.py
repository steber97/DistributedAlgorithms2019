import sys

if __name__ == "__main__":
    wrong_processes = [ int(x) for x in sys.argv[1:] ]
    print("wrong processes " + str(wrong_processes))
    f = open('membership_py', 'r')

    file = f.read()
    number_of_processes = int(file.split('\n')[0])
    number_of_messages = int(file.split('\n')[-2])
    f.close()

    wrong = False
    print("processes", number_of_processes)
    print("messages", number_of_messages)
    for i in range(1,number_of_processes+1):
        if i not in wrong_processes:
            # Analyze every da_proc file
            f = open("output/da_proc_" + str(i) + ".out")
            log = f.read().split('\n')[:-1]
            broadcast = [False for x in range(number_of_messages)]
            delivered = [ [False for x in range(number_of_messages)] for y in range(number_of_processes) ]
            for el in log:
                if el[0] == 'b':
                    # broadcast message
                    seq_number = int(el.split(' ')[1])

                    if not broadcast[seq_number-1]:
                        # expected behaviour, no duplicate delivery or broadcast of the same message.
                        broadcast[seq_number-1] = True
                    else:
                        print("Something went wrong, message " + str(seq_number) + " was broadcasted twice!")
                        wrong = True
                else:
                    # delivery
                    proc_number = int(el.split(' ')[1])
                    seq_number = int(el.split(' ')[2])

                    if not delivered[proc_number-1][seq_number-1]:
                        # expected behaviour, no duplicate delivery
                        delivered[proc_number-1][seq_number-1] = True
                    else:
                        print("Something went wrong, message " + str(seq_number) + " was delivered twice!")
                        wrong = True

            f.close()

            for k, el in enumerate(broadcast):
                if not el and k+1 not in wrong_processes:
                    print("Something went wrong, message " + str(k+1) + " not delivered")
                    wrong = True

            for l, row in enumerate(delivered):
                for m, el in enumerate(row):
                    if not el and l+1 not in wrong_processes:
                        print("Something went wrong, message from process " + str(l+1) + " seqn= " + str(m+1) + " not delivered by " + str(i) + " :/")
                        wrong = True

    if not wrong:
        print("Brilliant! :)")
