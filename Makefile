# Makefile for wavsilence
# 2003 - Dan Smith (dsmith@danplanet.com)

CC=gcc
CFLAGS=-Wall -Werror-implicit-function-declaration

all: wavinfo wavsilence

wavheader.o: wavheader.c wavheader.h
	$(CC) $(CFLAGS) -c -o wavheader.o wavheader.c

wavinfo: wavheader.o wavinfo.c
	$(CC) $(CFLAGS) -o wavinfo wavinfo.c wavheader.o

wavsilence: wavsilence.c wavheader.o wavsilence.h
	$(CC) $(CFLAGS) wavsilence.c wavheader.o -o wavsilence

clean:
	rm -f *.o *~ wavinfo wavsilence
