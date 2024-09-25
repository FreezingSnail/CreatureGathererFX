import argparse
import csv

def intToBin(num: int, length: int) -> str:
    return bin(num)[2:].zfill(length)

def TypeExpand(type: str) -> int:
    types = {
        "spirit": 0,
        "water": 1,
        "wind": 2,
        "earth": 3,
        "fire": 4,
        "lightning": 5,
        "plant": 6,
        "elder": 7,
        "status": 8,
        "none": 15
    }

    return types.get(type, 15)

def AccuracyExpand(accuracy: int) -> int:
    if accuracy == 100:
        return 0
    elif accuracy == 90:
        return 1
    elif accuracy == 80:
        return 2
    elif accuracy == 70:
        return 3
    
    return 0

def effect_to_int(effect: str) -> int:
    effects = {    
        "DPRSD": 0,
        "SOAKED": 1,
        "BUFTD": 2,
        "SOILED": 3,
        "SCRCHD": 4,
        "ZAPPED": 5,
        "TANGLD": 6,
        "REDCD": 7,
        "ENLTND": 8,
        "DRNCHD": 9,
        "AIRSWPT": 10,
        "GRNDED": 11,
        "KINDLD": 12,
        "CHRGD": 13,
        "ENRCHD": 14,
        "EVOLVD": 15,
        "ATKDWN": 16,
        "DEFDWN": 17,
        "SPCADWN": 18,
        "SPCDDWN": 19,
        "SPDDWN": 20,
        "ATKUP": 21,
        "DEFUP": 22,
        "SPCAUP": 23,
        "SPCDUP": 24,
        "SPDUP": 25,
        "SAPPD": 26,
        "INFSED": 27,
        "PINNED": 28,
        "CONCUSED": 29,

        "NONE": 255
    }

    e = effects.get(effect.upper(), -1) 
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
    


# TODO: change this to be the binary version of the move (16,8,8 bits) 
# and write as a raw binary file

def serialize_to_n_bytes(number, n):
    return format(number & ((1 << n) - 1), f'0{n}b')

def serialize_moves(moves):
    serialized_table = {}
    for move in moves:
        name = move.name
        serialize_moved = serialize_move(move)
        serialized_table[name] = serialize_moved
    return serialized_table

def serialize_move(move):
    serialized = ''
    serialized += serialize_to_n_bytes(move.type, 4)
    serialized += serialize_to_n_bytes(move.power, 5)
    serialized += serialize_to_n_bytes(move.phys, 1)
    serialized += serialize_to_n_bytes(move.accuracy, 2)
    serialized += serialize_to_n_bytes(0, 4)
    serialized += serialize_to_n_bytes(move.effect, 8)
    serialized += serialize_to_n_bytes(255, 8)
    byte_array = bytes(int(serialized[i:i+8], 2) for i in range(0, len(serialized), 8))
    return byte_array

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
            c.phys = int(row['physical'])
            c.accuracy = AccuracyExpand(int(row['accuracy']))
            c.effect = effect_to_int(row['effect'])
            moves.append(c)
    
    return moves


def write_bytes_to_file(file_path, serialized_table):
    with open(file_path, 'wb') as file:
        for _, serialized_values in serialized_table.items():
            file.write(serialized_values)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process a CSV file.')
    parser.add_argument('--csv_path', type=str, help='The path to the CSV file.')

    args = parser.parse_args()

    moves = toInt(args.csv_path)
    data = serialize_moves(moves)

    write_bytes_to_file('fxdata/generated/moves.bin', data)