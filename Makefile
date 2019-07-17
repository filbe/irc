CC = gcc

CFLAGS = -static -g -Wall  -pthread

SRC = src/

objects = server client
all: $(objects)

$(objects): %: $(SRC)%.c
	$(CC) $(CFLAGS) -o $@ $<
