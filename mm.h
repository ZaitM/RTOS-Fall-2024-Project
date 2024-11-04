// Memory manager functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef MM_H_
#define MM_H_
#include <stdint.h>
#include <stdbool.h>

#define NUM_SRAM_REGIONS 5

#define PSP_BASE 0x20008000
#define HEAP_START 0x20001000 // Marks the start of the heap
#define HEAP_END 0x20007FFF   // Marks the end of the heap.

#define MAX_NUM_ALLOCATIONS 40
#define MAX_SIZE 0x2000

#define TOTAL_REGIONS 5
#define TOTAL_8K_REGIONS 2

#define SUBREGIONS_PER_REGION 8

#define FREE 0
#define ALLOCATED 1

#define BLOCK_512 0x200
#define BLOCK_1024 0x400

#define REGION_8KB 0x2000
#define REGION_4KB 0x1000

/* 4KB region */
#define BASE_OS 0x20000000
#define END_OF_OS 0x20000FFF

/* 4KB region */
#define BASE_R0 0x20001000
#define END_OF_R0 0x20001FFF

/* 8KB region */
#define BASE_R1 0x20002000
#define END_OF_R1 0X20003FFF

/* 8KB region */
#define BASE_R2 0x20004000
#define END_OF_R2 0x20005FFF

/* 4KB region */
#define BASE_R3 0x20006000
#define END_OF_R3 0x20006FFF

/* 4KB region */
#define BASE_R4 0x20007000
#define END_OF_R4 0x20007FFF

#define SECTION_SIZE(size) ((size <= BLOCK_512) ? BLOCK_512 : BLOCK_1024)
#define ALIGN_SIZE(size) ((size <= BLOCK_512) ? (size + BLOCK_512 - 1) & ~(BLOCK_512 - 1) : (size + BLOCK_1024 - 1) & ~(BLOCK_1024 - 1))

#define FIND_PTR_BASE(ptr) ((ptr >= BASE_R0 && ptr < END_OF_R0) ? BASE_R0: (ptr >= BASE_R1 && ptr < END_OF_R1) ? BASE_R1: (ptr >= BASE_R2 && ptr < END_OF_R2) ? BASE_R2: \
(ptr >= BASE_R3 && ptr < END_OF_R3) ? BASE_R3: (ptr >= BASE_R4 && ptr <END_OF_R4) ? BASE_R4: -1)

#define FIND_PTR_SUBREGION_SIZE(ptr) ((ptr >= BASE_R0 && ptr < END_OF_R0) ? BLOCK_512: (ptr >= BASE_R1 && ptr < END_OF_R1) ? BLOCK_1024: (ptr >= BASE_R2 && ptr < END_OF_R2) ? BLOCK_1024: \
(ptr >= BASE_R3 && ptr < END_OF_R3) ? BLOCK_512: (ptr >= BASE_R4 && ptr < END_OF_R4) ? BLOCK_512: -1)

#define FIND_PTR_REGION_IDX(ptr) ((ptr >= BASE_R0 && ptr < END_OF_R0) ? R0_IDX: (ptr >= BASE_R1 && ptr < END_OF_R1) ? R1_IDX: (ptr >= BASE_R2 && ptr < END_OF_R2) ? R2_IDX: \
(ptr >= BASE_R3 && ptr < END_OF_R3) ? R3_IDX: (ptr >= BASE_R4 && ptr < END_OF_R4) ? R4_IDX: -1)

//-----------------------------------------------------------------------------
// Mask values for the MPU
//-----------------------------------------------------------------------------
#define FULL_ACCESS 0b011  // Full access for unprivileged
#define SRD_DISABLE 0xFF    // Disable all subregions
#define R0_IDX 0
#define R1_IDX 1
#define R2_IDX 2
#define R3_IDX 3
#define R4_IDX 4

typedef struct
{
    uint32_t baseAddress;
    uint16_t regionSize;    // 4KiB or 8KiB
    uint16_t subRegionSize; // 512B or 1024B
                            // |= 1 to set a subregion as allocated
                            // &= 0 to set a subregion as free
    uint8_t subRegionAllocated[8];
    uint16_t sizeOfAllocations[8];

} MEM_REGION;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void * mallocFromHeap(uint32_t size_in_bytes);
void *allocate_from_subregion(uint32_t size);   // Added 10/29/24
void freeToHeap(void *pMemory);

void enableMPU(void);

void allowFlashAccess(void);
void allowPeripheralAccess(void);
void setupSramAccess(void);
uint64_t createNoSramAccessMask(void);
void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes);
void applySramAccessMask(uint64_t srdBitMask);

#endif
