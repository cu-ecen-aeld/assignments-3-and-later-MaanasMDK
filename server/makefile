# Makefile for Assignment 5
CROSS_COMPILE =
ifeq ($(CC),)
	CC = $(CROSS_COMPILE)gcc
endif
ifeq ($(CFLAGS),)
	CFLAGS = -g -Wall -Werror -pthread
endif
ifeq ($(LDFLAGS),)
	LDFLAGS = -pthread -lrt
endif

SRC := aesdsocket.c
TARGET ?= aesdsocket

all: $(TARGET)

$(TARGET) : $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
