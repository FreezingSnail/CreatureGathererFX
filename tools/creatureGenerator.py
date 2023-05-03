import csv

def intToBin(num: int, length: int) -> str:
    return bin(num)[2:].zfill(length)

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
    
#cat this to a file lol
print("#pragma once")
print("#include \"creature/Creature.hpp\"")
print("static const CreatureSeed_t CreatureData[32] = { ")


	# uint32_t movelist;	//32 binary flips
	# unsigned char evoLevel:   	5;
	# unsigned char creatureID: 	3;
	# unsigned char type1: 		3;
	# unsigned char type2: 		3;
	# unsigned char EvoPtr:		5;
	# unsigned char hpSeed:		4;
	# unsigned char atkSeed:		4;
	# unsigned char defSeed:		4;	
	# unsigned char spcAtkSeed:	4;	
	# unsigned char spcDefSeed:	4;	
	# unsigned char spdSeed:		4;
	# unsigned char move1:		5;
	# unsigned char move2:		5;
	# unsigned char move3:		5;
	# unsigned char move4:		5;

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
        return "{ " + self.id + ", " + self.type1 + ", " + self.type2 + ", " + self.evelevel + ", " + self.atk + ", " + self.df + ", " + self.spcatk + ", " + self.spcdef + ", " + self.hp + ", " + self.spd + ", " + self.move1 + ", " + self.move2 + ", " + self.move3 + ", " + self.move4 + ":0000 }"

    def p(self):
        return "" + self.id + self.type1 + self.type2 + self.evelevel + self.atk + self.df + self.spcatk + self.spcdef + self.hp + self.spd + self.move1 + self.move2 + self.move3 + self.move4 + "0000"


with open('data/Creaturesheet', newline='') as csvfile:
    reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
    creatures = []
    for row in reader:
        c = Creature()
        #print(row)
        #c = row.split(" ")
        #print(c)
        # print(" { ", end='')
        # print('0b', end ='')
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
        # print(bin(int(row['id']))[2:].zfill(5), end =', ')
        # print('0b', end ='')
        # print(bin(int(row['atk']))[2:].zfill(4), end ='')
        # print(bin(int(row['def']))[2:].zfill(4), end ='')
        # print(bin(int(row['hp'] ))[2:].zfill(4), end ='')
        # print(bin(int(row['spd']))[2:].zfill(4), end =', ')
        # print('0b', end ='')
        # print(row['1'], end ='')
        # print(row['2'], end ='')
        # print(row['3'], end ='')
        # print(row['4'], end ='')
        # print(row['5'], end ='')
        # print(row['6'], end ='')
        # print(row['7'], end ='')
        # print(row['8'], end ='')
        # print(" },")

    data = ""
    for c in creatures:
        data += c.p() 
        print("0b" + creatures[0].p() + ",")
    
    # i = 0
    # out = ""
    # for c in data:
    #     if i == 0:
    #         out += "0b"
    #     out += c
    #     i += 1
    #     if i == 8:
    #         out += ", "
    #         i = 0 

    # print(out)

    print(len(creatures))
    print(len(creatures[0].p()))
    print(len(creatures[0].p())*len(creatures))

    #print('};')