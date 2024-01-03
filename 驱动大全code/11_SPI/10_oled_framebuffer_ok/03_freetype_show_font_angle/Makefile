objects := $(patsubst %.c,%.o,$(wildcard *.c))
TARGET ?= freetype_show_font_angle
CC := gcc
CFLAGS := -Wall -O2 -I /home/book/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/include/freetype2 \
-lm -lfreetype

.PHONY:all clean

all: $(TARGET)

$(TARGET) : $(objects)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $^

%.o:%.c 
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $^

clean :
	-rm $(TARGET) $(objects)
