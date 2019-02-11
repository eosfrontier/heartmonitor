DEVICE=attiny85
F_CPU=16500000

USBDRV_OBJECTS=usbdrv.o usbdrvasm.o oddebug.o osccal.o
OBJECTS=main.o usi_i2c_master.o $(USBDRV_OBJECTS)

CC=avr-gcc
CFLAGS=-mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -Wall -Os -I. -I../usbdrv -DDEBUG_LEVEL=0
LDFLAGS=-mmcu=$(DEVICE)

all: main.hex

%.o: %.c usbconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: ../usbdrv/%.c usbconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: ../usbdrv/%.S usbconfig.h
	$(CC) -x assembler-with-cpp $(CFLAGS) -c $< -o $@

# file targets:
main.bin: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

install: main.hex
	micronucleus --run $<

%.hex: %.bin
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size $@

clean:
	rm -f *.o *.bin *.hex
