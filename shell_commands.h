#ifndef SHELL_COMMNADS
#define SHELL_COMMANDS
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

void yield(void);
void reboot(void);
void ps(void);
void ipcs();
void kill(int32_t pid);
void pkill(const char name[]);
void pi(bool state);
void preempt(bool state);
void sched(bool prio_on);
void pidof(const char name[]);

bool inProcessesList(char list[][10], char name[]);

#endif
