CC=gcc
CFLAGS=-Wall -g -Iinclude 
LDFLAGS=-lcrypto -lm 
OBJS=bencode.o util.o
VPATH=src:include

all: bencode

bencode: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

bencode.o: bencode.c bencode.h util.h
	$(CC) $(CFLAGS) -c src/bencode.c

util.o: util.c bencode.h
	$(CC) $(CFLAGS) -c src/util.c

clean:
	rm -f *.o bencode
