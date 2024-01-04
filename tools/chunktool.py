import pprint
import json
from dataclasses import dataclass
from typing import List
from dataclasses_json import dataclass_json


@dataclass_json
@dataclass
class Layer:
    data: List[int]
    height: int
    id: int
    name: str
    opacity: int
    type: str
    visible: bool
    width: int
    x: int
    y: int


@dataclass_json
@dataclass
class Tileset:
    firstgid: int
    source: str


@dataclass_json
@dataclass
class Map:
    compressionlevel: int
    height: int
    infinite: bool
    layers: List[Layer]
    nextlayerid: int
    nextobjectid: int
    orientation: str
    renderorder: str
    tiledversion: str
    tileheight: int
    tilesets: List[Tileset]
    tilewidth: int
    type: str
    version: str
    width: int


def import_json(file_path):
    with open(file_path, 'r') as f:
        data = json.load(f)
    return Map.from_dict(data)


def layer_to_uint8_array(data: List, array_name: str):

    hex_data = [f"{d}" for d in data]

    # Generate C array
    c_array = "{"
    c_array += ', '.join(hex_data)
    c_array += "},\n"

    return c_array


def split_into_chunks(flat_list, n, m):
    # Reshape the list into a 2D array
    array = [flat_list[i*m:(i+1)*m] for i in range(n)]

    # Split the array into 4x4 chunks
    chunks = []
    for i in range(0, n, 4):
        for j in range(0, m, 4):
            chunk = [row[j:j+4] for row in array[i:i+4]]
            chunks.append(chunk)

    return chunks


def print_as_2d(flat_list, n, m):
    # Reshape the list into a 2D array
    array = [flat_list[i*m:(i+1)*m] for i in range(n)]

    # Print the 2D array
    for row in array:
        print(row)


def flatten_2d(array_2d):
    return [item for sublist in array_2d for item in sublist]


data = import_json('../tiled/testmap.tmj')
# pprint.pprint(data.to_dict())
layer = data.layers[0]
# test = split_into_chunks(layer.data, layer.width, layer.height)
# print(test)
# for i in test:
#     print(layer_to_uint8_array(flatten_2d(i), "layer_data"))
print(layer_to_uint8_array(layer.data, "testmap"))
