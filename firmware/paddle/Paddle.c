#include "Paddle.h"

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
USB_ClassInfo_HID_Device_t Mouse_HID_Interface =
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

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	char inbuf[33];
	int inptr = 0;

	SetupHardware();

	DDRD |= 0x02;
	PORTD &= ~0x02;

	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	unsigned int pzfreq = 0;
	unsigned int pzwait = 1;
	unsigned int pzdelay = 50;
	TCCR1A = 0x40;
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
					PORTD |= 0x02;
					TCCR1B = 0x09;
					for (int i = 0; i < ms; i++) {
						OCR1A = 1000 + (rand() & 0x3fff);
						Delay_MS(1);
					}
					PORTD &= ~0x02;
					fputs("Flashed\r\n", &USBSerialStream);
					TCCR1B = 0x08;
					pzfreq = 0;
				} else if (inptr >= 5 && memcmp(inbuf, "sound", 5) == 0) {
					int ms = 0;
					for (int rp = 5; rp < inptr; rp++) {
						if (inbuf[rp] >= '0' && inbuf[rp] <= '9') {
							ms = ms * 10 + (inbuf[rp] - '0');
						}
					}
					if (ms <= 0) ms = 30;
					pzdelay = ms;
					TCCR1B = 0x09;
					pzfreq = 0xFFFF;
					fputs("Sounding\r\n", &USBSerialStream);
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
				OCR1A = (pzfreq >> 3);
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

	uint8_t ButtonStatus_LCL = (PINB >> 2) & 1;

	ButtonReport->Buttons = ButtonStatus_LCL;

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
