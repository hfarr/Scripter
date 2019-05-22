print("Simulated output")
ignoreResponse = input()

#print("This output should be run by exec, accessible by the parent")
print("[22:19:55] [Server thread/INFO]: fredo joined the game")
handlerResponse = input()

with open("output.txt", 'a') as f:
    f.write("INFO: " + handlerResponse + '\n')
    f.close()

