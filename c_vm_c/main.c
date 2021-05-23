#include <stddef.h>
#include "elf.h"


int main() {
    readObjectFile("/home/glasser/toys/yavm/objs/main.o");

    parseElfHeader();

//    ELF_INFO info = {};
//    SET_VALUE(infoAddr, offsetof(ELF_INFO, abiOs), uint8_t, 5);

    return 0;
}
