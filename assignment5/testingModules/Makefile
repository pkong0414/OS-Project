#!/bin/bash
#this is the make file for project 4

CC = gcc
CFLAGS = -pthread -I -g
TARGETS = bitvec messageQ messageR queue
BITVEC = bitvec
MESSAGEQ = messageQ
MESSAGER = messageR
QUEUE = queue
SRC = bitfields.c
OBJ = bitfields.o
SRC2 = messageQs.c
OBJ2 = messageQs.o
SRC3 = messageRs.c
OBJ3 = messageRs.o
SRC4 = queue.c
OBJ4 = queue.o

all: $(TARGETS)

bitvec:  $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

messageQ: $(OBJ2)
	$(CC) $(CFLAGS) -o $@ $(OBJ2)

messageR: $(OBJ3)
	$(CC) $(CFLAGS) -o $@ $(OBJ3)

queue: $(OBJ4)
	$(CC) $(CFLAGS) -o $@ $(OBJ4)

%@.o             : %@.c

clean :
	/bin/rm '-f' *.o $(TARGETS)