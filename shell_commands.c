#include "shell_commands.h"
#include "shell_auxiliary.h"

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
    __asm(" SVC #12");
}

/**
 * @brief
 * Displays the process status.
 * For now, it displays the text "ps called"
 */
void ps(uint32_t* pidsArray, char namesOfTasks[][10], uint32_t* statesArray, uint8_t* mutex_semaphore_array)
{
    __asm(" SVC #13");
}

/**
 * @brief
 * Displays the inter-process (thread) communication status
 */
void ipcs()
{
    __asm(" SVC #14");
}

/**
 * @brief
 * Kills the process (thread) with the matching PID.
 * @param pid
 */
void kill(int32_t pid)
{
    // Will use the stopThread function in kernel.c
    __asm(" SVC #15");
}

void pkill(const char name[])
{
    // Will use the stopThread function in kernel.c
    __asm(" SVC #16");
}

/**
 * @brief
 * Turns priority inheritance on or off.
 * @param state
 */
void pi(bool state)
{
}

/**
 * @brief
 * Turns preemption on or off.
 * @param state
 */
void preempt(bool state)
{
    __asm(" SVC #18");
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
    __asm(" SVC #19");
}

/**
 * @brief
 * Display the PID of the process (thread).
 * @param name
 */
void pidof(const char name[], uint32_t *pid)
{
    __asm(" SVC #20");
}

/**
 * @brief 
 * Display the memory usage by all threas, including the base address and size of 
 * each allocation. Optionally display dynamic memory
 */
void meminfo(char namesOfTasks[][10], uint32_t *baseAddress, uint32_t *sizeOfTask, uint8_t *taskCount, uint32_t *dynamicMemOfEachTask)
{
    __asm(" SVC #21");
}

/**
 * @brief
 * Gets the list of processes
 */
void getListOfProcesses(char processList[][10], uint32_t *currentProcessCount)
{
    __asm(" SVC #22");
}

/**
 * @brief
 * Checks if a process is in the user defined list of processes
 */
bool inProcessesList(char list[][10], char processName[], uint8_t processesCount)
{
    uint8_t i = 0;
    for (i = 0; i < processesCount; i++)
    {
        if (strCmp(list[i], processName))
        {
            return true;
        }
    }
    return false;
}
