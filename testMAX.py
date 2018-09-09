#!/usr/bin/env python

import MAX30102
import time

mx = MAX30102.MAX30102()

while True:
    mx.update()
    print "Red: %18.4f IR: %18.4f (0x%06x 0x%06x" % (mx.red, mx.ir, mx.rawred, mx.rawir)
    time.sleep(0.02)
