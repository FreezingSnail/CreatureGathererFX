import csv

def intToBin(num: int, length: int) -> str:
    return bin(num)[2:].zfill(length)

def TypeExpand(type: str) -> str:
    if type == "spirit":
        return "0"
    elif type == "water":
        return "1"
    elif type == "wind":
        return "2"
    elif type == "earth":
        return "3"
    elif type == "fire":
        return "4"
    elif type == "lightning":
        return "5"
    elif type == "plant":
        return "6"
    elif type == "elder":
        return "7"
    elif type == "none":
        return "8"
    
    return type

    

class Move(object):
    def __init__(self):
        self.name = ""
        self.id = ""
        self.type	= ""
        self.power	= ""
        self.phys = ""


    def __str__(self):
        return "{ " + str(self.type) + ", " + str(self.power) + ", " + str(self.phys) +" },"
    


def toInt():        
    with open('data/movesheet', newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
        moves = []
        for row in reader:
            c = Move()
            c.name = row["name"]
            c.id = row["id"]
            c.power = (int(row['power']))
            c.type = TypeExpand(row['type'])
            c.phys = TypeExpand(row['physical'])
            moves.append(c)
    
    return moves

def names(moves: Move):
    for m in moves:
        print("const char move" + str(m.id) +"[] PROGMEM = \"" + m.name +"\";")

moves = toInt()
print("const MoveBitSet movePack PROGMEM = {")
for m in moves:
    print(m)
print("};")
print()
names(moves)

    #print('};')