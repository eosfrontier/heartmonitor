#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>
#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "osccal.h"

#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT             1

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"

#define CUSTOM_RQ_ECHO          0
#define CUSTOM_RQ_SET_STATUS    1
#define CUSTOM_RQ_GET_STATUS    2

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;
static uchar    dataBuffer[4];  /* buffer must stay valid when usbFunctionSetup returns */
static uchar bitstat = 0;

    bitstat ^= 1;
    if (bitstat) {
	LED_PORT_OUTPUT |= _BV(LED_BIT);
    } else {
        LED_PORT_OUTPUT &= ~_BV(LED_BIT);
    }
    if(rq->bRequest == CUSTOM_RQ_ECHO){ /* echo -- used for reliability tests */
        dataBuffer[0] = rq->wValue.bytes[0];
        dataBuffer[1] = rq->wValue.bytes[1];
        dataBuffer[2] = rq->wIndex.bytes[0];
        dataBuffer[3] = rq->wIndex.bytes[1];
        usbMsgPtr = dataBuffer;         /* tell the driver which data to return */
        return 4;
    }else if(rq->bRequest == CUSTOM_RQ_SET_STATUS){
        if(rq->wValue.bytes[0] & 1){    /* set LED */
            LED_PORT_OUTPUT |= _BV(LED_BIT);
        }else{                          /* clear LED */
            LED_PORT_OUTPUT &= ~_BV(LED_BIT);
        }
    }else if(rq->bRequest == CUSTOM_RQ_GET_STATUS){
        dataBuffer[0] = ((LED_PORT_OUTPUT & _BV(LED_BIT)) != 0);
        usbMsgPtr = dataBuffer;         /* tell the driver which data to return */
        return 1;                       /* tell the driver to send 1 byte */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int main(void)
{
    // uchar   i;

    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    cli();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    _delay_ms(250);
    usbDeviceConnect();
    sei();
    LED_PORT_DDR |= _BV(LED_BIT);   /* make the LED bit an output */
    // i = 0;
    for(;;){                /* main event loop */
	/*
	if (--i < 128) {
	    LED_PORT_OUTPUT |= _BV(LED_BIT);
	} else {
	    LED_PORT_OUTPUT &= ~_BV(LED_BIT);
	}
	*/
        // wdt_reset();
        usbPoll();
    }
}

/* ------------------------------------------------------------------------- */
