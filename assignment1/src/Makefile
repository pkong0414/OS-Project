#this is the make file for project 3

CC = gcc
CFLAGS = -Wall
TARGET = mydu
SRC = main.c
OBJ = main.o

$TARGET: $(OBJ)
	$(CC) $(CPPFLAGS) -o $(TARGET) $(OBJ)

main.o        : main.c

clean :
	/bin/rm '-f' *.o $(TARGET)
