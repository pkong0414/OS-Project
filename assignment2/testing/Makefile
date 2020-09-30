#!/bin/bash
#this is the make file for project 3

CC = gcc
TARGET = master
SRC = main.c palim.c
OBJ = main.o palim.o

$TARGET: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

main.o             : main.c
palim.o            : palim.c

clean :
	/bin/rm '-f' *.o $(TARGET)