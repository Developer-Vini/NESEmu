CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11 -Iinclude
SDL_CFLAGS ?= $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   ?= $(shell sdl2-config --libs 2>/dev/null)
LDFLAGS = $(SDL_LIBS) -lm

SRC     = src/palette.c \
          src/cartridge.c \
          src/cpu.c \
          src/ppu.c \
          src/bus.c \
          src/main.c

CORE_SRC = src/palette.c \
           src/cartridge.c \
           src/cpu.c \
           src/ppu.c \
           src/bus.c

TARGET  = nes_emulator
CPU_TEST = cpu_test

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ $(LDFLAGS)

$(CPU_TEST): $(CORE_SRC) cpu_test.c
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: $(CPU_TEST)
	./$(CPU_TEST)

clean:
	rm -f $(TARGET) $(TARGET).exe $(CPU_TEST) $(CPU_TEST).exe *.o
