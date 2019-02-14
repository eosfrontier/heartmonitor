#!/usr/bin/env python
import time
import hidapi

devlist = hidapi.enumerate(0x16c0,0x05df)

devpath = devlist.next()
print devpath.path
dev = hidapi.Device(path=devpath.path)
print "%s %s" % (dev.get_manufacturer_string(), dev.get_product_string())
while True:
    values = map(ord, dev.get_feature_report(chr(0), (6*8)+2))
    if values:
        errstate = values[0]
        nums = values[1]
        #print "Err: %02x Nums: %02x" % (errstate, nums)
        for sn in range(0,nums):
            red = (values[sn*6+2]*256+values[sn*6+3])*256+values[sn*6+4]
            ird = (values[sn*6+5]*256+values[sn*6+6])*256+values[sn*6+7]
            print "Red: %10d IR: %10d" % (red, ird)
    time.sleep(0.02)
