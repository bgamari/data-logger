PROG=data-logger
DEVICE ?= /dev/ttyACM0

SRCS += main.c acquire.c blink.c conductivity.c usb_console.c
SRCS += sensor.c adc_sensor.c temperature.c thermistor.c sample_store.c
SRCS += config.c
SRCS += data-logger.desc
include ../../toolchain/mchck.mk

LDFLAGS += -lm
CWARNFLAGS += -Werror -Wno-format

console : 
	picocom -b 115200 --imap lfcrlf --omap crlf --echo $(DEVICE)
