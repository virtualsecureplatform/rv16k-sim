#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "cpu.h"
#include "elf_parser.h"
#include "bitpat.h"
#include "inst.h"

#include <getopt.h>
#include <unistd.h>

extern int flag_quiet;

void print_usage(FILE *fh)
{
    fprintf(fh, "Usage: rv16k-sim [-q] [-m] [-t ROM] [-d RAM] [FILENAME] NCYCLES\n");
    fprintf(fh, "Options:\n");
    fprintf(fh, "  -q     : No log print\n");
    fprintf(fh, "  -m     : Dump memory\n");
    fprintf(fh, "  -t ROM : Initial ROM data\n");
    fprintf(fh, "  -d RAM : Initial RAM data\n");
}

_Noreturn void print_usage_to_exit(void)
{
    print_usage(stderr);
    exit(1);
}

void pc_update(struct cpu *c, uint16_t offset){
    c->pc += offset;
    log_printf("PC <= 0x%04X ", c->pc);
}

void pc_write(struct cpu *c, uint16_t addr){
    c->pc = addr;
    log_printf("PC <= 0x%04X ", c->pc);
}

uint16_t pc_read(struct cpu *c){
    return c->pc;
}

void reg_write(struct cpu *c, uint8_t reg_idx, uint16_t data){
    c->reg[reg_idx] = data;
    log_printf("Reg x%d <= 0x%04X ", reg_idx, data);
}

uint16_t reg_read(struct cpu *c, uint8_t reg_idx){
    return c->reg[reg_idx];
}

void mem_write_b(struct cpu *c, uint16_t addr, uint8_t data){
    log_printf("DataRam[0x%04X] <= 0x%04X ", addr, data);

    assert(addr < DATA_RAM_SIZE && "RAM write to invalid address!");

    c->data_ram[addr] = data;
}

void mem_write_w(struct cpu *c, uint16_t addr, uint16_t data){
    log_printf("DataRam[0x%04X] <= 0x%04X ", addr, data&0xFF);
    log_printf("DataRam[0x%04X] <= 0x%04X ", addr+1, data>>8);

    assert(addr < DATA_RAM_SIZE - 1 && "RAM write to invalid address!");
    c->data_ram[addr] = data&0xFF;
    c->data_ram[addr+1] = data>>8;
}

uint8_t mem_read_b(struct cpu *c, uint16_t addr){
    assert(addr < DATA_RAM_SIZE && "RAM read from invalid address!");
    return c->data_ram[addr];
}

uint16_t mem_read_w(struct cpu *c, uint16_t addr){
    assert(addr < DATA_RAM_SIZE - 1 && "RAM read from invalid address!");
    return c->data_ram[addr] + (c->data_ram[addr+1]<<8);
}

uint16_t rom_read_w(struct cpu *c){
    assert(c->pc < INST_ROM_SIZE - 1 && "ROM read from invalid address!");
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
    uint16_t sign = get_bits(t, sign_bit, sign_bit);
    for(int i=sign_bit;i<16;i++){
        sign_v |= (sign<<i);
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

void print_flags(struct cpu *c){
    log_printf("FLAGS(SZCV) <= %d%d%d%d ", c->flag_sign, c->flag_zero, c->flag_carry, c->flag_overflow);
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

void set_bytes_from_str(uint8_t *dst, const char * const src, int N)
{
    char *buf = (char *)malloc(strlen(src) + 1);
    strcpy(buf, src);

    int idst = 0;
    char *tp;
    tp = strtok(buf, " ");
    while (tp != NULL) {
        assert(idst < N && "Too large data!");
        uint8_t val = strtol(tp, NULL, 16);
        dst[idst++] = val;
        tp = strtok(NULL, " ");
    }

    free(buf);
}

void dump_memory(FILE *fh, uint8_t *mem, int size)
{
    for (int i = 0; i < size; i++) {
        fprintf(fh, "%02x ", mem[i]);
        if (i % 16 == 15)   fprintf(fh, "\n");
    }
}

int main(int argc, char *argv[]){
    struct cpu cpu;
    init_cpu(&cpu);

    int flag_load_elf = 1, flag_memory_dump = 0, opt;
    while((opt = getopt(argc, argv, "qmt:d:")) != -1) {
        switch(opt) {
            case 'q':
                flag_quiet = 1;
                break;

            case 'm':
                flag_memory_dump = 1;
                break;

            case 't':
                flag_load_elf = 0;
                set_bytes_from_str(cpu.inst_rom, optarg, INST_ROM_SIZE);
                break;

            case 'd':
                flag_load_elf = 0;
                set_bytes_from_str(cpu.data_ram, optarg, DATA_RAM_SIZE);
                break;

            default:
                print_usage_to_exit();
        }
    }

    if (optind >= argc) print_usage_to_exit();

    int iarg = optind;
    if (flag_load_elf)
        elf_parse(&cpu, argv[iarg++]);

    int ncycles = 0;
    if (iarg >= argc) print_usage_to_exit();
    ncycles = atoi(argv[iarg]);

    for(int i=0;i<ncycles;i++){
        uint16_t inst = rom_read_w(&cpu);
        uint8_t rs = get_bits(inst, 4, 7);
        uint8_t rd = get_bits(inst, 0, 3);
        uint16_t s_data;
        uint16_t d_data;
        uint16_t imm = 0;
        uint32_t res = 0;
        uint16_t res_w = 0;
        if(bitpat_match_s(inst, inst_bitpat[INST_NOP])){
            //NOP
            log_printf("Inst:NOP\t");
            pc_update(&cpu, 2);
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
            cpu.flag_carry = 0;
        }else if(bitpat_match_s(inst, inst_bitpat[INST_J])){
            log_printf("Inst:J\t");
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
            pc_update(&cpu, 2);
            imm = rom_read_w(&cpu);
            pc_update(&cpu, imm);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JAL])){
            log_printf("Inst:JAL\t");
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
            pc_update(&cpu, 2);
            reg_write(&cpu, 0, pc_read(&cpu)+2);
            imm = rom_read_w(&cpu);
            pc_update(&cpu, imm);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JALR])){
            log_printf("Inst:JALR\t");
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
            reg_write(&cpu, 0, pc_read(&cpu)+2);
            pc_write(&cpu, reg_read(&cpu, rs));
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JR])){
            log_printf("Inst:JR\t");
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
            pc_write(&cpu, reg_read(&cpu, rs));
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JL])){
            log_printf("Inst:JL\t");
            imm = sign_ext(get_bits(inst, 0, 6)<<1, 7) ;
            if(cpu.flag_sign != cpu.flag_overflow){
                pc_update(&cpu, imm);
            }else{
                pc_update(&cpu, 2);
            }
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JLE])){
            log_printf("Inst:JLE\t");
            imm = sign_ext(get_bits(inst, 0, 6)<<1, 7);
            if(cpu.flag_sign != cpu.flag_overflow || cpu.flag_zero == 1){
                pc_update(&cpu, imm);
            }else{
                pc_update(&cpu, 2);
            }
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JE])){
            log_printf("Inst:JE\t");
            imm = sign_ext(get_bits(inst, 0, 6)<<1, 7);
            if(cpu.flag_zero == 1){
                pc_update(&cpu, imm);
            }else{
                pc_update(&cpu, 2);
            }
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JNE])){
            log_printf("Inst:JNE\t");
            imm = sign_ext(get_bits(inst, 0, 6)<<1, 7);
            if(cpu.flag_zero == 0){
                pc_update(&cpu, imm);
            }else{
                pc_update(&cpu, 2);
            }
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JB])){
            log_printf("Inst:JB\t");
            imm = (sign_ext(get_bits(inst, 0, 6)<<1, 7));
            if(cpu.flag_carry == 1){
                pc_update(&cpu, imm);
            }else{
                pc_update(&cpu, 2);
            }
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
        }else if(bitpat_match_s(inst, inst_bitpat[INST_JBE])){
            log_printf("Inst:JBE\t");
            imm = sign_ext(get_bits(inst, 0, 6)<<1, 7);
            if(cpu.flag_carry == 1 || cpu.flag_zero == 1){
                pc_update(&cpu, imm);
            }else{
                pc_update(&cpu, 2);
            }
            cpu.flag_carry = 0;
            cpu.flag_sign = 0;
            cpu.flag_overflow = 0;
            cpu.flag_zero = 0;
        }else if(bitpat_match_s(inst, inst_bitpat[INST_LI])){
            log_printf("Inst:LI\t");
            pc_update(&cpu, 2);
            imm = rom_read_w(&cpu);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(imm&0xFFFF);
            cpu.flag_overflow = 0;
            cpu.flag_zero = flag_zero(imm&0xFFFF);
            reg_write(&cpu, rd, imm);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_SWSP])){
            log_printf("Inst:SWSP\t");
            imm = (get_bits(inst, 8, 11)<<5)+(rd<<1);
            s_data = reg_read(&cpu, 1);
            res = s_data+imm;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(imm, s_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            mem_write_w(&cpu, res&0xFFFF, reg_read(&cpu, rs));
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_SW])){
            log_printf("Inst:SW\t");
            pc_update(&cpu, 2);
            imm = rom_read_w(&cpu);
            d_data = reg_read(&cpu, rd);
            res = imm+d_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(imm, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            mem_write_w(&cpu, res&0xFFFF, reg_read(&cpu, rs));
            pc_update(&cpu, 2);

        }else if(bitpat_match_s(inst, inst_bitpat[INST_SB])){
            log_printf("Inst:SB\t");
            pc_update(&cpu, 2);
            imm = rom_read_w(&cpu);
            d_data = reg_read(&cpu, rd);
            res = imm+d_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(imm, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            mem_write_b(&cpu, res&0xFFFF, reg_read(&cpu, rs)&0xFF);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_LWSP])){
            log_printf("Inst:LWSP\t");
            imm = (get_bits(inst, 4, 11)<<1);
            d_data = reg_read(&cpu, 1);
            res = imm+d_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(imm, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            reg_write(&cpu, rd, mem_read_w(&cpu, res&0xFFFF));
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_LW])){
            log_printf("Inst:LW\t");
            pc_update(&cpu, 2);
            imm = rom_read_w(&cpu);
            s_data = reg_read(&cpu, rs);
            res = imm+s_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(imm, s_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            reg_write(&cpu, rd, mem_read_w(&cpu, res&0xFFFF));
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_LB])){
            log_printf("Inst:LB\t");
            pc_update(&cpu, 2);
            imm = rom_read_w(&cpu);
            s_data = reg_read(&cpu, rs);
            res = imm+s_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(imm, s_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            reg_write(&cpu, rd, sign_ext(mem_read_b(&cpu, res&0xFFFF), 7));
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_LBU])){
            log_printf("Inst:LBU\t");
            pc_update(&cpu, 2);
            imm = rom_read_w(&cpu);
            s_data = reg_read(&cpu, rs);
            res = imm+s_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(imm, s_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            reg_write(&cpu, rd, mem_read_b(&cpu, res&0xFFFF));
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_MOV])){
            log_printf("Inst:MOV\t");
            s_data = reg_read(&cpu, rs);
            reg_write(&cpu, rd, s_data);
            pc_update(&cpu, 2);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(s_data&0xFFFF);
            cpu.flag_overflow = 0;
            cpu.flag_zero = flag_zero(s_data&0xFFFF);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_ADD])){
            log_printf("Inst:ADD\t");
            s_data = reg_read(&cpu, rs);
            d_data = reg_read(&cpu, rd);
            uint32_t res = s_data+d_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            reg_write(&cpu, rd, res&0xFFFF);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_ADDI])){
            log_printf("Inst:ADDI\t");
            s_data = sign_ext(rs, 3);
            d_data = reg_read(&cpu, rd);
            uint32_t res = s_data+d_data;
            if(res > 0xFFFF){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            reg_write(&cpu, rd, res&0xFFFF);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_SUB])){
            log_printf("Inst:SUB\t");
            s_data = (~reg_read(&cpu, rs))+1;
            d_data = reg_read(&cpu, rd);
            uint32_t res = s_data+d_data;
            if(res > 0xFFFF || s_data == 0){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            reg_write(&cpu, rd, res&0xFFFF);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_CMP])){
            log_printf("Inst:CMP\t");
            s_data = (~reg_read(&cpu, rs))+1;
            d_data = reg_read(&cpu, rd);
            uint32_t res = s_data+d_data;
            if(res > 0xFFFF || s_data == 0){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_CMPI])){
            log_printf("Inst:CMPI\t");
            s_data = (~sign_ext(rs, 3))+1;
            d_data = reg_read(&cpu, rd);
            uint32_t res = s_data+d_data;
            if(res > 0xFFFF || s_data == 0){
                cpu.flag_carry = 0;
            }else{
                cpu.flag_carry = 1;
            }
            cpu.flag_sign = flag_sign(res&0xFFFF);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res&0xFFFF);
            cpu.flag_zero = flag_zero(res&0xFFFF);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_AND])){
            log_printf("Inst:AND\t");
            s_data = reg_read(&cpu, rs);
            d_data = reg_read(&cpu, rd);
            res_w = s_data&d_data;
            reg_write(&cpu, rd, res_w);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(res_w);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res_w);
            cpu.flag_zero = flag_zero(res_w);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_OR])){
            log_printf("Inst:OR\t");
            s_data = reg_read(&cpu, rs);
            d_data = reg_read(&cpu, rd);
            res_w = s_data|d_data;
            reg_write(&cpu, rd, res_w);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(res_w);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res_w);
            cpu.flag_zero = flag_zero(res_w);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_XOR])){
            log_printf("Inst:XOR\t");
            s_data = reg_read(&cpu, rs);
            d_data = reg_read(&cpu, rd);
            res_w = s_data^d_data;
            reg_write(&cpu, rd, res_w);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(res_w);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res_w);
            cpu.flag_zero = flag_zero(res_w);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_LSL])){
            log_printf("Inst:LSL\t");
            s_data = reg_read(&cpu, rs);
            d_data = reg_read(&cpu, rd);
            res_w = d_data << s_data;
            reg_write(&cpu, rd, res_w);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(res_w);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res_w);
            cpu.flag_zero = flag_zero(res_w);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_LSR])){
            log_printf("Inst:LSR\t");
            s_data = reg_read(&cpu, rs);
            d_data = reg_read(&cpu, rd);
            res_w = d_data >> s_data;
            reg_write(&cpu, rd, res_w);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(res_w);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res_w);
            cpu.flag_zero = flag_zero(res_w);
            pc_update(&cpu, 2);
        }else if(bitpat_match_s(inst, inst_bitpat[INST_ASR])){
            log_printf("Inst:ASR\t");
            s_data = reg_read(&cpu, rs);
            d_data = reg_read(&cpu, rd);
            res_w = ((int16_t)d_data) >> s_data;
            reg_write(&cpu, rd, res_w);
            cpu.flag_carry = 0;
            cpu.flag_sign = flag_sign(res_w);
            cpu.flag_overflow = flag_overflow(s_data, d_data, res_w);
            cpu.flag_zero = flag_zero(res_w);
            pc_update(&cpu, 2);
        }else{
            log_printf("Invalid Operation!\n");
            return 0;
        }
        print_flags(&cpu);
        log_printf("\n");

        if (flag_memory_dump){
            dump_memory(stdout, cpu.data_ram, DATA_RAM_SIZE);
            printf("\n");
        }
    }

    for (int i = 0; i < 16; i++){
        uint16_t val = reg_read(&cpu, i);
        printf("x%d=%d\t", i, val);
    }
    puts("");

    return 0;
}
