#!/bin/bash
#this is the make file for project 4

CC = gcc
CFLAGS = -pthread -I -g
TARGETS = bitvec messageQ messageR
BITVEC = bitvec
MESSAGEQ = messageQ
MESSAGER = messageR
SRC = bitfields.c
OBJ = bitfields.o
SRC2 = messageQs.c
OBJ2 = messageQs.o
SRC3 = messageRs.c
OBJ3 = messageRs.o

all: $(TARGETS)

bitvec:  $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

messageQ: $(OBJ2)
	$(CC) $(CFLAGS) -o $@ $(OBJ2)

messageR: $(OBJ3)
	$(CC) $(CFLAGS) -o $@ $(OBJ3)

%@.o             : %@.c

clean :
	/bin/rm '-f' *.o $(TARGETS)