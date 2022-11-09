# Non-Preemptive-Scheduling-C-
Implements 3 non-preemptive schedulers using multithreading and mutex locks to synchronize access to a shared FIFO data structure.

Compile with:
    g++ -o main.exe main.cpp

Run with these commands:

First Come First Serve Scheduling:
   ./main.exe FCFS FCFS.txt FCFS.out

Shortest Job First Scheduling:
   ./main.exe SJF SJF.txt SJF.out
  
Priority Scheduling:
   ./main.exe PRIORITY PRIORITY.txt PRIORITY.out
