#!/usr/bin/env python3
import argparse
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from moveGenerator import TypeExpand


FIXTURE_PATH = "tst/fxdatatest/generated/creature_data.hpp"


def parse_creatures(csv_path):
    with open(csv_path, newline="") as csv_file:
        return list(csv.DictReader(csv_file, delimiter="\t"))


def write_c_fixture(creatures, output_path):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w") as output:
        output.write("#pragma once\n")
        output.write("#include \"src/lib/DataTypes.hpp\"\n\n")
        output.write("// PROGMEM fixture; copy rows with memcpy_P (or read fields with pgm_read_byte).\n")
        output.write("const CreatureData_t creatureFixtures[] PROGMEM = {\n")
        for creature in creatures:
            fields = (
                int(creature["id"]),
                TypeExpand(creature["type1"].strip()),
                TypeExpand(creature["type2"].strip()),
                int(creature["evelevel"]),
                int(creature["atk"]),
                int(creature["deff"]),
                int(creature["spcatk"]),
                int(creature["spcdef"]),
                int(creature["hp"]),
                int(creature["spd"]),
                int(creature["move1"]),
                int(creature["move2"]),
                int(creature["move3"]),
                int(creature["move4"]),
            )
            output.write("    { " + ", ".join(map(str, fields)) + " },\n")
        output.write("};\n")
        output.write(f"constexpr uint8_t creatureFixtureCount = {len(creatures)};\n")


def main():
    parser = argparse.ArgumentParser(description="Emit creature PROGMEM control fixtures.")
    parser.add_argument("--csv-path", default="data/Creaturesheet.csv")
    parser.add_argument("--output", default=FIXTURE_PATH)
    args = parser.parse_args()
    write_c_fixture(parse_creatures(args.csv_path), args.output)


if __name__ == "__main__":
    main()
