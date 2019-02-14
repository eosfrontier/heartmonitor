#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>
#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include <string.h>
#include "usbdrv.h"
#include "i2c_master.h"

#define LED_BIT          1

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"

#define NUM_SAMPLES 8
#define REPORTSIZE (6*(NUM_SAMPLES)+2)
static uchar report_out[REPORTSIZE+1];
static uchar errstate;

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

uchar mx_readfifo();
static uchar report_ptr;

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    usbRequest_t *rq = (void *)data;
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {
            if (!mx_readfifo()) {
                report_ptr = REPORTSIZE;
                return 0;
            }
            report_ptr = 0;
            return USB_NO_MSG;
        }
    }
    return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
    if (len > (REPORTSIZE - report_ptr)) {
        len = (REPORTSIZE - report_ptr);
    }
    uchar ct = len;
    while (ct--) {
        *data++ = report_out[report_ptr++];
    }
    return len;
}

/* ------------------------------------------------------------------------- */

#define MX_I2C_ADDR 0x57
#define MX_MODE_CONF 0x09
#define MX_SPO2_CONF 0x0A
#define MX_TEMP_CONF 0x21
#define MX_RED_PULSEAMPLITUDE 0x0C
#define MX_IR_PULSEAMPLITUDE 0x0D
#define MX_FIFO_WPTR 0x04
#define MX_FIFO_OVFL 0x05
#define MX_FIFO_RPTR 0x06
#define MX_FIFO_DATA 0x07
#define MX_FIFO_CONF 0x08

void i2c_setreg(uchar reg, uchar val)
{
    errstate |= I2C_Master_Write(MX_I2C_ADDR, reg, &val, 1);
}

uchar i2c_readreg(uchar reg)
{
    uchar msg = 0;
    errstate |= I2C_Master_Read(MX_I2C_ADDR, reg, &msg, 1);
    return msg;
}

void mx_setup()
{
    i2c_setreg(MX_MODE_CONF, 0x03);             // Multi-led mode
    i2c_setreg(MX_SPO2_CONF, 0x07);             // 100 samples/second, 18 bits, ADC range 0-2048
    i2c_setreg(MX_FIFO_CONF, 0x00);             // 1 samples averaged, rollover enable, full at 0
    i2c_setreg(MX_RED_PULSEAMPLITUDE, 0x0f);
    i2c_setreg(MX_IR_PULSEAMPLITUDE, 0x07);
    // i2c_setreg(MX_TEMP_CONF, 0x01);
}

uchar mx_readfifo()
{
    uchar wp, rp, nums;
    wp = i2c_readreg(MX_FIFO_WPTR);
    rp = i2c_readreg(MX_FIFO_RPTR);
    if (rp >= wp) wp += 0x20;
    nums = wp - rp;
    if (nums > NUM_SAMPLES) nums = NUM_SAMPLES;
    report_out[0] = errstate;
    report_out[1] = nums;
    errstate |= I2C_Master_Read(MX_I2C_ADDR, MX_FIFO_DATA, report_out+2, nums*6);
    return nums;
}

int main(void)
{
    cli();
    errstate = 0;
    usbInit();
    usbDeviceDisconnect();    /* enforce re-enumeration, do this while interrupts are disabled! */
    i2c_setreg(MX_MODE_CONF, 0x40);  /* Reset */
    _delay_ms(250);
    usbDeviceConnect();
    sei();
    mx_setup();

    // memset(report_out, 0, sizeof(report_out));
    // usbSetInterrupt(report_out+1, 8);
    for(;;) {                /* main event loop */
        usbPoll();
    }
}

PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x06, 0x91, 0x00,               // USAGE_PAGE (Arcade)
    0x09, 0x01,                     // USAGE (GPIO Card)

    0xa1, 0x01,                     // COLLECTION (Application)
    0x15, 0x00,                     //   LOGICAL_MINIUMM (0)
    0x25, 0x02,                     //   LOGICAL_MAXIMUM (2)
    0x75, 0x08,                     //   REPORT_SIZE (8)
    0x95, 0x02,                     //   REPORT_COUNT (2)

    0x09, 0x30,                     //   USAGE (Analog Input)
    0x15, 0x00,                     //   LOGICAL_MINIUMM (0)
    0x26, 0xff, 0x00,               //   LOGICAL_MAXIMUM (255)
    0x75, 0x18,                     //   REPORT_SIZE (8)
    0x95, 0x08,                     //   REPORT_COUNT (12)
    0xb1, 0x02,                     //   OUTPUT (Data,Var,Abs)

    0xc0                            // END_COLLECTION
};

/* ------------------------------------------------------------------------- */
/* vim: ai:si:expandtab:ts=4:sw=4
 */
