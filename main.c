#include <stdio.h>
#include <stdint.h>

#define INST_ROM_SIZE 512
#define DATA_RAM_SIZE 512
struct cpu {
    uint16_t reg[16];
    uint16_t pc;
    uint8_t inst_rom[INST_ROM_SIZE];
    uint8_t data_ram[DATA_RAM_SIZE];
    uint8_t flag_sign;
    uint8_t flag_overflow;
    uint8_t flag_zero;
    uint8_t flag_carry;
};

void pc_update(struct cpu *c, uint16_t offset){
    c->pc += offset;
}

void reg_write(struct cpu *c, uint8_t reg_idx, uint16_t data){
    c->reg[reg_idx] = data;
}

uint16_t reg_read(struct cpu *c, uint8_t reg_idx){
    return c->reg[reg_idx];
}

void mem_write_b(struct cpu *c, uint16_t addr, uint8_t data){
    c->data_ram[addr] = data;
}

void mem_write_w(struct cpu *c, uint16_t addr, uint16_t data){
    c->data_ram[addr] = data&0xFF;
    c->data_ram[addr+1] = data>>8;
}

uint8_t mem_read_b(struct cpu *c, uint16_t addr){
    return c->data_ram[addr];
}

uint16_t mem_read_w(struct cpu *c, uint16_t addr){
    return c->data_ram[addr] + (c->data_ram[addr+1]<<8);
}

uint16_t rom_read_w(struct cpu *c){
    return c->inst_rom[c->pc] + (c->inst_rom[c->pc+1]<<8);
}

uint16_t get_bits(uint16_t t, int s, int e){
    int bit_len = e-s;
    uint32_t bit_mask = 1;
    for(int i=0;i<bit_len;i++){
        bit_mask = (bit_mask<<1)+1;
    }
    return (t>>s)&bit_mask;
}

uint16_t sign_ext(uint16_t t, uint8_t sign_bit){
    uint16_t sign_v = 0;
    for(int i=sign_bit;i<16;i++){
        sign_v |= (1<<i);
    }
    return t|sign_v;
}

uint8_t flag_zero(uint16_t res){
    return res == 0;
}

uint8_t flag_sign(uint16_t res){
    return get_bits(res, 15, 15);
}

uint8_t flag_overflow(uint16_t s1, uint16_t s2, uint16_t res){
    uint8_t s1_sign = get_bits(s1, 15, 15);
    uint8_t s2_sign = get_bits(s2, 15, 15);
    uint8_t res_sign = get_bits(res, 15, 15);
    return ((s1_sign^s2_sign) == 0)&((s2_sign^res_sign) == 1);
}

void init_cpu(struct cpu *c){
    for(int i=0;i<16;i++){
        c->reg[i] = 0;
    }
    for(int i=0;i<INST_ROM_SIZE;i++){
        c->inst_rom[i] = 0;
    }
    for(int i=0;i<DATA_RAM_SIZE;i++){
        c->data_ram[i] = 0;
    }
    c->pc = 0;

    c->flag_sign = 0;
    c->flag_overflow = 0;
    c->flag_zero = 0;
    c->flag_carry = 0;
}

int main(void){
    struct cpu cpu;
    init_cpu(&cpu);

    uint16_t inst = rom_read_w(&cpu);
    uint8_t rs = get_bits(inst, 4, 7);
    uint8_t rd = get_bits(inst, 0, 3);
    uint16_t s_data;
    uint16_t d_data;
    uint16_t imm = 0;
    switch(get_bits(inst, 14, 15)){
        case 0b00:
            //NOP
            pc_update(&cpu, 2);
            break;
        case 0b01:
            //J-Instruction
            break;
        case 0b10:
            //M-Instruction
            switch(get_bits(inst, 12, 13)){
                case 0b00:
                    //SWSP
                    imm = (get_bits(inst, 8, 11)<<5)+(rs<<1);
                    mem_write_w(&cpu, reg_read(&cpu, 1)+imm, reg_read(&cpu, rs));
                    pc_update(&cpu, 2);
                    break;
                case 0b01:
                    //SW,SB
                    pc_update(&cpu, 2);
                    imm = rom_read_w(&cpu);
                    if(get_bits(inst, 11, 11)){
                        //SW
                        mem_write_w(&cpu, reg_read(&cpu, rd)+imm, reg_read(&cpu, rs));
                    }else{
                        //SB
                        mem_write_b(&cpu, reg_read(&cpu, rd)+imm, reg_read(&cpu, rs)&0xFF);
                    }
                    pc_update(&cpu, 2);
                    break;
                case 0b10:
                    //LWSP
                    imm = (get_bits(inst, 4, 11)<<1);
                    reg_write(&cpu, rd, mem_read_w(&cpu, reg_read(&cpu, 1)+imm));
                    pc_update(&cpu, 2);
                    break;
                case 0b11:
                    //LW,LB[U]
                    pc_update(&cpu, 2);
                    imm = rom_read_w(&cpu);
                    switch(get_bits(inst, 10, 11)){
                        case 0b00:
                            //LW
                            reg_write(&cpu, rd, mem_read_w(&cpu, rs+imm));
                            break;
                        case 0b10:
                            //LBU
                            reg_write(&cpu, rd, mem_read_b(&cpu, rs+imm));
                            break;
                        case 0b11:
                            //LB
                            reg_write(&cpu, rd, sign_ext(mem_read_b(&cpu, rs+imm), 7));
                            break;
                    }
                    pc_update(&cpu, 2);
                    break;
            }
            break;
        case 0b11:
            //R-Instruction
            switch(get_bits(inst, 8, 11)){
                case 0b0000:
                    //MOV
                    reg_write(&cpu, rd, reg_read(&cpu, rs));
                    pc_update(&cpu, 2);
                    break;
                case 0b0010:
                    //ADD[I]
                    if(get_bits(inst, 12, 12) == 0){
                        //ADD
                        s_data = reg_read(&cpu, rs);
                        d_data = reg_read(&cpu, rd);
                        uint32_t res = s_data+d_data;
                        if(res > 0xFFFF){
                            cpu.flag_carry = 1;
                        }else{
                            cpu.flag_carry = 0;
                        }
                        reg_write(&cpu, rd, res&0xFFFF);
                    }else{
                        //ADDI
                        s_data = sign_ext(rs, 3);
                        d_data = reg_read(&cpu, rd);
                        uint32_t res = s_data+d_data;
                        if(res > 0xFFFF){
                            cpu.flag_carry = 1;
                        }else{
                            cpu.flag_carry = 0;
                        }
                        reg_write(&cpu, rd, res&0xFFFF);
                    }
                    pc_update(&cpu, 2);
                    break;
                case 0b0011:
                    //SUB,CMPI
                    if(get_bits(inst, 12, 12) == 0){
                        //SUB
                        s_data = (~reg_read(&cpu, rs))+1;
                        d_data = reg_read(&cpu, rd);
                        uint32_t res = s_data+d_data;
                        if(res > 0xFFFF){
                            cpu.flag_carry = 1;
                        }else{
                            cpu.flag_carry = 0;
                        }
                        reg_write(&cpu, rd, res&0xFFFF);
                    }else{
                        //CMPI
                        s_data = (~sign_ext(rs, 3))+1;
                        d_data = reg_read(&cpu, rd);
                        uint32_t res = s_data+d_data;
                        if(res > 0xFFFF){
                            cpu.flag_carry = 1;
                        }else{
                            cpu.flag_carry = 0;
                        }
                    }
                    pc_update(&cpu, 2);
                    break;
                case 0b0100:
                    //AND
                    reg_write(&cpu, rd, reg_read(&cpu, rd)&reg_read(&cpu, rs));
                    pc_update(&cpu, 2);
                    break;
                case 0b0101:
                    //OR
                    reg_write(&cpu, rd, reg_read(&cpu, rd)|reg_read(&cpu, rs));
                    pc_update(&cpu, 2);
                    break;
                case 0b0110:
                    //XOR
                    reg_write(&cpu, rd, reg_read(&cpu, rd)^reg_read(&cpu, rs));
                    pc_update(&cpu, 2);
                    break;
                case 0b1001:
                    //LSL
                    reg_write(&cpu, rd, reg_read(&cpu, rd) << rs);
                    pc_update(&cpu, 2);
                    break;
                case 0b1010:
                    //LSR
                    reg_write(&cpu, rd, reg_read(&cpu, rd) >> rs);
                    pc_update(&cpu, 2);
                    break;
                case 0b1100:
                    //ASR
                    reg_write(&cpu, rd, ((uint16_t)reg_read(&cpu, rd)) >> rs);
                    pc_update(&cpu, 2);
                    break;
            }
            break;
    }
    return 0;
}
