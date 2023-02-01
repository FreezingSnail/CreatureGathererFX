import csv
#cat this to a file lol
print("#pragma once")
print("static const monsterSeed_t monsterData[32] = { ")

with open('data/monstersheet', newline='') as csvfile:
    reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
    for row in reader:
        print(" { ", end='')
        print('0b', end ='')
        print(bin(int(row['type']))[2:].zfill(3), end ='')
        print(bin(int(row['id']))[2:].zfill(5), end =', ')
        print('0b', end ='')
        print(bin(int(row['atk']))[2:].zfill(4), end ='')
        print(bin(int(row['def']))[2:].zfill(4), end ='')
        print(bin(int(row['hp'] ))[2:].zfill(4), end ='')
        print(bin(int(row['spd']))[2:].zfill(4), end =', ')
        print('0b', end ='')
        print(row['1'], end ='')
        print(row['2'], end ='')
        print(row['3'], end ='')
        print(row['4'], end ='')
        print(row['5'], end ='')
        print(row['6'], end ='')
        print(row['7'], end ='')
        print(row['8'], end ='')
        print(" },")

    print('};')