#ifndef INST_H
#define INST_H

enum inst_record {
    INST_LW = 0,
    INST_LWSP = 1,
    INST_LBU = 2,
    INST_LB = 3,
    INST_SW = 4,
    INST_SWSP = 5,
    INST_SB = 6,
    INST_MOV = 7,
    INST_ADD = 8,
    INST_SUB = 9,
    INST_AND = 10,
    INST_OR = 11,
    INST_XOR = 12,
    INST_LSL = 13,
    INST_LSR = 14,
    INST_ASR = 15,
    INST_CMP = 16,
    INST_LI = 17,
    INST_ADDI = 18,
    INST_CMPI = 19,
    INST_J = 20,
    INST_JAL = 21,
    INST_JALR = 22,
    INST_JR = 23,
    INST_JL = 24,
    INST_JLE = 25,
    INST_JE = 26,
    INST_JNE = 27,
    INST_JB = 28,
    INST_JBE = 29,
    INST_NOP = 30
};

char *inst_bitpat[] = {
    "0b1011_0010_xxxx_xxxx", //LW
    "0b1010_xxxx_xxxx_xxxx", //LWSP
    "0b1011_1010_xxxx_xxxx", //LBU
    "0b1011_1110_xxxx_xxxx", //LB
    "0b1001_0010_xxxx_xxxx", //SW
    "0b1000_xxxx_xxxx_xxxx", //SWSP
    "0b1001_1010_xxxx_xxxx", //SB
    "0b1100_0000_xxxx_xxxx", //MOV
    "0b1110_0010_xxxx_xxxx", //ADD
    "0b1110_0011_xxxx_xxxx", //SUB
    "0b1110_0100_xxxx_xxxx", //AND
    "0b1110_0101_xxxx_xxxx", //OR
    "0b1110_0110_xxxx_xxxx", //XOR
    "0b1110_1001_xxxx_xxxx", //LSL
    "0b1110_1010_xxxx_xxxx", //LSR
    "0b1110_1101_xxxx_xxxx", //ASR
    "0b1100_0011_xxxx_xxxx", //CMP
    "0b0111_1000_xxxx_xxxx", //LI
    "0b1111_0010_xxxx_xxxx", //ADDI
    "0b1101_0011_xxxx_xxxx", //CMPI
    "0b0101_0010_0000_0000", //J
    "0b0111_0011_0000_0000", //JAL
    "0b0110_0001_xxxx_0000", //JALR
    "0b0100_0000_xxxx_0000", //JR
    "0b0100_0100_0xxx_xxxx", //JL
    "0b0100_0100_1xxx_xxxx", //JLE
    "0b0100_0101_0xxx_xxxx", //JE
    "0b0100_0101_1xxx_xxxx", //JNE
    "0b0100_0110_0xxx_xxxx", //JB
    "0b0100_0110_1xxx_xxxx", //JBE
    "0b0000_0000_0000_0000" //NOP
};
#endif
