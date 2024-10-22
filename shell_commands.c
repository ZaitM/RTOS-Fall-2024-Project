#include "shell_commands.h"

void yield(void)
{
    // Empty function called to yield the processor
}

void reboot(void)
{
    /*
        Can only be accessed from priveleged mode.
        Provides priority grouping control for the 
        - exception model
        - endian status
        - reset control of the system

        To write to the register, 0x05FA must be written to the VECTKEY field i.e bits 31:16
        otherwise the write is ignored.
    */
    NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}


void ps(void)
{
    putsUart0("ps called\n");
}

void ipcs()
{
    putsUart0("IPCS called\n");
}

void kill(int32_t pid)
{
  char str[10];

  itoa(pid, str, 10);
  putsUart0("PID killed: ");
  putsUart0(str);
  putsUart0("\n");

}

void pkill(const char name[])
{
    putsUart0("Process killed: ");
    putsUart0(name);
    putsUart0("\n");
}

void pi(bool state)
{
    state ? putsUart0("pi on\n") : putsUart0("pi off\n");
    
}

void preempt(bool state)
{
    state ? putsUart0("preempt on\n") : putsUart0("preempt off\n");
}

void sched(bool prio_on)
{
    prio_on ? putsUart0("sched prio\n") : putsUart0("sched rr\n");
}

void pidof(const char name[])
{
    putsUart0(name);
    putsUart0(" launched\n");
}

bool inProcessesList(char list[][10], char name[])
{
    uint8_t i = 0;
    for (i = 0; i < 5; i++)
    {
        if (strCmp(list[i], name))
        {
            return true;
        }
    }
    return false;
}