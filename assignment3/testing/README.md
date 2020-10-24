Name: Patrick Kong
Date: 10/09/2020
Assignment 4

This program will process a text file and detect if the strings inside the file are palindromes.

Usage: ./master -n: int -s:int -t: int <filename>

n:          dictates the max number of processes that will be created by the master.
s:          dictates the number of children that will be allowed to run by the program.
t:          time in seconds that the program will run before it terminates.
filename:   the textfile that contains strings.

Interrupts:
    
    -Timer: will signal an alarm causing the processes to terminate when the time limit expires.
    -User Interrupt: ctrl + c will signal an interrupt causing the program to stop.

File processed will populate files: palin.log and nopalin.log.

These files will contain respective palindromes and non- palindromes. 

ISSUES: