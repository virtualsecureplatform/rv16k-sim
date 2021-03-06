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
        for(int idx=0;inst_list[idx].bit_pattern != NULL;idx++){
            if(bitpat_match_s(inst, inst_list[idx].bit_pattern)){
                inst_list[idx].func(&cpu, inst);
                break;
            }
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
