# You can compile with either gcc or g++
CC = g++
# CC = gcc
CFLAGS = -I. -Wall -lm -ldl -no-pie
# disable the -DNDEBUG flag for the printing the freelist
OPTFLAG = -O2

all: deli

deli: deli.cc
	$(CC) -o deli thread.o deli.cc libinterrupt.a $(CFLAGS)

clean:
	rm -f deli


