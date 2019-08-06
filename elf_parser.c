#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "elf.h"
#include "cpu.h"

//#define DEBUG

void elf_parse(struct cpu *c, char* file_name){
    struct stat st;
    FILE *fp;
    int file_size;
    uint8_t* file_buffer;

    if(stat(file_name, &st) != 0){
        printf("Failed to get file size :%s\n", file_name);
        exit(1);
    }
    file_size = st.st_size;

    if((fp = fopen(file_name, "rb")) == NULL){
        printf("Failed to open file :%s\n", file_name);
        exit(1);
    }
    
    file_buffer = malloc(file_size);

    int load_size = fread(file_buffer, sizeof(uint8_t), file_size, fp);
    if(load_size != file_size){
        printf("File size is not matched: %s\n", file_name);
        exit(1);
    }
#ifdef DEBUG
    printf("Loaded File: %s (%dbyte)\n", file_name, load_size);
#endif

    Elf32_Ehdr *Ehdr = (Elf32_Ehdr *)file_buffer;
    if(!IS_ELF(*Ehdr)){
        printf("Unkown file format\n");
        exit(1);
    }
    if(!IS_ELF32(*Ehdr)){
        printf("Not ELF32 format\n");
        exit(1);
    }
#ifdef DEBUG
    printf("Type:ELF32\n");
#endif

    Elf32_Shdr *Shdr = (Elf32_Shdr *)(file_buffer + Ehdr->e_shoff);
    for(int i=0; i<Ehdr->e_shnum;i++){
#ifdef DEBUG
        if(Shdr[i].sh_flags & PF_W) printf("==Executable Section Header==\n");
        printf("Shdr:%d sh_type:%04X\n", i, Shdr[i].sh_type);
        printf("Shdr:%d sh_flags:%04X\n", i, Shdr[i].sh_flags);
        printf("Shdr:%d sh_addr:%04X\n", i, Shdr[i].sh_addr);
        printf("Shdr:%d sh_offset:%04X\n", i, Shdr[i].sh_offset);
        printf("Shdr:%d sh_size:%04X\n", i, Shdr[i].sh_size);
#endif
        if(Shdr[i].sh_flags & PF_W){
            uint16_t *obj = (uint16_t *)(file_buffer+Shdr[i].sh_offset);
            for(int j=0;j<Shdr[i].sh_size/2;j++){
                printf("%04X %04X\n", j, obj[j]);
            }
            printf("\n");
            for(int j=0;j<Shdr[i].sh_size/2;j++){
                c->inst_rom[j] = obj[j];
            }
        }
    }
}
