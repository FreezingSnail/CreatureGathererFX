from PIL import Image, ImageOps
import argparse
import re
import math


def split_image_into_tiles(image_path, tile_width, tile_height):
    # Open the image
    img = Image.open(image_path)

    # Calculate the number of tiles in each dimension
    num_tiles_x = img.width // tile_width
    num_tiles_y = img.height // tile_height

    # Split the image into tiles
    tiles = []
    for i in range(num_tiles_y):
        for j in range(num_tiles_x):
            left = j * tile_width
            upper = i * tile_height
            right = left + tile_width
            lower = upper + tile_height
            tile = img.crop((left, upper, right, lower))
            tiles.append(tile)

    return tiles


def string_to_indices(s, start_char='a'):
    return [ord(c) - ord(start_char) for c in s]


def create_image_from_tiles(tiles, indices, image_width, image_height, tile_width):
    # Create a new blank image
    img = Image.new('RGB', (image_width, image_height))

    # Paste each tile at the correct position
    for i, index in enumerate(indices):
        tile = tiles[index+97]
        img.paste(tile, (i * tile_width, 0))

    return img


def parse_file(file_path):
    # Open the file
    with open(file_path, 'r') as file:
        # Read the lines
        lines = file.readlines()

    # Parse the lines into key-value pairs
    kv_pairs = {}
    for line in lines:
        # Split the line into key and value
        key, value = line.split('=')
        key = key.strip()
        value = value.strip().strip('"')

        # Add the key-value pair to the dictionary
        kv_pairs[key] = value

    return kv_pairs


def parse_dimensions(filename):
    # Use a regular expression to find the dimensions in the filename
    match = re.search(r'(\d+)x(\d+)', filename)

    # If the dimensions were found, return them as integers
    if match:
        width, height = map(int, match.groups())
        return width, height

    # If the dimensions were not found, return None
    return None


def parse_file(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # Regex pattern to match namespaces and their content
    pattern = r'namespace (\w+) \{(.*?)\}'
    matches = re.findall(pattern, content, re.DOTALL)

    # Convert matches to a dictionary
    namespaces = {namespace: content.strip().split('\n')
                  for namespace, content in matches}

    return namespaces


def invert_image(img):
    # Invert the image
    inverted_img = ImageOps.invert(img)
    # Save the inverted image
    return inverted_img


def gen(namespaces, dims, tiles, args, mode="white", grey=False):
    buffer = []
    outdir = args.output_dir.split("/")[-1]

    # Process each namespace
    for namespace, kv_pairs in namespaces.items():
        keys = []
        if mode == "joined":
            buffer.append(f'namespace {namespace} {{')
        else:
            buffer.append(f'namespace {namespace}_{mode} {{')
        for kv_pair in kv_pairs:
            key, value = kv_pair.split('=')
            key = key.replace(" ", "")
            #value = value.replace(" ", "")
            value = value.replace('"', "")
            # Convert the value to indices
            indices = string_to_indices(value)

            image_width = len(indices) * dims[0]
            image_height = dims[1]
            if grey:
                image_height = math.ceil(image_height / 8) * 8

            # Create an image from the tiles using the indices
            output_img = create_image_from_tiles(
                tiles, indices, image_width, image_height, dims[0])

            image_name = ""
            if mode == "black" or mode == "white":
                image_name = f"{args.output_dir}/{key}_{mode}_{image_width}x{image_height}.png"
                if mode == "black":
                    output_img = invert_image(output_img)
                output_img.save(image_name)

            elif mode == "joined":
                image_name = f"{args.output_dir}/{key}_{image_width}x{image_height}.png"
                output_invert = invert_image(output_img)
                result = Image.new('RGB', (image_width * 2, image_height))
                result.paste(output_img, (0, 0))
                result.paste(output_invert, (image_width, 0))
                result.save(image_name)

            key, value = kv_pair.split(' = ')
            key = key.replace(" ", "")
            value = value.replace(" ", "")
            image_string = ""
            # Generate the string
            if mode == "black" or mode == "white":
                image_string = f'image_t {key} = "{outdir}/{key}_{mode}_{image_width}x{image_height}.png";'
            elif mode == "joined":
                image_string = f'image_t {key} = "{outdir}/{key}_{image_width}x{image_height}.png";'
            if not grey:
                buffer.append(image_string)
            keys.append(key)

        # Generate the creatureNames string
        creature_names_string = f'uint24_t  {namespace}[] = {{{", ".join(keys)}}};'
        buffer.append(creature_names_string)
        buffer.append('} namespace_end')

    txt_name = ""
    if mode == "joined":
        txt_name = f"{args.output_dir}/string_images.txt"
    else:
        txt_name = f"{args.output_dir}/string_images_{mode}.txt"
    with open(txt_name, "w") as file:
        file.write("\n".join(buffer))


def main():
    # Create the argument parser
    parser = argparse.ArgumentParser(
        description='Convert a string to an image using a tile map.')
    parser.add_argument(
        '--font', help='The path to the image file to use as the tile map.')
    parser.add_argument(
        '--output_dir', help='The directory where the output images and text will be saved.', required=True)
    parser.add_argument(
        '--input', help='The directory where the input textfile is located.', required=True)
    parser.add_argument('--greyscale', action='store_true',
                        help='greyscale mode')
    parser.add_argument('--mode', choices=['white', 'black', 'split', 'joined'],
                        help='set the mode of operation')

    args = parser.parse_args()
    dims = parse_dimensions(args.font.split("/")[-1])
    tiles = split_image_into_tiles(args.font, dims[0], dims[1])
    namespaces = parse_file(args.input)
    if args.mode == None:
        args.mode = "white"

    if args.mode == "split":
        gen(namespaces, dims, tiles, args, "black", args.greyscale)
        gen(namespaces, dims, tiles, args, "white", args.greyscale)
    else:
        gen(namespaces, dims, tiles, args, args.mode, args.greyscale)


if __name__ == "__main__":
    main()
