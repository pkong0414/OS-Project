#this is the make file for project 3

CC = gcc
CFLAGS = -Wall
TARGET = btTest
SRC = testDriver.c
OBJ = testDriver.o

$TARGET: $(OBJ)
	$(CC) $(CPPFLAGS) -o $(TARGET) $(OBJ)

testDriver.o        : testDriver.c

clean :
	/bin/rm '-f' *.o $(TARGET)
