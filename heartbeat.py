#!/usr/bin/env python

import time, sys, os, random, smbus, ctypes
from rgbmatrix import RGBMatrix, RGBMatrixOptions
import pyaudio, wave
from neopixel import *
import MAX30102
import nfc

class NFCReader(object):
    MC_AUTH_A = 0x60
    MC_AUTH_B = 0x61
    MC_READ = 0x30
    MC_WRITE = 0xA0
    card_timeout = 10

    def __init__(self):
        self.__context = None
        self.__device = None
        
        self._card_present = False
        self._card_last_seen = None
        self._card_uid = None
        
        mods = [(nfc.NMT_ISO14443A, nfc.NBR_106)]
        self.__modulations = (nfc.nfc_modulation * len(mods))()
        for i in range(len(mods)):
            self.__modulations[i].nmt = mods[i][0]
            self.__modulations[i].nbr = mods[i][1]

    def run(self):
        self.__context = ctypes.pointer(nfc.nfc_context())
        nfc.nfc_init(ctypes.byref(self.__context))
        conn_strings = (nfc.nfc_connstring * 10)()
        devices_found = nfc.nfc_list_devices(self.__context, conn_strings, 10)
        if devices_found >= 1:
            self.__device = nfc.nfc_open(self.__context, conn_strings[0])
            _ = nfc.nfc_initiator_init(self.__device)

    def poll(self):
        nt = nfc.nfc_target()
        res = nfc.nfc_initiator_poll_target(self.__device, self.__modulations, len(self.__modulations), 1, 1, ctypes.byref(nt))
        if res < 0:
            return None
        elif res == 0:
            return None
        uid = None
        if nt.nti.nai.szUidLen == 4:
            uid = "".join([format(nt.nti.nai.abtUid[i],'02x') for i in range(4)])
        return uid

    def __del__(self):
        nfc.nfc_close(self.__device)
        nfc.nfc_exit(self.__context)


leds = Adafruit_NeoPixel(2, 13, channel=1)
mx = MAX30102.MAX30102()

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

i2c.write_word_data(mcp, mcp_iodir, 0xFFFF)
i2c.write_word_data(mcp, mcp_gppu, 0xFFFF)
i2c.write_word_data(mcp, mcp_gpio, 0x0000)

leds.begin()

options = RGBMatrixOptions()
options.rows = 32
options.cols = 64
options.disable_hardware_pulsing = True
options.pixel_mapper_config = "Rotate:180"

nfcRdr = NFCReader()
nfcRdr.run()

matrix = RGBMatrix(options = options)

pixs = [255,255,255,255,255,0]
try:
    matrix.Fill(0, 0, 0)
    heartbeatdelay = -100
    cur = 0.4
    tgt = 0.4
    spd = 0.1
    beat = False
    while True:
        uid = nfcRdr.poll()
        if uid:
            print "Got NFC card "+uid
            if uid == 'e4c3b6c3' and heartbeatdelay != 120:
                print "Stop beat"
                heartbeatdelay = 120
                spd = 0.1
                tgt = 0.4
            if uid == '9349a524' and heartbeatdelay == 120:
                print "Start beat"
                heartbeatdelay = -100
                spd = 0.1
                tgt = 0.4
        for x in range(0, matrix.width):
            for y in range(0, matrix.height):
                matrix.SetPixel(x, y, 0, 0, 0)
                matrix.SetPixel(x+1, y, 0, 0, 0)
                matrix.SetPixel(x+2, y, 0, 0, 0)
                matrix.SetPixel(x+3, y, 0, 0, 0)
                matrix.SetPixel(x+4, y, 0, 0, 0)
            if heartbeatdelay == -100:
                mx.update()
                tgt = 0.5 - (mx.ir / 1200.0)
                spd = 1.0
                if tgt > 1.0:
                    tgt = 1.0
                if tgt < 0.0:
                    tgt = 0.0
                if tgt > 0.8 and not beat:
                    beat = True
                    stream.write(beep)
                if tgt < 0.4 and beat:
                    beat = False
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
            if heartbeatdelay > -100 and heartbeatdelay <= 100:
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
            time.sleep(0.01)
except KeyboardInterrupt:
    leds.setPixelColor(0, Color(0, 0, 0))
    leds.setPixelColor(1, Color(0, 0, 0))
    leds.show()
    #mx.close()
    sys.exit(0)
