#!/usr/bin/env python
import usb.core
import usb.util
import time


print usb.core.show_devices()

# find our device
dev = usb.core.find(idVendor=0x16c0, idProduct=0x05df)

# was it found?
if dev is None:
    raise ValueError('Device not found')

print dev

# set the active configuration. With no arguments, the first
# configuration will be the active one
dev.set_configuration()

for x in range(0,10):
    time.sleep(1.0)
    print dev.ctrl_transfer(0x21, 9, 0x200, 0x00, [0x80, 0x40, 0x20])
    time.sleep(1.0)
    print dev.ctrl_transfer(0x21, 9, 0x200, 0x00, [0x40, 0x40, 0x40])
    time.sleep(1.0)
    print dev.ctrl_transfer(0x21, 9, 0x200, 0x00, [0x00, 0xff, 0x00])
    time.sleep(1.0)
    print dev.ctrl_transfer(0x21, 9, 0x200, 0x00, [0x00, 0x00, 0x00])
