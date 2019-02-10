DEVICE=attiny85
F_CPU=16500000

USBDRV_OBJECTS=usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o usbdrv/osccal.o
PADDLE_OBJECTS=paddle.o $(USBDRV_OBJECTS)

CC=avr-gcc
CFLAGS=-mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -Wall -Os -I. -Iusbdrv -DDEBUG_LEVEL=0
LDFLAGS=-mmcu=$(DEVICE)

all: paddle.hex

%.o: %.c usbconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) -x assembler-with-cpp $(CFLAGS) -c $< -o $@

# file targets:
paddle.bin: $(PADDLE_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

install-paddle: paddle.hex
	micronucleus --run $<

%.hex: %.bin
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size $@

clean:
	rm -f *.o *.bin *.hex usbdrv/*.o
