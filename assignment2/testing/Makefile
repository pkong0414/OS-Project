#!/bin/bash
#this is the make file for project 3

CC = gcc
TARGET = master
SRC = main.c palim.c detachandremove.c
OBJ = main.o palim.o detachandremove.o

$TARGET: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

main.o             : main.c
palim.o            : palim.c
detachandremove.o  : detachandremove.c

clean :
	/bin/rm '-f' *.o $(TARGET)