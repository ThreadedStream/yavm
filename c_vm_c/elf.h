#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#define SET_VALUE(destination, offset, type, value) *((type*)(destination + offset)) = value

#define FORCE_INLINE __attribute((always_inline)) inline

#define BYTE sizeof(uint8_t)

// TODO: add support for x86 architecture

typedef struct {
    uint8_t *elf_ptr;
    uint8_t *ph_ptr;
    uint8_t *sh_ptr;
} MEMORY_BANK;

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
} ELF_HEADER;

typedef struct __attribute__((packed, aligned(8))){
    uint32_t ph_type; // 4
    uint32_t ph_flags; // 4
    uint32_t ph_segmentFlags; // 4
    uint64_t ph_fileImageSegmentOffset64; // 8
    uint64_t ph_segmentVAddr64; // 8
    uint64_t ph_segmentPhysAddr64; // 8
    uint64_t ph_segmentSizeInFileImage64; // 8
    uint64_t ph_segmentSizeInMemory64; // 8
    uint64_t ph_align64; // 8
    uint8_t padding[4];
} PROGRAM_HEADER;


typedef struct {
    uint32_t sh_offsetToNameString;
    uint32_t sh_type;
    union {
        uint32_t sh_flags32;
        uint64_t sh_flags64;
    };
    union {
        uint32_t sh_vAddr32;
        uint64_t sh_vAddr64;
    };
    union {
        uint32_t sh_fileImageSize32;
        uint64_t sh_fileImageSize64;
    };
    uint32_t sh_sectionIndex;
    uint32_t sh_info;
    union {
        uint32_t sh_addrAlignment32;
        uint64_t sh_addrAlignment64;
    };
    union {
        uint32_t sh_entrySize32;
        uint64_t sh_entrySize64;
    };
} SECTION_HEADER;


//Header sizes
enum {
    ELF_HEADER_MAX_SIZE = 0x40,
    PROGRAM_HEADER_MAX_SIZE = 0x38,
    SECTION_HEADER_MAX_SIZE = 0x40
};

// ELF header offsets
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

// Program header offsets
enum {
    P_TYPE_OFFSET = 0x0,
    P_FLAGS_OFFSET_64_BIT = 0x4, // only available for 64-bit architectures
    P_FILE_IMAGE_OFFSET_32_BIT = 0x4,
    P_FILE_IMAGE_OFFSET_64_BIT = 0x8,
    P_VIRTUAL_ADDRESS_OFFSET_32_BIT = 0x8,
    P_VIRTUAL_ADDRESS_OFFSET_64_BIT = 0x10,
    P_PHYS_ADDRESS_OFFSET_32_BIT = 0xC,
    P_PHYS_ADDRESS_OFFSET_64_BIT = 0x18,
    P_FILE_SIZE_OFFSET_32_BIT = 0x10,
    P_FILE_SIZE_OFFSET_64_BIT = 0x20,
    P_MEMORY_SIZE_OFFSET_32_BIT = 0x14,
    P_MEMORY_SIZE_OFFSET_64_BIT = 0x28,
    P_FLAGS_OFFSET_32_BIT = 0x18,
    P_ALIGN_OFFSET_32_BIT = 0x1C,
    P_ALIGN_OFFSET_64_BIT = 0x30,
    P_END_OF_PROGRAM_HEADER_32 = 0x20,
    P_END_OF_PROGRAM_HEADER_64 = 0x38
};

// Section header offsets
enum{
    SH_NAME_OFFSET = 0x0,
    SH_TYPE_OFFSET = 0x4,
    SH_FLAGS_OFFSET = 0x8,
    SH_ADDR_OFFSET_32 = 0xC,
    SH_ADDR_OFFSET_64 = 0x10,
    SH_OFFSET_OFFSET_32 = 0x10,
    SH_OFFSET_OFFSET_64 = 0x18,
    SH_SIZE_OFFSET_32 = 0x14,
    SH_SIZE_OFFSET_64 = 0x20,
    SH_LINK_OFFSET_32 = 0x18,
    SH_LINK_OFFSET_64 = 0x28,
    SH_INFO_OFFSET_32 = 0x1C,
    SH_INFO_OFFSET_64 = 0x2C,
    SH_ADDRALIGN_OFFSET_32 = 0x20,
    SH_ADDRALIGN_OFFSET_64 = 030,
    SH_ENTSIZE_OFFSET_32 = 0x24,
    SH_ENTSIZE_OFFSET_64 = 0x38,
    END_OF_SECTION_HEADER_OFFSET_32 = 0x28,
    END_OF_SECTION_HEADER_OFFSET_64 = 0x40
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

// Segment type in program header
enum {
    PT_NULL = 0x0,
    PT_LOAD = 0x1,
    PT_DYNAMIC = 0x2,
    PT_INTERP = 0x3,
    PT_NOTE = 0x4,
    PT_SHLIB = 0x5,
    PT_PHDR = 0x6,
    PT_TLS = 0x7,
    PT_LOOS = 0x60000000,
    PT_HIOS = 0x6FFFFFFF,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7FFFFFFF
};

// Section header type
enum{
    SHT_NULL = 0x0,
    SHT_PROGBITS = 0x1,
    SHT_SYMTAB = 0x2,
    SHT_STRTAB = 0x3,
    SHT_RELA = 0x4,
    SHT_HASH = 0x5,
    SHT_DYNAMIC = 0x6,
    SHT_NOTE = 0x7,
    SHT_NOBITS = 0x8,
    SHT_REL = 0x9,
    SHT_SHLIB = 0x0A,
    SHT_DYNSYM = 0x0B,
    SHT_INIT_ARRAY = 0x0E,
    SHT_FINI_ARRAY = 0x0F,
    SHT_PREINIT_ARRAY = 0x10,
    SHT_GROUP = 0x11,
    SHT_SYMTAB_SHNDX = 0x12,
    SHT_NUM = 0x13,
    SHT_LOOS = 0x60000000 // OS-specific
};

// Section attributes
enum{
    SHF_WRITE = 0x2 >> 1, // 1
    SHF_ALLOC = 0x2, // 2
    SHF_EXECINSTR = 0x2 << 1, // 4
    SHF_MERGE = 0x2 << 3, // 16
    SHF_STRINGS = 0x2 << 4, // 32
    SHF_INFO_LINK = 0x2 << 5, // 64
    SHF_LINK_ORDER = 0x2 << 6, // 128
    SHF_OS_NONCONFORMING = 0x2 << 7, // 256
    SHF_GROUP = 0x2 << 8, // 512
    SHF_TLS = 0x2 << 9, // 1024
    SHF_MASKOS = 0x0ff00000, // OS-specific
    SHF_MASKPROC = 0xf0000000, // Processor-specific
    SHF_ORDERED = 0x4000000, // Special ordering requirement (Solaris)
    SHF_EXCLUDE = 0x8000000 // Section is excluded unless referenced or allocated (Solaris)
};

void readObjectFile(const char *path, MEMORY_BANK *mem_bank);

MEMORY_BANK initMemoryBank();

void destroyMemoryBank(MEMORY_BANK *mem_bank);

ELF_HEADER parseElfHeader(const MEMORY_BANK *mem_bank);

PROGRAM_HEADER parseProgramHeader(const MEMORY_BANK *mem_bank);

SECTION_HEADER parseSectionHeader(const MEMORY_BANK *mem_bank, uint16_t bit_depth);

void
storeData(const uint8_t *elf_ptr, void *destination, uint16_t destinationMemberOffset, uint16_t elfOffset32,
          uint16_t elfOffset64,
          uint8_t sizeDependent, uint8_t bytes);

const char *stringifyAbiOs(uint16_t abiOs);

const char *stringifyIsa(uint16_t isa);

const char *stringifyElfType(uint16_t type);

const char *stringifyProgramHeaderType(uint32_t type);

const char *stringifySectionHeaderType(uint32_t type);

void
printElfData(const ELF_HEADER *elf_info);

void
printProgramHeaderData(const PROGRAM_HEADER *programHeaderData);

void
printSectionHeaderData(const SECTION_HEADER *sectionHeader);


FORCE_INLINE uint16_t
swap16(uint16_t value);

FORCE_INLINE uint32_t
swap32(uint32_t value);

FORCE_INLINE uint64_t
swap64(uint64_t value);

