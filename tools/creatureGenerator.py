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

 
def typeToBin(type: str) ->str:
    if type == "spirit":
        return bin(0)[2:].zfill(3)
    elif type == "water":
        return bin(1)[2:].zfill(3)
    elif type == "wind":
        return bin(2)[2:].zfill(3)
    elif type == "earth":
        return bin(3)[2:].zfill(3)
    elif type == "fire":
        return bin(4)[2:].zfill(3)
    elif type == "lightning":
        return bin(5)[2:].zfill(3)
    elif type == "plant":
        return bin(6)[2:].zfill(3)
    elif type == "elder":
        return bin(7)[2:].zfill(3)
    
    return ""
    

class Creature(object):
    def __init__(self):
        self.id = ""
        self.type1	= ""
        self.type2	= ""
        self.evelevel	= ""
        self.atk	= ""
        self.df	= ""
        self.spcatk	= ""
        self.spcdef	= ""
        self.hp	= ""
        self.spd	= ""
        self.move1	= ""
        self.move2	= ""
        self.move3	= ""
        self.move4 = ""


    def __str__(self):
        return "{ " + str(self.id) + ", " + str(self.type1) + ", " + str(self.type2) + ", " + str(self.evelevel) + ", " + str(self.atk) + ", " + str(self.df) + ", " + str(self.spcatk) + ", " + str(self.spcdef) + ", " + str(self.hp) + ", " + str(self.spd) + ", " + str(self.move1) + ", " + str(self.move2) + ", " + str(self.move3) + ", " + str(self.move4) + " },"

    def p(self):
        return "" + self.id + self.type1 + self.type2 + self.evelevel + self.atk + self.df + self.spcatk + self.spcdef + self.hp + self.spd + self.move1 + self.move2 + self.move3 + self.move4 


def toBin():
    with open('data/Creaturesheet', newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
        creatures = []
        for row in reader:
            c = Creature()
            c.id = intToBin(int(row['id']), 5)
            c.type1 = typeToBin(row['type1'])
            if row['type2'] != "none":
                c.type2 = typeToBin(row['type2'])
            else:
                c.type2 = typeToBin(row['type1'])
            c.evelevel = intToBin(int(row['evelevel']), 5)
            c.atk = intToBin(int(row['atk']), 4)
            c.df =  intToBin(int(row["def"]), 4)
            c.spcatk = intToBin(int(row["spcatk"]), 4)
            c.spcdef = intToBin(int(row["spcdef"]), 4)
            c.hp = intToBin(int(row["hp"]), 4)
            c.spd = intToBin(int(row["spd"]), 4)
            c.move1 = intToBin(int(row["move1"]), 5)
            c.move2 = intToBin(int(row["move2"]), 5)
            c.move3 = intToBin(int(row["move3"]), 5)
            c.move4 = intToBin(int(row["move4"]), 5)
            print(c)
            creatures.append(c)


        data = ""
        for c in creatures:
            data += c.p() 
            print("0b" + creatures[0].p() + ",")

    return creatures

def toInt():        
    with open('data/Creaturesheet', newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
        creatures = []
        for row in reader:
            c = Creature()
            c.id = (int(row['id']))
            c.type1 = TypeExpand(row['type1'])
            c.type2 = TypeExpand(row['type2'])
            c.evelevel = (int(row['evelevel']))
            c.atk = (int(row['atk']))
            c.df =  (int(row["def"]))
            c.spcatk = (int(row["spcatk"]))
            c.spcdef = (int(row["spcdef"]))
            c.hp = (int(row["hp"]))
            c.spd = (int(row["spd"]))
            c.move1 = (int(row["move1"]))
            c.move2 = (int(row["move2"]))
            c.move3 = (int(row["move3"]))
            c.move4 = (int(row["move4"]))
            print(c)
            creatures.append(c)
    
    return creatures

creatures = toInt()
print(len(creatures))

    #print('};')