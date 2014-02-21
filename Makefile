PROG=data-logger
DEVICE ?= /dev/ttyACM0

SENSOR_SRCS += conductivity.c core_temp.c lm19.c thermistor.c nmea.c bmp085.c
SENSOR_SRCS += bmp085_sensor.c adc_sensor.c

SRCS += main.c acquire.c blink.c usb_console.c nv_config.c
SRCS += sensor.c sample_store.c
SRCS += radio.c flash_list.c power.c spiflash_utils.c
SRCS += $(addprefix sensors/,$(SENSOR_SRCS))
SRCS += config.c version.c
SRCS += data-logger.desc
include ../../toolchain/mchck.mk

LDFLAGS += -lm
CWARNFLAGS += -Werror -Wno-format -Wstack-usage=64
CFLAGS += -DCOMMIT_ID=\"$(shell git rev-parse HEAD)\"
PRINTF_WITH = FIXPOINT

console : 
	picocom -b 115200 --imap lfcrlf --omap crlf --echo $(DEVICE)

# trick to always rebuild taken from Linux build system
.PHONY : FORCE
FORCE :

version.c : FORCE
