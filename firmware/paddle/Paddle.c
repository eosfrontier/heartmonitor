#include "Paddle.h"
#include <stdlib.h>

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint                 =
					{
						.Address                = CDC_TX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.DataOUTEndpoint                =
					{
						.Address                = CDC_RX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.NotificationEndpoint           =
					{
						.Address                = CDC_NOTIFICATION_EPADDR,
						.Size                   = CDC_NOTIFICATION_EPSIZE,
						.Banks                  = 1,
					},
			},
	};

/** Buffer to hold the previously generated Button HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevButtonHIDReportBuffer[sizeof(USB_ButtonReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Button_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber                = INTERFACE_ID_Button,
				.ReportINEndpoint               =
					{
						.Address                = BUTTON_EPADDR,
						.Size                   = BUTTON_EPSIZE,
						.Banks                  = 1,
					},
				.PrevReportINBuffer             = PrevButtonHIDReportBuffer,
				.PrevReportINBufferSize         = sizeof(PrevButtonHIDReportBuffer),
			},
	};


static FILE USBSerialStream;

#define FPIN1 0x02 // PD1
#define FPIN2 0x01 // PD0
#define FPIN3 0x10 // PD4
#define FPIN4 0x40 // PC6
#define FPIN5 0x80 // PD7
#define FPIN6 0x40 // PE6
#define FPIN7 0x10 // PB4
#define FPIN8 0x20 // PB5
#define FPINSB 0x30 // PB4,PB5
#define FPINSC 0x40 // PC6
#define FPINSD 0x93 // PD0,PD1,PD4,PD7
#define FPINSE 0x40 // PE6

#define LED_BIT             1
#define LED_PORT            PORTB
#define NS_PER_CYCLE (1000000000 / F_CPU)
#define NS_TO_CYCLES(n) ((n)/NS_PER_CYCLE)
#define DELAY_CYCLES(n,s) ((((NS_TO_CYCLES(n)-s)>0) ? (NS_TO_CYCLES(n)-s) : (0)))

void send_bit(unsigned char) __attribute__ ((optimize(0)));

void send_bit(unsigned char bitval)
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
                [port]      "I" (_SFR_IO_ADDR(LED_PORT)),
                [bit]       "I" (LED_BIT),
                [onCycles]  "I" (DELAY_CYCLES(900,2)),
                [offCycles] "I" (DELAY_CYCLES(600,9))
                );
    } else {
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
                [port]      "I" (_SFR_IO_ADDR(LED_PORT)),
                [bit]       "I" (LED_BIT),
                [onCycles]  "I" (DELAY_CYCLES(400,2)),
                [offCycles] "I" (DELAY_CYCLES(900,10))
                );
    }
}

static void send_pixels(int cols[6])
{
    cli();
    for (int i = 0; i < 6; i++) {
	    for (unsigned char x = 0x80; x > 0; x >>= 1) {
		    send_bit(cols[i] & x);
	    }
    }
    sei();
}

static void clear_pixels(int cols[6])
{
    cli();
    for (int i = 0; i < 6; i++) {
	    for (unsigned char x = 0x80; x > 0; x >>= 1) {
		    send_bit(0);
	    }
    }
    sei();
}

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	char inbuf[33];
	int inptr = 0;

	SetupHardware();

	DDRB |= FPINSB;
	PORTB &= ~FPINSB;
	DDRC |= FPINSC;
	PORTC &= ~FPINSC;
	DDRD |= FPINSD;
	PORTD &= ~FPINSD;
	DDRE |= FPINSE;
	PORTE &= ~FPINSE;
	
	DDRB |= 0x40;  // Piezo PB6 (OC1B)
	DDRB |= (1 << LED_BIT);
	PORTB |= 0x04; // Button PB2

	clear_pixels();

	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	unsigned int pzfreq = 0;
	unsigned int pzwait = 1;
	unsigned int pzdelay = 50;
	TCCR1A = 0x10;
	TCCR1B = 0x08;

	for (;;)
	{
		int c = getc(&USBSerialStream);
		if (c >= 0) {
			if (c == '\n') {
				inbuf[inptr] = 0;
				if (inptr >= 5 && memcmp(inbuf, "flash", 5) == 0) {
					int ms = 0;
					for (int rp = 5; rp < inptr; rp++) {
						if (inbuf[rp] >= '0' && inbuf[rp] <= '9') {
							ms = ms * 10 + (inbuf[rp] - '0');
						}
					}
					if (ms <= 0) ms = 200;
					PORTB |= FPINSB;
					PORTC |= FPINSC;
					PORTD |= FPINSD;
					PORTE |= FPINSE;
					TCCR1B = 0x09;
					for (int i = 0; i < ms; i++) {
						OCR1A = 1000 + (rand() & 0x3fff);
						Delay_MS(1);
					}
					PORTB &= ~FPINSB;
					PORTC &= ~FPINSC;
					PORTD &= ~FPINSD;
					PORTE &= ~FPINSE;
					fputs("Flashed\r\n", &USBSerialStream);
					TCCR1B = 0x08;
					pzfreq = 0;
				} else if (inptr >= 8 && memcmp(inbuf, "soundoff", 8) == 0) {
					TCCR1B = 0x08;
					pzfreq = 0;
					fputs("Sound off\r\n", &USBSerialStream);
				} else if (inptr >= 5 && memcmp(inbuf, "sound", 5) == 0) {
					int ms = 0;
					for (int rp = 5; rp < inptr; rp++) {
						if (inbuf[rp] >= '0' && inbuf[rp] <= '9') {
							ms = ms * 10 + (inbuf[rp] - '0');
						}
					}
					if (ms <= 0) ms = 10;
					pzdelay = ms;
					TCCR1B = 0x09;
					pzfreq = 0xFFFF;
					fputs("Sounding\r\n", &USBSerialStream);
				} else if (inptr >= 5 && memcmp(inbuf, "color", 5) == 0) {
					int cols[6] = { 0, 0, 0, 0, 0, 0 };
					int cp = 0;
					for (int rp = 5; rp < inptr; rp++) {
						if (inbuf[rp] >= '0' && inbuf[rp] <= '9') {
							cols[cp] = cols[cp] * 10 + (inbuf[rp] - '0');
						}
						if (inbuf[rp] == ',') {
							cp++;
							if (cp >= 6) break;
						}
					}
					if (cp < 3) {
						cols[3] = cols[0];
						cols[4] = cols[1];
						cols[5] = cols[2];
					}
					for (cp = 0; cp < 6; cp++) {
						if (cols[cp] < 0) cols[cp] = 0;
						if (cols[cp] > 255) cols[cp] = 255;
					}
					send_pixels(cols);
					fprintf(&USBSerialStream, "Color(%d,%d,%d)\r\n", cols[0], cols[1], cols[2]);
				} else if (inptr >= 5 && memcmp(inbuf, "reset", 5) == 0) {
					*(uint16_t *)0x0800 = 0x7777;
					wdt_enable(WDTO_120MS);
					for (;;) { /* Wait for watchdog reset */ }
				} else {
					fputs("Error(", &USBSerialStream);
					fputs(inbuf, &USBSerialStream);
					fputs(")\r\n", &USBSerialStream);
				}
				inptr = 0;
			}
			if (c >= 0x20 && c <= 0x7f && inptr < (sizeof(inbuf)-1)) {
				inbuf[inptr++] = c;
			}
		}
		if (pzfreq > 16384) {
			if (!pzwait--) {
				pzwait = pzdelay;
				pzfreq--;
				OCR1A = (pzfreq >> 4);
			}
		}

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		HID_Device_USBTask(&Button_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Button_HID_Interface);
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

	USB_Device_EnableSOFEvents();

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
	HID_Device_ProcessControlRequest(&Button_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Button_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	USB_ButtonReport_Data_t* ButtonReport = (USB_ButtonReport_Data_t*)ReportData;

	if (!(PINB & 0x04)) {
		ButtonReport->Buttons |= 1;
	}

	*ReportSize = sizeof(USB_ButtonReport_Data_t);
	return true;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	// Unused (but mandatory for the HID class driver) in this demo, since there are no Host->Device reports
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
	/* You can get changes to the virtual CDC lines in this callback; a common
	   use-case is to use the Data Terminal Ready (DTR) flag to enable and
	   disable CDC communications in your application when set to avoid the
	   application blocking while waiting for a host to become ready and read
	   in the pending data from the USB endpoints.
	*/
	bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) != 0;

	(void)HostReady;
}
