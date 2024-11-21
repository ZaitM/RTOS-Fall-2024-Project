// Tasks
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
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "tasks.h"

#define RED_LED PORTA, 2    // off-board red LED. Originally Port E 0
#define ORANGE_LED PORTE, 0 // off-board orange LED. Originally Port A 2
#define YELLOW_LED PORTA, 4 // off-board yellow LED. Originally Port A 3
#define GREEN_LED PORTA, 3  // off-board green LED. Originally Port A 4
#define BLUE_LED PORTF, 2   // on-board blue LED

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives included by programmer
//-----------------------------------------------------------------------------

#include "mm.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs
    enablePort(PORTA);
    enablePort(PORTE);
    enablePort(PORTF);
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(ORANGE_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);

    // Setup RGB LED
    selectPinPushPullOutput(PORTF, 1); // RED
    selectPinPushPullOutput(PORTF, 2); // BLUE
    selectPinPushPullOutput(PORTF, 3); // GREEN

    // Setup pushbuttons
    enablePort(PORTC);
    enablePort(PORTD);
    selectPinDigitalInput(PORTC, 4); // 0x10
    selectPinDigitalInput(PORTC, 5); // 0x20
    selectPinDigitalInput(PORTC, 6); // 0x40
    selectPinDigitalInput(PORTC, 7); // 0x80
    selectPinDigitalInput(PORTD, 6); // 0x40
    setPinCommitControl(PORTD, 7);   // 0x80
    selectPinDigitalInput(PORTD, 7); // 0x80
    enablePinPullup(PORTC, 4);
    enablePinPullup(PORTC, 5);
    enablePinPullup(PORTC, 6);
    enablePinPullup(PORTC, 7);
    enablePinPullup(PORTD, 6);
    enablePinPullup(PORTD, 7);

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    uint8_t pbValues = 0;

    pbValues = ~(getPortValue(PORTC) >> 4) & 0x0F;
    pbValues |= ~(getPortValue(PORTD) >> 2) & 0x30;

    return pbValues;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while (true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}

void idle2(void)
{
    while (true)
    {
        //        lock(resource);
        setPinValue(BLUE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(BLUE_LED, 0);
        yield();
    }
}
void flash4Hz(void)
{
    while (true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while (true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    mem = (uint8_t *)malloc_from_heap_wrapper(5000 * sizeof(uint8_t));

    while (true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED)); 
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while (true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while (true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while (true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t *p = (uint32_t *)0x20000000;
    while (true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while (true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}
