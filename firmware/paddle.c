#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>
#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "osccal.h"

#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
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

uchar cur_rgb[3];
uchar tgt_rgb[3];

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;
static uchar    dataBuffer[4];  /* buffer must stay valid when usbFunctionSetup returns */

    if(rq->bRequest == CUSTOM_RQ_ECHO){ /* echo -- used for reliability tests */
        dataBuffer[0] = rq->wValue.bytes[0];
        dataBuffer[1] = rq->wValue.bytes[1];
        dataBuffer[2] = rq->wIndex.bytes[0];
        dataBuffer[3] = rq->wIndex.bytes[1];
        usbMsgPtr = dataBuffer;         /* tell the driver which data to return */
        return 4;
    }else if(rq->bRequest == CUSTOM_RQ_SET_STATUS){
	tgt_rgb[0] = rq->wValue.bytes[0];
	tgt_rgb[1] = rq->wValue.bytes[1];
	tgt_rgb[2] = rq->wIndex.bytes[0];
    }else if(rq->bRequest == CUSTOM_RQ_GET_STATUS){
        dataBuffer[0] = cur_rgb[0];
        dataBuffer[1] = cur_rgb[1];
        dataBuffer[2] = cur_rgb[2];
        usbMsgPtr = dataBuffer;         /* tell the driver which data to return */
        return 3;                       /* tell the driver to send 3 byte */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
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
		[port]      "I" (_SFR_IO_ADDR(LED_PORT_OUTPUT)),
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
		[port]      "I" (_SFR_IO_ADDR(LED_PORT_OUTPUT)),
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
    cur_rgb[0] = 1;
    cur_rgb[1] = 1;
    cur_rgb[2] = 1;
    tgt_rgb[0] = 0;
    tgt_rgb[1] = 0;
    tgt_rgb[2] = 0;
    cli();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    _delay_ms(250);
    usbDeviceConnect();
    sei();
    LED_PORT_DDR |= _BV(LED_BIT);
    int delay = 0;
    for(;;){                /* main event loop */
        usbPoll();
	if (delay <= 0) {
	    if (cur_rgb[0] != tgt_rgb[0] || cur_rgb[1] != tgt_rgb[1] || cur_rgb[2] != tgt_rgb[2]) {
		if (cur_rgb[0] < tgt_rgb[0]) { cur_rgb[0]++; }
		else if (cur_rgb[0] > tgt_rgb[0]) { cur_rgb[0]--; }
		if (cur_rgb[1] < tgt_rgb[1]) { cur_rgb[1]++; }
		else if (cur_rgb[1] > tgt_rgb[1]) { cur_rgb[1]--; }
		if (cur_rgb[2] < tgt_rgb[2]) { cur_rgb[2]++; }
		else if (cur_rgb[2] > tgt_rgb[2]) { cur_rgb[2]--; }
		send_led();
	    }
	    delay = 20;
	} else {
	    delay--;
	}
	_delay_ms(1);
    }
}

/*
PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x80,                    //   REPORT_COUNT (128)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
*/

/* ------------------------------------------------------------------------- */
