
import sys

def compute_correct_processes(wrong_processes):
    correct_processes = []
    for p in range(1,number_of_processes+1):
        if p not in wrong_processes:
            correct_processes.append(p)

    return correct_processes


def check_validity(wrong_processes, broadcast, delivered, number_of_processes, number_of_messages, wrong):
    """
    If pi and pj are correct, then evey message broadcasted by pi is eventually
    delivered by pj.
    :param wrong_processes:
    :return:
    """
    correct_processes = compute_correct_processes(wrong_processes)

    print("correct processes", str(correct_processes))

    for p in correct_processes:
        for i, m in enumerate(broadcast[p-1]):
            if m:
                # the message was broadcasted, and process p is correct.
                # Therefore every correct process (including p) must have delivered the message
                for p2 in correct_processes:
                    if not delivered[p2-1][p-1][i]:
                        print("Something went wrong, validity broken")
                        wrong = True

    return wrong


def check_no_creation(wrong_messages, broadcast, delivered, number_of_processes, number_of_messages, wrong):
    """
    No message is delivered if it was not sent.
    :param wrong_messages:
    :param broadcast:
    :param delivered:
    :param number_of_processes:
    :param number_of_messages:
    :return:
    """
    for i, deliv_pi in enumerate(delivered):
        for j, deliv_pj in enumerate(deliv_pi):
            for k, m in enumerate(deliv_pj):
                if m:
                    # the message has been delivered
                    if not broadcast[j][k]:
                        print("Something went wrong, no creation broken")
                        wrong = True
    return wrong


def check_urb4(wrong_messages, broadcast, delivered, number_of_processes, number_of_messages, wrong):
    """
    for any message m, if a process delivers m, then every correct process delivers message m
    :param wrong_messages:
    :param broadcast:
    :param delivered:
    :param number_of_processes:
    :param number_of_messages:
    :return:
    """
    correct_processes = compute_correct_processes(wrong_processes)
    for i, deliv_pi in enumerate(delivered):
        for j, deliv_pj in enumerate(deliv_pi):
            for k, m in enumerate(deliv_pj):
                if m:
                    for p in correct_processes:
                        # Every other correct process must have delivered it!
                        if not delivered[p-1][j][k]:
                            print("Something went wrong, urb4 broken!")
                            wrong = True

    return wrong


if __name__=="__main__":
    wrong_processes = [ int(x) for x in sys.argv[2:] ]
    print("wrong processes " + str(wrong_processes))
    f = open('membership_py', 'r')

    file = f.read()
    number_of_processes = int(file.split('\n')[0])
    number_of_messages = int(sys.argv[1])
    f.close()

    broadcast = []
    delivered = []

    wrong = False

    for i in range(1,number_of_processes+1):

        # Analyze every da_proc file
        f = open("output/da_proc_" + str(i) + ".out")
        log = f.read().split('\n')[:-1]
        broadcast_pi = [False for x in range(number_of_messages)]
        delivered_pi = [ [False for x in range(number_of_messages)] for y in range(number_of_processes) ]
        for el in log:
            if el[0] == 'b':
                # broadcast message
                seq_number = int(el.split(' ')[1])

                if not broadcast_pi[seq_number-1]:
                    # expected behaviour, no duplicate delivery or broadcast of the same message.
                    broadcast_pi[seq_number-1] = True
                else:
                    print("Something went wrong, message " + str(seq_number) + " was broadcasted twice!")
                    wrong = True
            else:
                # delivery
                proc_number = int(el.split(' ')[1])
                seq_number = int(el.split(' ')[2])

                if not delivered_pi[proc_number-1][seq_number-1]:
                    # expected behaviour, no duplicate delivery
                    delivered_pi[proc_number-1][seq_number-1] = True
                else:
                    print("Something went wrong, message " + str(seq_number) + " was delivered twice!")
                    wrong = True

        f.close()
        broadcast.append(broadcast_pi)
        delivered.append(delivered_pi)


    wrong = check_validity(wrong_processes, broadcast, delivered, number_of_processes, number_of_messages, wrong)
    wrong = check_no_creation(wrong_processes, broadcast, delivered, number_of_processes, number_of_messages, wrong)
    wrong = check_urb4(wrong_processes, broadcast, delivered, number_of_processes, number_of_messages, wrong)

    if not wrong:
        print("Brilliant :)")
