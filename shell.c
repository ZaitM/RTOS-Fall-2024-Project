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
    char proc_name_strs[5][10] = {"proc0", "proc1", "proc2", "proc3", "proc4"};

    while (true)
    {
        if (kbhitUart0())
        {
            bool foo = 0;
            // Delete first character in terminal that triggered the kbhitUart0() function
            putsUart0("\b");

            // Clear the FIFO buffer
            /*
             * true if data to be read
             * false if no data to be read
             */
            getcUart0();

            putsUart0("> ");

            // Get the string from the user
            getsUart0(&data);

            // Echo back to the user of the TTY interface for testing

            putsUart0("Output:\n");
            putsUart0(data.buffer);
            putcUart0('\n');

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
                bool state;
                if (strCmp(getFieldString(&data, 1), "on"))
                {
                    state = true;
                    preempt(state);
                    foo = true;
                }
                else if (strCmp(getFieldString(&data, 1), "off"))
                {
                    state = false;
                    preempt(state);
                    foo = true;
                }
            }
            else if (isCommand(&data, "sched", 1))
            {
                bool state;
                if (strCmp(getFieldString(&data, 1), "rr"))
                {
                    state = false;
                    sched(state);
                    foo = true;
                }
                else if (strCmp(getFieldString(&data, 1), "prio"))
                {
                    state = true;
                    sched(state);
                    foo = true;
                }
            }
            else if (isCommand(&data, "pidof", 1))
            {
                pidof(getFieldString(&data, 1));
                foo = true;
            }
            if (inProcessesList(proc_name_strs, &data.buffer[data.fieldPosition[0]]))
                RED_LED = 1;
            else
                RED_LED = 0;

            foo ? putsUart0("\nValid command!\n") : putsUart0("\nInvalid command!\n");
            putsUart0("********************\n");

            clearStruct(&data);
        }
        yield();
    }
}

