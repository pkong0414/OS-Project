#!/bin/bash
#this is the make file for project 2

CC = gcc
CFLAGS = -pthread -I. -g
TARGETS = master palimCheck
MASTER = master
PALIN = palimCheck
SRC = main.c detachandremove.c sharedheap.h
OBJ = main.o detachandremove.o
SRC2 = palim.c
PALIN_OBJS = palim.o

all: $(TARGETS)
m: $(MASTER)
p: $(PALIN)

master:  $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

palimCheck: $(PALIN_OBJS)
	$(CC) -o $@ $(PALIN_OBJS)

%@.o             : %@.c

clean :
	/bin/rm '-f' *.o $(TARGETS)