import csv

class EncounterTable(object):
    def __init__(self):
        self.AreaId = ""
        self.c1 = ""
        self.c2	= ""
        self.c3	= ""
        self.c4 = ""
        self.r1 = ""
        self.r2	= ""
        self.r3	= ""
        self.r4 = ""


    def __str__(self):
        return "{  0x" + self.toCreatureList() + ", 0x" + self.toRates() +" },"
    
    def toCreatureList(self) -> str:
        cList = [0]*32
        cList[int(self.c1)] = 1 
        cList[int(self.c2)] = 1 
        cList[int(self.c3)] = 1 
        cList[int(self.c4)] = 1
        res = ""
        for i in cList:
            res += str(i)
        return res

    def toRates(self) -> str:
        rates = ""
        rates += bin(int(self.r1))[2:].zfill(4)
        rates += bin(int(self.r2))[2:].zfill(4)
        rates += bin(int(self.r3))[2:].zfill(4)
        rates += bin(int(self.r4))[2:].zfill(4)

        return rates


def toInt():        
    with open('data/EncounterTables.csv', newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter='\t', quotechar='|')
        encounters = []
        for row in reader:
            c = EncounterTable()
            c.AreaId = row["AreaId"]
            c.c1 = row["c1"]
            c.c2 = row["c2"]
            c.c3 = row["c3"]
            c.c4 = row["c4"]
            c.r1 = row["r1"]
            c.r2 = row["r2"]
            c.r3 = row["r3"]
            c.r4 = row["r4"]

            encounters.append(c)
    
    return encounters

def names(rates):
    print("const encounterRates[]" + " PROGMEM = {")
    for e in rates:
        print(e)

    print("};")

encounters = toInt()
names(encounters)
print()


