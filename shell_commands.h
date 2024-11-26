#ifndef SHELL_COMMNADS
#define SHELL_COMMANDS
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

//-----------------------------------------------------------------------------
// RTOS service calls for shell commands
//-----------------------------------------------------------------------------

void reboot(void);
void ps(uint32_t *pidsArray, char namesOfTasks[][10], uint32_t *statesArray, uint8_t *mutex_semaphore_array);
void ipcs();
void kill(int32_t pid);
void pkill(const char name[]);
void pi(bool state);
void preempt(bool state);
void sched(bool prio_on);
void pidof(const char name[], uint32_t *pid);
void meminfo(char namesOfTasks[][10], uint32_t *baseAddress, uint32_t *sizeOfTask, uint8_t *taskCount, uint32_t *dynamicMemOfEachTask);
void getListOfProcesses(char processList[][10], uint32_t *currentProcessCount);

bool inProcessesList(char list[][10], char processName[], uint8_t processesCount)

#endif
