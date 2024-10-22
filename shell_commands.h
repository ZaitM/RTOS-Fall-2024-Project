#ifndef SHELL_COMMNADS
#define SHELL_COMMANDS
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

/**
 * @brief Empty function called to yield the processor
 */
void yield(void);


/**
 * @brief 
 * Reboots the microcontroller. Will be implemenented 
 * as part of the mini project. 
 */
extern void reboot(void);

/**
 * @brief 
 * Displays the process status. 
 * For now, it displays the text "ps called" 
 */
void ps(void);

/**
 * @brief 
 * Displays the inter-process (thread) communication status
 */
void ipcs();

/**
 * @brief 
 * Kills the process (thread) with the matching PID. 
 * @param pid 
 */
void kill(int32_t pid);

/**
 * @brief 
 * Kills the thread based on the process name  
 * 
 * @param name
 */
void pkill(const char name[]);

/**
 * @brief 
 * Turns priority inheritance on or off. 
 * @param state  
 */
void pi(bool state);

/**
 * @brief 
 * Turns preemption on or off. 
 * @param state 
 */
void preempt(bool state);

/**
 * @brief 
 * Selects scheduling algorithm.
 * Either priority or round robin. 
 * 
 * @param prio_on
 */
void sched(bool prio_on);

/**
 * @brief 
 * Display the PID of the process (thread).
 * @param name 
 */
void pidof(const char name[]);

/**
 * @brief 
 * Checks if a process is in the user defined list of processes
 */
bool inProcessesList(char list[][10], char name[]);

#endif
