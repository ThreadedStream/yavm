#pragma once

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#define assert_zero_extend(X, BIT_COUNT, EXPECTED) assert((EXPECTED) == zero_extend((X), (BIT_COUNT)))

static int running;

enum {
    MR_KBSR = 0xFE00,
    MR_KBDR = 0xFE02,
    MR_DSR  = 0xfe04,
    MR_DDR  = 0xfe06,
    MR_MCR  = 0xfffe,
    STATUS_BIT = 1 << 15,
};

enum {
    TRAP_GETC   = 0x20,
    TRAP_OUT    = 0x21,
    TRAP_PUTS   = 0x22,
    TRAP_IN     = 0x23,
    TRAP_PUTSP  = 0x24,
    TRAP_HALT   = 0x25,
};

enum {
    FL_POS = 0,
    FL_ZR,
    FL_NEG
};

enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};

enum {
    OP_BR = 0, // branch
    OP_ADD,  // add
    OP_LD,     // load
    OP_ST,     // store
    OP_JSR,    // jump register
    OP_AND,    // bitwise and
    OP_LDR,    // load register
    OP_STR,    // store register
    OP_RTI,    // privilege-mode instruction
    OP_NOT,    // bitwise not
    OP_LDI,    // indirect load
    OP_STI,    // indirect store
    OP_JMP,    // jump
    OP_RES,
    OP_LEA,    // load effective address
    OP_TRAP    // trap
};


static uint16_t memory[UINT16_MAX];
static uint16_t registers[R_COUNT];

void emulate();

uint16_t signExtend(uint16_t x, int bit_count);

uint16_t zeroExtend(uint16_t x, int bit_count);

void updateFlags(uint16_t reg);

uint16_t memoryRead(uint16_t address);

void memoryWrite(uint16_t address, uint16_t value);

uint16_t checkKey();

uint16_t toLittleEndian16(uint16_t x);

//instruction definition
void add(uint16_t instruction);

void ld(uint16_t instruction); // load instruction
void ldi(uint16_t instruction); // indirect load
void ldr(uint16_t instruction); //register load
void st(uint16_t instruction); //store
void sti(uint16_t instruction); //indirect store
void str(uint16_t instruction); //register store
void lea(uint16_t instruction); //load effective address
void and(uint16_t instruction); //bitwise and
void not(uint16_t instruction); //bitwise complement
void br(uint16_t instruction); //branch
void jmp(uint16_t instruction); //jump
void jsr(uint16_t instruction); //register jump
void trap(uint16_t instruction); // trap

static struct termios original_tio;

void disableInputBuffering();

void restoreInputBuffering();

void handleInterrupt();

void setup();


//trap procedures
void trapPuts();

void trapGetc();

void trapOut();

void trapIn();

void trapPutsp();

void trapHalt();

//file operations
void readImageFile(const char *path);