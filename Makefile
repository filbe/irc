CC = gcc
CFLAGS = -static -g -Wall

SRC = 

objects = server client
all: $(objects)

$(objects): %: $(SRC)%.c
	$(CC) $(CFLAGS) -o $@ $<
