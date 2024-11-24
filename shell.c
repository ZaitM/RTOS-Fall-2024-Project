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

#define RED_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC - 0x40000000) * 32 + 1 * 4)))

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
#endif DEBUG
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
                ps();
                foo = true;
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
            if (!foo)
                putsUart0("Invalid command!\n\n");
            clearStruct(&data);
        }
        yield();
    }
}
