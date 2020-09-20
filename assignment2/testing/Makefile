#!/bin/bash
#this is the make file for project 3

CC = gcc
TARGET = master
SRC = main.c palin.c
OBJ = palin.o

$TARGET: $(OBJ)
	$(CC) $(CPPFLAGS) -o $(TARGET) $(OBJ)

main.o             : main.c
palin.o            : palin.c

clean :
	/bin/rm '-f' *.o $(TARGET)