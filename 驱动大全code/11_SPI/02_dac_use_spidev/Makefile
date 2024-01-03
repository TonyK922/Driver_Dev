objects := $(patsubst %.c,%.o,$(wildcard *.c))
TARGET = dac_app
CC = gcc
CFLAGS = -Wall -O2

.PHONY:all clean

all: $(TARGET)

$(TARGET) : $(objects)
	$(CROSS_COMPILE)$(CC) -o $@ $^

%.o:%.c
	$(CROSS_COMPILE)$(CC) -c $^

clean :
	-rm $(TARGET) $(objects)