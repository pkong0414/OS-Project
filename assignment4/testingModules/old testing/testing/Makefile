#!/bin/bash
#this is the make file for project 4

CC = gcc
CFLAGS = -pthread -I -g
TARGETS = oss user
OSS = oss
USER = user
SRC = oss.c detachandremove.c queue.c
OBJ = oss.o detachandremove.o queue.o
SRC2 = user.c detachandremove.c
USER_OBJ = user.o detachandremove.o

all: $(TARGETS)
o:   $(OSS)
u:   $(USER)

oss:  $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

user: $(USER_OBJ)
	$(CC) $(CFLAGS) -o $@ $(USER_OBJ)

%@.o             : %@.c

clean :
	/bin/rm '-f' *.o $(TARGETS)