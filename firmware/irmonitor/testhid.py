#!/usr/bin/env python
import time
import hidapi

devlist = hidapi.enumerate(0x16c0,0x05df)

devpath = devlist.next()
print devpath.path
dev = hidapi.Device(path=devpath.path)
print "%s %s" % (dev.get_manufacturer_string(), dev.get_product_string())
while True:
    values = dev.get_feature_report(chr(0), 0x0e)
    if values:
        print "Read: %s" % map(lambda x:"%02x"%ord(x), values)
    time.sleep(0.1)
