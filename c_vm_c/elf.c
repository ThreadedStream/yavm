#include "elf.h"


uint16_t swap16(uint16_t value){
    // 0x4000 -> 0x0040, 0x3875 -> 0x7538
    return (value >> 8) | (value << 8);
}

/*
    @params
    elf_info - pointer to ELF_INFO structure. We are mostly interested in 2 fields, namely

*/

// TODO: Add support for storing data given arbitrary number of bytes
void storeData(void *destination, uint16_t destinationMemberOffset, uint16_t elfOffset32, uint16_t elfOffset64,
               uint8_t sizeDependent, uint8_t bytes) {
    uint16_t curr_idx;
    // Indirectly fetch bit depth. May be a bug-prone endeavour
    uint16_t bitDepth = *((uint16_t *) (destination + offsetof(ELF_INFO, bit_depth)));
    // ELF has only two cases if entry is size dependent (32, 64 bit)
    if (sizeDependent) {
        uint32_t value32 = 0;
        uint64_t value64 = 0;
        if (bitDepth == 32) {
            curr_idx = elfOffset32;
            uint16_t endingPosition = curr_idx + (3 * BYTE);
            while (curr_idx < endingPosition) {
                value32 |= *(ptr + curr_idx);
                value32 <<= 8;
                curr_idx += BYTE;
            }
            value32 |= *(ptr + curr_idx);
            SET_VALUE(destination, destinationMemberOffset, uint32_t, value32);
        } else {
            curr_idx = elfOffset64;
            uint16_t endingPosition = curr_idx + (7 * BYTE);
            while (curr_idx < endingPosition) {
                value64 |= *(ptr + curr_idx);
                value64 <<= 8;
                curr_idx += BYTE;
            }
            value64 |= *(ptr + curr_idx);
            printf("%lu", value64);
            SET_VALUE(destination, destinationMemberOffset, uint64_t, value64);
        }
    }
}

void readObjectFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Wrong path");
        exit(-1);
    }

    ptr = calloc(ELF_HEADER_SIZE, sizeof(uint8_t));

    uint32_t read = fread(ptr, sizeof(uint8_t), ELF_HEADER_SIZE, file);
    if (!read) {
        fprintf(stderr, "Failed to carry out read operation");
        exit(-1);
    }

    fclose(file);
}


void parseElfHeader() {
    // "'Who in the world am I?' Ah, that's the great puzzle!"

    ELF_INFO elf_info = {};

    // First, parse first 4 bytes signifying a magic number
    // I'm intentionally writing the code in more verbose way, so i do not forget anything if look into it after some while
    uint16_t curr_idx = E_MAGIC_NUMBER_OFFSET;
    uint16_t endingPosition = 0;
    if (*(ptr + curr_idx) != 0x7f && *(ptr + curr_idx + 1) != 45 && *(ptr + curr_idx + 2) != 0x4c &&
        *(ptr + curr_idx + 3) != 46) {
        // It's not an ELF format
        fprintf(stderr, "Incorrect magic number");
        exit(-1);
    }

    //Parsing bit depth (0x1 -> 32-bit or 0x2 -> 64-bit)
    curr_idx = E_BIT_DEPTH_OFFSET;

    switch (*(ptr + curr_idx)) {
        case 0x1:
            elf_info.bit_depth = 32;
            break;
        case 0x2:
            elf_info.bit_depth = 64;
            break;
        default:
            fprintf(stderr, "Could not determine bit depth");
            exit(-1);
    }

    //Parsing endianness (0x1 -> little endian, 0x2 -> big endian)

    curr_idx = E_ENDIANNESS_OFFSET;
    switch (*(ptr + curr_idx)) {
        case 0x1:
            elf_info.isLittleEndian = 1;
            break;
        case 0x2:
            elf_info.isLittleEndian = 0;
            break;
        default:
            fprintf(stderr, "Could not determine endianness");
            exit(-1);
    }

    // Parsing elf version information
    curr_idx = E_ELF_VERSION_OFFSET;
    if (*(ptr + curr_idx) == 0x1) {
        elf_info.isElfOriginalVersion = 1;
    } else {
        // It may be considered as a bug, since there may be any other value
        elf_info.isElfOriginalVersion = 0;
    }

    curr_idx = E_OS_ABI_OFFSET;
    if (*(ptr + curr_idx) < 0x0 || *(ptr + curr_idx) > 0x12) {
        fprintf(stderr, "Unknown abi os");
        exit(-1);
    }
    elf_info.abiOs = *(ptr + curr_idx);

    //Parsing abi version
    curr_idx = E_ABI_VERSION_OFFSET;
    elf_info.abiVersion = *(ptr + curr_idx);

    // Parsing reserved EI_PAD
    curr_idx = E_EI_PAD_OFFSET;
    endingPosition = curr_idx + (7 * BYTE);
    while (curr_idx < endingPosition) {
        if (*(ptr + curr_idx) != 0x0) {
            fprintf(stderr, "EI_PAD should be zero");
            exit(-1);
        }
        curr_idx += BYTE;
    }

    // Parsing type
    curr_idx = E_TYPE_OFFSET;

    // for now, we take this value for granted, without checking its validity
    elf_info.type |= *(ptr + curr_idx);
    elf_info.type <<= 8;
    elf_info.type |= *(ptr + curr_idx + BYTE);

    // TODO: Put the logic below into inlined function
    //Parsing isa
    curr_idx = E_ISA_OFFSET;
    elf_info.isa |= *(ptr + curr_idx);
    elf_info.isa <<= 8;
    elf_info.isa |= *(ptr + curr_idx + BYTE);

    curr_idx = E_VERSION_OFFSET;
    // Skip this step for now

    // Parsing memory address of the entry point
    storeData((void *) &elf_info, offsetof(ELF_INFO, entryPointOffset), E_ENTRY_OFFSET, E_ENTRY_OFFSET, 1, -1);

    // Parsing program header table offset
    storeData((void *) &elf_info, offsetof(ELF_INFO, programHeaderOffset), E_PROGRAM_HEADER_32_BIT_OFFSET,
              E_PROGRAM_HEADER_64_BIT_OFFSET, 1, -1);

    // Parsing section header offset
    storeData((void *) &elf_info, offsetof(ELF_INFO, sectionHeaderOffset), E_SECTION_HEADER_32_BIT_OFFSET,
              E_SECTION_HEADER_64_BIT_OFFSET, 1, -1);

    // Parsing flags
    if (elf_info.bit_depth == 64) {
        curr_idx = E_FLAGS_64_BIT_OFFSET;
    } else {
        curr_idx = E_FLAGS_32_BIT_OFFSET;
    }
    endingPosition = curr_idx + (3 * BYTE);
    while (curr_idx < endingPosition) {
        elf_info.flags |= *(ptr + curr_idx);
        elf_info.flags <<= 8;
        curr_idx += BYTE;
    }
    elf_info.flags |= *(ptr + curr_idx);

    // Parsing elf header size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_ELF_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_ELF_HEADER_SIZE_32_BIT_OFFSET;
    }
    elf_info.elfHeaderSize |= *(ptr + curr_idx);
    elf_info.elfHeaderSize <<= 8;
    curr_idx += BYTE;
    elf_info.elfHeaderSize |= *(ptr + curr_idx);

    // Swap byte order
    elf_info.elfHeaderSize = swap16(elf_info.elfHeaderSize);

    // Parsing program header size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_PROGRAM_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_PROGRAM_HEADER_SIZE_32_BIT_OFFSET;
    }
    elf_info.programHeaderSize |= *(ptr + curr_idx);
    elf_info.programHeaderSize <<= 8;
    curr_idx += BYTE;
    elf_info.programHeaderSize |= *(ptr + curr_idx);

    elf_info.programHeaderSize = swap16(elf_info.programHeaderSize);

    // Parsing program header's number of entries
    if (elf_info.bit_depth == 64) {
        curr_idx = E_PROGRAM_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET;
    } else {
        curr_idx = E_PROGRAM_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET;
    }
    elf_info.programHeaderEntriesNum |= *(ptr + curr_idx);
    elf_info.programHeaderEntriesNum <<= 8;
    curr_idx += BYTE;
    elf_info.programHeaderEntriesNum |= *(ptr + curr_idx);

    elf_info.programHeaderEntriesNum = swap16(elf_info.programHeaderEntriesNum);

    // Parsing section header's size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_SIZE_32_BIT_OFFSET;
    }
    elf_info.sectionHeaderSize |= *(ptr + curr_idx);
    elf_info.sectionHeaderSize <<= 8;
    curr_idx += BYTE;
    elf_info.sectionHeaderSize |= *(ptr + curr_idx);

    elf_info.sectionHeaderSize = swap16(elf_info.sectionHeaderSize);

    // Parsing section header's number of entries
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET;
    }
    elf_info.sectionHeaderEntriesNum |= *(ptr + curr_idx);
    elf_info.sectionHeaderEntriesNum <<= 8;
    curr_idx += BYTE;
    elf_info.sectionHeaderEntriesNum |= *(ptr + curr_idx);

    elf_info.sectionHeaderEntriesNum = swap16(elf_info.sectionHeaderEntriesNum);

    // Parsing section header index containing entry names
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_32_BIT_OFFSET;
    }

    elf_info.sectionHeaderIndex |= *(ptr + curr_idx);
    elf_info.sectionHeaderIndex <<= 8;
    curr_idx += BYTE;
    elf_info.sectionHeaderIndex |= *(ptr + curr_idx);

    elf_info.sectionHeaderIndex = swap16(elf_info.sectionHeaderIndex);

    // THE END
}

