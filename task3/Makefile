CC=gcc
CFLAGS=-Wall -Wextra

all: myinit testproc

myinit: myinit.c
	$(CC) $(CFLAGS) -o myinit myinit.c

testproc: testproc.c
	$(CC) $(CFLAGS) -o testproc testproc.c

clean:
	rm -f myinit testproc