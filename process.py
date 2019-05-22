f = open("output.txt", 'w')


def outToFile(text, f):
    f.write(text + "\n")

print("Simulated output fredo")
outToFile(input(), f)

print("this is the end")
outToFile(input(), f)

print("[22:19:55] [Server thread/INFO]: fredo joined the game")
outToFile(input(), f)

print("who")
outToFile(input(), f)

print("what")
outToFile(input(), f)

f.close()

