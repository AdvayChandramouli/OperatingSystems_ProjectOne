/*
 * Advay Chandramouli
 * CS4348.501 - Prof. Greg Ozbirn
 * Project One: Exploring Multiple Processes and IPC
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

#define MEMBLOCK 2000
#define SUCCESS 1
#define FAILURE 0
#define READ 0
#define WRITE 1
#define EXITPROCESS -1

// Function to initialize memory block by reading instructions from a file.
// Parameters:
//   - mem_ptr: Pointer to the memory block to be initialized
//   - file_name: Name of the file containing memory initialization instructions
// Reads instructions from the file and initializes the memory block accordingly.
// Each line in the file is expected to contain either a memory address offset or a value to be stored in memory.
// Memory addresses are indicated by lines starting with '.', followed by the address offset.
// Values to be stored in memory are expected in subsequent lines, which are parsed and stored at the corresponding memory addresses.
// If the file cannot be opened, an error message is displayed, and the program exits with an error code.
void init_memory(int* mem_ptr, char* file_name)
{
    FILE *input_file = fopen(file_name, "r");
    char line[100];
    int value;
    int address = 0;

    if (input_file == NULL)
    {
        printf("Error! Could not open file\n");
        exit(-1);
    }

    while (fgets(line, sizeof(line), input_file) != NULL) {
        if (line[0] == '.')
        {
            // Extract memory address offset
            line[0] = ' ';
            sscanf(line, "%d", &address);
        }
        else
        {
            // Parse value from the line and store it in memory
            if(sscanf(line, "%d", &value) == 1) //Skips empty line when parsing...
            {
                mem_ptr[address] = value;
                address++;
            }
        }
    }
    fclose(input_file); // Close the file after reading
}


// Function either reads from or writes to memory, based on parameters provided.
// Communication between CPU and memory processes is handled via pipes.
// Parameters:
//   - type: Type of operation (READ or WRITE)
//   - address: Memory address for operation
//   - value: Value to write to memory (if performing a write operation)
//   - pd1: Array representing the pipe for communication from CPU to memory
//   - pd2: Array representing the pipe for communication from memory to CPU
// Returns:
//   - If type is READ, returns the value read from memory
//   - If type is WRITE, writes the value to the specified memory address
int read_write_from_memory(int type, int address, int value, int pd1[], int pd2[])
{
    int res;
    int params[3]; // params[2] used if writing value back to memory
    if (type == READ)
    {
        // Prepare parameters for reading from memory
        params[0] = READ;
        params[1] = address;
        // Write parameters to pipe for memory process
        write(pd1[1], params, sizeof(params));
        // Read result from memory via pipe
        read(pd2[0], &res, sizeof(int));
        return res; // Return value read from memory
    }
    else if (type == WRITE)
    {
        // Prepare parameters for writing to memory
        params[0] = WRITE;
        params[1] = address;
        params[2] = value;
        // Write parameters to pipe for memory process
        write(pd1[1], params, sizeof(params));
    }
}

// Function to check memory access permissions based on the provided memory address and stack pointer.
// Parameters:
//   - addr: Memory address being accessed
//   - stack_ptr: Stack pointer value indicating the current mode (user or system)
// Checks whether the memory access is valid based on the current mode (user or system).
// If the stack pointer is less than 1000, it indicates user mode.
// In user mode, accessing memory addresses 1000 and beyond is considered a violation, as they belong to the system.
// If a violation is detected, an error message is printed indicating the attempt to access a system address in user mode.
void check_mem_access(int addr, int stack_ptr)
{
    if(stack_ptr < 1000) // User Mode if true
    {
        if(addr >= 1000) // User mode can't access address 1000 & beyond
        {
            printf("Memory violation: accessing system address %d in user mode.\n", addr);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Invalid number of arguments! ");
        exit(-1);
    }
    int pd1[2]; //parent --> child pipe
    int pd2[2]; //child --> parent ipc

    if (pipe(pd1) == -1) {
        printf("Pipe failed!");
    }
    if (pipe(pd2) == -1) {
        printf("Pipe failed!");
    }
    int timer;
    sscanf(argv[2], "%d", &timer);

    //Fork to create CPU and Memory Processes
    int id = fork();
    if (id == -1) {
        printf("Fork Failed!");
        return 0;
    }
    if (id == 0) //Memory Process (CHILD)
    {
        int *memory;
        int val = SUCCESS;
        //Allocate 2000 spaces for integer instructions to be read from sample files
        memory = malloc(MEMBLOCK * sizeof(int));

        if (memory == NULL) {
            printf("Cannot allocate memory space.");
        }
        init_memory(memory, argv[1]);

        //Communicate to CPU that memory is initialized, via a pipe
        write(pd2[1], &val, sizeof(int));
        int values[3];
        int address;
        int result;

        while (1) {
            read(pd1[0], values, sizeof(values)); //Check if any read/write operation
            if (values[0] == READ) {
                address = values[1];
                result = memory[address];
                write(pd2[1], &result, sizeof(int));
            } else if (values[0] == WRITE) {
                address = values[1];
                memory[address] = values[2];
            } else if (values[0] == EXITPROCESS) {
                break;
            }
        }
    }

    //CPU Process (PARENT)
    else {
        int result;
        read(pd2[0], &result, sizeof(int)); //Check whether memory initialization completed.

        // Initialize registers and variables
        int PC = 0;
        int USP = 999; //User Stack Pointer (0-999)
        int SSP = 1999; //System Stack Pointer (starts at 1999)
        int *SP ; //Stack Pointer which can be switched to point to user or system stack
        int IR;
        int AC, X, Y, temp;
        int return_address;
        int val;
        int isr = 0; //interrupt service routine
        int counter = 0;
        int temp_sp;
        SP = &USP ; //Stack pointer defaults to user stack pointer (user mode)

        //Fetches instructions till exit instruction (50)
        while (1) {
            //Decrement timer if greater than zero
            if (timer > 0) {
                timer--;
            }
            if ((timer == 0) && (isr == 0)) //check timer 0 && disable nested interrupts
            {
                isr = 1;
                SP = &SSP;
                (*SP)--;
                read_write_from_memory(WRITE, (*SP), PC, pd1, pd2);
                (*SP)--;
                read_write_from_memory(WRITE, *SP, USP, pd1, pd2); //store user stack pointer
                PC = 1000;
            }
            //Fetch instruction into IR
            IR = read_write_from_memory(READ, PC, 0, pd1, pd2);
            PC++; //increment program counter to address of next instruction
            //Decode and Execute instruction
            switch (IR) {
                case 1: //Read value from memory into AC register
                    AC = read_write_from_memory(READ, PC, 0, pd1, pd2);//Load value into the AC
                    PC++;
                    break;
                case 2: //Read value at provided address into AC
                    temp = read_write_from_memory(READ, PC, 0, pd1, pd2); //Read value at the address
                    check_mem_access(temp, *SP); //Check to see if still within bounds of user mode
                    AC = read_write_from_memory(READ, temp, 0, pd1, pd2);
                    PC++;
                    break;
                case 3: //Read value pointed to, into AC
                    temp = read_write_from_memory(READ, PC, 0, pd1, pd2); //Read value at the address
                    int temp2 = read_write_from_memory(READ, temp, 0, pd1, pd2);
                    AC = read_write_from_memory(READ, temp2, 0, pd1, pd2);
                    PC++;
                    break;
                case 4: //Load value at (address+X) into AC
                    temp = read_write_from_memory(READ, PC, 0, pd1, pd2); //argument passed at instruction
                    AC = read_write_from_memory(READ, (temp + X), 0, pd1, pd2);
                    PC++;
                    break;
                case 5: //Load the value at (address+Y) into the AC
                    temp = read_write_from_memory(READ, PC, 0, pd1, pd2); //argument passed at instruction
                    AC = read_write_from_memory(READ, (temp + Y), 0, pd1, pd2);
                    PC++;
                    break;
                case 6:
                    if ((*SP + X) < 999) //If address is within user memory region
                    {
                        AC = read_write_from_memory(READ, (*SP + X), 0, pd1, pd2);
                    } else //Else exit with error code
                    {
                        printf("Error: %d %d outside user memory space.\n", *SP, X);
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 7: //Store value in AC into address parameter
                    temp = read_write_from_memory(READ, PC, 0, pd1, pd2); //Read address
                    read_write_from_memory(WRITE, temp, AC, pd1, pd2);
                    PC++;
                    break;
                case 8: //Generate a random number from 1-100 into AC
                    // Seed the random number generator
                    srand(time(NULL));
                    // Generate a random number between 1 and 100
                    AC = rand() % 100 + 1;
                    PC++;
                    break;
                case 9: //Print value in AC to screen
                    temp = read_write_from_memory(READ, PC, 0, pd1, pd2);
                    //if argument equals one: print as a number
                    if (temp == 1) {
                        printf("%d", AC);
                    }
                    //if argument equals two: print as a character
                    if (temp == 2) {
                        printf("%c", AC);
                    }
                    PC++;
                    break;
                case 10: //Add X to AC and store in AC
                    AC += X;
                    break;
                case 11: //Add Y to AC and store in AC
                    AC += Y;
                    break;
                case 12: //Subtract X from AC
                    AC -= X;
                    break;
                case 13: //Subtract Y from AC
                    AC -= Y;
                    break;
                case 14: //Copy value in AC to X
                    X = AC;
                    break;
                case 15: //Copy value in X to AC
                    AC = X;
                    break;
                case 16: //Copy value in AC to Y
                    Y = AC;
                    break;
                case 17: //Copy value in Y to AC
                    AC = Y;
                    break;
                case 18: //Copy value in AC to SP
                    *SP = AC;
                    printf("18 SP = AC %d\n", *SP);
                    break;
                case 19: //Copy value in SP to AC
                    AC = *SP;
                    break;
                case 20: //Jump to address
                    PC = read_write_from_memory(READ, PC, 0, pd1, pd2);
                    break;
                case 21: //JEQ if Zero
                    if (AC == 0) {
                        PC = read_write_from_memory(READ, PC, 0, pd1, pd2);
                    } else {
                        PC++;
                    }
                    break;
                case 22: //JNE to Zero
                    if (AC != 0) {
                        PC = read_write_from_memory(READ, PC, 0, pd1, pd2);
                    } else {
                        PC++;
                    }
                    break;
                case 23: //
                    temp = read_write_from_memory(READ, PC, 0, pd1, pd2);
                    return_address = PC + 1;
                    (*SP)--;
                    read_write_from_memory(WRITE, *SP, return_address, pd1, pd2);
                    PC = temp;
                    break;
                case 24: //Pop return address from stack & jump
                    return_address = read_write_from_memory(READ, *SP, 0, pd1, pd2);
                    (*SP)++;
                    PC = return_address;
                    break;
                case 25: //Increment value in X
                    X++;
                    break;
                case 26: //Decrement value in X
                    X--;
                    break;
                case 27: //Push value from stack
                    (*SP)--;
                    read_write_from_memory(WRITE, *SP, AC, pd1, pd2);
                    break;
                case 28: //Pop value from stack
                    AC = read_write_from_memory(READ, *SP, 0, pd1, pd2);
                    (*SP)++;
                    break;
                case 29: //Decrement & Write Approach to system call
                    isr = 1; //Switch to interrupt service routine (kernel mode)
                    SP = &SSP; //switch to point to system stack pointer, once we enter interrupt subroutine
                    return_address = PC;
                    (*SP)--; //Decrement stack pointer
                    read_write_from_memory(WRITE, *SP, return_address, pd1, pd2);
                    (*SP)--; //Decrement stack pointer
                    read_write_from_memory(WRITE, *SP, USP, pd1, pd2);
                    PC = 1500;
                    break;
                case 30: //Return from syscall
                    isr = 0;
                    temp_sp = read_write_from_memory(READ, *SP, 0, pd1, pd2);
                    (*SP)++;
                    return_address = read_write_from_memory(READ, *SP, 0, pd1, pd2);
                    (*SP)++;
                    PC = return_address;
                    SP = &USP;
                    break;
                case 50: //Exit process & synchronize termination
                    val = EXITPROCESS;
                    write(pd1[1], &val, sizeof(val));
                    wait(NULL);
                    exit(EXIT_SUCCESS);
            }
            if (timer == 0) //Reset timer when decrements to zero
            {
                sscanf(argv[2], "%d", &timer);
            }
        }
    }
    return 0;
}