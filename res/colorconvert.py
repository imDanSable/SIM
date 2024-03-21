#!/usr/bin/env python3
import os
import re
import sys
import argparse
from xml.dom import minidom

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

# Always use the dark_to_light mapping
colors = dark_to_light

# Compile a regex pattern for color codes
color_pattern = re.compile('|'.join(re.escape(color) for color in colors.keys()))


def hideLayer(layerName, doc):
    for g in doc.getElementsByTagName('g'):
        if g.getAttribute('inkscape:label') == layerName:
            g.setAttribute('style', 'display:none')

def showLayer(layerName, doc):
    for g in doc.getElementsByTagName('g'):
        if g.getAttribute('inkscape:label') == layerName:
            g.setAttribute('style', 'display:inline')

to_dir = './panels/light'

# Iterate over all SVG files in the to_dir
for filename in os.listdir(to_dir):
    if filename.endswith('.svg'):
        filepath = os.path.join(to_dir, filename)

        # Parse the SVG file
        doc = minidom.parse(filepath)

        # Hide and show layers
        hideLayer("fluff", doc)
        showLayer("boring", doc)

        # Write the modified content back to the same file
        with open(filepath, 'w') as file:
            doc.writexml(file)

        # Read the file
        with open(filepath, 'r') as file:
            content = file.read()

        # Replace the colors
        content = color_pattern.sub(lambda match: colors[match.group(0)], content)

        # Write the file to the to_dir
        with open(filepath, 'w') as file:
            file.write(content)