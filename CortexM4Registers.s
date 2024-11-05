; Zait Martinez 
; 09/22/2024
; Used to execute assembly functions 

.text
.const

; The following are SCB registers using the 
; Cortex-M4F naming convention

RESERVED    .field 0x00040000   ; Reserved
XN          .field 0xE00FFFFF   ; Bit-band and bit-band alias region
AIRCR       .field 0xE000ED0C
CCR         .field 0xE000ED14   ; Configuration Control Register Enable div0 trap

                                ; Active, Enable, and Pending bits
SHCSR       .field 0xE000ED24   ; System Handler Control Status Register Usage Fault Handler

                                ; Contains the cause of the usage fault, bus fault, or mem fault
CFSR        .field 0xE000ED28   ; Control Fault Status Register * Byte accessible. 
UFSR        .field 0xE000ED2A   ; Usage Fault Status Register   * Executed a divide by zero


SHCSR_EN_MASK    .field 0x00040000   ; Usage Fault Enable Mask
VECTKEY_MASK:     .field 0x05FA        ; Key to unlock the AIRCR register
SYSRESETREQ_MASK: .field 0x00000004   ; System Reset Request Mask
VECTKEY_SYSRESETREQ_MASK .field 0x05FA0004

;    .def reboot
; reboot:
;   ldr r0, AIRCR 
;    ldr r1, [r0]
;    ldr r2 , VECTKEY_SYSRESETREQ_MASK 
;    orr r1, r1, r2 
;    str r1, [r0]
;    ldr r1, [r0]
;    isb
;    bx lr
;******************************************************************************** 
	.def setASP
setASP:
    mrs    r0, control  ; Read the control register
    orr    r0, r0, #2   ; Modify (Set) the ASP bit
    msr     control, r0 ; Write the modified value back to the control register
    isb
    bx      lr

    .def setPSP
setPSP:
    msr psp, r0
    isb
    bx lr

; Sets the thread mode privilege level
	.def setTMPL
setTMPL:
    mrs    r0, control  ; Read the control register
    orr    r0, r0, #1   ; Set to unprivilieged mode
    msr     control, r0 ; Write the modified value back to the control register
    isb
    bx      lr

    .def getPSP
getPSP:
    mrs r0, psp
    bx lr

    .def getMSP
getMSP:
    mrs r0, msp
    bx lr

    .def getPC
getPC:
    mov r0, pc
    bx lr

    .def getxPSR
getxPSR:
    mrs r0, apsr
    bx lr


;******************************************************************************** 

    .def enableBusFault
enableBusFault:
    ldr r1, SHCSR      ; Load the adress of the System Handler Control Status Register
    ldr r0, [r1]       ; Load the value of the SHCSR into R0
    orr r0, r0, #0x00020000; Set the bit in the prior register
    str r0, [r1]       ; Dereference SCHSR and store the the value of R2
    ; ldr r0, [r1]     ; Read the value of SCHSR into R0

    bx lr;

    .def causeBusFault
causeBusFault:
    ldr r1, RESERVED   ; Load the address of the reserved register
    ldr r0, [r1]       ; Load the value of the reserved register
;******************************************************************************** 

    .def enableUsageFault
enableUsageFault:
    ldr r1, SHCSR   ; Load the adress of the System Handler Control Status Register
    ldr r0, [r1]    ; Load the value of the SHCSR into R2
    orr r0, r0, #0x00040000; Set the bit in the prior register
    str r0, [r1]    ; Dereference SCHSR and store the the value of R2

    bx lr;

    .def setDiv0Trap 
setDiv0Trap:
    ldr r1, CCR         ; Read the addres of the configuration control register
    ldr r2, [r1]        ; load the value of the configuration control register
    orr r2, r2, #0x10   ; set the trap divide by zero in the configuration control register
    str r2, [r1]        ; Write the div0 mask in the configuration control register
    bx lr

    
	.def causeUsageFault
causeUsageFault:
    mov r0, #0x0 
    ldr r1 , [r0]
    udiv r1, r1, #0x0

    bx lr
;********************************************************************************
    .def enableMPUHandler
enableMPUHandler:
    ldr r1, SHCSR   ; Load the adress of the System Handler Control Status Register
    ldr r0, [r1]    ; Load the value of the SHCSR into R2
    orr r0, r0, #0x00010000; Set the bit in the prior register    
    str r0, [r1]    ; Dereference SCHSR and store the the value of R2

    bx lr

    .def causeMemFault

causeMemFault:
    ldr r0, XN
    mov r1, r0

    bx r1

