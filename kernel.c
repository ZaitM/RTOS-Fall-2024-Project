// Kernel functions
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
#include "mm.h"
#include "kernel.h"

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives by programmer
//-----------------------------------------------------------------------------
#include "shell_auxiliary.h"
#include "CortexM4Registers.h"
#include "faults.h"
#define EXC_RETURN_THREAD_PSP 0xFFFFFFFD

//-----------------------------------------------------------------------------
// Macros for the SVC calls
//-----------------------------------------------------------------------------
#define SVC_START_R 0
#define SVC_YIELD 1
#define SVC_RESTART_T 2
#define SVC_STOP_T 3
#define SVC_SET_PRIORITY_T 4
#define SVC_SLEEP 5
#define SVC_LOCK 6
#define SVC_UNLOCK 7
#define SVC_WAIT 8
#define SVC_POST 9
#define SVC_MALLOC_WRAPPER 10
#define SVC_FREE_WRAPPER 11

#define SVC_REBOOT 12
#define SVC_PS 13
#define SVC_IPCS 14
#define SVC_KILL 15
#define SVC_PKILL 16
#define SVC_PI 17
#define SVC_PREEMPT 18
#define SVC_SCHED 19
#define SVC_PIDOF 20
#define SVC_MEMINFO 21

/*
    The PSR is a combination of the following:
    - APSR: Application Program Status Register
    - IPSR: Interrupt Program Status Register
    - EPSR: Execution Program Status Register
*/
#define EPSR_THUMB_MASK (1 << 24)
//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID 0           // no task
#define STATE_STOPPED 1           // stopped, all memory freed
#define STATE_READY 2             // has run, can resume at any time
#define STATE_DELAYED 3           // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX 4     // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

// task
uint8_t taskCurrent = 0; // index of last dispatched task
uint8_t taskCount = 0;   // total number of valid tasks

// control
bool priorityScheduler = PRIORITY_SCHEDULER; // priority (true) or round-robin (false)
bool priorityInheritance = false;            // priority inheritance for mutexes
bool preemption = PREEMPTIVE;                // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES 16
struct _tcb
{
    uint8_t state;           // see STATE_ values above
    void *pid;               // used to uniquely identify thread (add of task fn)
    void *spInit;            // original top of stack
    void *sp;                // current stack pointer
    uint8_t priority;        // 0=highest
    uint8_t currentPriority; // 0=highest (needed for pi)
    uint32_t ticks;          // ticks until sleep complete
    uint64_t srd;            // MPU subregion disable bits
    char name[16];           // name of task used in ps command
    uint8_t mutex;           // index of the mutex in use or blocking the thread
    uint8_t semaphore;       // index of the semaphore that is blocking the thread
    uint32_t sizeOfStack;    // size of the stack
    uint32_t baseAdress;     // Base adress
} tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: initialize systick for 1ms system timer
void initRtos(void)
{
    uint8_t i;

    // no tasks running
    taskCount = 0;

    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }

    // Disable the SyshellsTick timer
    NVIC_ST_CTRL_R = 0;

    // Set the clock source to the system clock
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC;

    // Enables SysTick exception request
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_INTEN; // TICKINT

    // Set the reload value
    NVIC_ST_RELOAD_R = 40000 - 1;

    // Enable the SysTick timer
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE;
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
uint8_t rtosScheduler(void)
{
    bool ok;
    static uint8_t task = 0xFF;
    ok = false;

    if (!priorityScheduler)
    {
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY);
        }
    }
    else
    {
        /*
            - Look for the highest priority
            - If the task is ready at that priority
              then dispatch that task
            - Dispatch tasks in order for that priority level
            - Each priority level will have an index to keep track
            - The concept of priority rings
        */
        static uint8_t currentPriority = 0x00;
        uint8_t highestPrioAndReadyTask = 0xFF;

        static int64_t nextTaskWithSamePriority[NUM_PRIORITIES] =
            {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

        uint8_t x = 0, i = 0;
        bool found = false;

        // If there is a -1 then go fill the queue for that prio
        // else dispatch the task at the current priority

        if (nextTaskWithSamePriority[currentPriority] == -1)
        {
            // First time in answer one question only; Who is the highest priority and is ready
            for (i = currentPriority; i < NUM_PRIORITIES; i++)
            {
                for (x = 0; x < taskCount; x++)
                {
                    if (tcb[x].state == STATE_READY && tcb[x].priority == currentPriority)
                    {
                        highestPrioAndReadyTask = x;
                        nextTaskWithSamePriority[currentPriority] = highestPrioAndReadyTask;
                        // The bottom retain 6 in the  queue and sign extended
                        nextTaskWithSamePriority[currentPriority] = (nextTaskWithSamePriority[currentPriority] & 0xF) | ~0xF;
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
                currentPriority++;
            }

            // See if there are tasks that are ready and have the same priority within the same priority ring
            // Will execute if it finds an index of a task that is ready and has the same priority
            // Checking if it is not the default priority
            if (highestPrioAndReadyTask != 0xFF)
            {
                uint8_t j = 0;
                uint8_t iterator = 1; // Cannot start at zero. Only reason it is here is cause it there is someone in the queue

                // Find the task with same priority and is STATE_READY
                for (j = x + 1; j < taskCount; j++) // Don't bother checking the rest. So set to x. From where I am. Current prio
                {
                    if (tcb[j].priority == (currentPriority) && tcb[j].state == STATE_READY)
                    {
                        // j represents the idx of the tcb of the next task to run in the prio ring
                        // Perform left shift to store the index of the next task with the same priority
                        nextTaskWithSamePriority[currentPriority] &= ~(0xF << (iterator * 4)); // Clear the bit field
                        nextTaskWithSamePriority[currentPriority] |= (j << (iterator++ * 4));
                    }
                }
            }
        }

        // Would need to see the next task with the prior priority
        uint8_t nextTask = nextTaskWithSamePriority[currentPriority] & 0x0F;
        nextTaskWithSamePriority[currentPriority] >>= 4; // This will mark it as -1 when there is only one task

        // If it is not empty stay at same level and rr
        currentPriority = nextTaskWithSamePriority[currentPriority] != -1 ? currentPriority : 0x00;

        task = nextTask; // Faulting here due to being assigned the default value when nothing is found
    }

    return task;
}

void launchTask()
{
    __asm(" SVC #0");
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn
void startRtos(void)
{
    setPSP(TOP_OF_HEAP);
    setASP();
    setTMPL();

    launchTask(); // Does a service call (Goes to privileged mode)
}

// REQUIRED:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// spInit (Not required this semester 10/09/24)
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;

    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            // Gets the address of the function
            // Double purpose. PID number and the address of the function
            found = (tcb[i++].pid == fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID)
            {
                i++;
            }
             uint32_t size = ALIGN_SIZE(stackBytes);
            // 1. Create a void pointer. Preallocate memory for the thread
            void *ptr = mallocFromHeap(size);

            tcb[i].baseAdress = (uint32_t)ptr;

            tcb[i].sizeOfStack = size;

            // 2. Store the thread name
            strCopy(tcb[i].name, name);

            tcb[i].state = STATE_READY;
            tcb[i].pid = fn; //
            // Adjust the sp to the top of the stack
            tcb[i].sp = (void *)((uint32_t)ptr + size);
            tcb[i].spInit = (void *)((uint32_t)ptr + size); // May not need this as mentioned in class
            tcb[i].priority = priority;                           //
            tcb[i].srd = createNoSramAccessMask();

            // 3.  Configure/Modify the srd bit mask
            addSramAccessWindow(&tcb[i].srd, ptr, size);

            // 4. Make the thread appea as if it has run before
            uint32_t *psp = (uint32_t *)tcb[i].sp;

            *(psp - 1) = EPSR_THUMB_MASK;       // Set the Thumb bit in the EPSR
            *(psp - 2) = (uint32_t)tcb[i].pid;  // Set the PC to the function address
            *(psp - 3) = EXC_RETURN_THREAD_PSP; // Set the LR to thread mode and use the PSP
            *(psp - 4) = 0x00000000;            // Zero out R12
            *(psp - 5) = 0x00000000;            // Zero out R3
            *(psp - 6) = 0x00000000;            // Zero out R2
            *(psp - 7) = 0x00000000;            // Zero out R1
            *(psp - 8) = 0x00000000;            // Zero out R0
            /*
                Page 152 ARM Optimizing C/C++ Compiler User Guide:
                Preserve any dedicated registers
                - Save-on-reentry registers (R4-R11, LR)
                - SP R13
            */
            // Simulate the pushing of the registers
            *(psp - 9) = EXC_RETURN_THREAD_PSP; // Set the LR to thread mode and use the PSP
            *(psp - 10) = 0x44444444;           // Zero out R11
            *(psp - 11) = 0x00000000;           // Zero out R10
            *(psp - 12) = 0x00000000;           // Zero out R9
            *(psp - 13) = 0x00000000;           // Zero out R8
            *(psp - 14) = 0x00000000;           // Zero out R7
            *(psp - 15) = 0x00000000;           // Zero out R6
            *(psp - 16) = 0x00000000;           // Zero out R5
            *(psp - 17) = 0x00000000;           // Zero out R4 (Lowest numbered register using the lowest memory address)

            psp -= 17; // Move the stack pointer to the next available memory location
            tcb[i].sp = (void *)psp;
            /*
                Page 41 of the Cortex-M4 Generic User Guide

                Exception return
                Occurs when the processor is in handler mode and executes
                one of the following instructions to load EXC_RETURN into the PC:
                - LDM or POP with the PC in the list
                - LDR with PC as the destination register
                - BX instruction using any register

                IMPORTANT:
                EXC_RETURN is the value loaded on to LR on exception entry.
                When it is loaded to PC it indicates to the processor that the
                exception is complete.
            */
            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
}

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm(" SVC #1 "); // PC 0x00002328 -> DFFE
                       // SP 0x20007FF8
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    /*
        Will mark the task as delayed and save the context necessary
        for resuming the task later.

        The task is then delayed until a kernel determines that a
        period of time_ms has expired.

        Once the time has expired, the task that called sleep() will be
        marked as ready so that the scheduler can resume the task later.
    */

    __asm(" SVC #5");
}

// REQUIRED: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm(" SVC #6");
}

// REQUIRED: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm(" SVC #7");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm(" SVC #8");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm(" SVC #9");
}

void *malloc_from_heap_wrapper(uint32_t size)
{
    __asm(" SVC #10");
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    /*
        For all tasks if delayed
        - Decrement the ticks
        - If the ticks are zero, mark the task as ready
    */

    uint8_t i;

    for (i = 0; i < taskCount; i++)
    {
        if (tcb[i].state == STATE_DELAYED)
        {
            tcb[i].ticks--;
            if (tcb[i].ticks == 0)
            {
                tcb[i].state = STATE_READY;
            }
        }
    }

    if (preemption)
        setPendSV();
}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
__attribute__((naked)) void pendSvIsr(void)
{
    // ALL TASK SWITCHING WILL BE DONE HERE
    /*
        Step 7:
        - Push registers
        - Save psp
        - Call the scheduler
        - Restore psp
        - Restore SRD bits
        - Pop the registers to make a seamless transition aka task switch
        Page 110 of the Cortex-M4 Generic User Guide -> Exception Stack Frame

        From my notes on 10/22
        - Push the registers on the stack (R4-R11)
        - Call the scheduler
        - Pop the registers from the stack (R4-R11)
        - Set srd bits based on the memory allocation
    */
    /*
        Page 97 of ARM Optimizing C/C++ Compiler User Guide

        * __asm keyword is used to write inline assembly code in C/C++.
          Embeds instructions or directives

        * The naked attribute can be used to identify functions that are written as
          embedded assembly functions.

        * IMPORTANT: Page 132 of the ARM Optimizing C/C++ Compiler User Guide
          The compiler does not generate prologue or epilogue sequences for naked functions t.
    */

    // clear the ierr bit and
    if ((NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR == 1) || (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR == 2))
    {
        // clear bits
        NVIC_FAULT_STAT_R &= ~NVIC_FAULT_STAT_IERR;
        NVIC_FAULT_STAT_R &= ~NVIC_FAULT_STAT_DERR;
    }

    __asm(" mov r12, lr");
    pushR4R11(); // Saving the registers

    tcb[taskCurrent].sp = getPSP();            // Save the stack pointer
    taskCurrent = rtosScheduler();             // Call the scheduler
    setPSP(tcb[taskCurrent].sp);               // Restore the stack pointer
    applySramAccessMask(tcb[taskCurrent].srd); // Restore the SRD bits

    // Pop the registers from the stack (R4-R11)
    popR4R11(); // Restoring the registers

    // Hardware will pop exception stack frame
    // R0-R3, R12, LR, PC, xPSR
}

// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    /*
        Is an exception that is triggered by the SVC instruction.
        In an OS environment applications can use SVC instructions to
        access kernel functions and device drivers

        *  Page 106 ARM Optimizing C/C++ Compiler User Guide:
           Cortex-M architecutres, C SWI handlers cannot return values.
    */
    /*
         - Will do switch case statement
         - Unprivileged code can call SVC instructions to access kernel functions
    */
    uint32_t svcNum;
    uint32_t *pc = getPSP();

    svcNum = *(pc + 6);
    svcNum -= 2;
    svcNum = *(uint32_t *)svcNum & 0xFF;

    switch (svcNum)
    {
    case SVC_START_R:
        // Step 5: startRTOS() to call the scheduler and
        // then switch to privileged mode when launching the first task
        taskCurrent = rtosScheduler();
        applySramAccessMask(tcb[taskCurrent].srd);
        setPSP(tcb[taskCurrent].sp);
        popR4R11();

        break;

    case SVC_YIELD:
        setPendSV(); // Does the task switching
        break;

    case SVC_SLEEP:
        /*
            - Set state to delayed
            - Set the ticks
            - pendSV
        */
        tcb[taskCurrent].ticks = *(getPSP()); // Set the ticks
        tcb[taskCurrent].state = STATE_DELAYED;

        setPendSV(); // Does the task switching
        break;

    case SVC_LOCK:
    {
        /*
            - Locks the mutex
            - Returns if a resource is available
            - Marks the task as blocked on a mutex
            - Records the task in the mutex process queue
        */

        // Need to grab the parameter which will represent the mutex
        uint8_t mutexIdx;
        mutexIdx = *(getPSP());

        if (!mutexes[mutexIdx].lock) // Checking if we are free
        {
            mutexes[mutexIdx].lock = true;            // Lock the mutex
            mutexes[mutexIdx].lockedBy = taskCurrent; // Record the task that locked the mutex
        }
        else if (mutexes[mutexIdx].lockedBy != taskCurrent) // Check that the task that is trying to lock it has done it in the past
        {
            /// Mark the task as blocked  will be important in the ipcs command
            tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;                              // Set the state to blocked
            mutexes[mutexIdx].processQueue[mutexes[mutexIdx].queueSize] = taskCurrent; // The processQueue is up to 2 tasks
            mutexes[mutexIdx].queueSize++;
        }

        setPendSV(); // Does the task switching

        break;
    }
    case SVC_UNLOCK:
    {
        /*
            - Only the thread that locked the mutex can unlock it
        */
        // Need to grab the paramter which will represent the mutex
        uint8_t mutexIdx;
        mutexIdx = *(getPSP());

        // If the task that locked the mutex is the one trying to unlock it
        if (mutexes[mutexIdx].lockedBy == taskCurrent)
        {
            mutexes[mutexIdx].lock = false; // Unlock the mutex
            // Check if there are any tasks in the queue
            if (mutexes[mutexIdx].queueSize > 0)
            {
                mutexes[mutexIdx].lockedBy = mutexes[mutexIdx].processQueue[0]; // Set the lockedBy to the next task in the queue
                mutexes[mutexIdx].lock = true;                                  // Added according to Leo
                // Mark the task in queue as ready
                tcb[mutexes[0].lockedBy].state = STATE_READY;

                // Move the tasks in the queue
                int i;
                for (i = 0; i < mutexes[mutexIdx].queueSize; i++)
                {
                    mutexes[mutexIdx].processQueue[i] = mutexes[0].processQueue[i + 1];
                }
                mutexes[mutexIdx].queueSize--; // Decrement the queue size
            }
        }
        // TBD: else -> delete the thread

        break;
    }
    case SVC_WAIT:
    {
        /*
            - Decrements the semaphore count and
              returns if a resource is available.

            - If not available marks the task as blocked on a semaphore, and
              records the task in the semaphore process queue
        */

        // Grab the semaphore index from the parameter
        uint8_t semaphoreIdx;
        semaphoreIdx = *(getPSP());

        if (semaphores[semaphoreIdx].count > 0)
        {
            semaphores[semaphoreIdx].count--; // Equivalent to CONSUMING a resource?
            return;
        }
        else
        {
            // Place in the processQueue by setting the index to queueSize
            semaphores[semaphoreIdx].processQueue[semaphores[semaphoreIdx].queueSize] = taskCurrent;
            semaphores[semaphoreIdx].queueSize++;
            tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
            tcb[taskCurrent].semaphore = semaphoreIdx;
            setPendSV();
        }
        break;
    }
    case SVC_POST:
    {
        /*
            Increases the semaphore count.
            If a process is waiting in the queue, decrement the count and mark the task as ready
        */

        // Grab the semaphore index from the parameter
        uint8_t semaphoreIdx = *(getPSP());

        // Increment the semaphore count for posting
        // For that specific semaphore
        semaphores[semaphoreIdx].count++; // Equivalent to PRODUCING a resource?

        // Check if there are any tasks in the queue
        if (semaphores[semaphoreIdx].queueSize > 0)
        {
            semaphores[semaphoreIdx].count--; // Decrement the count

            // Set the task in the queue as ready
            tcb[semaphores[semaphoreIdx].processQueue[0]].state = STATE_READY;

            // Move the tasks in the queue
            uint8_t i;
            for (i = 0; i < semaphores[semaphoreIdx].queueSize; i++)
            {
                semaphores[semaphoreIdx].processQueue[i] = semaphores[semaphoreIdx].processQueue[i + 1];
            }
            semaphores[semaphoreIdx].queueSize--; // Decrement the queue size
        }

        break;
    }
    case SVC_MALLOC_WRAPPER:
    {
        // This function will be called within unprivileged tasks

        // Get the size of the memory to allocate from R0
        uint32_t size = *(getPSP());
        size = ALIGN_SIZE(size);

        void *ptr = mallocFromHeap(size);

        dynamicMemoryOfEachTask[taskCurrent] += size;

        addSramAccessWindow(&tcb[taskCurrent].srd, (uint32_t *)ptr, size);
        // Apply the current task's SRD bits
        applySramAccessMask(tcb[taskCurrent].srd);

        // Write the memory allocated to R0
        *(getPSP()) = (uint32_t)ptr;

        break;
    }
    case SVC_REBOOT:
    {
        NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
        break;
    }
    case SVC_PREEMPT:
    {
        bool status = *getPSP();
        preemption = status;

        break;
    }
    case SVC_SCHED:
    {
        bool prio_status = *(getPSP());

        // Set the priority scheduler
        priorityScheduler = prio_status;

        break;
    }
    case SVC_PIDOF:
    {
        // R0: Process name
        // R1: Address to a uint32_t variable
        uint8_t i;
        for (i = 0; i < taskCount; i++)
        {
            if (strCmp(tcb[i].name, (char *)*(getPSP())))
            {
                // Write to R1 the PID of the task
                uint32_t *psp = getPSP(); // Getting R0. const char []
                uint32_t *r1 = (psp + 1); // Getting R1. address of uint32_t

                uint32_t *pid = (uint32_t *)*r1; // Dereferencing R1 to access the address of the parameter and casting to pointer type
                *pid = (uint32_t)tcb[i].pid;     // Dereferencing the adress to store the value

                break;
            }
        }
        break;
    }
    case SVC_MEMINFO:
    {
        // R0: Address to an array of char
        // R1: Address to an array of uint32_t
        // R2: Address to an array of uint32_t for the sizes of each task
        // R3: Pointer to the task count
        // Let shell know how many tasks are running i.e. taskCount (global variable in kernel.c)

        char(*namesOfTasks)[10] =       (char(*)[10]) * (getPSP());
        uint32_t *baseAddress =         ( uint32_t *)  *((getPSP()) + 1);
        uint32_t *sizeOfTask =           (uint32_t *)  *((getPSP()) + 2);
        uint8_t *taskCountToShell =      (uint8_t *)   *((getPSP()) + 3); // Dereference to access the address of the parameter
        uint32_t *dynamicMemOfEachTask = (uint32_t *)  (*(getPSP()) - MAX_TASKS * 4);
        uint8_t i;
        for (i = 0; i < taskCount; i++)
        {
            strCopy(namesOfTasks[i], tcb[i].name);
            baseAddress[i] = (uint32_t)tcb[i].baseAdress;
            sizeOfTask[i] = tcb[i].sizeOfStack;
            dynamicMemOfEachTask[i] = dynamicMemoryOfEachTask[i] != 0 ? dynamicMemoryOfEachTask[i] : 0;

        }
        *taskCountToShell = taskCount;
        break;
    }
    }
}

void *getPID(void)
{
    return tcb[taskCurrent].pid;
}

//-----------------------------------------------------------------------------
// Glossary
//-----------------------------------------------------------------------------
/*
    Reentrant:
    A function that can be interrupted in the middle of its execution
    Can be called again before the previous call is completed
*/
