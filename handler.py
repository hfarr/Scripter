import sys


with open("handlerout.txt", 'w') as f:

   
    print("This should be written to the process output file")

    while(True):
        result = input()

        f.write(result + "\n");

