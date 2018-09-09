from smbus import SMBus
import time

class MAX30102(object):
    ALPHA = 0.95
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

    def __init__(self):
        self.i2c = SMBus(1)
        self.pdstate = 0
        self.redcurrent = 0x0f
        self.ircurrent = 0x07
        self.rawred = 0
        self.red = 0
        self.redw = 0
        self.rawir = 0
        self.ir = 0
        self.irw = 0
        self.pulse = False
        self.set_reg(self.MODE_CONF, 0x40)
        time.sleep(0.01)
        self.set_reg(self.MODE_CONF, 0x03)
        self.set_reg(self.SPO2_CONF, ((0x00 << 5) | (0x01 << 2) | 0x03))
        self.set_currents()
        self.set_reg(self.TEMP_CONF, 0x01)

    def set_reg(self, reg, byte):
        self.i2c.write_byte_data(self.I2C_ADDRESS, reg, byte)

    def set_currents(self):
        self.i2c.write_byte_data(self.I2C_ADDRESS, self.RED_AMPL, self.redcurrent)
        self.i2c.write_byte_data(self.I2C_ADDRESS, self.IR_AMPL, self.ircurrent)

    def update(self):
        wptr = self.i2c.read_byte_data(self.I2C_ADDRESS, self.FIFO_WPTR)
        rptr = self.i2c.read_byte_data(self.I2C_ADDRESS, self.FIFO_RPTR)
        if (wptr < rptr):
            wptr += 0x20
        pulse = False
        for r in range(rptr, wptr):
            self._read()
            if self.pulse:
                pulse = True
        self.pulse = pulse

    def _read(self):
        buf = self.i2c.read_i2c_block_data(self.I2C_ADDRESS, self.FIFO_DATA, 6)
        self.rawir  = (buf[0]<<16) + (buf[1]<<8) + buf[2]
        self.rawred = (buf[3]<<16) + (buf[4]<<8) + buf[5]

        irw = float(self.rawir) + self.ALPHA * self.irw
        self.ir = irw - self.irw
        self.irw = irw

        redw = float(self.rawred) + self.ALPHA * self.redw
        self.red = redw - self.redw
        self.redw = redw
        # TODO: filters, pulse detection
        

#vim: ai:si:expandtab:ts=4:sw=4
