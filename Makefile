main: main.c elf_parser.c bitpat.c
	gcc -o main main.c elf_parser.c bitpat.c
clean:
	rm main
