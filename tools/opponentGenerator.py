import csv
#cat this to a file lol
print("#pragma once")

with open('data/opponents.csv', newline='') as csvfile:
    reader = csv.DictReader(csvfile, delimiter=',', quotechar='|')
    for row in reader:
        print("static cosnt oppSeed_t op",row['opID'], ' = { ', sep='',end ='')
        print('0b', end ='')
        print(bin(int(row['id1']))[2:].zfill(5), end ='')
        print(bin(int(row['lvl1']))[2:].zfill(5), end ='')
        print(bin(int(row['move11']))[2:].zfill(5), end ='')
        print(bin(int(row['move12']))[2:].zfill(5), end ='')
        print(bin(int(row['move13'] ))[2:].zfill(5), end ='')
        print(bin(int(row['move14']))[2:].zfill(5), end ='00, ')
        print('0b', end ='')
        print(bin(int(row['id2']))[2:].zfill(5), end ='')
        print(bin(int(row['lvl2']))[2:].zfill(5), end ='')
        print(bin(int(row['move21']))[2:].zfill(5), end ='')
        print(bin(int(row['move22']))[2:].zfill(5), end ='')
        print(bin(int(row['move23'] ))[2:].zfill(5), end ='')
        print(bin(int(row['move24']))[2:].zfill(5), end ='00, ')
        print('0b', end ='')
        print(bin(int(row['id3']))[2:].zfill(5), end ='')
        print(bin(int(row['lvl3']))[2:].zfill(5), end ='')
        print(bin(int(row['move31']))[2:].zfill(5), end ='')
        print(bin(int(row['move32']))[2:].zfill(5), end ='')
        print(bin(int(row['move33'] ))[2:].zfill(5), end ='')
        print(bin(int(row['move34']))[2:].zfill(5), end ='')
        
        print("00 };")