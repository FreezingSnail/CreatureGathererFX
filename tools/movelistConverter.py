import csv
import json
import pprint
from collections import defaultdict

columns = defaultdict(list)
with open('data/movelists.csv', newline='') as csvfile:
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


#print(output)


# import json to a dict
with open('data/json/moves.json') as json_file:
    moves = json.load(json_file)
    #print(moves)

with open('data/json/creatures.json') as json_file:
    creatures = json.load(json_file)
    #print(creatures)

for creature_index, list in enumerate(output):
    creatures[creature_index]['moveList'] = []
    for i, c in enumerate(list):
        if c == '1':
            creatures[creature_index]['moveList'].append(moves[i])

#save to json
with open('data/json/creatures.json', 'w') as outfile:
    json.dump(creatures, outfile)

# print("namespace MoveLists {")
# print("\tuint32_t moveList[] = {")
# i = 0
# for l in output:
#     if (i < 31):
#         print("\t\t", int(l, 2), " ,")
#     else:
#         print("\t\t", int(l, 2), " ")
#     i += 1
# print("\t};")
# print("} namespace_end")
