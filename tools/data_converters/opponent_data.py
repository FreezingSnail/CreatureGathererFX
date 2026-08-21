import csv
import struct
import base64
import os

from helper import *
import argparse



class CreatureSeed:
    def __init__(self):
        self.id = 0
        self.lvl = 0
        self.moves = 0

class OpponentSeed:
    def __init__(self):
        self.firstCreature = CreatureSeed()
        self.secondCreature = CreatureSeed()
        self.thirdCreature = CreatureSeed()

def print_opponent_seed(opponent_seed):
    def print_moves(moves):
        print("Move 1:", moves & 0xFF)
        print("Move 2:", (moves >> 8) & 0xFF)
        print("Move 3:", (moves >> 16) & 0xFF)
        print("Move 4:", (moves >> 24) & 0xFF)

    print("First Creature:")
    print("ID:", opponent_seed.firstCreature.id)
    print("Level:", opponent_seed.firstCreature.lvl)
    print("Moves:")
    print_moves(opponent_seed.firstCreature.moves)
    print("Serialized:", serialize_creature_seed(opponent_seed.firstCreature))

    print("Second Creature:")
    print("ID:", opponent_seed.secondCreature.id)
    print("Level:", opponent_seed.secondCreature.lvl)
    print("Moves:")
    print_moves(opponent_seed.secondCreature.moves)
    print("Serialized:", serialize_creature_seed(opponent_seed.secondCreature))

    print("Third Creature:")
    print("ID:", opponent_seed.thirdCreature.id)
    print("Level:", opponent_seed.thirdCreature.lvl)
    print("Moves:")
    print_moves(opponent_seed.thirdCreature.moves)
    print("Serialized:", serialize_creature_seed(opponent_seed.thirdCreature))

def serialize_creature_seed(creature_seed):
    # Format: < for little-endian, B for uint8_t, I for uint32_t
    binary_data = struct.pack('<BBI', creature_seed.id, creature_seed.lvl, creature_seed.moves)
    # Convert binary data to a binary string
    binary_string = ''.join(format(byte, '08b') for byte in binary_data)
    return binary_data

def serialize_opponent_seed(opponent_seed):
    serialized_first_creature = serialize_creature_seed(opponent_seed.firstCreature)
    serialized_second_creature = serialize_creature_seed(opponent_seed.secondCreature)
    serialized_third_creature = serialize_creature_seed(opponent_seed.thirdCreature)
    return [serialized_first_creature, serialized_second_creature, serialized_third_creature]

class Opponent:
    def __init__(self):
        self.id1 = 0
        self.id2 = 0
        self.id3 = 0
        self.lvl1 = 0
        self.lvl2 = 0
        self.lvl3 = 0
        self.name = ""
        self.move11 = 0
        self.move12 = 0
        self.move13 = 0
        self.move14 = 0
        self.move21 = 0
        self.move22 = 0
        self.move23 = 0
        self.move24 = 0
        self.move31 = 0
        self.move32 = 0
        self.move33 = 0
        self.move34 = 0


def parse_opponent_data(file_path):
    opponents = []
    with open(file_path, 'r') as file:
        csv_reader = csv.reader(file)
        next(csv_reader)  # Skip the header row
        for row in csv_reader:
            opponent = Opponent()
            opponent.id1 = creature_string_to_number(row[0])
            opponent.id2 = creature_string_to_number(row[1])
            opponent.id3 = creature_string_to_number(row[2])
            opponent.lvl1 = int(row[3])
            opponent.lvl2 = int(row[4])
            opponent.lvl3 = int(row[5])
            opponent.name = row[6]
            opponent.move11 = move_string_to_number(row[7])
            opponent.move12 = move_string_to_number(row[8])
            opponent.move13 = move_string_to_number(row[9])
            opponent.move14 = move_string_to_number(row[10])
            opponent.move21 = move_string_to_number(row[11])
            opponent.move22 = move_string_to_number(row[12])
            opponent.move23 = move_string_to_number(row[13])
            opponent.move24 = move_string_to_number(row[14])
            opponent.move31 = move_string_to_number(row[15])
            opponent.move32 = move_string_to_number(row[16])
            opponent.move33 = move_string_to_number(row[17])
            opponent.move34 = move_string_to_number(row[18])
            opponents.append(opponent)
    return opponents

def opponent_to_opponent_seed(opponent):
    opponent_seed = OpponentSeed()

    opponent_seed.firstCreature.id = opponent.id1
    opponent_seed.firstCreature.lvl = opponent.lvl1
    opponent_seed.firstCreature.moves = opponent.move11 | (opponent.move12 << 8) | (opponent.move13 << 16) | (opponent.move14 << 24)

    opponent_seed.secondCreature.id = opponent.id2
    opponent_seed.secondCreature.lvl = opponent.lvl2
    opponent_seed.secondCreature.moves = opponent.move21 | (opponent.move22 << 8) | (opponent.move23 << 16) | (opponent.move24 << 24)

    opponent_seed.thirdCreature.id = opponent.id3
    opponent_seed.thirdCreature.lvl = opponent.lvl3
    opponent_seed.thirdCreature.moves = opponent.move31 | (opponent.move32 << 8) | (opponent.move33 << 16) | (opponent.move34 << 24)

    return opponent_seed

def opponets_to_opponent_seeds(opponents):
    opponent_seeds = []
    for opponent in opponents:
        opponent_seed = opponent_to_opponent_seed(opponent)
        opponent_seeds.append(opponent_seed)
    return opponent_seeds

def seeds_to_serialized_opponent_seeds(opponent_seeds):
    serialized_opponent_seeds = []
    for opponent_seed in opponent_seeds:
        serialized_opponent_seed = serialize_opponent_seed(opponent_seed)
        serialized_opponent_seeds.append(serialized_opponent_seed)
    return serialized_opponent_seeds

def output_opponent_seeds_as_single_c_struct(opponent_seeds):
    output_file_path = "tst/fxdatatest/generated/opponent_data.hpp"
    os.makedirs(os.path.dirname(output_file_path), exist_ok=True)
    with open(output_file_path, "w") as output:
        output.write("#pragma once\n")
        output.write("#include \"src/lib/DataTypes.hpp\"\n\n")
        output.write("// PROGMEM fixture; copy each OpponentSeed with memcpy_P before comparing.\n")
        output.write("const OpponentSeed opponentSeeds[] PROGMEM = {\n")
        for opponent_seed in opponent_seeds:
            output.write("    {\n")
            for creature in (
                opponent_seed.firstCreature,
                opponent_seed.secondCreature,
                opponent_seed.thirdCreature,
            ):
                output.write(
                    f"        {{ {creature.id}, {creature.lvl}, 0x{creature.moves:08X}UL }},\n"
                )
            output.write("    },\n")
        output.write("};\n")
        output.write(f"constexpr uint8_t opponentSeedCount = {len(opponent_seeds)};\n")

def write_fx_data(seeds):
    serialized_seeds = seeds_to_serialized_opponent_seeds(seeds)
    binary_output_path = "fxdata/generated/opponents.bin"
    with open(binary_output_path, 'wb') as file:
        for serialized_seed in serialized_seeds:
            for seed in serialized_seed:
                file.write(seed)

    source_output_path = "fxdata/generated/opponents.txt"
    with open(source_output_path, 'w') as output:
        for index, serialized_seed in enumerate(serialized_seeds):
            label = "opponent_seeds" if index == 0 else f"opponent_seed_{index}"
            row = b"".join(serialized_seed)
            output.write(f"uint8_t {label}[] = {{\n")
            for offset in range(0, len(row), 6):
                values = ", ".join(f"0x{byte:02X}" for byte in row[offset:offset + 6])
                output.write(f"    {values},\n")
            output.write("};\n\n")

        output.write("uint24_t opts[] = {\n")
        output.write("    opponent_seeds,\n")
        for index in range(1, len(serialized_seeds)):
            output.write(f"    opponent_seed_{index},\n")
        output.write("};\n")

    print("Serialized seeds written to:", binary_output_path)
    print("FX opponent rows written to:", source_output_path)

def write_fx_data_c_structs(seeds):
    output_opponent_seeds_as_c_structs(seeds)


def main():
    parser = argparse.ArgumentParser(description='Creature Gatherer FX Data Converter')
    parser.add_argument('--format', choices=['fx', 'c'], default='fx', help='Output format (fx or c)')
    args = parser.parse_args()

    file_path = "data/opponents.csv"
    opponents = parse_opponent_data(file_path)
    seeds = opponets_to_opponent_seeds(opponents)

    if args.format == 'fx':
        write_fx_data(seeds)
    elif args.format == 'c':
        output_opponent_seeds_as_single_c_struct(seeds)

    

if __name__ == "__main__":
    main()