DEVICE=attiny85
F_CPU=16500000

USBDRV_OBJECTS=v-usb/usbdrv/usbdrv.o v-usb/usbdrv/usbdrvasm.o v-usb/usbdrv/oddebug.o
PADDLE_OBJECTS=paddle.o $(USBDRV_OBJECTS)

CC=avr-gcc
CFLAGS=-mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -Wall -Os -Iv-usb/usbdrv -I. -DDEBUG_LEVEL=0
LDFLAGS=-mmcu=$(DEVICE)

all: paddle.hex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) -x assembler-with-cpp $(CFLAGS) -c $< -o $@

# file targets:
paddle.bin: $(PADDLE_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install-paddle: paddle.hex
	micronucleus $<

%.hex: %.bin
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size $@

clean:
	rm -f *.o *.bin *.hex
