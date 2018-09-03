#!/usr/bin/env python

import time, sys, os, random, smbus
from rgbmatrix import RGBMatrix, RGBMatrixOptions
import pyaudio, wave
from neopixel import *

leds = Adafruit_NeoPixel(2, 13, channel=1)

beepwav = wave.open("audio/beep.wav","rb")

pa = pyaudio.PyAudio()
stream = pa.open(format = pa.get_format_from_width(beepwav.getsampwidth()),
                 channels = beepwav.getnchannels(),
                 rate = beepwav.getframerate(),
                 output = True)

beep = beepwav.readframes(4096)

i2c = smbus.SMBus(1)
mcp = 0x20
mcp_iodir = 0x00
mcp_gppu = 0x0c
mcp_gpio = 0x12

try:
    i2c.write_word_data(mcp, mcp_iodir, 0xFFFF)
    i2c.write_word_data(mcp, mcp_gppu, 0xFFFF)
    i2c.write_word_data(mcp, mcp_gpio, 0x0000)
except:
    pass

leds.begin()
leds.setPixelColor(0, Color(255, 255, 255))
leds.setPixelColor(1, Color(255, 255, 255))
leds.show()

options = RGBMatrixOptions()
options.rows = 32
options.cols = 64
options.disable_hardware_pulsing = True
options.pixel_mapper_config = "Rotate:180"

matrix = RGBMatrix(options = options)

pixs = [255,255,255,255,255,0]
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
                tgt = random.randint(15,25)/100.0
                spd = 0.1
            elif heartbeatdelay == 15:
                tgt = random.randint(90,90)/100.0
                spd = 0.3
                stream.write(beep)
            elif heartbeatdelay == 18:
                tgt = random.randint(5,15)/100.0
                spd = 0.2
            elif heartbeatdelay == 22:
                tgt = random.randint(50,60)/100.0
                spd = 0.1
            elif heartbeatdelay >= 27 and heartbeatdelay < 35:
                tgt = random.randint(35,45)/100.0
                spd = 0.1
            elif heartbeatdelay ==35:
                tgt = 0.4
                spd = 0.1
            elif heartbeatdelay == 40:
                heartbeatdelay = random.randint(-5,5)
            ypos1 = int(matrix.height/2 * (1.0-cur))
            if cur < tgt:
                cur = cur + spd
                if cur > tgt:
                    cur = tgt
                    spd = 0.0
                ypos2 = int(matrix.height/2 * (1.0-cur))
                for y in range(ypos2, ypos1):
                    matrix.SetPixel(x, y, 0, 255, 0)
            elif cur > tgt:
                cur = cur - spd
                if cur < tgt:
                    cur = tgt
                    spd = 0.0
                ypos2 = int(matrix.height/2 * (1.0-cur))
                for y in range(ypos1, ypos2):
                    matrix.SetPixel(x, y, 0, 255, 0)
            matrix.SetPixel(x, ypos1, 0, 200, 0)
            heartbeatdelay += 1

            try:
                buttons = i2c.read_word_data(mcp, mcp_gpio)
                for x in range(0,16):
                    if ((buttons >> x) & 1) == 0:
                        if x >= 0 and x <= 4:
                            pixs[x] = 0
                        matrix.SetPixel(x*4+2, 30, 0, 0, 255)
                        matrix.SetPixel(x*4+3, 30, 0, 64, 64)
                        matrix.SetPixel(x*4+1, 30, 0, 64, 64)
                        matrix.SetPixel(x*4+2, 31, 0, 64, 64)
                        matrix.SetPixel(x*4+2, 29, 0, 64, 64)
                    else:
                        if x >= 0 and x <= 4:
                            pixs[x] += 2
                            if pixs[x] >= 255:
                                pixs[x] = 255
                        matrix.SetPixel(x*4+2, 30, 0, 0, 0)
                        matrix.SetPixel(x*4+3, 30, 0, 0, 0)
                        matrix.SetPixel(x*4+1, 30, 0, 0, 0)
                        matrix.SetPixel(x*4+2, 31, 0, 0, 0)
                        matrix.SetPixel(x*4+2, 29, 0, 0, 0)
                leds.setPixelColor(0, Color(pixs[2], pixs[1], pixs[0]))
                leds.setPixelColor(1, Color(pixs[5], pixs[4], pixs[3]))
                leds.show()
            except:
                pass
            time.sleep(0.02)
except KeyboardInterrupt:
    leds.setPixelColor(0, Color(0, 0, 0))
    leds.setPixelColor(1, Color(0, 0, 0))
    leds.show()
    sys.exit(0)
