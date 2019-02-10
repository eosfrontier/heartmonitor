#!/usr/bin/env python
import time
import hidapi

devlist = hidapi.enumerate(0x16c0,0x05df)

devpath = devlist.next()
dev = hidapi.Device(path=devpath.path)
try:
    while True:
        btn = dev.read(1, timeout_ms=1000)
        if btn:
            print "%s" % ord(btn)
        dev.write('\x80\x00\x00')
        time.sleep(0.5)
        dev.write('\x00\x80\x00')
        time.sleep(0.5)
        dev.write('\x00\x00\x00')
        btn = dev.read(1, timeout_ms=1000)
        if btn:
            print "%s" % ord(btn)
        dev.write('\xff\x00\x00\xff\xff\xff')
        time.sleep(0.5)
        dev.write('\x00\xff\x00')
        time.sleep(0.5)
        dev.write('\x00\x00\xff')
        time.sleep(0.5)
        dev.write('\x00\x00\x00\x00\x00\x00')
except KeyboardInterrupt:
    dev.write('\x00\x00\x00\x00\x00\x00')
