#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>
#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "osccal.h"

#define BUTTON_BIT          0
#define LED_BIT             2

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"

#define CUSTOM_RQ_ECHO          0
#define CUSTOM_RQ_SET_STATUS    1
#define CUSTOM_RQ_GET_STATUS    2

static uchar cur_rgb[3];
static uchar tgt_rgb[3];

static uchar report_out[1];

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    usbRequest_t    *rq = (void *)data;

    if ((rq->bRequest & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        if (rq->bRequest == USBRQ_HID_SET_REPORT) {
            return USB_NO_MSG;
        }
    } else {
        /* Do nothing */
    }
    return 0;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
    uchar i;
    if (len > 3) len = 3;
    for (i = 0; i < len; i++) {
        tgt_rgb[i] = data[i];
    }
    return 1;
}

/* ------------------------------------------------------------------------- */

#define NS_PER_CYCLE (1000000000 / F_CPU)
#define NS_TO_CYCLES(n) ((n)/NS_PER_CYCLE)
#define DELAY_CYCLES(n) ((((n)>0) ? __builtin_avr_delay_cycles(n) : __builtin_avr_delay_cycles(0)))

void send_bit(uchar) __attribute__ ((optimize(0)));

void send_bit(uchar bitval)
{
    if (bitval) {
        asm volatile (
                "sbi %[port], %[bit] \n\t"
                ".rept %[onCycles] \n\t"
                "nop \n\t"
                ".endr \n\t"
                "cbi %[port], %[bit] \n\t"
                ".rept %[offCycles] \n\t"
                "nop \n\t"
                ".endr \n\t"
                ::
                [port]      "I" (_SFR_IO_ADDR(PORTB)),
                [bit]       "I" (LED_BIT),
                [onCycles]  "I" (NS_TO_CYCLES(900)-2),
                [offCycles] "I" (NS_TO_CYCLES(600)-10)
                );
    } else {
        asm volatile (
                "cli \n\t"
                "sbi %[port], %[bit] \n\t"
                ".rept %[onCycles] \n\t"
                "nop \n\t"
                ".endr \n\t"
                "cbi %[port], %[bit] \n\t"
                "sei \n\t"
                ".rept %[offCycles] \n\t"
                "nop \n\t"
                ".endr \n\t"
                ::
                [port]      "I" (_SFR_IO_ADDR(PORTB)),
                [bit]       "I" (LED_BIT),
                [onCycles]  "I" (NS_TO_CYCLES(400)-2),
                [offCycles] "I" (NS_TO_CYCLES(900)-10)
                );
    }
}

void send_led()
{
    for (uchar x = 0x80; x > 0; x >>= 1) { send_bit(cur_rgb[0] & x); }
    for (uchar x = 0x80; x > 0; x >>= 1) { send_bit(cur_rgb[1] & x); }
    for (uchar x = 0x80; x > 0; x >>= 1) { send_bit(cur_rgb[2] & x); }
    for (uchar x = 0x80; x > 0; x >>= 1) { send_bit(cur_rgb[0] & x); }
    for (uchar x = 0x80; x > 0; x >>= 1) { send_bit(cur_rgb[1] & x); }
    for (uchar x = 0x80; x > 0; x >>= 1) { send_bit(cur_rgb[2] & x); }
}

int main(void)
{
    cli();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    _delay_ms(250);
    usbDeviceConnect();
    sei();
    DDRB |= _BV(LED_BIT);

    DDRB &= ~_BV(BUTTON_BIT);
    PORTB |= _BV(BUTTON_BIT);

    cur_rgb[0] = 1;
    cur_rgb[1] = 1;
    cur_rgb[2] = 1;
    tgt_rgb[0] = 0;
    tgt_rgb[1] = 0;
    tgt_rgb[2] = 0;
    report_out[0] = 0;
    usbSetInterrupt(report_out, 1);
    int delay = 0;
    for(;;){                /* main event loop */
        usbPoll();
        if (++delay >= 10) {
            if (cur_rgb[0] != tgt_rgb[0] || cur_rgb[1] != tgt_rgb[1] || cur_rgb[2] != tgt_rgb[2]) {
                if (cur_rgb[0] < tgt_rgb[0]) { cur_rgb[0]++; }
                else if (cur_rgb[0] > tgt_rgb[0]) { cur_rgb[0]--; }
                if (cur_rgb[1] < tgt_rgb[1]) { cur_rgb[1]++; }
                else if (cur_rgb[1] > tgt_rgb[1]) { cur_rgb[1]--; }
                if (cur_rgb[2] < tgt_rgb[2]) { cur_rgb[2]++; }
                else if (cur_rgb[2] > tgt_rgb[2]) { cur_rgb[2]--; }
                send_led();
            }
            delay = 0;
        }
        if (usbInterruptIsReady()) {
            uchar report = (PINB & _BV(BUTTON_BIT)) ? 0 : 1;
            if (report != report_out[0]) {
                report_out[0] = report;
                usbSetInterrupt(report_out, 1);
            }
        }
        _delay_ms(1);
    }
}

PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                     // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                     // USAGE (Game Pad)
    0xa1, 0x01,                     // COLLECTION (Application)

    0x05, 0x09,                     //   USAGE_PAGE (Button)
    0x19, 0x01,                     //   USAGE_MINIMUM (1)
    0x29, 0x01,                     //   USAGE_MAXIMUM (1)
    0x15, 0x00,                     //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                     //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                     //   REPORT_SIZE (1)
    0x95, 0x01,                     //   REPORT_COUNT (1)
    0x81, 0x02,                     //   INPUT (Data,Var,Abs)

    0x06, 0x00, 0xff,               //   USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                     //   USAGE (Vendor Usage 1)
    0x15, 0x00,                     //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,               //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                     //   REPORT_SIZE (8)
    0x95, 0x03,                     //   REPORT_COUNT (3)
    0x91, 0x02,                     //   OUTPUT (Data, Var, Abs)
    0xc0                            // END_COLLECTION
};

/* ------------------------------------------------------------------------- */
/* vim: ai:si:expandtab:ts=4:sw=4
 */
