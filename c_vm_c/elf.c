#include "elf.h"

/*
    Produced by x86-64 gcc 11.1 with no optimization enabled
        push    rbp
        mov     rbp, rsp
        mov     eax, edi
        mov     WORD elf_ptr [rbp-4], ax
        movzx   eax, WORD elf_ptr [rbp-4]
        shr     ax, 8
        mov     edx, eax
        movzx   eax, WORD elf_ptr [rbp-4]
        sal     eax, 8
        or      eax, edx
        pop     rbp
        ret

    However, once one tunes optimization parameters, the above defined routine turns into the splendid assembly consisting of only 3 instructions.
    -O1, -O2, -O3
        mov     eax, edi
        rol     ax, 8
        ret
    It's not surprising the the following is nothing but a mere bit rotation

*/

// 0x4000 -> 0x0040, 0x3875 -> 0x7538

FORCE_INLINE uint16_t
swap16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

//0x10203040 -> 0x40302010
FORCE_INLINE uint32_t
swap32(uint32_t value) {
    return swap16(value >> 16) | (swap16(value) << 16);
}

// 0x1020304050607080 -> 0x8070605040302010
FORCE_INLINE uint64_t
swap64(uint64_t value) {
    return swap32(value >> 32) | ((uint64_t) swap32(value) << 32);
}

// TODO: Add support for storing data given arbitrary number of bytes
void storeData(const uint8_t *ptr, void *destination, uint16_t destinationMemberOffset, uint16_t elfOffset32,
               uint16_t elfOffset64,
               uint8_t sizeDependent, uint8_t bitDepth) {
    uint16_t curr_idx;
    // Indirectly fetch bit depth. May be a bug-prone endeavour
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
            SET_VALUE(destination, destinationMemberOffset, uint64_t, value64);
        }
    }
}


MEMORY_BANK initMemoryBank() {
    MEMORY_BANK mem_bank = {};
    mem_bank.elf_ptr = calloc(ELF_HEADER_MAX_SIZE, sizeof(uint8_t));
    mem_bank.ph_ptr = calloc(PROGRAM_HEADER_MAX_SIZE, sizeof(uint8_t));
    mem_bank.sh_ptr = calloc(SECTION_HEADER_MAX_SIZE, sizeof(uint8_t));
    return mem_bank;
}

void destroyMemoryBank(MEMORY_BANK *mem_bank) {
    free(mem_bank->elf_ptr);
    mem_bank->elf_ptr = NULL;
    free(mem_bank->ph_ptr);
    mem_bank->ph_ptr = NULL;
    free(mem_bank->sh_ptr);
    mem_bank->sh_ptr = NULL;
}


void readObjectFile(const char *path, MEMORY_BANK *mem_bank) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Wrong path");
        exit(-1);
    }


    uint32_t read = fread(mem_bank->elf_ptr, sizeof(uint8_t), ELF_HEADER_MAX_SIZE, file);
    if (!read) {
        fprintf(stderr, "Failed to carry out read operation on elf header's contents");
        exit(-1);
    }

    read = fread(mem_bank->ph_ptr, sizeof(uint8_t), PROGRAM_HEADER_MAX_SIZE, file);
    if (!read) {
        fprintf(stderr, "Failed to carry out read operation on program header's contents");
        exit(-1);
    }

    read = fread(mem_bank->sh_ptr, sizeof(uint8_t), SECTION_HEADER_MAX_SIZE, file);
    if (!read) {
        fprintf(stderr, "Failed to carry out read operation on section header's contents");
        exit(-1);
    }

    fclose(file);
}


ELF_HEADER parseElfHeader(const MEMORY_BANK *mem_bank) {
    // "'Who in the world am I?' Ah, that's the great puzzle!"

    ELF_HEADER elf_info = {};
    uint8_t *elf_ptr = mem_bank->elf_ptr;

    // First, parse first 4 bytes signifying a magic number
    // I'm intentionally writing the code in more verbose way, so i do not forget anything if look into it after some while
    uint16_t curr_idx = E_MAGIC_NUMBER_OFFSET;
    uint16_t endingPosition = 0;
    if (*(elf_ptr + curr_idx) != 0x7f && *(elf_ptr + curr_idx + 1) != 45 && *(elf_ptr + curr_idx + 2) != 0x4c &&
        *(elf_ptr + curr_idx + 3) != 46) {
        // It's not an ELF format
        fprintf(stderr, "Incorrect magic number");
        exit(-1);
    }

    //Parsing bit depth (0x1 -> 32-bit or 0x2 -> 64-bit)
    curr_idx = E_BIT_DEPTH_OFFSET;

    switch (*(elf_ptr + curr_idx)) {
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
    switch (*(elf_ptr + curr_idx)) {
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
    if (*(elf_ptr + curr_idx) == 0x1) {
        elf_info.isElfOriginalVersion = 1;
    } else {
        // It may be considered as a bug, since there may be any other value
        elf_info.isElfOriginalVersion = 0;
    }

    curr_idx = E_OS_ABI_OFFSET;
    if (*(elf_ptr + curr_idx) < 0x0 || *(elf_ptr + curr_idx) > 0x12) {
        fprintf(stderr, "Unknown abi os");
        exit(-1);
    }
    elf_info.abiOs = *(elf_ptr + curr_idx);

    //Parsing abi version
    curr_idx = E_ABI_VERSION_OFFSET;
    elf_info.abiVersion = *(elf_ptr + curr_idx);

    // Parsing reserved EI_PAD
    curr_idx = E_EI_PAD_OFFSET;
    endingPosition = curr_idx + (7 * BYTE);
    while (curr_idx < endingPosition) {
        if (*(elf_ptr + curr_idx) != 0x0) {
            fprintf(stderr, "EI_PAD should be zero");
            exit(-1);
        }
        curr_idx += BYTE;
    }

    // Parsing type
    curr_idx = E_TYPE_OFFSET;

    // for now, we take this value for granted, without checking its validity
    elf_info.type |= *(elf_ptr + curr_idx);
    elf_info.type <<= 8;
    elf_info.type |= *(elf_ptr + curr_idx + BYTE);

    elf_info.type = swap16(elf_info.type);

    // TODO: Put the logic below into inlined function
    //Parsing isa
    curr_idx = E_ISA_OFFSET;
    elf_info.isa |= *(elf_ptr + curr_idx);
    elf_info.isa <<= 8;
    elf_info.isa |= *(elf_ptr + curr_idx + BYTE);

    elf_info.isa = swap16(elf_info.isa);
    curr_idx = E_VERSION_OFFSET;
    // Skip this step for now

    // Parsing memory address of the entry point
    storeData(elf_ptr, (void *) &elf_info, offsetof(ELF_HEADER, entryPointOffset), E_ENTRY_OFFSET, E_ENTRY_OFFSET, 1,
              elf_info.bit_depth);

    // swap byte order
    elf_info.entryPointOffset = swap64(elf_info.entryPointOffset);
    // Parsing program header table offset
    storeData(elf_ptr, (void *) &elf_info, offsetof(ELF_HEADER, programHeaderOffset), E_PROGRAM_HEADER_32_BIT_OFFSET,
              E_PROGRAM_HEADER_64_BIT_OFFSET, 1, elf_info.bit_depth);

    // swap byte order
    elf_info.programHeaderOffset = swap64(elf_info.programHeaderOffset);
    // Parsing section header offset
    storeData(elf_ptr, (void *) &elf_info, offsetof(ELF_HEADER, sectionHeaderOffset), E_SECTION_HEADER_32_BIT_OFFSET,
              E_SECTION_HEADER_64_BIT_OFFSET, 1, elf_info.bit_depth);

    //swap byte order
    elf_info.sectionHeaderOffset = swap64(elf_info.sectionHeaderOffset);

    // Parsing flags
    if (elf_info.bit_depth == 64) {
        curr_idx = E_FLAGS_64_BIT_OFFSET;
    } else {
        curr_idx = E_FLAGS_32_BIT_OFFSET;
    }
    endingPosition = curr_idx + (3 * BYTE);
    while (curr_idx < endingPosition) {
        elf_info.flags |= *(elf_ptr + curr_idx);
        elf_info.flags <<= 8;
        curr_idx += BYTE;
    }
    elf_info.flags |= *(elf_ptr + curr_idx);

    // Parsing elf header size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_ELF_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_ELF_HEADER_SIZE_32_BIT_OFFSET;
    }
    elf_info.elfHeaderSize |= *(elf_ptr + curr_idx);
    elf_info.elfHeaderSize <<= 8;
    curr_idx += BYTE;
    elf_info.elfHeaderSize |= *(elf_ptr + curr_idx);

    // Swap byte order
    elf_info.elfHeaderSize = swap16(elf_info.elfHeaderSize);

    // Parsing program header size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_PROGRAM_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_PROGRAM_HEADER_SIZE_32_BIT_OFFSET;
    }
    elf_info.programHeaderSize |= *(elf_ptr + curr_idx);
    elf_info.programHeaderSize <<= 8;
    curr_idx += BYTE;
    elf_info.programHeaderSize |= *(elf_ptr + curr_idx);

    elf_info.programHeaderSize = swap16(elf_info.programHeaderSize);

    // Parsing program header's number of entries
    if (elf_info.bit_depth == 64) {
        curr_idx = E_PROGRAM_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET;
    } else {
        curr_idx = E_PROGRAM_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET;
    }
    elf_info.programHeaderEntriesNum |= *(elf_ptr + curr_idx);
    elf_info.programHeaderEntriesNum <<= 8;
    curr_idx += BYTE;
    elf_info.programHeaderEntriesNum |= *(elf_ptr + curr_idx);

    elf_info.programHeaderEntriesNum = swap16(elf_info.programHeaderEntriesNum);

    // Parsing section header's size
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_SIZE_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_SIZE_32_BIT_OFFSET;
    }
    elf_info.sectionHeaderSize |= *(elf_ptr + curr_idx);
    elf_info.sectionHeaderSize <<= 8;
    curr_idx += BYTE;
    elf_info.sectionHeaderSize |= *(elf_ptr + curr_idx);

    elf_info.sectionHeaderSize = swap16(elf_info.sectionHeaderSize);

    // Parsing section header's number of entries
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET;
    }
    elf_info.sectionHeaderEntriesNum |= *(elf_ptr + curr_idx);
    elf_info.sectionHeaderEntriesNum <<= 8;
    curr_idx += BYTE;
    elf_info.sectionHeaderEntriesNum |= *(elf_ptr + curr_idx);

    elf_info.sectionHeaderEntriesNum = swap16(elf_info.sectionHeaderEntriesNum);

    // Parsing section header index containing entry names
    if (elf_info.bit_depth == 64) {
        curr_idx = E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_64_BIT_OFFSET;
    } else {
        curr_idx = E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_32_BIT_OFFSET;
    }

    elf_info.sectionHeaderIndex |= *(elf_ptr + curr_idx);
    elf_info.sectionHeaderIndex <<= 8;
    curr_idx += BYTE;
    elf_info.sectionHeaderIndex |= *(elf_ptr + curr_idx);

    elf_info.sectionHeaderIndex = swap16(elf_info.sectionHeaderIndex);
    // THE END

    return elf_info;
}

PROGRAM_HEADER parseProgramHeader(const MEMORY_BANK *mem_bank) {

    PROGRAM_HEADER programHeader;

    uint8_t *ph_ptr = mem_bank->ph_ptr;

    // Parsing p_type
    uint16_t curr_idx = P_TYPE_OFFSET;
    uint16_t endingPosition = curr_idx + (3 * BYTE);
    while (curr_idx < endingPosition) {
        programHeader.ph_type |= *(ph_ptr + curr_idx);
        programHeader.ph_type <<= 8;
        curr_idx += BYTE;
    }
    programHeader.ph_type |= *(ph_ptr + BYTE);
    programHeader.ph_type = swap32(programHeader.ph_type);

    // Parsing x86-64 specific flags
#ifdef __x86_64__
    curr_idx = P_FLAGS_OFFSET_64_BIT;
    endingPosition = curr_idx + (3 * BYTE);
    while (curr_idx < endingPosition) {
        programHeader.ph_flags |= *(ph_ptr + curr_idx);
        programHeader.ph_flags <<= 8;
        curr_idx += BYTE;
    }
    programHeader.ph_flags |= *(ph_ptr + BYTE);
    programHeader.ph_flags = swap32(programHeader.ph_flags);
#else
    programHeader.ph_flags = 0;
#endif

#ifdef __x86_64__
    // Parsing file image segment offset
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_fileImageSegmentOffset64),
              P_FILE_IMAGE_OFFSET_32_BIT,
              P_FILE_IMAGE_OFFSET_64_BIT, 1, 64);
#else
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_fileImageSegmentOffset64),
              P_FILE_IMAGE_OFFSET_32_BIT,
              P_FILE_IMAGE_OFFSET_64_BIT, 1, 32);
#endif

#ifdef __x86_64__
    programHeader.ph_fileImageSegmentOffset64 = swap64(programHeader.ph_fileImageSegmentOffset64);
#else
    programHeader.ph_fileImageSegmentOffset64 = swap32(programHeader.ph_fileImageSegmentOffset64);
}
#endif

#ifdef __x86_64__
    // Parsing virtual address of a segment
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentVAddr64),
              P_VIRTUAL_ADDRESS_OFFSET_32_BIT,
              P_VIRTUAL_ADDRESS_OFFSET_64_BIT, 1, 64);
#else
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentVAddr64),
              P_VIRTUAL_ADDRESS_OFFSET_32_BIT,
              P_VIRTUAL_ADDRESS_OFFSET_64_BIT, 1, 32);
#endif

#ifdef __x86_64__
    programHeader.ph_segmentVAddr64 = swap64(programHeader.ph_segmentVAddr64);
#else
    programHeader.ph_segmentVAddr64 = swap32(programHeader.ph_segmentVAddr64);
}
#endif

#ifdef __x86_64__
    // Parsing physical address of a segment
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentPhysAddr64),
              P_PHYS_ADDRESS_OFFSET_32_BIT,
              P_PHYS_ADDRESS_OFFSET_64_BIT, 1, 64);
#else
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentPhysAddr64),
              P_PHYS_ADDRESS_OFFSET_32_BIT,
              P_PHYS_ADDRESS_OFFSET_64_BIT, 1, 32);
#endif


#ifdef __x86_64__
    programHeader.ph_segmentPhysAddr64 = swap64(programHeader.ph_segmentPhysAddr64);
#else
    programHeader.ph_segmentPhysAddr64 = swap32(programHeader.ph_segmentVAddr64);
#endif

#ifdef __x86_64__
    // Parsing segment size in file image
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentSizeInFileImage64),
              P_FILE_SIZE_OFFSET_32_BIT,
              P_FILE_SIZE_OFFSET_64_BIT, 1, 64);
#else
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentSizeInFileImage32),
              P_FILE_SIZE_OFFSET_32_BIT,
              P_FILE_SIZE_OFFSET_64_BIT, 1, 32);
#endif

#ifdef __x86_64__
    programHeader.ph_segmentSizeInFileImage64 = swap64(programHeader.ph_segmentSizeInFileImage64);
#else
    programHeader.ph_segmentSizeInFileImage64 = swap32(programHeader.ph_segmentSizeInFileImage32);
#endif

#ifdef __x86_64__
    // Parsing memory size in file image
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentSizeInMemory64),
              P_MEMORY_SIZE_OFFSET_32_BIT,
              P_MEMORY_SIZE_OFFSET_64_BIT, 1, 64);
#else
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_segmentSizeInMemory64),
              P_MEMORY_SIZE_OFFSET_32_BIT,
              P_MEMORY_SIZE_OFFSET_64_BIT, 1, 32);
#endif

#ifdef __x86_64__
    programHeader.ph_segmentSizeInMemory64 = swap64(programHeader.ph_segmentSizeInMemory64);
#else
    programHeader.ph_segmentSizeInMemory64 = swap32(programHeader.ph_segmentSizeInMemory64);
#endif

#ifdef __x86_64__
    programHeader.ph_segmentFlags = 0;
#else
    // Parsing x86 specific segment flags
    curr_idx = P_FLAGS_OFFSET_32_BIT;
    endingPosition = curr_idx + (3 * BYTE);
    while (curr_idx < endingPosition) {
        programHeader.ph_segmentFlags |= *(ph_ptr + curr_idx);
        programHeader.ph_segmentFlags <<= 8;
        curr_idx += BYTE;
    }
    programHeader.ph_segmentFlags |= *(ph_ptr + BYTE);
    programHeader.ph_segmentFlags = swap32(programHeader.ph_segmentFlags);
#endif

#ifdef __x86_64__
    // Parsing alignment
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_align64),
              P_ALIGN_OFFSET_32_BIT,
              P_ALIGN_OFFSET_64_BIT, 1, 64);
#else
    storeData(ph_ptr, (void *) &programHeader, offsetof(PROGRAM_HEADER, ph_align64),
              P_ALIGN_OFFSET_32_BIT,
              P_ALIGN_OFFSET_64_BIT, 1, 32);
#endif

#ifdef __x86_64__
        programHeader.ph_align64 = swap64(programHeader.ph_align64);
#else
        programHeader.ph_align64 = swap32(programHeader.ph_align64);
#endif

    return programHeader;
}

SECTION_HEADER parseSectionHeader(const MEMORY_BANK *mem_bank, uint16_t bit_depth) {
    SECTION_HEADER sectionHeader;

    uint8_t *sh_ptr = mem_bank->sh_ptr;

    // Parsing p_type
    uint16_t curr_idx = SH_NAME_OFFSET;
    uint16_t endingPosition = curr_idx + (3 * BYTE);
    while (curr_idx < endingPosition) {
        sectionHeader.sh_offsetToNameString |= *(sh_ptr + curr_idx);
        sectionHeader.sh_offsetToNameString <<= 8;
        curr_idx += BYTE;
    }
    sectionHeader.sh_offsetToNameString |= *(sh_ptr + BYTE);
    sectionHeader.sh_offsetToNameString = swap32(sectionHeader.sh_offsetToNameString);

    curr_idx = SH_TYPE_OFFSET;
    endingPosition = curr_idx + (3 * BYTE);
    while (curr_idx < endingPosition) {
        sectionHeader.sh_type |= *(sh_ptr + curr_idx);
        sectionHeader.sh_type <<= 8;
        curr_idx += BYTE;
    }
    sectionHeader.sh_type |= *(sh_ptr + BYTE);
    sectionHeader.sh_type = swap32(sectionHeader.sh_type);

}


const char *stringifyAbiOs(uint16_t abiOs) {
    switch (abiOs) {
        case SystemV:
            return "System V (Unix)";
        case HP_UX:
            return "HP_UX";
        case NetBSD:
            return "NetBSD";
        case Linux:
            return "Linux";
        case GNU_HURD:
            return "GNU_HURD";
        case SOLARIS:
            return "SOLARIS";
        case AIX:
            return "AIX";
        case IRIX:
            return "IRIX";
        case FreeBSD:
            return "FreeBSD";
        case Tru64:
            return "Tru64";
        case NovellModesto:
            return "NovellModesto";
        case OpenBSD:
            return "OpenBSD";
        case OpenVMS:
            return "OpenVMS";
        case NonStopKernel:
            return "NonStopKernel";
        case AROS:
            return "AROS";
        case FenixOs:
            return "FenixOs";
        case CloudABI:
            return "CloudABI";
        case StratusTechnologiesOpenVOS:
            return "StratusTechnologiesOpenVOS";
        default:
            return "Undetermined";
    }
}

const char *stringifyIsa(uint16_t isa) {
    switch (isa) {
        case NoSpecificInstructionSet:
            return "NoSpecificInstructionSet";
        case AT_T_WE_32100:
            return "AT_T_WE_32100";
        case SPARC:
            return "SPARC";
        case X86:
            return "X86";
        case Motorola6800:
            return "Motorola6800";
        case Motorola8800:
            return "Motorola8800";
        case IntelMCU:
            return "IntelMCU";
        case Intel80860:
            return "Intel80860";
        case MIPS:
            return "MIPS";
        case IBM_System_370:
            return "IBM_System_370";
        case MIPS_RS3000_Little_Endian:
            return "MIPS_RS3000_Little_Endian";
        case HP_PA_RISC:
            return "HP_PA_RISC";
        case INTEL_80960:
            return "INTEL_80960";
        case POWERPC:
            return "POWERPC";
        case POWERPC64:
            return "POWERPC64";
        case S390:
            return "S390";
        case IBM_SPU:
            return "IBM_SPU";
        case NEC_V800:
            return "NEC_V800";
        case FujitsuFR20:
            return "FujitsuFR20";
        case TRW_RH_32:
            return "TRW_RH_32";
        case MotorolaRCE:
            return "MotorolaRCE";
        case ARM:
            return "ARM";
        case DigitalAlpha:
            return "DigitalAlpha";
        case SuperH:
            return "SuperH";
        case SPARC9:
            return "SPARC9";
        case SiemensTriCore:
            return "SiemensTriCore";
        case ArgonautRisc:
            return "ArgonautRisc";
        case HitachiH8_300:
            return "HitachiH8_300";
        case HitachiH8_300H:
            return "HitachiH8_300H";
        case HitachiH8S:
            return "HitachiH8S";
        case HitachiH8_500:
            return "HitachiH8_500";
        case IA64:
            return "IA64";
        case StanfordMIPS_X:
            return "StanfordMIPS_X";
        case MotorolaColdFire:
            return "MotorolaColdFire";
        case MotorolaM68HC12:
            return "MotorolaM68HC12";
        case FujitsuMMA:
            return "FujitsuMMA";
        case SiemensPCP:
            return "SiemensPCP";
        case SonyNCpuRisc:
            return "SonyNCpuRisc";
        case DensoNDR1:
            return "DensoNDR1";
        case MotorolaStarCore:
            return "MotorolaStarCore";
        case ToyotaME16:
            return "ToyotaME16";
        case STM_ST_100:
            return "STM_ST_100";
        case TinyJ:
            return "TinyJ";
        case AMD_X86_64:
            return "AMD_X86_64";
        case TMS320C6000:
            return "TMS320C6000";
        case ARM_64:
            return "ARM_64";
        case RISC_V:
            return "RISC_V";
        case BerkeleyPacketFilter:
            return "BerkeleyPacketFilter";
        case WDC_65C816:
            return "WDC_65C816";
        default:
            return "Undetermined";
    }
}

//
const char *stringifyElfType(uint16_t type) {
    switch (type) {
        case ET_REL:
            return "A relocatable file";
        case ET_EXEC:
            return "An executable file";
        case ET_DYN:
            return "A shared object";
        case ET_CORE:
            return "A core file";
        case ET_LOPROC:
        case ET_HIPROC:
        case ET_LOOS:
        case ET_HIOS:
            return "Processor-specific";
        default:
            return "An Unknown type";
    }
}

const char *stringifyProgramHeaderType(uint32_t type) {
    switch (type) {
        case PT_NULL:
            return "Program header table entry unused";
        case PT_LOAD:
            return "Loadable segment";
        case PT_DYNAMIC:
            return "Dynamic linking information";
        case PT_INTERP:
            return "Interpreter information";
        case PT_NOTE:
            return "Auxiliary information";
        case PT_SHLIB:
            return "Reserved";
        case PT_PHDR:
            return "Program header";
        case PT_TLS:
            return "Thread-Local Storage template";
        case PT_LOOS:
        case PT_HIOS:
        case PT_LOPROC:
        case PT_HIPROC:
            return "Processor-specific";
        default:
            return "Undetermined";
    }
}

const char *stringifySectionHeaderType(uint32_t type) {
    switch (type) {
        case SHT_NULL:
            return "Section header table entry unused";
        case SHT_PROGBITS:
            return "Program data";
        case SHT_SYMTAB:
            return "Symbol table";
        case SHT_STRTAB:
            return "String table";
        default:
            return "Undetermined";
    }
}


void printElfData(const ELF_HEADER *elf_info) {
    printf("ELF header table\n");
    printf("Endianness: %s\n", elf_info->isLittleEndian ? "Little Endian" : "Big Endian");
    printf("Is ELF original version: %s\n", elf_info->isElfOriginalVersion ? "true" : "false");
    printf("Abi os: %s\n", stringifyAbiOs(elf_info->abiOs));
    printf("Abi version: 0x%x\n", elf_info->abiVersion);
    printf("Bit depth: 0x%x\n", elf_info->bit_depth);
    printf("Type: %s\n", stringifyElfType(elf_info->type));
    printf("Isa: %s\n", stringifyIsa(elf_info->isa));
    printf("Elf header size: 0x%x\n", elf_info->elfHeaderSize);
    printf("Program header size: 0x%x\n", elf_info->programHeaderSize);
    printf("Number of program header entries: 0x%x\n", elf_info->programHeaderEntriesNum);
    printf("Section header size: 0x%x\n", elf_info->sectionHeaderSize);
    printf("Number of section header entries: 0x%x\n", elf_info->sectionHeaderEntriesNum);
    printf("Section header index: 0x%x\n", elf_info->sectionHeaderIndex);
    printf("Flags: 0x%x\n", elf_info->flags);
    printf("Entry point offset: 0x%" PRIx64 "\n", elf_info->entryPointOffset);
    printf("Program header offset: 0x%" PRIx64 "\n", elf_info->programHeaderOffset);
    printf("Section header offset: 0x%" PRIx64 "\n", elf_info->sectionHeaderOffset);
}

void printProgramHeaderData(const PROGRAM_HEADER *programHeader) {
    printf("\n\n");
    printf("Program header table information\n");
    printf("Type segment: %s\n", stringifyProgramHeaderType(programHeader->ph_type));
    printf("Segment-dependent flags(64 bit): 0x%x\n", programHeader->ph_flags);
    printf("Offset of the segment in the file image: 0x%" PRIx64 "\n", programHeader->ph_fileImageSegmentOffset64);
    printf("Virtual address of the segment in memory: 0x%"PRIx64 "\n", programHeader->ph_segmentVAddr64);
    printf("Physical address of the segment in memory: 0x%" PRIx64 "\n", programHeader->ph_segmentPhysAddr64);
    printf("File image size: 0x%" PRIx64 "\n", programHeader->ph_segmentSizeInFileImage64);
    printf("Size of segment in memory: 0x%" PRIx64 "\n", programHeader->ph_segmentSizeInMemory64);
    printf("Segment-dependent flags(32 bit): 0x%x\n", programHeader->ph_segmentFlags);
    printf("Alignment: 0x%" PRIx64 "\n", programHeader->ph_align64);
}


void printSectionHeaderData(const SECTION_HEADER *sectionHeader) {
    printf("\n\n");
    printf("Section header information\n");
    printf("Offset to header's name: %d", sectionHeader->sh_offsetToNameString);
    printf("Header's type: %d", sectionHeader->sh_type);
}


