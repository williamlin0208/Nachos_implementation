# Nachos Implementation

## MP1 (implement system call)

Implement SC_Halt, SC_Create, SC_Open, SC_Print

## MP2 (Pagination)

We want to load the address at excuting time, so we need to implement logiacal address(pages) to each process, and provide available physical address(frames) corresponds to them.

## MP3

Scheduling the process.  
We need to implement Queues with 3 levels and aging machanism. 

- L1: preemptive SJF (shortest job first) algorithm  

    - new process has pproximated burst time:  
    - $t_i = 0.5 * T + 0.5 * t_{i-1}$  

- L2: preemptive SJF (shortest job first) algorithm  

- L3: round-robin scheduling algorithm with time-quantum 100 ticks  

Aging mechanism: Priority of a process is increased by 10 after waiting for more than 1500 ticks.

## MP4

- Implement file control block (FCB) to record informations about files and we used linked list to link all the blocks together.
- Construct a directory to store all the files.