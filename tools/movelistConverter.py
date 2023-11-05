import csv
from collections import defaultdict

columns = defaultdict(list)
with open('tools/data/movelists.csv', newline='') as csvfile:
    reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
    # read a row as {column1: value1, column2: value2,...}
    for row in reader:
        for (k, v) in row.items():  # go over each column name and value
            # append the value into the appropriate list
            columns[k].append(v)
            # based on column name k
output = []
for i in columns:
    c = columns[i]
    tmp = ""
    for d in c:
        tmp += d
    output.append(tmp)

print("namespace MoveLists {")
print("\tuint32_t moveList[] = {")
i = 0
for l in output:
    if (i < 31):
        print("\t\t", int(l, 2), " ,")
    else:
        print("\t\t", int(l, 2), " ")
    i += 1
print("\t};")
print("} namespace_end")
