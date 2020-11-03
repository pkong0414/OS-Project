Name: Patrick Kong
Date: 10/23/2020
Assignment 4

This program will spawn a bunch of processes which will queue itself until it finishes processing

processes consist of real time and user processes.

real time processes will receive maximum base quantum or 10 quantums (10ms).
user processes will receive quantum based on its priority status ( 10/ (2^priority) ).
    priority 1: 5 quantum or 5ms
    priority 2: 2.5 quantums or 2.5ms
    priority 3: 1.25 quantums or 1.25ms
    
processes will have a limited time in queue before it drops in priority.
    queue 1: 2 times
    queue 2: 3 times:
    queue 3: infinite

Usage: ./oss

Interrupts:
    
    -Timer: will signal an alarm causing the processes to terminate when the time limit expires. ( 3 seconds )
    -User Interrupt: will signal an interrupt causing the program to stop. ( ctrl + c )

ISSUES:
    Got queue and exec along with initial premise of message queue working. There's still a lot to go.