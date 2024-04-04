#!/usr/bin/env python3
import os
import shutil
import re
from xml.dom import minidom

funky_to_light = {
'#ffffff': '#000001', #white to black
'#333333': '#bcbcbe', #dark gray to light gray
'#0af1ff': '#c3c3c4', #Intense blue (Title) to light gray
'#ac30a1': '#000002', #Intense purple to near black
'#dfa858': '#020203', #mild yellow a dark gray
'#ee7eff': '#000000', #mild pink medium gray
'#9a9a9b': '#777777', #mild gray to black (line around port background)
'#94dce9': '#aaaaab', #mild blue to light gray
'#f8f8f8': '#feeeff',  #almost white (sim logo) warm gray
'b2b2b2ff': '#111111', #dark gray to near black (normalled lines)
}

funky_to_dark = {
#'#ffffff': '#000001', #white to black
'#333333': '#222222', #dark gray to light gray
'#0af1ff': '#888888', #Intense blue (Title) gray
'#ac30a1': '#FFFFFE', #Intense purple to near white
'#dfa858': '#fefefe', #mild yellow a dark gray (same as light)
'#ee7eff': '#cccccc', #mild pink medium gray
'#94dce9': '#bbbbbb', #mild blue to light gray/
'#f8f8f8': '#feeeff',  #almost white (sim logo) warm gray
'#b2b2b2': '#b0b0b0', #dark gray to lighter gray (normalled lines)
}

themes = [(funky_to_light, '../res/panels/light'), (funky_to_dark, '../res/panels/dark')]



def hideLayer(layerName, doc):
    for g in doc.getElementsByTagName('g'):
        if g.getAttribute('inkscape:label') == layerName:
            g.setAttribute('style', 'display:none')

def showLayer(layerName, doc):
    for g in doc.getElementsByTagName('g'):
        if g.getAttribute('inkscape:label') == layerName:
            g.setAttribute('style', 'display:inline')

from_dir = '../res/panels/vapor'

for colors_table, to_dir in themes:
    # Delete all SVG files in the to_dir
    for filename in os.listdir(to_dir):
        if filename.endswith('.svg'):
            os.remove(os.path.join(to_dir, filename))

    # Copy all SVG files from from_dir to to_dir
    for filename in os.listdir(from_dir):
        if filename.endswith('.svg'):
            shutil.copy(os.path.join(from_dir, filename), to_dir)

    # Compile a regex pattern for color codes
    color_pattern = re.compile('|'.join(re.escape(color) for color in colors_table.keys()))


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
            content = color_pattern.sub(lambda match: colors_table[match.group(0)], content)
            # Write the file to the to_dir
            with open(filepath, 'w') as file:
                file.write(content)