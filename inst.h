#ifndef INST_H
#define INST_H

enum inst_record {
    LW = 0,
    LWSP = 1,
    LBU = 2,
    LB = 3,
    SW = 4,
    SWSP = 5,
    SB = 6,
    MOV = 7,
    ADD = 8,
    SUB = 9,
    AND = 10,
    OR = 11,
    XOR = 12,
    LSL = 13,
    LSR = 14,
    ASR = 15,
    CMP = 16,
    LI = 17,
    ADDI = 18,
    CMPI = 19,
    J = 20,
    JAL = 21,
    JALR = 22,
    JR = 23,
    JL = 24,
    JLE = 25,
    JE = 26,
    JNE = 27,
    JB = 28,
    JBE = 29,
    NOP = 30
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
