#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#define SET_VALUE(destination, offset, type, value) *((type*)(destination + offset)) = value

#define BYTE sizeof(uint8_t)
// A very, very bad idea, i guess.
static uint8_t *ptr;

// 40 bytes for 32-bit architecture, 44 for 64-bit one (not accounting for a padding)
typedef struct __attribute__((packed, aligned(8))) {
    uint8_t isLittleEndian; // 1
    uint8_t isElfOriginalVersion; // 1
    uint8_t abiOs; // 1
    uint8_t abiVersion; // 1
    uint16_t bit_depth; // 2
    uint16_t type; // 2
    uint16_t isa; // 2
    uint16_t elfHeaderSize; // 2
    uint16_t entryHeaderSize; // 2
    uint16_t programHeaderSize; // 2
    uint16_t programHeaderEntriesNum; // 2
    uint16_t sectionHeaderSize; // 2
    uint16_t sectionHeaderEntriesNum; // 2
    uint16_t sectionHeaderIndex; // 2
    uint32_t flags; // 4
    uint64_t entryPointOffset; // 8
    uint64_t programHeaderOffset; // 8
    uint64_t sectionHeaderOffset; // 8
    uint32_t padding64[5]; //20 bytes padding, although compiler would do it anyway
//   uint32_t padding32[6];
} ELF_INFO;


//ELF header size
enum {
    ELF_HEADER_SIZE = 0x40
};

// ELF header constants
enum {
    E_MAGIC_NUMBER_OFFSET = 0x0,
    E_BIT_DEPTH_OFFSET = 0x4,
    E_ENDIANNESS_OFFSET = 0x5,
    E_ELF_VERSION_OFFSET = 0x06,
    E_OS_ABI_OFFSET = 0x07,
    E_ABI_VERSION_OFFSET = 0x08,
    E_EI_PAD_OFFSET = 0x09,
    E_TYPE_OFFSET = 0x10,
    E_ISA_OFFSET = 0x12,
    E_VERSION_OFFSET = 0x14,
    E_ENTRY_OFFSET = 0x18,
    E_PROGRAM_HEADER_32_BIT_OFFSET = 0x1C,
    E_PROGRAM_HEADER_64_BIT_OFFSET = 0x20,
    E_SECTION_HEADER_32_BIT_OFFSET = 0x20,
    E_SECTION_HEADER_64_BIT_OFFSET = 0x28,
    E_FLAGS_32_BIT_OFFSET = 0x24,
    E_FLAGS_64_BIT_OFFSET = 0x30,
    E_ELF_HEADER_SIZE_32_BIT_OFFSET = 0x28,
    E_ELF_HEADER_SIZE_64_BIT_OFFSET = 0x34,
    E_PROGRAM_HEADER_SIZE_32_BIT_OFFSET = 0x2A,
    E_PROGRAM_HEADER_SIZE_64_BIT_OFFSET = 0x36,
    E_PROGRAM_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET = 0x2C,
    E_PROGRAM_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET = 0x38,
    E_SECTION_HEADER_SIZE_32_BIT_OFFSET = 0x2E,
    E_SECTION_HEADER_SIZE_64_BIT_OFFSET = 0x3A,
    E_SECTION_HEADER_NUM_OF_ENTRIES_32_BIT_OFFSET = 0x30,
    E_SECTION_HEADER_NUM_OF_ENTRIES_64_BIT_OFFSET = 0x3c,
    E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_32_BIT_OFFSET = 0x32,
    E_SECTION_HEADER_ENTRY_INDEX_CONTAINING_NAMES_64_BIT_OFFSET = 0x3E,
    E_END_OF_ELF_HEADER_OFFSET_32_BIT = 0x34,
    E_END_OF_ELF_HEADER_OFFSET_64_BIT = 0x40,
};

// Object file types
enum {
    ET_NONE = 0x0,
    ET_REL = 0x1,
    ET_EXEC = 0x2,
    ET_DYN = 0x3,
    ET_CORE = 0x4,
    ET_LOOS = 0xFE00,
    ET_HIOS = 0xFEFF,
    ET_LOPROC = 0xFF00,
    ET_HIPROC = 0xFFFF
};


enum OS_ABI {
    SystemV = 0x0,
    HP_UX,
    NetBSD,
    Linux,
    GNU_HURD,
    SOLARIS,
    AIX,
    IRIX,
    FreeBSD,
    Tru64,
    NovellModesto,
    OpenBSD,
    OpenVMS,
    NonStopKernel,
    AROS,
    FenixOs,
    CloudABI,
    StratusTechnologiesOpenVOS
};

enum ISA {
    NoSpecificInstructionSet = 0x0,
    AT_T_WE_32100 = 0x1,
    SPARC = 0x2,
    X86 = 0x3,
    Motorola6800 = 0x4,
    Motorola8800 = 0x5,
    IntelMCU = 0x6,
    Intel80860 = 0x7,
    MIPS = 0x8,
    IBM_System_370 = 0x9,
    MIPS_RS3000_Little_Endian = 0xA,
    RESERVED_0 = 0xB,
    RESERVED_1 = 0xC,
    RESERVED_2 = 0xD,
    HP_PA_RISC = 0xE,
    RESERVED_3 = 0xF,
    INTEL_80960 = 0x13,
    POWERPC = 0x14,
    POWERPC64 = 0x15,
    S390 = 0x16,
    IBM_SPU = 0x17,
    RESERVED_4 = 0x18,
    RESERVED_5 = 0x19,
    RESERVED_6 = 0x20,
    RESERVED_7 = 0x21,
    RESERVED_8 = 0x22,
    RESERVED_9 = 0x23,
    NEC_V800 = 0x24,
    FujitsuFR20 = 0x25,
    TRW_RH_32 = 0x26,
    MotorolaRCE = 0x27,
    ARM = 0x28,
    DigitalAlpha = 0x29,
    SuperH = 0x2A,
    SPARC9 = 0x2B,
    SiemensTriCore = 0x2C,
    ArgonautRisc = 0x2D,
    HitachiH8_300 = 0x2E,
    HitachiH8_300H = 0x2F,
    HitachiH8S = 0x30,
    HitachiH8_500 = 0x31,
    IA64 = 0x32,
    StanfordMIPS_X = 0x33,
    MotorolaColdFire = 0x34,
    MotorolaM68HC12 = 0x35,
    FujitsuMMA = 0x36,
    SiemensPCP = 0x37,
    SonyNCpuRisc = 0x38,
    DensoNDR1 = 0x39,
    MotorolaStarCore = 0x3A,
    ToyotaME16 = 0x3B,
    STM_ST_100 = 0x3C,
    TinyJ = 0x3D,
    AMD_X86_64 = 0x3E,
    TMS320C6000 = 0x8C,
    ARM_64 = 0xB7,
    RISC_V = 0xF3,
    BerkeleyPacketFilter = 0xF7,
    WDC_65C816 = 0x101
};

void readObjectFile(const char *path);

void parseElfHeader();

void
storeData(void *destination, uint16_t destinationMemberOffset, uint16_t elfOffset32, uint16_t elfOffset64,
          uint8_t sizeDependent, uint8_t bytes);

__attribute__((always_inline)) uint16_t
swap16(uint16_t value);

__attribute__((always_inline)) uint32_t
swap32(uint32_t value);

__attribute__((always_inline)) uint64_t
swap64(uint64_t value);

