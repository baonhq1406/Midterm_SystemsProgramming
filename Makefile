CC       ?= cc
CPPFLAGS ?= -D_DEFAULT_SOURCE -Iinclude
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2
LDFLAGS  ?=

TARGET := myls
SOURCES := src/main.c src/options.c src/util.c src/format.c src/listing.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

src/%.o: src/%.c include/ls.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	sh tests/smoke_test.sh
