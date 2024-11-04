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
#include "shell_auxiliary.h"

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
MEM_REGION regions[TOTAL_REGIONS] =
    {
        {BASE_R0, REGION_4KB, BLOCK_512, {FREE}, {0}},
        {BASE_R1, REGION_8KB, BLOCK_1024, {FREE}, {0}},
        {BASE_R2, REGION_8KB, BLOCK_1024, {FREE}, {0}},
        {BASE_R3, REGION_4KB, BLOCK_512, {FREE}, {0}},
        {BASE_R4, REGION_4KB, BLOCK_512, {FREE}, {0}},
};

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void *mallocFromHeap(uint32_t size_in_bytes)
{
    uint32_t alignedSize = ALIGN_SIZE(size_in_bytes);
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

            /*
                Sweep
                For each needed section check if the section is allocated
            */

            for (j = 0; j < neededSections; j++)
            {
                /*
                    If the section is
                    - allocated
                    - or the remaining size is 0
                    - or the section index is out of bounds
                    then break
                */
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
        Need to recalculate the number of sections needed if
        we ran out of space in the 8K regions
    */
    neededSections = remainingSize / BLOCK_512;

    // Check 512B sections
    for (fourKregionIdx; fourKregionIdx < TOTAL_REGIONS; fourKregionIdx++)
    {
        if (regions[fourKregionIdx].subRegionSize == BLOCK_512)
        {
            // Different counter used
            uint32_t innerNeededSections = neededSections;

            // Once we have allocated exit and don't check the other regions
            if (remainingSize <= 0)
                break;

            uint32_t sectionIdx = 0;
            /*
                When I am in region 0 I need to sweep the region
                to check that there is enough space to allocate.
                Therefore in the for loop I need to check the difference
                between the number of sections needed and the number of
                sections in the region.
                -> Momentarily I will set the needed sections to 0 if in region 0

                When I am in region 3 and 4 I do not
                need to stringently check the number of sections I need.
            */
            // Comment this out if you want the memory block to be allocated in the 4k region to be contiguous
            if (fourKregionIdx != R0_IDX)
                innerNeededSections = FREE;

            for (sectionIdx; sectionIdx <= (SUBREGIONS_PER_REGION - innerNeededSections); sectionIdx++)
            {
                // Once we have allocated exit
                if (!remainingSize)
                    break;

                // Sweep
                for (j = 0; j < neededSections; j++)
                {
                    /*
                    If the section is
                    - allocated
                    - or the remaining size is 0
                    - or the section index is out of bounds
                    then break
                    */
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

    // Number of sections needed according to the section size
    uint8_t subRegionIdx = ((uint32_t)pMemory - FIND_PTR_BASE(pMemory)) / FIND_PTR_SUBREGION_SIZE(pMemory);
    uint8_t regionIdx = FIND_PTR_REGION_IDX(pMemory);
    uint8_t subRegionIdxToStart = subRegionIdx;
    uint32_t sizeOfAllocation = regions[regionIdx].sizeOfAllocations[subRegionIdx];
    /*
        Check the accuracy of results of the operaion
        below.
        Let's say we allocated 4 KiB
        I have to either free:
        - 4 sections
        - 8 sectinos (When both 8 KiB regions are exhausted)

    */
    uint8_t sectionToFree = sizeOfAllocation / SECTION_SIZE(sizeOfAllocation);

    int32_t remainingSize = sizeOfAllocation;
    uint32_t j = 0, eightKregionIdx = regionIdx, fourKregionIdx = regionIdx;

    if (eightKregionIdx >= R1_IDX && eightKregionIdx <= R2_IDX)
    {
        // First check 1KB sections
        for (eightKregionIdx; eightKregionIdx <= TOTAL_8K_REGIONS; eightKregionIdx++)
        {
            // Sweep
            for (j = 0; j < sectionToFree; j++)
            {
                /*
                    If the section is
                    - free
                    - or the remaining size is less 0
                    - or the section index is out of bounds
                    then break
                */
                if (regions[eightKregionIdx].subRegionAllocated[subRegionIdx + j] == FREE || remainingSize <= FREE || (subRegionIdx + j) >= SUBREGIONS_PER_REGION)
                {
                    /*
                     * Set subRegionIdx to zero so that when it goes to the next region it starts at idx = 0
                     */
                    subRegionIdx = 0;
                    break;
                }
                else
                {

                    regions[eightKregionIdx].subRegionAllocated[subRegionIdx + j] = FREE;
                    remainingSize -= BLOCK_1024;
                }

                /**
                 * @brief
                 *  In the case when we are in R2 and need to go to R3
                 */
                fourKregionIdx = regions[R3_IDX].subRegionAllocated[0] == ALLOCATED ? R3_IDX : regionIdx;
            }
            // Once we have freed everyting exit
            if (!remainingSize)
            {
                regions[regionIdx].sizeOfAllocations[subRegionIdxToStart] = FREE;
                break;
            }
        }
    }

    sectionToFree = remainingSize / BLOCK_512;
    // Check 512B sections
    for (fourKregionIdx; fourKregionIdx < TOTAL_REGIONS; fourKregionIdx++)
    {
        if (regions[fourKregionIdx].subRegionSize == BLOCK_512)
        {

            // Sweep
            for (j = 0; j < sectionToFree; j++)
            {
                /*
                    If the section is
                    - free
                    - or the remaining size is 0
                    - or the section index is out of bounds
                    then break
                */
                if (regions[fourKregionIdx].subRegionAllocated[subRegionIdx + j] == FREE || remainingSize == FREE || (subRegionIdx + j) >= SUBREGIONS_PER_REGION)
                {
                    subRegionIdx = 0;
                    break;
                }
                regions[fourKregionIdx].subRegionAllocated[subRegionIdx + j] = FREE;
                remainingSize -= BLOCK_512;
            }

            // Once we have freed everything exit
            if (!remainingSize)
            {
                regions[regionIdx].sizeOfAllocations[subRegionIdxToStart] = FREE;
                break;
            }
        }
    }
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
    NVIC_MPU_NUMBER_R = R0_IDX;
    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20001000;

    // Every subregion is disabled
    NVIC_MPU_ATTR_R = (SRD_DISABLE << 8);

    // Set the region size to 4KiB
    // Set to 11 for 4KB
    // 2^(11 + 1) = 4KB
    NVIC_MPU_ATTR_R |= 0b01011 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= FULL_ACCESS << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 1
    NVIC_MPU_NUMBER_R = R1_IDX;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20002000;

    // Set the region size to 8KiB
    // Set to 12 for 8KB
    // 2^(12 + 1) = 8KB
    NVIC_MPU_ATTR_R |= 0b01100 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= FULL_ACCESS << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 2
    NVIC_MPU_NUMBER_R = R2_IDX;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20004000;

    // Set the region size to 8Kib
    NVIC_MPU_ATTR_R |= 0b01100 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= FULL_ACCESS << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 3
    NVIC_MPU_NUMBER_R = R3_IDX;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20006000;

    // Set the region size to 4KiB
    // Set to 11 for 4KB
    // 2^(11 + 1) = 4KB
    NVIC_MPU_ATTR_R |= 0b01011 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= FULL_ACCESS << 24;

    // Enable the region
    NVIC_MPU_ATTR_R |= 0b1 << 0;

    /********************************************************************************/
    // Set the region number to 4
    NVIC_MPU_NUMBER_R = R4_IDX;

    // Set the base address to the start of the SRAM
    NVIC_MPU_BASE_R = 0x20007000;

    // Set the region size to 4KiB
    // Set to 11 for 4KB
    // 2^(11 + 1) = 4KB
    NVIC_MPU_ATTR_R |= 0b01011 << 1;

    // Set rw for unpriv
    NVIC_MPU_ATTR_R |= FULL_ACCESS << 24;

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
    /*
        Create a function that adds access to the
        requested SRAM address range

        We are going to assume that programmer does not surpass the
        subregion disable bits

        What that means is that if the baseAdd is in R0, the parameter
        size_in_bytes should not exceed 4096
    */

    // Step 1: Determine base region
    uint32_t baseRegion = FIND_PTR_BASE(baseAdd);
    uint32_t startSubregion = ((uint32_t)baseAdd - baseRegion) / FIND_PTR_SUBREGION_SIZE(baseAdd); // Holds a value between 0 and 7
    uint32_t mask = 0;

    // Step 2: Calculate the appropriate mask for the given size
    if (baseRegion != BASE_R1 && baseRegion != BASE_R2)
    {
        switch (size_in_bytes)
        {
        case 512:
            mask = 0x01;
            break;
        case 1024:
            mask = 0x03;
            break;
        case 1536:
            mask = 0x07;
            break;
        case 2048:
            mask = 0x0F;
            break;
        case 2560:
            mask = 0x1F;
            break;
        case 3072:
            mask = 0x3F;
            break;
        case 3584:
            mask = 0x7F;
            break;
        case 4096:
            mask = 0xFF;
            break;
        default:
            putsUart0("Error: Unsupported size\n");
            return;
        }
    }
    else    // Base region is R1 or R2
    {
        switch (size_in_bytes)
        {
        case 1024:
            mask = 0x01;
            break;
        case 2048:
            mask = 0x03;
            break;
        case 3072:
            mask = 0x07;
            break;
        case 4096:
            mask = 0x0F;
            break;
        case 5120:
            mask = 0x1F;
            break;
        case 6144:
            mask = 0x3F;
            break;
        case 7168:
            mask = 0x7F;
            break;
        case 8192:
            mask = 0xFF;
            break;
        default:
            putsUart0("Error: Unsupported size\n");
            return;
        }
    }

    // Step 3: Shift the mask to the appropriate subregion position
    // Again startSubregion is between 0 and 7
    // Does not overflow because of the assumption that the programmer does not surpass the subregion disable bits
    mask <<= startSubregion;



    // Step 4: Apply the mask to the SRD bit mask based on the base region
    switch (baseRegion)
    {
    case BASE_R0:
        *srdBitMask &= ~(mask);
        break;
    case BASE_R1:
        *srdBitMask &= ~(mask << 8);
        break;
    case BASE_R2:
        *srdBitMask &= ~(mask << 16);
        break;
    case BASE_R3:
        *srdBitMask &= ~(mask << 24);
        break;
    case BASE_R4:
        *srdBitMask &= ~(mask << 32);
        break;
    default:
        putsUart0("Error: Invalid base region\n");
        return;
    }
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

void enableMPU(void)
{
    // Enable the MPU
    /*
        The MPU_CTRL register:
        - Enables the MPU
        - Enables the default memory map background region
        - Enables the use of the MPU whne in th hard fault, NMI, and FAULTMASK escalated handlers

        PRIVDEFENA: When enabled, the background region acts as if it is region number -1.
        ANY REGION THAT HAS BEEN DEFINED AND ENABLES HAS PRIORITY OVER THIS DEFAULT MAP

        ENABLE: Enables the MPU

     */
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_PRIVDEFEN | NVIC_MPU_CTRL_ENABLE;
}
