PROG=temp-gather
DEVICE ?= /dev/ttyACM0

SRCS += temp-gather.c acquire.c blink.c conductivity.c
SRCS += temp-gather.desc
include ../../toolchain/mchck.mk

console : 
	picocom -b 115200 --imap lfcrlf --omap crlf --echo $(DEVICE)
