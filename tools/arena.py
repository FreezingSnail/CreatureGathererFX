import csv

with open('data/arena.csv', newline='') as csvfile:
    reader = csv.DictReader(csvfile, delimiter=' ')
    print("uint24_t teamList[] = {")
    for r in reader:
        print("\t{", end='')
        i = 0
        for k in reader.fieldnames:
            if i < len(reader.fieldnames) - 1:
                print(r[k], ', ', end='')
            else:
                print(r[k], end='')
            i += 1
        print("},")
    print("}")
