CC = gcc
CFLAGS = -Wall
all: pmacierzysty.c process1.c process2.c process2.c
	$(CC) $(CFLAGS) pmacierzysty.c -o pmacierzysty
	$(CC) $(CFLAGS) process1.c -o process1
	$(CC) $(CFLAGS) process2.c -o process2
	$(CC) $(CFLAGS) process3.c -o process3

clean:
	rm -f pmacierzysty process1 process2