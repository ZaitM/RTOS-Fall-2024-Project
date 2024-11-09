#ifndef CORTEX_M4_REGISTERS
#define CORTEX_M4_REGISTERS

#include <stdint.h>
extern void setASP();
extern void setPSP(uint32_t value);
extern void setTMPL(void);
extern uint32_t *getPSP(void);
extern uint32_t getMSP(void);
extern uint32_t getPC(void);
extern uint32_t getxPSR(void);
/********************************************************************************/
/********************************************************************************/
extern void enableBusFault(void);
extern void causeBusFault(void);

/********************************************************************************/
/********************************************************************************/
extern void enableUsageFault(void);
extern void setDiv0Trap(void);
extern void causeUsageFault(void);

/********************************************************************************/
/********************************************************************************/
extern void enableMPUHandler(void);
extern void causeMemFault(void);
void enableMPU(void);

/********************************************************************************/
/********************************************************************************/
void causeHardFault(void);


/********************************************************************************/
/********************************************************************************/
extern void pushR4R11(void);
extern void popR4R11(void);

#endif
