// Shell functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "shell.h"
#include "CortexM4Registers.h"
#include "kernel.h"

// REQUIRED: Add header files here for your strings functions, ...
#include "shell_auxiliary.h"
#include "shell_commands.h"


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    USER_DATA data;
    data.fieldCount = 0;

    // Clear the screen and move the cursor to the top left
    putsUart0("\033[2J\033[H");

    while (true)
    {
        if (kbhitUart0())
        {
            bool foo = 0;

            // Get the string from the user
            getsUart0(&data);

            // Echo back to the user of the TTY interface for testing
#ifdef DEBUG
            putsUart0("Output:\n");
            putsUart0(data.buffer);
            putcUart0('\n');
#endif

            // Parse fields
            parseFields(&data);

#ifdef DEBUG
            uint8_t i = 0;
            for (i = 0; i < data.fieldCount; i++)
            {
                putsUart0("Field ");
                putcUart0(i + 48);
                putsUart0(" :");
                putcUart0(data.fieldType[i]);
                putcUart0('\t');
                putsUart0(&data.buffer[data.fieldPosition[i]]);
                putsUart0("\n\n");
            }
#endif

            if (isCommand(&data, "reboot", 0))
            {
                reboot();
                foo = true;
            }
            else if (isCommand(&data, "ps", 0))
            {
                uint32_t pidsArray[MAX_TASKS] = {0};
                char namesOfTasks[MAX_TASKS][10] = {0};
                uint32_t statesArray[MAX_TASKS] = {0};
                uint8_t mutex_semaphore_array[MAX_TASKS] = {0};

                ps(pidsArray, namesOfTasks, statesArray, mutex_semaphore_array);

                uint8_t i = 0;
                putsUart0("\nPID\t\tName\t\tCPU%\tState\t\tMutex/Semaphore\n");
                putsUart0("------------------------------------------------------------------------------\n\n");
                for (i = 0; i < MAX_TASKS; i++)
                {
                    if (pidsArray[i])
                    {
                        uint8_t j = 0;

                        char str[20] = {0};

                        // Printing out the PID
                        putsUart0("0x");
                        itoa(pidsArray[i], str, 16);
                        putsUart0(str);
                        for (j = stringLength(str) + 2; j < 16; j++)
                            putcUart0(' ');

                        // Printing the thread name
                        putsUart0(namesOfTasks[i]);
                        for (j = stringLength(namesOfTasks[i]); j < 16; j++)
                            putcUart0(' ');

                        // Print the CPU percentage
                        // Print zero for now
                        putcUart0('0');
                        for (j = 0; j < 8; j++)
                            putcUart0(' ');

                        // Printing out the state of the thread
                        statesArray[i] == 0 ? strCopy(str, "INVALID") : statesArray[i] == 1 ? strCopy(str, "STOPPED")
                                                                      : statesArray[i] == 2   ? strCopy(str, "READY")
                                                                      : statesArray[i] == 3   ? strCopy(str, "DELAYED")
                                                                      : statesArray[i] == 4   ? strCopy(str, "BLOCKED_MUTEX")
                                                                      : statesArray[i] == 5   ? strCopy(str, "BLOCKED_SEMAPHORE")
                                                                      : strCopy(str, "UNKNOWN");
                        putsUart0(str);
                        for (j = stringLength(str); j < 20; j++)
                            putcUart0(' ');

                        // Printing out the mutex or semaphore
                        itoa(mutex_semaphore_array[i], str, 10);
                        putsUart0(str);
                        putcUart0('\n');
                    }
                }
                putsUart0("\n");
            }
            else if (isCommand(&data, "ipcs", 0))
            {
                ipcs();
                foo = true;
            }
            else if (isCommand(&data, "kill", 1))
            {
                kill(getFieldInteger(&data, 1));
                foo = true;
            }
            else if (isCommand(&data, "pkill", 1))
            {
                pkill(getFieldString(&data, 1));
                foo = true;
            }
            else if (isCommand(&data, "pi", 1))
            {
                bool state;
                if (strCmp(getFieldString(&data, 1), "on"))
                {
                    state = true;
                    pi(state);
                    foo = true;
                }
                else if (strCmp(getFieldString(&data, 1), "off"))
                {
                    state = false;
                    pi(state);
                    foo = true;
                }
            }
            else if (isCommand(&data, "preempt", 1))
            {
                if (strCmp(getFieldString(&data, 1), "ON"))
                {
                    putsUart0("Preemptive\n\n");
                    preempt(PREEMPTIVE);
                    foo = true;
                }
                else if (strCmp(getFieldString(&data, 1), "OFF"))
                {
                    putsUart0("Cooperative\n\n");
                    preempt(COOPERATIVE);
                    foo = true;
                }
            }
            else if (isCommand(&data, "sched", 1))
            {
                if (strCmp(getFieldString(&data, 1), "RR"))
                {

                    putsUart0("Round Robin Scheduler\n\n");
                    sched(ROUND_ROBIN_SCHEDULER);
                    foo = true;
                }
                else if (strCmp(getFieldString(&data, 1), "PRIO"))
                {
                    putsUart0("Priority Scheduler\n\n");
                    sched(PRIORITY_SCHEDULER);
                    foo = true;
                }
            }
            else if (isCommand(&data, "pidof", 1))
            {
                uint32_t pid = 0;
                char str[20] = {0};

                pidof(getFieldString(&data, 1), &pid);
                if (pid)
                {
                    putsUart0("PID: 0x");
                    itoa(pid, str, 16);
                    putsUart0(str);
                    putcUart0('\n');
                    putcUart0('\n');
                }
                else
                    putsUart0("Process not found\n\n");

                foo = true;
            }
            else if (isCommand(&data, "meminfo", 0))
            {
                char listOfTasks[MAX_TASKS][10] = {0};
                char strBuffer[MAX_CHARS] = {0};
                uint32_t baseAddress[MAX_TASKS] = {0};
                uint32_t sizeOfTask[MAX_TASKS] = {0};
                uint32_t dynamicMemOfEachTask[MAX_TASKS] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
                uint8_t taskCount = 0;

                putsUart0("\nTask Name\tBase Address\tSize\t\tDynamic Memory\n");
                putsUart0("------------------------------------------------------------\n\n");
                meminfo(listOfTasks, baseAddress, sizeOfTask, &taskCount, dynamicMemOfEachTask);

                uint8_t i = 0;

                for (i = 0; i < taskCount; i++)
                {
                    uint8_t j = 0;

                    // Print the task name
                    putsUart0(listOfTasks[i]);
                    for (j = stringLength(listOfTasks[i]); j < 16; j++)
                        putcUart0(' ');

                    // Print the base address
                    itoa(baseAddress[i], strBuffer, 16);
                    putsUart0("0x");
                    putsUart0(strBuffer);
                    for (j = stringLength(strBuffer) + 2; j < 16; j++)
                        putcUart0(' ');

                    // Print the size of the task
                    itoa(sizeOfTask[i], strBuffer, 10);
                    putsUart0(strBuffer);
                    for (j = stringLength(strBuffer); j < 16; j++)
                        putcUart0(' ');

                    // Print the dynamic memory of each task
                    itoa(dynamicMemOfEachTask[i], strBuffer, 10);
                    putsUart0(strBuffer);

                    putcUart0('\n');
                }
                putcUart0('\n');

                foo = true;
            }
            else if (isCommand(&data, "clear", 0))
            {
                // Clear the screen and move the cursor to the top left
                putsUart0("\033[2J\033[H");
            }
            else if (!foo)
                putsUart0("Invalid command!\n\n");
            clearStruct(&data);
        }
        yield();
    }
}

