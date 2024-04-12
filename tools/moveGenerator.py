import argparse
import csv

def intToBin(num: int, length: int) -> str:
    return bin(num)[2:].zfill(length)

def TypeExpand(type: str) -> str:
    types = {
        "spirit": "0",
        "water": "1",
        "wind": "2",
        "earth": "3",
        "fire": "4",
        "lightning": "5",
        "plant": "6",
        "elder": "7",
        "none": "8"
    }

    return types.get(type, type)

def effect_to_int(effect: str) -> int:
    effects = {
        "NONE": 0,
        "EGOED": 1,
        "DRENCHED": 2,
        "BUFFETED": 3,
        "STUMBLED": 4,
        "BURNED": 5,
        "SHOCKED": 6,
        "ENTANGLED": 7,
        "CURSED": 8
    }

    return effects.get(effect.upper(), -1) 

    

class Move(object):
    def __init__(self):
        self.name = ""
        self.id = ""
        self.type	= ""
        self.power	= ""
        self.phys = ""
        self.accuracy = ""
        self.effect = ""


    def __str__(self):
        return ("{ " + str(self.type) + ", " + str(self.power) + ", " +
        str(self.phys) + ", " + str(self.accuracy) +", "+str(self.effect)+ " },")
    


def toInt(csv_path):        
    with open(csv_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
        moves = []
        for row in reader:
            c = Move()
            c.name = row["name"]
            c.id = row["id"]
            c.power = (int(row['power']))
            c.type = TypeExpand(row['type'])
            c.phys = TypeExpand(row['physical'])
            c.accuracy = (int(row['accuracy']))
            c.effect = effect_to_int(row['effect'])
            moves.append(c)
    
    return moves

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process a CSV file.')
    parser.add_argument('--csv_path', type=str, help='The path to the CSV file.')

    args = parser.parse_args()

    moves = toInt(args.csv_path)
    print("namespace MoveData {")
    print("uint8_t movePack[] = {")
    for m in moves:
        print(m)
    print("};")
    print("} namespace_end")