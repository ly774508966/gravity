import sys
import os
import re
import subprocess

def executeCmd(cmd):
    try:
        out = subprocess.check_output(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        return (e.returncode, e.output)
    return (0, out)

class FlareExtractor:

    def __init__(self, filename, gridWidth, gridHeight):
        self.source = filename
        self.gridWidth = gridWidth
        self.gridHeight = gridHeight
        self.animationList = []

    def addAnimation(self, name, row, col, count):
        self.animationList.append({'name':name, 'row':row, 'col':col, 'count':count})

    def outputAllDirections(self, dir):
        appendFlags = ' '
        for animation in self.animationList:
            top = 0
            for row in range(0, 8):
                left = animation['col'] * self.gridWidth
                for col in range(animation['col'], animation['col'] + animation['count']):
                    outputFilename = '{}/{}_{}_{}.png'.format(dir, animation['name'], row, col - animation['col'])
                    print(outputFilename)
                    executeCmd('convert {} -crop {}x{}+{}+{} {} {}'.format(
                        self.source, self.gridWidth, self.gridHeight, left, top, appendFlags, outputFilename))
                    left += self.gridWidth
                top += self.gridHeight

    def output(self, dir):
        appendFlags = ' '
        for animation in self.animationList:
            top = animation['row'] * self.gridHeight
            left = animation['col'] * self.gridWidth
            for col in range(animation['col'], animation['col'] + animation['count']):
                outputFilename = '{}/{}_{}_{}.png'.format(dir, animation['name'],
                    animation['row'], col - animation['col'])
                print(outputFilename)
                executeCmd('convert {} -crop {}x{}+{}+{} {} {}'.format(
                    self.source, self.gridWidth, self.gridHeight, left, top, appendFlags, outputFilename))
                left += self.gridWidth

if __name__ == "__main__":
    tiled = FlareExtractor("flare/tileset_cave_1.png", 64, 32)
    tiled.addAnimation("base", 0, 0, 16);
    tiled.addAnimation("base", 1, 0, 16);
    tiled.addAnimation("base", 2, 0, 16);
    tiled.output('images');
    archer = FlareExtractor("flare/orc_archer_0.png", 128, 128)
    archer.addAnimation("still", 0, 0, 4)
    archer.addAnimation("run", 0, 5, 7)
    archer.outputAllDirections('images')
    wyvern = FlareExtractor("flare/wyvern_composite.png", 256, 256)
    wyvern.addAnimation("wyvern_still", 0, 0, 5)
    wyvern.outputAllDirections('images');
