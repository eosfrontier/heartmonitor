DEVICE=attiny85

PADDLE_OBJECTS=paddle.o

CC=avr-gcc
CFLAGS=-mmcu=$(DEVICE) -DF_CPU=16500000 -Wall -Os
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

clean:
	rm -f *.o *.bin *.hex
