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


SHCSR_EN_MASK    .field 0x00040000      ; Usage Fault Enable Mask
VECTKEY_MASK:     .field 0x05FA         ; Key to unlock the AIRCR register
SYSRESETREQ_MASK: .field 0x00000004     ; System Reset Request Mask
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
; @brief
; Set the ASP bit in the CONTROL register
; so that the thread code uses the PSP stack
	.def setASP
setASP:
    mrs    r0, control  ; Read the control register
    orr    r0, r0, #2   ; Modify (Set) the ASP bit (BIT 1)
    msr     control, r0 ; Write the modified value back to the control register
    isb
    bx      lr

; @brief
; Sets the PSP register to the value passed as an argument
    .def setPSP
setPSP:
    msr psp, r0
    isb
    bx lr

; Sets the thread mode privilege level
	.def setTMPL
setTMPL:
    mrs    r0, control  ; Read the control register
    orr    r0, r0, #1   ; Set to unprivilieged mode (BIT 0)
    msr     control, r0 ; Write the modified value back to the control register
    isb
    bx      lr

;******************************************************************************** 
; @brief
; Gets the value of the PSP register
; @return uint32_t
    .def getPSP
getPSP:
    mrs r0, psp
    bx lr

; @brief
; Get the value of the MSP register
    .def getMSP
getMSP:
    mrs r0, msp
    bx lr

; @brief
; Get the value of the PC register
    .def getPC
getPC:
    mov r0, pc
    bx lr

; @brief
; Get the value of the xPSR register
; Process status register
; @return uint32_t
    .def getxPSR
getxPSR:
    mrs r0, apsr
    bx lr

;******************************************************************************** 
; @brief
; Enables the bus fault handler in SHCSR
; Set bit 17 in the SHCR
;
; A bus fault is an exception that occurs
; because of a memory-related fault for an instruction or data memory transaction.
; such as a prefetch fault or a memory access fault.
    .def enableBusFault
enableBusFault:
    ldr r1, SHCSR           ; Load the adress of the System Handler Control Status Register
    ldr r0, [r1]            ; Load the value of the SHCSR into R0
    orr r0, r0, #0x00020000 ; Set the bit in the prior register
    str r0, [r1]            ; Dereference SCHSR and store the the value of R2
    ; ldr r0, [r1]          ; Read the value of SCHSR into R0

    bx lr;

; @brief
; This function will cause a bus fault exception.
; Especifically it causes and set the Precise data bus error bit
; in the configurable fault status register.
    .def causeBusFault
causeBusFault:
    ldr r1, RESERVED   ; Load the address of the reserved register
    ldr r0, [r1]       ; Load the value of the reserved register

;******************************************************************************** 
; @brief
; Set bit 18 in the SHCR
    .def enableUsageFault
enableUsageFault:
    ldr r1, SHCSR           ; Load the adress of the System Handler Control Status Register
    ldr r0, [r1]            ; Load the value of the SHCSR into R2
    orr r0, r0, #0x00040000 ; Set the bit in the prior register
    str r0, [r1]            ; Dereference SCHSR and store the the value of R2

    bx lr;

; @brief Set the Div0 Trap object
; Bit 4 in the configuration and control register (CCR) in ARM documentation
; sets the divide by zero trap.
    .def setDiv0Trap 
setDiv0Trap:
    ldr r1, CCR         ; Read the addres of the configuration control register
    ldr r2, [r1]        ; load the value of the configuration control register
    orr r2, r2, #0x10   ; set the trap divide by zero in the configuration control register
    str r2, [r1]        ; Write the div0 mask in the configuration control register
    bx lr

;@brief
; This function will cause a usage fault exception.
; It is vector number 6 with a an address of 0x00000018
; Usage Fault Status (UFAULTSTAT 31:16) bit name DIV0 (bit # 25) it
; is a subregister of the Configurable Fault Status Register.
	.def causeUsageFault
causeUsageFault:
    mov r0, #0x0 
    ldr r1 , [r0]
    udiv r1, r1, #0x0

    bx lr

;********************************************************************************
; @brief Enables the MemManage fault handler in SHCSR
    .def enableMPUHandler
enableMPUHandler:
    ldr r1, SHCSR           ; Load the adress of the System Handler Control Status Register
    ldr r0, [r1]            ; Load the value of the SHCSR into R2
    orr r0, r0, #0x00010000 ; Set the bit in the prior register    
    str r0, [r1]            ; Dereference SCHSR and store the the value of R2

    bx lr

; @brief
; This fault is used to to abort instruction access to Execute Never (XN) memory
; regions, even if the MPU is disabled.  .
    .def causeMemFault
causeMemFault:
    ldr r0, XN
    mov r1, r0

    bx r1

;********************************************************************************
    .def pushR4R11
pushR4R11:
    mrs r0, psp
    stmdb r0!, {r4-r12}     ; pushing data onto a Full Descending stack
                            ; (pg 79 ARM Cortex-M4 Generic User Guide)
    msr psp, r0
    bx lr

    .def popR4R11
popR4R11:
	mrs r0, psp
    ldmia r0!, {r4-r11, lr} ; popping data from a Full Descending stack
                            ; (pg 79 ARM Cortex-M4 Generic User Guide)
                            ; '!' the final address that is loaded is written back to the base
                            ; register
    msr psp, r0
    bx lr

