// Faults functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef FAULTS_H_
#define FAULTS_H_
#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void mpuFaultIsr(void);
void hardFaultIsr(void);
void busFaultIsr(void);
void usageFaultIsr(void);

#endif
