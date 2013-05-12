# Makefile for wavsilence
# 2003 - Dan Smith (dsmith@danplanet.com)

all: wavinfo wavsilence

wavheader.o: wavheader.c wavheader.h
	gcc -c -o wavheader.o wavheader.c

wavinfo: wavheader.o wavinfo.c
	gcc -o wavinfo wavinfo.c wavheader.o

wavsilence: wavsilence.c wavheader.o wavsilence.h
	gcc wavsilence.c wavheader.o -o wavsilence -g

clean: 
	rm -f *.o *~ wavinfo wavsilence
