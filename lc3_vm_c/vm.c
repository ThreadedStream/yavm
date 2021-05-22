#include "vm.h"


uint16_t signExtend(uint16_t x, int bit_count) {
    if ((x >> (bit_count - 1)) & 0x1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

uint16_t checkKey() {
    static fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(STDIN_FILENO, &fdSet);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &fdSet, NULL, NULL, &timeout) != 0;
}

uint16_t toLittleEndian16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

void disableInputBuffering() {
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new = original_tio;
    new.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
}

void restoreInputBuffering() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handleInterrupt() {
    restoreInputBuffering();
    printf("\n");
    exit(-2);
}

void setup() {
    signal(SIGINT, handleInterrupt);
    disableInputBuffering();
}

uint16_t memoryRead(uint16_t address) {
    if (address == MR_KBSR) {
        static fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        return select(1, &readfds, NULL, NULL, &timeout) ? STATUS_BIT : 0;
    } else if (address == MR_KBDR) {
        if (memoryRead(MR_KBSR)) {
            return getchar();
        } else {
            return 0;
        }
    } else if (address == MR_DSR) {
        return STATUS_BIT;
    } else if (address == MR_DDR) {
        return 0;
    }

    return memory[address];
}

void memoryWrite(uint16_t address, uint16_t value) {
    memory[address] = value;
}

void readImageFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("Wrong path");
        exit(-1);
    }
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = toLittleEndian16(origin);

    uint16_t maxRead = UINT16_MAX - origin;
    uint16_t *ptr = memory + origin;
    uint32_t read = fread(ptr, sizeof(uint16_t), maxRead, file);
    while (read-- > 0) {
        *ptr = toLittleEndian16(*ptr);
        ++ptr;
    }
    fclose(file);
}

void emulate() {
    const int startAddr = 0x3000;
    registers[R_PC] = startAddr;
    running = 1;
    while (running) {
        uint16_t instruction = memoryRead(registers[R_PC]++);
        uint16_t op = instruction >> 12;
        switch (op) {
            case OP_ADD:
                add(instruction);
                break;
            case OP_AND:
                and(instruction);
                break;
            case OP_BR:
                br(instruction);
                break;
            case OP_JMP:
                jmp(instruction);
                break;
            case OP_JSR:
                jsr(instruction);
                break;
            case OP_LD:
                ld(instruction);
                break;
            case OP_LDI:
                ldi(instruction);
                break;
            case OP_LDR:
                ldr(instruction);
                break;
            case OP_LEA:
                lea(instruction);
                break;
            case OP_NOT:
                not(instruction);
                break;
            case OP_ST:
                st(instruction);
                break;
            case OP_STI:
                sti(instruction);
                break;
            case OP_STR:
                str(instruction);
                break;
            case OP_TRAP:
                trap(instruction);
        }
    }


}

void updateFlags(uint16_t reg) {
    if (registers[reg] == 0) {
        registers[R_COND] = FL_ZR;
    } else if ((registers[reg] >> 15) & 0x1) {
        registers[R_COND] = FL_NEG;
    } else {
        registers[R_COND] = FL_POS;
    }
}

//Add instruction
/*
    Two cases:
    First:
        ===========================================================
        |0xF...0xC| 0xB...0x9|0x8...0x6|  0x5 |0x4...0x3|0x2...0x0|
        |   0001  |    DR    |   SR1   |   0  |    00   |  SR2    |
        ===========================================================
   Second:
        =================================================
        |0xF...0xC| 0xB...0x9|0x8...0x6|  0x5 |0x4...0x0|
        |   0001  |    DR    |   SR1   |   1  |    imm5 |
        =================================================
*/
void add(uint16_t instruction) {
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t sr1 = (instruction >> 6) & 0x7;

    //Checking imm flag
    if ((instruction >> 5) & 0x1) {
        uint16_t imm = signExtend(instruction & 0x1F, 5);
        registers[dr] = registers[sr1] + imm;
    } else {
        uint16_t sr2 = instruction & 0x7;
        registers[dr] = registers[sr1] + registers[sr2];
    }
    updateFlags(dr);
}


void ld(uint16_t instruction) {
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instruction & 0x1FF, 9);
    registers[dr] = memoryRead(registers[R_PC] + pcOffset);
    updateFlags(dr);
}

void ldi(uint16_t instruction) {
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instruction & 0x1FF, 9);
    registers[dr] = memoryRead(memoryRead(registers[R_PC] + pcOffset));
}

void ldr(uint16_t instruction) {
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t baseR = (instruction >> 6) & 0x7;
    uint16_t pcOffset = signExtend(instruction & 0x3F, 6);
    registers[dr] = memoryRead(registers[baseR] + pcOffset);
    updateFlags(dr);
}

void st(uint16_t instruction) {
    uint16_t sr = (instruction >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instruction & 0x1FF, 9);
    memoryWrite(registers[R_PC] + pcOffset, registers[sr]);
}

void sti(uint16_t instruction) {
    uint16_t sr = (instruction >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instruction & 0x1FF, 9);
    memoryWrite(memoryRead(registers[R_PC] + pcOffset), registers[sr]);
}

// 0x0000 0000 0011 1111
void str(uint16_t instruction) {
    uint16_t sr = (instruction >> 9) & 0x7;
    uint16_t baseR = (instruction >> 6) & 0x7;
    uint16_t pcOffset6 = signExtend(instruction & 0x3F, 6);
    memoryWrite(registers[baseR] + pcOffset6, registers[sr]);
}

void lea(uint16_t instruction) {
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t pcOffset = signExtend(instruction & 0x1FF, 9);
    registers[dr] = registers[R_PC] + pcOffset;
    updateFlags(dr);
}

void and(uint16_t instruction) {
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t sr1 = (instruction >> 6) & 0x7;
    if ((instruction >> 5) & 0x1) {
        uint16_t imm = signExtend(instruction & 0x1F, 5);
        registers[dr] = registers[sr1] & imm;
    } else {
        uint16_t sr2 = instruction & 0x7;
        registers[dr] = registers[sr1] & registers[sr2];
    }
}

void not(uint16_t instruction) {
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t sr = (instruction >> 6) & 0x7;
    registers[dr] = ~registers[sr];
    updateFlags(dr);
}

void br(uint16_t instruction) {
    uint16_t negBit = (instruction >> 0xB) & 0x1;
    uint16_t zeroBit = (instruction >> 0xA) & 0x1;
    uint16_t posBit = (instruction >> 0x9) & 0x1;
    uint16_t pcOffset = signExtend(instruction & 0x1FF, 9);

    if (negBit | zeroBit | posBit) {
        registers[R_PC] += pcOffset;
    }
}

void jmp(uint16_t instruction) {
    uint16_t baseR = (instruction >> 6) >> 0x7;
    registers[R_PC] = registers[baseR];
}

void jsr(uint16_t instruction) {
    uint16_t eleventhBit = (instruction >> 0xB) & 0x1;
    registers[R_R7] = registers[R_PC];
    if (eleventhBit) {
        uint16_t pcOffset = signExtend(instruction & 0x7FF, 11);
        registers[R_PC] += pcOffset;
    } else {
        uint16_t baseR = (instruction >> 6) & 0x7;
        registers[R_PC] = registers[baseR];
    }
}

void trap(uint16_t instruction) {
    switch (instruction & 0xFF) {
        case TRAP_GETC:
            trapGetc();
            break;
        case TRAP_IN:
            trapIn();
            break;
        case TRAP_OUT:
            trapOut();
            break;
        case TRAP_PUTSP:
            trapPutsp();
            break;
        case TRAP_PUTS:
            trapPuts();
            break;
        case TRAP_HALT:
            trapHalt();
            break;
    }
}

uint16_t zeroExtend(uint16_t x, int bit_count) {
    uint16_t factor = 16 - bit_count;
    x |= (x >> factor);
    return x;
}

void trapPuts() {
    uint16_t *ptr = memory + registers[R_R0];
    while (*ptr) {
        fputc((char) *ptr, stdout);
        ptr++;
    }
    fflush(stdout);
}

void trapGetc() {
    registers[R_R0] = (uint16_t) getchar();
}

void trapOut() {
    fputc((char) registers[R_R0], stdout);
    fflush(stdout);
}

void trapIn() {
    printf("Type in a character");
    char c = (char) getchar();
    fputc(c, stdout);
    fflush(stdout);
}

void trapPutsp() {
    uint16_t *ptr = memory + registers[R_R0];

    while (*ptr) {
        char c1 = *ptr & 0xFF;
        fputc(c1, stdout);
        char c2 = *ptr >> 8;
        if (c2) fputc(c2, stdout);
        ++ptr;
    }
    fflush(stdout);
}

void trapHalt() {
    puts("Halting...");
    fflush(stdout);
    running = 0;
}

