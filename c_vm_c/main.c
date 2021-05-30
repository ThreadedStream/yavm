#include <stddef.h>
#include "elf.h"


int main() {
    MEMORY_BANK mem_bank = initMemoryBank();
    readObjectFile("/home/glasser/toys/yavm/objs/main.o", &mem_bank);

    ELF_HEADER elf_info = parseElfHeader(&mem_bank);
    PROGRAM_HEADER ph_data = parseProgramHeader(&mem_bank, elf_info.bit_depth);

    printElfData(&elf_info);
    printProgramHeaderData(&ph_data);

    destroyMemoryBank(&mem_bank);
//    ELF_INFO info = {};
//    SET_VALUE(infoAddr, offsetof(ELF_INFO, abiOs), uint8_t, 5);
    return 0;
}
