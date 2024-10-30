// Memory manager functions
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

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void *mallocFromHeap(uint32_t size_in_bytes)
{
    uint32_t alignedSize = ALIGN_SIZE(size);
    void *address = allocate_from_subregion(alignedSize);

    return address != NULL ? address : NULL;
}

void *allocate_from_subregion(uint32_t size)
{
    // Number of sections needed according to the section size
    uint32_t neededSections = size / SECTION_SIZE(size);
    uint32_t remainingSize = size;
    uint32_t j = 0, eightKregionIdx = 1, fourKregionIdx = 0;

    void *ptrSubRegion = NULL;

    // First check 1KB sections
    for (eightKregionIdx; eightKregionIdx <= TOTAL_8K_REGIONS; eightKregionIdx++)
    {
        // Once we have allocated exit and don't check the other regions
        if (remainingSize <= 0)
            break;

        uint32_t sectionIdx = 0;

        for (sectionIdx; sectionIdx <= SUBREGIONS_PER_REGION; sectionIdx++)
        {
            // Once we have allocated exit
            if (!remainingSize)
                break;

            // Sweep
            for (j = 0; j < neededSections; j++)
            {
                // Added logic to check out of bounds indexing
                if (regions[eightKregionIdx].subRegionAllocated[sectionIdx + j] == ALLOCATED || remainingSize == FREE || (sectionIdx + j) >= SUBREGIONS_PER_REGION)
                    break;

                else
                {
                    if (ptrSubRegion == NULL)
                    {
                        ptrSubRegion = regions[eightKregionIdx].baseAddress + sectionIdx * BLOCK_1024;
                        regions[eightKregionIdx].sizeOfAllocations[sectionIdx] = size;
                    }

                    regions[eightKregionIdx].subRegionAllocated[sectionIdx + j] = ALLOCATED;
                    remainingSize -= BLOCK_1024;

                    /*
                        If we ran out of space in the 8K region
                        check if there is space in the following
                        4K region and set the 4K region index to R3

                        IF the first index of the 4K region is not allocated
                        then we can allocate in that region
                    */
                    fourKregionIdx = regions[R3_IDX].subRegionAllocated[0] == FREE ? R3_IDX : R0_IDX;
                }
            }
        }
    }

    /*
        Need to recaluclate the number of sections needed if
        we ran out of space in the 8K regions
    */
    neededSections = remainingSize / BLOCK_512;

    // Check 512B sections
    for (fourKregionIdx; fourKregionIdx < TOTAL_REGIONS; fourKregionIdx++)
    {
        if (regions[fourKregionIdx].subRegionSize == BLOCK_512)
        {
            // Once we have allocated exit and don't check the other regions
            if (remainingSize <= 0)
                break;

            uint32_t sectionIdx = 0;

            for (sectionIdx; sectionIdx <= (SUBREGIONS_PER_REGION - neededSections); sectionIdx++)
            {
                // Once we have allocated exit
                if (!remainingSize)
                    break;

                // Sweep
                for (j = 0; j < neededSections; j++)
                {
                    // Added logic to check out of bounds indexing
                    if (regions[fourKregionIdx].subRegionAllocated[sectionIdx + j] == ALLOCATED || remainingSize == FREE || (sectionIdx + j) >= SUBREGIONS_PER_REGION)
                        break;

                    else
                    {
                        if (ptrSubRegion == NULL)
                        {
                            ptrSubRegion = regions[fourKregionIdx].baseAddress + sectionIdx * BLOCK_512;
                            regions[fourKregionIdx].sizeOfAllocations[sectionIdx] = size;
                        }

                        regions[fourKregionIdx].subRegionAllocated[sectionIdx + j] = ALLOCATED;
                        remainingSize -= BLOCK_512;
                    }
                }
            }
        }
    }

    // Not enough space
    if (remainingSize > 0)
        return NULL;
    else
        return ptrSubRegion;
}

// REQUIRED: add your free code here and update the SRD bits for the current thread
void freeToHeap(void *pMemory)
{
}

/**
 * @brief
 * Create a full-access MPU aperture for flash
 * with the RWX access for both privileged and unprivileged access
 */
// REQUIRED: include your solution from the mini project
void allowFlashAccess(void)
{
    // Set the region number to 6
    NVIC_MPU_NUMBER_R = 6;
    // Set the base address to the start of the flash memory
    NVIC_MPU_BASE_R = 0x00000000;
    // Set the region size to 256Kb
    NVIC_MPU_ATTR_R |= 0b10001 << 1;
    // Set the region to RWX
    NVIC_MPU_ATTR_R |= 0b011 << 24;
    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;
}

/**
 * @brief
 * Create a full-access MPU aperture to peripherals and peripheral bitbanded addresses
 * with RW access for both privileged and unprivileged access
 *
 * I chose the all access rule, this may not be needed
 */
void allowPeripheralAccess(void)
{
    // Set the region number to 5
    NVIC_MPU_NUMBER_R = 5;
    // Set the base address to the start of the peripherals
    NVIC_MPU_BASE_R = 0x40000000;
    // Set the region size to 64 MB
    NVIC_MPU_ATTR_R |= 25 << 1;
    // Set the region to be rw
    NVIC_MPU_ATTR_R |= 0b011 << 24;
    // Set the Ins fetches to disable
    NVIC_MPU_ATTR_R |= 0b001 << 28;
    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;
}

/**
 * @brief
 * Creates multiple MPU regions to cover 32 KiB SRAM
 * Each MPU region covers 8 KiB or 4 KiB with subregions
 * of 512B or 1 KiB each with RW access for privileged and no access for unprivileged
 *
 * -> Disable the subregions to start
 */
void setupSramAccess(void)
{
    /*
    Give rw to unpriv
    */
    /********************************************************************************/
    // Set the region number to 0
    NVIC_MPU_NUMBER_R = 0;
    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20001000;

    // Every subregion is disabled
    NVIC_MPU_ATTR_R = (0xFF << 8);

    // Set the region size to 4KiB
    // Set to 11 for 4KB
    // 2^(11 + 1) = 4KB
    NVIC_MPU_ATTR_R |= 0b01011 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= 0b011 << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 1
    NVIC_MPU_NUMBER_R = 1;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20002000;

    // Set the region size to 8KiB
    // Set to 12 for 8KB
    // 2^(12 + 1) = 8KB
    NVIC_MPU_ATTR_R |= 0b01100 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= 0b11 << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 2
    NVIC_MPU_NUMBER_R = 2;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20004000;

    // Set the region size to 8Kib
    NVIC_MPU_ATTR_R |= 0b01100 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= 0b011 << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 3
    NVIC_MPU_NUMBER_R = 3;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20006000;

    // Set the region size to 4KiB
    // Set to 11 for 4KB
    // 2^(11 + 1) = 4KB
    NVIC_MPU_ATTR_R |= 0b01011 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= 0b011 << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 4
    NVIC_MPU_NUMBER_R = 4;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20007000;

    // Set the region size to 4KiB
    // Set to 11 for 4KB
    // 2^(11 + 1) = 4KB
    NVIC_MPU_ATTR_R |= 0b01011 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= 0b011 << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;
}

uint64_t createNoSramAccessMask(void)
{
    return 0x000000FFFFFFFFFF;
}

/**
 * @brief
 * Adds access to the requested SRAM adress range
 */
void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
}

/**
 * @brief
 * Applies the SRD bits to the MPU regions.
 * This function will only be called in privileged mode
 *
 * Will I modify the RASR register?
 */
void applySramAccessMask(uint64_t srdBitMask)
{
    /*
    What the region number is and set the
*/
    // example
    NVIC_MPU_NUMBER_R = 0;
    // Zero out SRD bits Easier to convert the SRDBITMASK
    NVIC_MPU_ATTR_R &= ~(0x0FF << 8);
    NVIC_MPU_ATTR_R |= (((srdBitMask >> 0) & 0xFF) << 8);

    NVIC_MPU_NUMBER_R = 1;
    // Zero out SRD bits Easier to convert the SRDBITMASK
    NVIC_MPU_ATTR_R &= ~(0x0FF << 8);
    NVIC_MPU_ATTR_R |= (((srdBitMask >> 8) & 0xFF) << 8);

    NVIC_MPU_NUMBER_R = 2;
    // Zero out SRD bits Easier to convert the SRDBITMASK
    NVIC_MPU_ATTR_R &= ~(0x0FF << 8);
    NVIC_MPU_ATTR_R |= (((srdBitMask >> 16) & 0xFF) << 8);

    NVIC_MPU_NUMBER_R = 3;
    // Zero out SRD bits Easier to convert the SRDBITMASK
    NVIC_MPU_ATTR_R &= ~(0x0FF << 8);
    NVIC_MPU_ATTR_R |= (((srdBitMask >> 24) & 0xFF) << 8);

    NVIC_MPU_NUMBER_R = 4;
    // Zero out SRD bits Easier to convert the SRDBITMASK
    NVIC_MPU_ATTR_R &= ~(0x0FF << 8);
    NVIC_MPU_ATTR_R |= (((srdBitMask >> 32) & 0xFF) << 8);
}
