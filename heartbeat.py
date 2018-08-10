#!/usr/bin/env python

import time, sys, os, random
from rgbmatrix import RGBMatrix, RGBMatrixOptions

options = RGBMatrixOptions()
options.rows = 32
options.cols = 64
options.pixel_mapper_config = "Rotate:180"

matrix = RGBMatrix(options = options)

try:
    matrix.Fill(0, 0, 0)
    heartbeatdelay = 0
    cur = 0.4
    tgt = 0.4
    spd = 0.1
    while True:
        for x in range(0, matrix.width):
            for y in range(0, matrix.height):
                matrix.SetPixel(x, y, 0, 0, 0)
                matrix.SetPixel(x+1, y, 0, 0, 0)
                matrix.SetPixel(x+2, y, 0, 0, 0)
                matrix.SetPixel(x+3, y, 0, 0, 0)
                matrix.SetPixel(x+4, y, 0, 0, 0)
            if heartbeatdelay == 10:
                tgt = 0.2
                spd = 0.1
            elif heartbeatdelay == 15:
                tgt = 0.93
                spd = 0.3
            elif heartbeatdelay == 18:
                tgt = 0.1
                spd = 0.2
            elif heartbeatdelay == 22:
                tgt = 0.5
                spd = 0.1
            elif heartbeatdelay == 27:
                tgt = 0.4
                spd = 0.1
            elif heartbeatdelay == 40:
                heartbeatdelay = random.randint(-5,5)
            ypos1 = int(matrix.height * (1.0-cur))
            if cur < tgt:
                cur = cur + spd
                if cur > tgt:
                    cur = tgt
                    spd = 0.0
                ypos2 = int(matrix.height * (1.0-cur))
                for y in range(ypos2, ypos1):
                    matrix.SetPixel(x, y, 0, 200, 0)
            elif cur > tgt:
                cur = cur - spd
                if cur < tgt:
                    cur = tgt
                    spd = 0.0
                ypos2 = int(matrix.height * (1.0-cur))
                for y in range(ypos1, ypos2):
                    matrix.SetPixel(x, y, 0, 200, 0)
            matrix.SetPixel(x, ypos1, 0, 200, 0)
            heartbeatdelay += 1
            time.sleep(0.02)
except KeyboardInterrupt:
    sys.exit(0)
