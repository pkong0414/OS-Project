Name: Patrick Kong
Date: 11/13/2020
Assignment 5

This program will spawn a bunch of processes which will simulate a resource manager.
The resource manager will apply Banker's algorithm in order to prevent deadlock.

It will consist of 2 parts:
oss:
    This is the main scheduler
    It will enqueue processes that request for resources.
        -Processes that are approved will receive resources from shared memory and resume processing.
        -Processes that are not approved will move to the back of the line in queue.
        -oss will only make one rotation around the queue before it resumes in waiting for resource requests,
            resource release as well as termination requests. This will cause oss to run Banker's Algo,
            since one is asking for resources, release or termination means more resources to grant 
            resource requests.
    
processes:
    These processes will roll to do 3 things:
    -terminate
    -request resources:
        requests that are denied will put the process in wait until appropriate resources can be allocated.
    -release resources

Usage: ./oss [-h][-v]

    -h: this will cause the program to print help
    -v: this will cause the program to be in verbose mode.

Interrupts:
    
    -Timer: will signal an alarm causing the processes to terminate when the time limit expires. ( 3 seconds )
    -User Interrupt: will signal an interrupt causing the program to stop. ( ctrl + c )

ISSUES:
    