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
#include "faults.h"
#include "CortexM4Registers.h"
#include "shell_auxiliary.h"
//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
volatile bool foo = false;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    char str[32];
    uint32_t *psp = getPSP();
    uint32_t pc = getPC();

    putsUart0("MMU fault in PID:\n\n");
    putsUart0("Values of the core registers:\n");

    putsUart0("PSP = 0x");
    itoa(psp, str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("MSP = 0x");
    itoa(getMSP(), str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("mfault = 0x");
    itoa(NVIC_FAULT_STAT_R, str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("Address of offending instruction = 0x");
    itoa(*(psp + 6), str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("Displaying the stack dump:\n");
    putsUart0("R0  = 0x");
    itoa(*(psp), str, 16); // PC
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R1  = 0x");
    itoa(*(psp + 1), str, 16); // LR
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R2  = 0x");
    itoa(*(psp + 2), str, 16); // R12
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R3  = 0x");
    itoa(*(psp + 3), str, 16); // R3
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R12 = 0x");
    itoa(*(psp + 4), str, 16); // R2
    putsUart0(str);
    putcUart0('\n');

    putsUart0("LR  = 0x");
    itoa(*(psp + 5), str, 16); // R1
    putsUart0(str);
    putcUart0('\n');

    putsUart0("PC  = 0x");
    itoa(*(psp + 6), str, 16); // R0
    putsUart0(str);
    putcUart0('\n');

    putsUart0("xPSR  = 0x");
    itoa(*(psp + 7), str, 16); // R0
    putsUart0(str);
    putsUart0("\n\n");

    /**
     * -> Need to clear the IACCVIOL (bit 0) by writing a one
     *    in the MMFSR (CFSR) register in the SCB

     * -> Need to clear the MEMFAULTACT (bit 0) in the SHCSR
     *
     * -> Need to cause a PendSV ISR bit 28 in the ICSR register
     */
    NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_IERR;
    // NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMA;
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;

    // Set the pc to the link register
    *(psp + 6) = *(psp + 5);
}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    char str[32];
    uint32_t *psp = (uint32_t *)getPSP();
    putsUart0("Hard fault in PID: \n");
    putsUart0("Values of the core registers:\n");

    putsUart0("PSP = 0x");
    itoa(psp, str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("MSP = 0x");
    itoa(getMSP(), str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("mfault = 0x");
    itoa(NVIC_FAULT_STAT_R, str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("Address of offending instruction = 0x");
    itoa(getPC(), str, 16);
    putsUart0(str);
    putcUart0('\n');

    putsUart0("Displaying the stack dump:\n");
    putsUart0("R0  = 0x");
    itoa((psp++), str, 16); // PC
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R1  = 0x");
    itoa((psp++), str, 16); // LR
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R2  = 0x");
    itoa((psp++), str, 16); // R12
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R3  = 0x");
    itoa((psp++), str, 16); // R3
    putsUart0(str);
    putcUart0('\n');

    putsUart0("R12 = 0x");
    itoa((psp++), str, 16); // R2
    putsUart0(str);
    putcUart0('\n');

    putsUart0("LR  = 0x");
    itoa((psp++), str, 16); // R1
    putsUart0(str);
    putcUart0('\n');

    putsUart0("PC  = 0x");
    itoa((psp), str, 16); // R0
    putsUart0(str);
    putsUart0("\n\n");

    while (true)
    {
    };
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    if (foo)
    {
        causeBusFault();
    }

    char str[32];
    putsUart0("Bus fault in PID:!\n\n");
    itoa(NVIC_FAULT_ADDR_R, str, 16);
    putsUart0("Address that caused the bus fault: 0x");
    putsUart0(str);
    putsUart0("\n\n");
    while (true)
    {
    };
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    char str[32];
    itoa(NVIC_FAULT_STAT_R, str, 16); // Read the 25th bit of the NVIC_FAULT_STAT_R (CFSR)
    putsUart0("Usage fault in PID:\n");
    putsUart0("CFSR: 0x");
    putsUart0(str);
    putsUart0("\n\n");
    while (true)
    {
    };
}

/**
 * @brief
 * Causes a hard fault exception via causing a usage fault exception in the usage fault handler
 */
void causeHardFault(void)
{
    foo = true;
    causeBusFault(); // Tries to read reserved memory
}

/**
 * @brief
 * Set the PendSV bit in the ICSR register
 */
void setPendSV(void)
{
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}
