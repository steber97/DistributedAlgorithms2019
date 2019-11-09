import sys

"""
usage of the script:

python3 generate_membership_file.py number_of_processes number_of_messages

any other usage is going to result in an undefined behaviour.
"""

def pad_with_zero(i):
    if i<10:
        return "00" + str(i)
    if i < 100:
        return "0" + str(i)
    return str(i)

if __name__=='__main__':
    number_of_processes = int(sys.argv[1])

    f = open('membership', 'w')
    f.write(str(number_of_processes) + "\n")
    for i in range(1, number_of_processes+1):
        f.write(str(i) + " ")
        f.write('127.0.0.1')
        f.write(" ")
        f.write("12" + pad_with_zero(i) + "\n")
