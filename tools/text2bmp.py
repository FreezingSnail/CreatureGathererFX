from PIL import Image, ImageOps
import argparse
import re


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


def create_image_from_tiles(tiles, indices, tile_width, tile_height):
    # Calculate the size of the output image
    image_width = len(indices) * tile_width
    image_height = tile_height

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


def invert_image(image_path):
    # Open the image file
    img = Image.open(image_path)
    # Invert the image
    inverted_img = ImageOps.invert(img)
    # Save the inverted image
    inverted_img.save(image_path)


def gen(namespaces, dims, tiles, args, color):
    buffer = []
    outdir = args.output_dir.split("/")[-1]

    # Process each namespace
    for namespace, kv_pairs in namespaces.items():
        print(kv_pairs)
        keys = []
        buffer.append(f'namespace {namespace}_{color} {{')
        for kv_pair in kv_pairs:
            key, value = kv_pair.split('=')
            key = key.replace(" ", "")
            value = value.replace(" ", "")
            value = value.replace('"', "")
            # Convert the value to indices
            indices = string_to_indices(value)

            # Create an image from the tiles using the indices
            output_img = create_image_from_tiles(
                tiles, indices, dims[0], dims[1])

            # Save the output image as key.png in the output directory
            output_img.save(f"{args.output_dir}/{key}_{color}.png")
            if color == "black":
                invert_image(f"{args.output_dir}/{key}_{color}.png")

        for kv_pair in kv_pairs:
            key, value = kv_pair.split(' = ')
            key = key.replace(" ", "")
            value = value.replace(" ", "")
            # Generate the string
            image_string = f'image_t {key} = "{outdir}/{key}_{color}.png";'
            buffer.append(image_string)
            keys.append(key)

        # Generate the creatureNames string
        creature_names_string = f'uint24_t  {namespace}[] = {{{", ".join(keys)}}};'
        buffer.append(creature_names_string)
        buffer.append('} namespace_end')

    with open(f"{args.output_dir}/string_images_{color}.txt", "w") as file:
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

    args = parser.parse_args()
    dims = parse_dimensions(args.font.split("/")[-1])
    print(args.font, dims)
    tiles = split_image_into_tiles(args.font, dims[0], dims[1])
    namespaces = parse_file(args.input)

    gen(namespaces, dims, tiles, args, "black")
    gen(namespaces, dims, tiles, args, "white")


if __name__ == "__main__":
    main()
