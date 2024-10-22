#ifndef CORTEX_M4_REGISTERS
#define CORTEX_M4_REGISTERS

#include <stdint.h>
/**
 * @brief
 * Set the ASP bit in the CONTROL register
 * so that the thread code uses the PSP stack
 */
extern void setASP();

/**
 * @brief
 * Sets the PSP register to the value passed as an argument
 *
 */
extern void setPSP(uint32_t value);

/**
 * @brief
 * Get the value of the PSP register
 * @return uint32_t
 */
extern uint32_t getPSP(void);

/**
 * @brief
 * Get the value of the MSP register
 */
extern uint32_t getMSP(void);

/**
 * @brief
 * Get the value of the PC register
 */
extern uint32_t getPC(void);

/**
 * @brief
 * Get the value of the xPSR register
 * Process status register
 * @return uint32_t
 */
extern uint32_t getxPSR(void);
/********************************************************************************/
/********************************************************************************/

/**
 * @brief
 *  Enables the bus fault handler in SHCSR
 *  Set bit 17 in the SHCR
 *
 * A bus fault is an exception that occurs
 * because of a memory-related fault for an instruction or data memory transaction.
 * such as a prefetch fault or a memory access fault.
 */
extern void enableBusFault(void);

/**
 * @brief
 *  This function will cause a bus fault exception.
 *  Especifically it causes and set the Precise data bus error bit
 *  in the configurable fault status register.
 */
extern void causeBusFault(void);

/********************************************************************************/
/********************************************************************************/

/**
 * @brief
 * Set bit 18 in the SHCR
 */
extern void enableUsageFault(void);

/**
 * @brief Set the Div0 Trap object
 * Bit 4 in the configuration and control register (CCR) in ARM documentation
 * sets the divide by zero trap.
 */
extern void setDiv0Trap(void);

/**
 * @brief
 * This function will cause a usage fault exception.
 * It is vector number 6 with a an address of 0x00000018
 *
 * Usage Fault Status (UFAULTSTAT 31:16) bit name DIV0 (bit # 25) it
 * is a subregister of the Configurable Fault Status Register.
 *
 */
extern void causeUsageFault(void);

/********************************************************************************/
/********************************************************************************/

/**
 * @brief
 * Causes a hard fault exception via causing a usage fault exception in the usage fault handler
 *
 */
void causeHardFault(void);

/********************************************************************************/
/********************************************************************************/

/**
 * @brief Enable the MPU fault handler
 */
extern void enableMPUHandler(void);

/**
 * @brief
 * Enable the MPU
 */
void enableMPU(void);
/**
 * @brief
 * This fault is used to to abort instruction access to Execute Never (XN) memory
 * regions, even if the MPU is disabled.  .
 *
 */
extern void causeMemFault(void);

#endif
