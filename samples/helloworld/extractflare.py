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

    def addAnimation(self, name, col, count):
        self.animationList.append({'name':name, 'col':col, 'count':count})

    def output(self, dir):
        #appendFlags = ' -trim '
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


if __name__ == "__main__":
    archer = FlareExtractor("flare/orc_archer_0.png", 128, 128)
    archer.addAnimation("still", 0, 4)
    archer.addAnimation("run", 5, 7)
    archer.output('images')
