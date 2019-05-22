import sys

if (len(sys.argv) > 1):

    str_in = sys.argv[1]
    if "fredo" in str_in:
        print("fredo was mentioned")
    else:
        print("fredo was not mentioned in:", sys.argv[1], len(sys.argv))

#stringIn = input("")
#print("This is the response from the handler")
