# Project Description
CS4348 - Operating Systems w/ Prof. Greg Ozbirn  

Advay Chandramouli

## Language/Platform

The project must be written in C, C++, or Java.  
If using C or C++, you must use a Unix fork to create processes and a Unix pipe for communication.  
If using Java, you must use the Runtime exec method to create processes and streams for communication.  
Your project will receive no credit if not using processes or if using threads instead of processes.  
All code must run successfully on our `cs1.utdallas.edu` server or `csgrads1.utdallas.edu` server.  
Any other method requires instructor approval.

## Problem Overview

The project will simulate a simple computer system consisting of a CPU and Memory.  
The CPU and Memory will be simulated by separate processes that communicate.  
Memory will contain one program that the CPU will execute and then the simulation will end.

## Objectives

1. Learn how multiple processes can communicate and cooperate.
2. Understand low-level concepts important to an operating system.
    a. Processor interaction with main memory.
    b. Processor instruction behavior.
    c. Role of registers.
    d. Stack processing.
    e. Procedure calls.
    f. System calls.
    g. Interrupt handling.
    h. Memory protection.
    i. I/O.
    j. Virtualization/emulation

## Problem Details

### CPU
- It will have these registers: PC, SP, IR, AC, X, Y.
- It will support the instructions shown on the next page of this document.
- It will run the user program at address 0.
- Instructions are fetched into the IR from memory. The operand can be fetched into a local variable.
- Each instruction should be executed before the next instruction is fetched.
- The user stack resides at the end of user memory and grows down toward address 0.
- The system stack resides at the end of system memory and grows down toward address 0.
- There is no hardware enforcement of stack size.
- The program ends when the End instruction is executed. The 2 processes should end at that time.
- The user program cannot access system memory (exits with error message).

### Memory
- It will consist of 2000 integer entries, 0-999 for the user program, 1000-1999 for system code.
- It will support two operations:
    - `read(address)` - returns the value at the address
    - `write(address, data)` - writes the data to the address
- Memory will read an input file containing a program into its array, before any CPU fetching begins.
- Note that the memory is simply storage; it has no real logic beyond reading and writing.

### Timer
- A timer will interrupt the processor after every X instructions, where X is a command-line parameter.
- The timer is always counting, whether in user mode or kernel mode.

### Interrupt processing
- There are two forms of interrupts: the timer and a system call using the `int` instruction.
- In both cases, the CPU should enter kernel mode.
- The stack pointer should be switched to the system stack.
- The SP and PC registers (and only these registers) should be saved on the system stack by the CPU.
- The handler may save additional registers.
- A timer interrupt should cause execution at address 1000.
- The `int` instruction should cause execution at address 1500.
- The `iret` instruction returns from an interrupt.
- Interrupts should be disabled during interrupt processing to avoid nested execution.
- To make it easy, do not allow interrupts during system calls or vice versa.

### Instruction Set
| Opcode | Description                                      |
|--------|--------------------------------------------------|
| 1      | Load the value into the AC                       |
| 2      | Load the value at the address into the AC        |
| 3      | Load the value from the address found in the given address into the AC (for example, if LoadInd 500, and 500 contains 100, then load from 100) |
| 4      | Load the value at (address+X) into the AC (for example, if LoadIdxX 500, and X contains 10, then load from 510) |
| 5      | Load the value at (address+Y) into the AC        |
| 6      | Load from (Sp+X) into the AC (if SP is 990, and X is 1, load from 991) |
| 7      | Store the value in the AC into the address       |
| 8      | Gets a random int from 1 to 100 into the AC     |
| 9      | If port=1, writes AC as an int to the screen; If port=2, writes AC as a char to the screen |
| 10     | Add the value in X to the AC                     |
| 11     | Add the value in Y to the AC                     |
| 12     | Subtract the value in X from the AC              |
| 13     | Subtract the value in Y from the AC              |
| 14     | Copy the value in the AC to X                    |
| 15     | Copy the value in X to the AC                    |
| 16     | Copy the value in the AC to Y                    |
| 17     | Copy the value in Y to the AC                    |
| 18     | Copy the value in AC to the SP                   |
| 19     | Copy the value in SP to the AC                   |
| 20     | Jump to the address                              |
| 21     | Jump to the address only if the value in the AC is zero |
| 22     | Jump to the address only if the value in the AC is not zero |
| 23     | Push return address onto stack, jump to the address |
| 24     | Pop return address from the stack, jump to the address |
| 25     | Increment the value in X                         |
| 26     | Decrement the value in X                         |
| 27     | Push AC onto stack                               |
| 28     | Pop from stack into AC                           |
| 29     | Perform system call                              |
| 30     | Return from system call                          |
| 50     | End execution                                    |

### Running this Project:
compile:    gcc project1.c -o project1  
command line format: [project1] [sample_filename.txt] [timer]  
run:        project1 sample1.txt 30   
