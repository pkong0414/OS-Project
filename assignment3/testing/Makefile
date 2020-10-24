#!/bin/bash
#this is the make file for project 2

CC = gcc
CFLAGS = -pthread -I -g
TARGETS = master palimCheck
MASTER = master
PALIN = palimCheck
SRC = main.c detachandremove.c semstuff.c sharedheap.h
OBJ = main.o detachandremove.o semstuff.o
SRC2 = palim.c semstuff.c
PALIN_OBJS = palim.o semstuff.o

all: $(TARGETS)
m: $(MASTER)
p: $(PALIN)

master:  $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

palimCheck: $(PALIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PALIN_OBJS)

%@.o             : %@.c

clean :
	/bin/rm '-f' *.o $(TARGETS)