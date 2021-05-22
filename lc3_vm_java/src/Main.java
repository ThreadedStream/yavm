import java.io.*;

public class Main {
    private final static int PC_START = 0x3000; // Starting address
    private static boolean running;
    private static int UINT16_MAX = 65536;
    //65536 slots of 16-bit addressable memory
    static int[] memory = new int[UINT16_MAX];

    static int[] registers = new int[GPRegister.R_COUNT];

    // 1 1 1 1 1
    // 5 bits
    // 0x1111 1111 1111
    //      0x0000 0001 1111
    // OR   0x0000 0001 1111
    //    --------------------
    //      0x0000 0001 1111
    private static int signExtend(int x, int bit_count) {
        //Checking if the number is negative
        //For that matter, we shifting the value to the right by the factor of bit_count-1 bits
        if (((x >> (bit_count - 1)) & 1) == 1) {
            x |= (0xFFFF << bit_count);
        }
        return x;
    }

    // Update of condition flags
    private static void updateFlags(int reg) {
        if (registers[reg] == 0) {
            registers[GPRegister.R_COND] = ConditionFlags.FL_ZR;
        } else if (registers[reg] >> 15 == 1) {
            registers[GPRegister.R_COND] = ConditionFlags.FL_NEG;
        } else {
            registers[GPRegister.R_COND] = ConditionFlags.FL_POS;
        }
    }

    //Memory read utility function
    private static int memoryRead(int where) {
        //Populate it with io specific reads
        return memory[where];
    }

    //Memory write utility function
    private static void memoryWrite(int address, int value) {
        memory[address] = value;
    }

    private static int swap16(int x){
        return (x << 8) | (x >> 8);
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
    private static void addOp(int instruction) {
        int dr = (instruction >> 9) & 0x7;
        int sr1 = (instruction >> 6) & 0x7;
        int imm_flag = (instruction >> 5) & 0x1;

        if (imm_flag == 1) {
            int imm5 = signExtend(instruction & 0x1F, 5);
            registers[dr] = registers[sr1] + imm5;
        } else {
            int sr2 = instruction & 0x7;
            registers[dr] = registers[sr1] + registers[sr2];
        }
        updateFlags(dr);
    }

    private static void load(int instruction) {
        int dr = (instruction >> 9) & 0x7;
        int pcOffset = signExtend(instruction & 0x1FF, 9);
        registers[dr] = memoryRead(registers[GPRegister.R_PC] + pcOffset);
        updateFlags(dr);
    }

    // LDI instruction
    /*
            ===================================
            |0xF...0xC| 0xB...0x9|0x8 ... 0x0 |
            |   1010  |    DR    |  PcOffset  |
            ===================================
     */
    private static void loadIndirect(int instruction) {
        int dr = (instruction >> 9) & 0x7;
        int pcOffset = signExtend(instruction & 0x1FF, 9);
        registers[dr] = memoryRead(memoryRead(registers[GPRegister.R_PC] + pcOffset));
        updateFlags(dr);
    }

    // 0x000 0000 0011 1111
    private static void loadRegister(int instruction) {
        int dr = (instruction >> 9) & 0x7;
        int baseR = (instruction >> 6) & 0x7;
        int pcOffset = signExtend(instruction & 0x3F, 6);

        registers[dr] = memoryRead(baseR + pcOffset);
        updateFlags(dr);
    }

    private static void store(int instruction) {
        int sr = (instruction >> 9) & 0x7;
        int pcOffset = signExtend(instruction & 0x1FF, 9);
        memoryWrite(registers[GPRegister.R_PC] + pcOffset, registers[sr]);
    }

    private static void storeIndirect(int instruction) {
        int sr = (instruction >> 9) & 0x7;
        int pcOffset = signExtend(instruction & 0x1FF, 9);
        memoryWrite(memoryRead(GPRegister.R_PC + pcOffset), registers[sr]);
    }

    private static void storeRegister(int instruction) {
        int sr = (instruction >> 9) & 0x7;
        int baseR = (instruction >> 6) & 0x7;
        int pcOffset = signExtend(instruction & 0x1F, 5);
        memoryWrite(baseR + pcOffset, registers[sr]);
    }

    //0x0000 0001 1111 1111
    private static void loadEffectiveAddress(int instruction) {
        int dr = (instruction >> 9) & 0x7;
        int pcOffset = signExtend(instruction & 0x1FF, 9);
        registers[dr] = registers[GPRegister.R_PC] + pcOffset;
        updateFlags(dr);
    }

    private static void bitwiseAnd(int instruction) {
        int dr = (instruction >> 9) & 0x7;
        int sr1 = (instruction >> 6) & 0x7;
        int immFlag = (instruction >> 5) & 0x1;
        if (immFlag == 1) {
            int imm = signExtend(instruction & 0x1F, 5);
            registers[dr] = registers[sr1] & imm;
        } else {
            int sr2 = instruction & 0x7;
            registers[dr] = registers[sr1] & registers[sr2];
        }
        updateFlags(dr);
    }

    private static void bitwiseNot(int instruction) {
        int dr = (instruction >> 9) & 0x7;
        int sr = (instruction >> 6) & 0x7;
        registers[dr] = ~registers[sr];
        updateFlags(dr);
    }

    private static void branch(int instruction) {
        int negBit = (instruction >> 0xB) & 0x1;
        int zeroBit = (instruction >> 0xA) & 0x1;
        int posBit = (instruction >> 0x9) & 0x1;

        // 0x1111 1111 1111 1111
        if ((negBit | zeroBit | posBit) == 1) {
            registers[GPRegister.R_PC] += signExtend(instruction & 0x1FF, 9);
        }
    }

    private static void jump(int instruction) {
        int baseR = instruction >> 0x7;
        registers[GPRegister.R_PC] = registers[baseR];
    }

    // 0x1111 1111 1111 1111
    private static void jumpRegister(int instruction) {
        int eleventhBit = (instruction >> 0xB) & 0x1;
        registers[GPRegister.R_R7] = registers[GPRegister.R_PC];
        if (eleventhBit == 1) {
            int pcOffset = signExtend(instruction & 0x7FF, 11);
            registers[GPRegister.R_PC] += pcOffset;
        } else {
            int baseR = (instruction >> 6) & 0x7;
            registers[GPRegister.R_PC] = baseR;
        }
    }

    private static void puts() {
        int currentAddress = registers[GPRegister.R_R0];
        for (char c = (char) memoryRead(currentAddress); Character.isLetter(c); currentAddress++)
            System.out.print(c);
    }

    private static void getc() throws IOException {
        registers[GPRegister.R_R0] = System.in.read();
    }

    private static void out() {
        char c = (char) registers[GPRegister.R_R0];
        System.out.print(c);
    }

    private static void in() throws IOException {
        System.out.println("Type in a character");
        char c = (char) System.in.read();
        System.out.print(c);
        registers[GPRegister.R_R0] = c;
    }

    private static void putsp() {
        int currentAddress = registers[GPRegister.R_R0];
        for (char c = (char) memoryRead(currentAddress); Character.isLetter(c); currentAddress++) {
            int c1 = ((int) c) & 0xFF;
            System.out.print((char) c1);
            int c2 = ((int) c) >> 8;
            if (c2 > 0) System.out.print((char) c2);
        }
    }

    private static void halt() {
        System.out.print("Halting...");
        running = false;
    }

    private static void readImageFile(String path) throws IOException {
        DataInputStream dataInStream = new DataInputStream(
                new BufferedInputStream(new FileInputStream(path)));

        byte origin = dataInStream.readByte();
        origin = (byte) swap16(origin);

        int maxRead = UINT16_MAX - origin;
        dataInStream.readShort();
        byte[] data = new byte[UINT16_MAX];
        int readBytes = dataInStream.read(data);
    }

    public static void main(String... args) throws IOException {
        readImageFile("/home/glasser/toys/yavm/vm/src/rogue.obj");
        running = true;
        while (running) {
            int instruction = memoryRead(registers[GPRegister.R_PC]++);
            int operation = instruction >> 12;
            switch (operation) {
                case InstructionSet.OP_ADD:
                    // Handle addition
                    break;
                case InstructionSet.OP_AND:
                    //Handle bitwise and
                    break;
                case InstructionSet.OP_NOT:
                    //Handle bitwise not
                    break;
                case InstructionSet.OP_BR:
                    //Handle branch
                    break;
                case InstructionSet.OP_JSR:
                    //Handle jsr jump
                    break;
                case InstructionSet.OP_JMP:
                    //Handle jump
                    break;
                case InstructionSet.OP_LD:
                    //Handle load
                    break;
                case InstructionSet.OP_LDI:
                    //Handle indirect load
                    break;
                case InstructionSet.OP_LDR:
                    break;
                case InstructionSet.OP_LEA:
                    break;
                case InstructionSet.OP_ST:
                    break;
                case InstructionSet.OP_STR:
                    break;
                case InstructionSet.OP_STI:
                    break;
                case InstructionSet.OP_TRAP:
                    break;
                case InstructionSet.OP_RES:
                case InstructionSet.OP_RTI:
                default:
                    break;
            }
        }
    }
}
