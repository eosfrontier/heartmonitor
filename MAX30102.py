from smbus import SMBus
import time,numpy
import hidapi

class MAX30102(object):
    ALPHA = 0.92
    AVGSIZE = 100

    I2C_ADDRESS = 0x57

    FIFO_WPTR = 0x04
    FIFO_OVFL = 0x05
    FIFO_RPTR = 0x06
    FIFO_DATA = 0x07
    FIFO_CONF = 0x08
    MODE_CONF = 0x09
    SPO2_CONF = 0x0a
    RED_AMPL  = 0x0c
    IR_AMPL   = 0x0d
    LED_SLOTS = 0x11
    TEMP_INT  = 0x1f
    TEMP_FRAC = 0x20
    TEMP_CONF = 0x21

    USB_VENDORID = 0x16c0
    USB_PRODUCTID = 0x05df
    USB_MANUFACTURER = 'monsuwe.nl'
    USB_PRODUCT = 'HeartMonitor'

    def __init__(self, conntype = 'usb'):
        self.pdstate = 0
        self.redcurrent = 0x08
        self.ircurrent = 0x10

        self.rawred = 0
        self.redw = 0.0
        self.redv = 0.0
        self.red = 0.0
        self.redsum = 0.0

        self.rawir = 0
        self.irw = 0.0
        self.irv = 0.0
        self.ir = 0.0
        self.irsum = 0.0

        self.redbuf = numpy.zeros(self.AVGSIZE, dtype=int)
        self.irbuf = numpy.zeros(self.AVGSIZE, dtype=int)
        self.bufidx = 0

        self.pulse = False
        self.i2c = None
        self.usb = None

        if conntype == 'i2c':
            try:
                self.i2c = SMBus(1)
                self.set_reg(self.MODE_CONF, 0x40)
                time.sleep(0.01)
                self.set_reg(self.MODE_CONF, 0x03)
                self.set_reg(self.SPO2_CONF, ((0x00 << 5) | (0x01 << 2) | 0x03))
                self.set_currents()
                self.set_reg(self.TEMP_CONF, 0x01)
            except:
                print "Heartbeat I2C sensor failed"
                self.i2c = None
        if conntype == 'usb':
            try:
                for dev in hidapi.enumerate(self.USB_VENDORID, self.USB_PRODUCTID):
                    print "Checking %s" % (dev.path)
                    if dev.manufacturer_string == self.USB_MANUFACTURER and dev.product_string == self.USB_PRODUCT:
                        self.usb = hidapi.Device(path=dev.path)
            except:
                self.usb = None
            if not self.usb:
                print "Heartbeat USB sensor failed"

    def close(self):
        self.set_reg(self.MODE_CONF, 0x80)

    def set_reg(self, reg, byte):
        if self.i2c:
            self.i2c.write_byte_data(self.I2C_ADDRESS, reg, byte)

    def set_currents(self):
        if self.i2c:
            self.i2c.write_byte_data(self.I2C_ADDRESS, self.RED_AMPL, self.redcurrent)
            self.i2c.write_byte_data(self.I2C_ADDRESS, self.IR_AMPL, self.ircurrent)

    def update(self):
        if self.usb:
            self._update_usb()
        elif self.i2c:
            self._update_i2c()
        else:
            self.ir = 0.0
            self.red = 0.0

    def _update_usb(self):
        values = map(ord, self.usb.get_feature_report(chr(0), (6*8)+2))
        if values:
            nums = values[1]
            pulse = False
            for sn in range(0,nums):
                self._process(values[sn*6+2:sn*6+8])
                if self.pulse:
                    pulse = True
            self.pulse = pulse

    def _update_i2c(self):
        wptr = self.i2c.read_byte_data(self.I2C_ADDRESS, self.FIFO_WPTR)
        rptr = self.i2c.read_byte_data(self.I2C_ADDRESS, self.FIFO_RPTR)
        if (wptr < rptr):
            wptr += 0x20
        pulse = False
        for r in range(rptr, wptr):
            self._process(self.i2c.read_i2c_block_data(self.I2C_ADDRESS, self.FIFO_DATA, 6))
            if self.pulse:
                pulse = True
        self.pulse = pulse

    def _process(self, buf):
        self.rawir  = (buf[0]<<16) + (buf[1]<<8) + buf[2]
        self.rawred = (buf[3]<<16) + (buf[4]<<8) + buf[5]

        irw = float(self.rawir) + self.ALPHA * self.irw
        self.ir = irw - self.irw
        self.irw = irw

        irv = (0.1367287359973195227 * self.ir) + (0.72654252800536101020 * self.irv)
        self.ir = self.irv + irv
        self.irv = irv

        redw = float(self.rawred) + self.ALPHA * self.redw
        self.red = redw - self.redw
        self.redw = redw

        redv = (0.1367287359973195227 * self.red) + (0.72654252800536101020 * self.redv)
        self.red = self.redv + redv
        self.redv = redv

        """
        self.irsum -= self.irbuf[self.bufidx] 
        self.irbuf[self.bufidx] = self.ir
        self.irsum += self.ir
        self.ir = self.ir - (self.irsum / self.AVGSIZE)

        self.redsum -= self.redbuf[self.bufidx] 
        self.redbuf[self.bufidx] = self.red
        self.redsum += self.red
        self.red = self.red - (self.redsum / self.AVGSIZE)

        self.bufidx = (self.bufidx+1)%self.AVGSIZE
        """

        # TODO: filters, pulse detection
        

#vim: ai:si:expandtab:ts=4:sw=4
