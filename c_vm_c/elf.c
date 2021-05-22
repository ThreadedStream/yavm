#include "elf.h"


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
    elf_info.type |= *(ptr+curr_idx);
    elf_info.type <<= 8;
    elf_info.type |= *(ptr+curr_idx + BYTE);

    //Parsing isa
    curr_idx = E_ISA_OFFSET;
    elf_info.isa |= *(ptr+curr_idx);
    elf_info.isa <<=8;
    elf_info.isa |= *(ptr+curr_idx + BYTE);

    curr_idx = E_VERSION_OFFSET;
    // Skip this step for now

    //TODO: Write a routine that extracts 4-8 bytes offsets from ptr

    // Parsing memory address of the entry point
    curr_idx = E_ENTRY_OFFSET;
    elf_info.entryPointOffset = *(ptr + curr_idx);

    // Parsing program header table offset
    if (elf_info.bit_depth == 64) {
        curr_idx = E_PROGRAM_HEADER_64_BIT_OFFSET;

    } else {
        curr_idx = E_PROGRAM_HEADER_32_BIT_OFFSET;
    }
    elf_info.programHeaderOffset = *(ptr + curr_idx);

    // Parsing section header offset
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_32_BIT_OFFSET;
    }
    elf_info.sectionHeaderOffset = *(ptr + curr_idx);

    // Parsing flags
    if (elf_info.bit_depth == 64) {
        curr_idx = E_FLAGS_64_BIT_OFFSET;
    } else {
        curr_idx = E_FLAGS_32_BIT_OFFSET;
    }
    elf_info.flags = *(ptr + curr_idx);

    // Parsing elf header size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_ELF_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_ELF_HEADER_SIZE_32_BIT_OFFSET;
    }

    elf_info.elfHeaderSize = *(ptr + curr_idx);

    // Parsing program header size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_PROGRAM_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_PROGRAM_HEADER_SIZE_32_BIT_OFFSET;
    }
    elf_info.programHeaderSize = *(ptr + curr_idx);

    // Parsing program header's number of entries
    if (elf_info.bit_depth == 64) {
        curr_idx = E_PROGRAM_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET;
    } else {
        curr_idx = E_PROGRAM_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET;
    }
    elf_info.programHeaderEntriesNum = *(ptr + curr_idx);

    // Parsing section header's size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_SIZE_32_BIT_OFFSET;
    }

    elf_info.sectionHeaderSize = *(ptr + curr_idx);


    // Parsing section header's number of entries
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET;
    }

    elf_info.sectionHeaderEntriesNum = *(ptr + curr_idx);

    // Parsing section header index containing entry names
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_32_BIT_OFFSET;
    }

    elf_info.sectionHeaderEntriesNum = *(ptr + curr_idx);
}

