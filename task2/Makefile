CC=gcc
CFLAGS=-Wall -Wextra -O2

all: lock

lock: lock.c
	$(CC) $(CFLAGS) lock.c -o lock

clean:
	rm -f lock *.lck stat.txt