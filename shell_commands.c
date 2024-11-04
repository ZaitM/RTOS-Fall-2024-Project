#include "shell_commands.h"
#include "shell_auxiliary.h"

// void yield(void)
// {
//     // Empty function called to yield the processor
// }

/**
 * @brief 
 * Reboots the microcontroller. Will be implemenented 
 * as part of the mini project. 
 */
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

/**
 * @brief 
 * Displays the process status. 
 * For now, it displays the text "ps called" 
 */
void ps(void)
{
    putsUart0("ps called\n");
}

/**
 * @brief 
 * Displays the inter-process (thread) communication status
 */
void ipcs()
{
    putsUart0("IPCS called\n");
}

/**
 * @brief 
 * Kills the process (thread) with the matching PID. 
 * @param pid 
 */
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

/**
 * @brief
 * Turns priority inheritance on or off.
 * @param state
 */
void pi(bool state)
{
    state ? putsUart0("pi on\n") : putsUart0("pi off\n");
    
}

/**
 * @brief
 * Turns preemption on or off.
 * @param state
 */
void preempt(bool state)
{
    state ? putsUart0("preempt on\n") : putsUart0("preempt off\n");
}

/**
 * @brief
 * Selects scheduling algorithm.
 * Either priority or round robin.
 *
 * @param prio_on
 */
void sched(bool prio_on)
{
    prio_on ? putsUart0("sched prio\n") : putsUart0("sched rr\n");
}

/**
 * @brief
 * Display the PID of the process (thread).
 * @param name
 */
void pidof(const char name[])
{
    putsUart0(name);
    putsUart0(" launched\n");
}

/**
 * @brief
 * Checks if a process is in the user defined list of processes
 */
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