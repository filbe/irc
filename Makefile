CC = gcc

CFLAGS = -static -g -Wall

SRC = 
=======
CFLAGS = -Wall

SRC = src/

objects = server client
all: $(objects)

$(objects): %: $(SRC)%.c
	$(CC) $(CFLAGS) -o $@ $<
