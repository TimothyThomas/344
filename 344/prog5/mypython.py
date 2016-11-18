'''
Tim Thomas
OSU CS 344, Fall 2016
Program 5 -- Python Exploration
'''

import string
import random


def get_10_rand_lowers():
    """Returns a string containing 10 random lowercase characters followed by a newline."""
    ten_letters = [string.ascii_lowercase[random.randint(0,25)] for i in range(10)] + ['\n']
    return ''.join(ten_letters)


for file_name in ['file1', 'file2', 'file3']:
    with open(file_name, 'w') as f:
        s = get_10_rand_lowers()
        f.write(s)
        print("Contents of {}: {}".format(file_name, s), end='')

rand1 = random.randint(1,42)
rand2 = random.randint(1,42)

print("Random Integer 1: {}".format(rand1))
print("Random Integer 2: {}".format(rand2))
print("Product: {}".format(rand1*rand2))
