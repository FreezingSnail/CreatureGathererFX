#!/usr/bin/env python3
import argparse
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from moveGenerator import TypeExpand


FX_DATA_PATH = "fxdata/data/creatureData.txt"
FIXTURE_PATH = "tst/fxdatatest/generated/creature_data.hpp"
FIELDS = (
    "id", "type1", "type2", "evelevel", "atk", "deff", "spcatk", "spcdef",
    "hp", "spd", "move1", "move2", "move3", "move4",
)
VALID_TYPES = {
    "spirit", "water", "wind", "earth", "fire", "lightning", "plant", "elder",
    "status", "none",
}


def parse_creatures(csv_path):
    creatures = []
    with open(csv_path, newline="") as csv_file:
        reader = csv.DictReader(csv_file, delimiter="\t")
        if reader.fieldnames is None or tuple(reader.fieldnames[1:]) != FIELDS:
            raise ValueError(f"{csv_path}: expected columns Name followed by {', '.join(FIELDS)}")

        for row_number, row in enumerate(reader, start=2):
            try:
                type1 = row["type1"].strip().lower()
                type2 = row["type2"].strip().lower()
                if type1 not in VALID_TYPES or type2 not in VALID_TYPES:
                    raise ValueError(f"unknown type {type1!r}, {type2!r}")
                fields = (
                    int(row["id"]), TypeExpand(type1), TypeExpand(type2),
                    *(int(row[field]) for field in FIELDS[3:]),
                )
            except (KeyError, ValueError) as error:
                raise ValueError(f"{csv_path}:{row_number}: {error}") from error

            if any(value < 0 or value > 255 for value in fields):
                raise ValueError(f"{csv_path}:{row_number}: values must fit uint8_t")
            if fields[0] != len(creatures):
                raise ValueError(
                    f"{csv_path}:{row_number}: id {fields[0]} must equal row index {len(creatures)}"
                )
            creatures.append(fields)

    if not creatures:
        raise ValueError(f"{csv_path}: no creature rows")
    return creatures


def write_fx_data(creatures, output_path):
    with open(output_path, "w") as output:
        output.write("// Generated from data/Creaturesheet.csv by tools/data_converters/creature_data.py.\n")
        output.write("// Do not edit: regenerate with python3 tools/data_converters/creature_data.py.\n")
        output.write("namespace CreatureData {\n\n")
        output.write("uint8_t creatureData[] = {\n")
        for creature in creatures:
            output.write("    {" + ", ".join(map(str, creature)) + "},\n")
        output.write("};\n}\nnamespace_end\n")


def write_c_fixture(creatures, output_path):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w") as output:
        output.write("#pragma once\n")
        output.write("#include \"src/lib/DataTypes.hpp\"\n\n")
        output.write("// Generated from the canonical FX creature serialization; read with memcpy_P.\n")
        output.write("const CreatureData_t creatureFixtures[] PROGMEM = {\n")
        for creature in creatures:
            output.write("    { " + ", ".join(map(str, creature)) + " },\n")
        output.write("};\n")
        output.write(f"constexpr uint8_t creatureFixtureCount = {len(creatures)};\n")


def main():
    parser = argparse.ArgumentParser(
        description="Generate FX creature data and fixtures from the canonical creature CSV."
    )
    parser.add_argument("--csv-path", default="data/Creaturesheet.csv")
    parser.add_argument("--fx-output", default=FX_DATA_PATH)
    parser.add_argument("--output", default=FIXTURE_PATH)
    args = parser.parse_args()

    creatures = parse_creatures(args.csv_path)
    write_fx_data(creatures, args.fx_output)
    write_c_fixture(creatures, args.output)


if __name__ == "__main__":
    main()
