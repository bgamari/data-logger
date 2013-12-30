PROG=temp-gather
DEVICE ?= /dev/ttyACM0

SRCS += temp-gather.c acquire.c blink.c conductivity.c usb_console.c
SRCS += sensor.c adc_sensor.c temperature.c thermistor.c
SRCS += config.c
SRCS += temp-gather.desc
include ../../toolchain/mchck.mk

LDFLAGS += -lm
CFLAGS+=-DSTDIO_OUTBUF_SIZE=64

console : 
	picocom -b 115200 --imap lfcrlf --omap crlf --echo $(DEVICE)
