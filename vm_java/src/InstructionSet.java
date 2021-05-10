public final class InstructionSet {
    public static final int OP_BR = 0;  // Branch
    public static final int OP_ADD = 1; // Add
    public static final int OP_LD = 2;  // Load
    public static final int OP_ST = 3;  // Store
    public static final int OP_JSR = 4; // Jump
    public static final int OP_AND = 5; // And
    public static final int OP_LDR = 6; // Load register
    public static final int OP_STR = 7; // Store register
    public static final int OP_RTI = 8;
    public static final int OP_NOT = 9;  //Not
    public static final int OP_LDI = 10; //Indirect load
    public static final int OP_STI = 11; //Indirect store
    public static final int OP_JMP = 12; // Jump
    public static final int OP_RES = 13; // Reserved
    public static final int OP_LEA = 14; //Load effective address
    public static final int OP_TRAP = 15; // Trap
}
