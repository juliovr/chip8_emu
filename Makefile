CC = gcc
CFLAGS = -Og -g

all: chip8

clean:
	rm bin/chip8

chip8: src/chip8.cpp
	$(CC) $(CFLAGS) -o bin/chip8 src/chip8.cpp
