#!/usr/bin/env python3
import argparse
import csv
import os
import re


FIXTURE_PATH = "tst/fxdatatest/generated/arena_data.hpp"
STOCK_CREATURE_PATTERN = re.compile(r"^\s*uint8_t\s+(\w+)\[\]\s*=\s*\{\s*([0-9,\s]+)\s*\};")


def write_team_list():
    with open("data/arena.csv", newline="") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=" ")
        print("uint24_t teamList[] = {")
        for row in reader:
            print("\t{", end="")
            for index, key in enumerate(reader.fieldnames):
                print(row[key], end=", " if index < len(reader.fieldnames) - 1 else "")
            print("},")
        print("}")


def parse_stock_creatures(teams_path):
    fixtures = []
    with open(teams_path) as teams_file:
        for line in teams_file:
            match = STOCK_CREATURE_PATTERN.match(line)
            if match:
                values = [int(value.strip()) for value in match.group(2).split(",")]
                if len(values) != 5:
                    raise ValueError(f"{match.group(1)} must contain five arena bytes")
                fixtures.append(values)
    return fixtures


def write_c_fixture(fixtures, output_path):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w") as output:
        output.write("#pragma once\n")
        output.write("#include \"src/lib/DataTypes.hpp\"\n\n")
        output.write("// PROGMEM fixture; read fields with pgm_read_byte(&arenaFixtures[row][column]).\n")
        output.write("// Rows mirror Teams::StockCreatures in fxdata/data/teams.txt, including null.\n")
        output.write("const uint8_t arenaFixtures[][5] PROGMEM = {\n")
        for fixture in fixtures:
            output.write("    { " + ", ".join(map(str, fixture)) + " },\n")
        output.write("};\n")
        output.write(f"constexpr uint8_t arenaFixtureCount = {len(fixtures)};\n")


def main():
    parser = argparse.ArgumentParser(description="Generate arena data or PROGMEM fixtures.")
    parser.add_argument("--format", choices=["fx", "c"], default="fx")
    parser.add_argument("--teams-path", default="fxdata/data/teams.txt")
    parser.add_argument("--output", default=FIXTURE_PATH)
    args = parser.parse_args()

    if args.format == "c":
        write_c_fixture(parse_stock_creatures(args.teams_path), args.output)
    else:
        write_team_list()


if __name__ == "__main__":
    main()
