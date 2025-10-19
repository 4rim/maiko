CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -g -ggdb

maiko: maiko.o
	$(CC) $(CFLAGS) $< -o $@

maiko.o: maiko.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o maiko
