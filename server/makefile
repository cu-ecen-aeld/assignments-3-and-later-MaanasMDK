# Makefile for Assignment 5
CC = gcc
CROSS_COMPILE =
CFLAGS := -g -Wall -Werror -pthread
LDFLAGS := -pthread
SRC := aesdsocket.c
TARGET = aesdsocket

all: $(TARGET)

$(TARGET) : $(SRC)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map