#!/bin/bash
#this is the make file for project 3

CC = gcc
TARGETS = master palimCheck
MASTER = master
PALIN = palimCheck
SRC = main.c detachandremove.c
OBJ = main.o detachandremove.o
SRC2 = palim.c
OBJ2 = palim.o

all: $(TARGETS)

$(TARGET):$(OBJ)    $(CC) -o $@ $(OBJ)

$(TARGET2):$(OBJ2)  $(CC) -o $@ $(OBJ2)

main.o             : main.c
palim.o            : palim.c
detachandremove.o  : detachandremove.c

clean :
	/bin/rm '-f' *.o $(TARGET) $(TARGET2)