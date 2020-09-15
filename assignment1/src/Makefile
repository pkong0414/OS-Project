#this is the make file for project 3

CC = gcc
CFLAGS = -Wall
TARGET = mydu
SRC = diskusage.c
OBJ = diskusage.o

$TARGET: $(OBJ)
	$(CC) $(CPPFLAGS) -o $(TARGET) $(OBJ)

diskusage.o        : diskusage.c

clean :
	/bin/rm '-f' *.o $(TARGET)
