CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
TARGET = pageload

all: $(TARGET)

$(TARGET): pageload.c
	$(CC) $(CFLAGS) -o $(TARGET) pageload.c

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
