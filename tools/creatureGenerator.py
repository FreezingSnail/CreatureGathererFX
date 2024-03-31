#!/bin/env python3
import csv
import argparse
import matplotlib.pyplot as plt
from matplotlib.offsetbox import OffsetImage, AnnotationBbox
import matplotlib.colors as mcolors
import io
from matplotlib.backends.backend_agg import FigureCanvasAgg
from PIL import Image
from PIL import ImageDraw, ImageFont
from PIL import ImageOps
import colorsys




class Creature:
    def __init__(self, Name, id, type1, type2, evelevel, atk, deff, spcatk, spcdef, hp, spd, move1, move2, move3, move4, image=None):
        self.name = Name.capitalize().strip()
        self.id = int(id)
        self.type1 = type1.strip().capitalize()
        self.type2 = type2.strip().capitalize()
        self.evelevel = int(evelevel)
        self.atk = int(atk)
        self.deff = int(deff)
        self.spcatk = int(spcatk)
        self.spcdef = int(spcdef)
        self.hp = int(hp)
        self.spd = int(spd)
        self.move1 = int(move1)
        self.move2 = int(move2)
        self.move3 = int(move3)
        self.move4 = int(move4)
        self.image = self.image = process_image(image, self.type1)

def get_pastel_color(creature_type):
    # Map each type to a pastel color
    type_colors = {
        'Spirit': '#FFD1DC',  # Pastel pink
        'Water': '#89CFF0',  # Pastel blue
        'Wind': '#A2B5CD',   # Grey blue
        'Earth': '#f9ab75',  # Rust
        'Fire': '#FF6961',   # Pastel red
        'Lightning': '#FDFD96',  # Pastel yellow
        'Plant': '#c8e1cc',  # Pastel green
        'Elder': '#B19CD9'   # Pastel purple
    }

    # Return the pastel color for the given type
    return type_colors.get(creature_type, '#FFFFFF')

def hex_to_rgb(hex_color):
    # Convert a hex color string to an RGB tuple
    return tuple(int(hex_color[i:i+2], 16) for i in (1, 3, 5))

def darken_color(rgb_color):
    # Convert the RGB color to HLS
    r, g, b = [x/255.0 for x in rgb_color]  # Normalize to [0, 1]
    h, l, s = colorsys.rgb_to_hls(r, g, b)

    # Darken the color by reducing the lightness by 50%
    l *= 0.5

    # Convert the HLS color back to RGB
    r, g, b = colorsys.hls_to_rgb(h, l, s)

    # Convert back to the range [0, 255]
    return tuple(int(x * 255) for x in (r, g, b))


def color_darken(color):
    rgb_color = hex_to_rgb(color)

    # Darken the color
    dark_color = darken_color(rgb_color)
    dark_color_hex = '#%02x%02x%02x' % dark_color
    return dark_color_hex

def process_image(image, creature_type):
    # Resize the image
    image = image.resize((image.width * 6, image.height * 6), Image.NEAREST)
    image = image.convert("RGBA")
    data = image.getdata()

    # Replace transparent pixels 
    pastel_rgb = hex_to_rgb(get_pastel_color(creature_type))
    newData = []
    for item in data:
        if item[3] == 0:
            newData.append(pastel_rgb)
        else:
            newData.append(item)
    image.putdata(newData)

    return image

def parse_csv(file_path, image_path):
    creatures = []
    image = Image.open(image_path)
    width, height = image.size
    tile_width, tile_height = width, 32

    with open(file_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile, delimiter='\t')
        for i, row in enumerate(reader):
            tile = image.crop((0, i*tile_height, tile_width, (i+1)*tile_height))
            creature = Creature(**row, image=tile)
            creatures.append(creature)
    return creatures

def plot_creature_stats(creature):
    # Define a color for each stat type
    stat_colors = ['red', 'green', 'blue', 'yellow', 'purple', 'orange']
    ordered_stats = ['hp', 'atk', 'deff', 'spcatk', 'spcdef', 'spd']
    values = [getattr(creature, stat) for stat in ordered_stats]
    # Reverse the lists cause lazy
    stat_colors.reverse()
    ordered_stats.reverse()

    fig, ax = plt.subplots()
    # Set the facecolor of the figure and the axes
    color = get_pastel_color(creature.type1)
    fig.patch.set_facecolor(color)
    #ax.set_facecolor(color)

    # Plot the background bars with desaturated colors
    #ax.barh(ordered_stats, [16] * 6, color=[mcolors.to_rgba(c, alpha=0.2) for c in stat_colors],  linewidth=1.5, height=1)
    ax.barh(ordered_stats, [21] * 6, left=-5, color=[mcolors.to_rgba(c, alpha=0.2) for c in stat_colors],  linewidth=1.5, height=1)
    bars = ax.barh(ordered_stats, values, color=stat_colors)
    ax.xaxis.set_visible(False)
    ax.yaxis.set_visible(False)

     # Add the value name onto the graph bar
    bar_names = ['Speed', 'Special Def.', 'Special Atk', 'Defense', 'Attack', 'Health']
    for bar, stat, value in zip(bars, bar_names, values):
        ax.text(-4.7, bar.get_y() + bar.get_height() / 2, f'{stat}', va='center', ha='left', fontweight='bold')
        ax.text(-0.5, bar.get_y() + bar.get_height() / 2, f'{value}', va='center', ha='right', fontweight='bold')   
        
    # Tighten the inside of the graph so no background shows
    ax.set_xlim(-5, 16)
    ax.set_ylim(-0.5, len(ordered_stats) - 0.5)
    plt.tight_layout()
    
    # Save the graph as an in-memory image
    buf = io.BytesIO()
    canvas = FigureCanvasAgg(fig)
    canvas.print_png(buf)
    buf.seek(0)
    creature.graph = Image.open(buf)

    plt.close(fig)

def compose_image(creature, output_dir):
    # Convert the graph to an image
    graph_image = creature.graph.convert("RGBA")

    # Create a new image with the size of the sprite and the graph
    new_image = Image.new("RGBA", (creature.image.width + graph_image.width+100, max(creature.image.height, graph_image.height) + 60))

    # Create a draw object and specify the font
    draw = ImageDraw.Draw(new_image)
    font = ImageFont.truetype("/home/snail/.local/share/fonts/NerdFonts/ArimoNerdFont-Bold.ttf", 32)

    text = creature.name
    text_width = draw.textlength(text, font=font)
    position = ((new_image.width - text_width) / 2, 20)
    draw.text(position, text, font=font, fill='black', anchor='mm')

    # Paste the sprite onto the new image, lowered by 60 pixels
    new_image.paste(creature.image, (0, 60), creature.image)

    # Paste the graph into the new image to the right of the sprite, lowered by 60 pixels
    new_image.paste(graph_image, (creature.image.width, 60), graph_image)

    # Add type text under the sprite
    type1_text = creature.type1
    type1_position = (20, creature.image.height + 70)
    rec_position = (10, creature.image.height + 70)
    # Draw a rectangle behind the text with the color of the type
    draw.rectangle([rec_position, (max(type1_position[0] + draw.textlength(type1_text, font=font), 100)+10, type1_position[1] + 40)], fill=color_darken(get_pastel_color(creature.type1)))
    draw.text(type1_position, type1_text, font=font, fill='white')


    type2_text = creature.type2
    if creature.type2 != 'None':
        type2_position = (20, creature.image.height + 110)
        rec_position = (10, creature.image.height + 110)
        draw.rectangle([rec_position, (max(type2_position[0] + draw.textlength(type2_text, font=font), 100)+10, type2_position[1] + 40)], fill=color_darken(get_pastel_color(creature.type2)))
        draw.text(type2_position, type2_text, font=font, fill='white')


    # Paste the graph into the new image to the right of the sprite, lowered by 60 pixels
    new_image.paste(graph_image, (creature.image.width, 60))

    # Fill in any transparent background with the creature's type color
    type_color = get_pastel_color(creature.type1)
    new_image = Image.alpha_composite(Image.new("RGBA", new_image.size, color=type_color), new_image)

    # Convert the hex color to RGB
    rgb_color = hex_to_rgb(type_color)

    # Darken the color
    dark_color = darken_color(rgb_color)
    dark_color_hex = '#%02x%02x%02x' % dark_color

    # Add a 6-pixel black border around the composed image
    new_image = ImageOps.expand(new_image, border=12, fill=dark_color_hex)

    # Create a rounded rectangle mask
    mask = Image.new('L', new_image.size, 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle([0, 0, *new_image.size], fill=255, radius=20)

    # Apply the mask to the image
    new_image.putalpha(mask)

    # Save the new image to the output directory
    new_image.save(f'{output_dir}/{creature.name}_stats.png')

    # Save the new image to the creature object
    creature.composed_image = new_image

def create_allstats_image(creatures, output_dir):
    # Get the composed images from all creatures
    images = [creature.composed_image for creature in creatures]

    # Find the maximum width and height of all images
    max_width = max(image.width for image in images)
    max_height = max(image.height for image in images)

    # Define the number of rows and columns for the grid
    num_cols = 8
    num_rows = 4

    # Create a new image with the total width and height
    allstats_image = Image.new('RGBA', (max_width * num_cols, max_height * num_rows))

    # Paste each image into the new image
    for i, image in enumerate(images):
        row = i // num_cols
        col = i % num_cols
        allstats_image.paste(image, (col * max_width, row * max_height))

    # Save the new image to the output directory
    allstats_image.save(f'{output_dir}/allstats.png')

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='Parse a CSV file and create a list of Creature objects.')
    parser.add_argument('--csv_path', help='The path to the CSV file.')
    parser.add_argument('--output_dir', help='The directory to save the graphs.')
    parser.add_argument('--img_path', help='The path to creature sprites')


    args = parser.parse_args()

    # Parse CSV and print creature names
    creatures = parse_csv(args.csv_path, args.img_path)
    for creature in creatures:
        plot_creature_stats(creature)
        compose_image(creature, args.output_dir)

    # Create an image with all the stats
    create_allstats_image(creatures, args.output_dir)
    print(len(creatures), "creatures created")


if __name__ == "__main__":
    main()