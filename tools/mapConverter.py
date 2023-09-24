import json
import sys


class Map:
    name = ""
    height = 0
    width = 0
    layer1 = []

    def print(self):
        out = "\tuint8_t " + self.name + "Height = " + str(self.height) + " \n"
        out += "\tuint8_t " + self.name + "Width = " + str(self.width) + " \n"
        out += "\tuint8_t "+self.name + \
            " = {\n"
        for row in self.layer1:
            out += "\t\t{ "
            for v in row:
                out += str(v-1) + ", "
            out = out[:len(out) - 2]
            out += "},\n"
        out = out[:len(out) - 2]
        out += "\n\t};"
        print(out)


def loadFile(path):
    with open(path, 'r') as f:
        data = json.load(f)
        return data


def processMap(data):
    layers = data["layers"]
    m = Map()
    for d in layers:
        mp = d["data"]
        m.height = d["height"]
        m.width = d["width"]
        m.name = d["name"]
        grid = []
        for y in range(m.height):
            row = []
            for x in range(m.width):
                index = x+(y*m.width)
                row.append(mp[index])
            grid.append(row)
        m.layer1 = grid

    return m


if __name__ == "__main__":
    print("namespace MapData {")
    path = sys.argv[1]
    data = loadFile(path)
    mapdata = processMap(data)
    mapdata.print()
    print("} namespace_end")
