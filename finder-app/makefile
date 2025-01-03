#Author: Joseph Garwood
CC := $(CROSS_COMPILE)gcc

CFLAGS = -Werror -Wall

all:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) writer.c -o writer

clean:
	rm -f writer *.o