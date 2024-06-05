import csv
import argparse

#type	spirit	water	wind	earth	fire	lightning	plant	elder
#spirit	1	1	1	1	1	1	1	0

modifier_enum = {
    0:0, #none
    0.5:2, #half
    1:3, #same
    2:4, #double
}

class TypeTableData:
    def __init__(self, file_path):
        self.table = {}
        with open(file_path, 'r') as file:
            reader = csv.reader(file, delimiter='\t')
            header = next(reader)
            for row in reader:
                type_name = row[0]
                type_values = [int(modifier_enum[float(value)]) for value in row[1:]]
                self.table[type_name] = type_values


def serialize_table(table):
    serialized_table = {}
    for type_name, type_values in table.items():
        serialized_values = [bytes([value]) for value in type_values]
        # print(type_values)
        # print(serialized_values)
        serialized_table[type_name] = serialized_values
    return serialized_table

def write_bytes_to_file(file_path, serialized_table):
    with open(file_path, 'wb') as file:
        for type_name, serialized_values in serialized_table.items():
            for value in serialized_values:
                file.write(value)

def main():
    file_path = "data/typetable.csv"
    type_table = TypeTableData(file_path)
    parser = argparse.ArgumentParser(description='Creature Gatherer FX Data Converter')
    parser.add_argument('--format', choices=['fx', 'c'], default='fx', help='Output format (fx or c)')
    args = parser.parse_args()

    if args.format == 'fx':
        data = serialize_table(type_table.table)
        write_bytes_to_file("fxdata/generated/typetable.bin", data)
    elif args.format == 'c':
        print("TODO")
        #output_opponent_seeds_as_single_c_struct(seeds)

    
    
    
if __name__ == "__main__":
    main()