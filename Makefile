PROG=data-logger
DEVICE ?= /dev/ttyACM0

SRCS += main.c acquire.c blink.c conductivity.c usb_console.c nv_config.c
SRCS += sensor.c adc_sensor.c temperature.c thermistor.c sample_store.c
SRCS += radio.c
SRCS += config.c version.c
SRCS += data-logger.desc
include ../../toolchain/mchck.mk

LDFLAGS += -lm
CWARNFLAGS += -Werror -Wno-format
CFLAGS += -DCOMMIT_ID=\"$(shell git rev-parse HEAD)\"

console : 
	picocom -b 115200 --imap lfcrlf --omap crlf --echo $(DEVICE)

# trick to always rebuild taken from Linux build system
.PHONY : FORCE
FORCE :

version.c : FORCE
