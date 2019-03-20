MCU=atmega32u4
ARCH=AVR8
BOARD=LEONARDO
F_CPU=16000000
F_USB=$(F_CPU)
OPTIMIZATION=s
TARGET=Paddle
SRC=$(TARGET).c Descriptors.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH=../lufa/LUFA
CC_FLAGS=-DUSE_LUFA_CONFIG_HEADER
LD_FLAGS=

AVRDUDE_PROGRAMMER=avr109
AVRDUDE_PORT=/dev/ttyACM0
AVRDUDE_FLAGS=-b57600

all:

DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

DMBS_PATH ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk
