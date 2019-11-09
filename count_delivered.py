import sys

if __name__=='__main__':
    proc_number = sys.argv[1]

    f = open('da_proc_{}.out'.format(proc_number))

    broadcasted = 0
    delivered = 0
    for l in f:
        if l[0] == 'b':
            broadcasted += 1
        elif l[0] == 'd':
            delivered += 1

    print("process {} broadcasted {} and delivered {}".format(
        proc_number,
        broadcasted,
        delivered
    ))
