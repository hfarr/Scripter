with open("output.txt", 'w') as f:

    value = ""
    while (value != 'stop'):

        value = input()

        f.write(value + '\n')
        print("Line written: {}".format(value))
        
