#include "elf.h"


int main() {
    readObjectFile("/home/glasser/toys/yavm/objs/main.o");

    parseElfHeader();
    return 0;
}
