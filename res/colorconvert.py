#!/usr/bin/env python3
import os
import re
import sys
import argparse

dark_to_light = {
'#ffffff': '#000001', #white to black
'#333333': '#bcbcbe', #dark gray to light gray
'#0af1ff': '#c3c3c4', #Intense blue somegray
'#ac30a1': '#000002', #Intense purple to near black
'#dfa858': '#020203', #mild yellow a dark gray
'#ee7eff': '#747475', #mild pink medium gray
'#94dce9': '#aaaaab', #mild blue to light gray
'#f8f8f8': '#feeeff'  #almost white (sim logo) warm gray
}

# Parse command-line arguments
parser = argparse.ArgumentParser(description='Convert SVG colors.')
parser.add_argument('--todark', action='store_true', help='Convert colors to dark.')
parser.add_argument('--tolight', action='store_true', help='Convert colors to light.')
args = parser.parse_args()

# Determine the direction of the color conversion
if args.todark:
    colors = {v: k for k, v in dark_to_light.items()}  # Reverse the mapping
elif args.tolight:
    colors = dark_to_light
else:
    print('Please specify --todark or --tolight.')
    sys.exit(1)
# Compile a regex pattern for color codes
color_pattern = re.compile('|'.join(re.escape(color) for color in colors.keys()))

# Iterate over all SVG files in the directory
if args.todark:
    dirs = ['./panels/dark']
elif args.tolight:
    dirs = ['./panels/light']

for svg_dir in dirs:
    for filename in os.listdir(svg_dir):
        if filename.endswith('.svg'):
            filepath = os.path.join(svg_dir, filename)
            
            # Read the file
            with open(filepath, 'r') as file:
                content = file.read()
            
            # Replace the colors
            content = color_pattern.sub(lambda match: colors[match.group(0)], content)
            
            # Write the file
            with open(filepath, 'w') as file:
                file.write(content)