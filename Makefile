main: main.c elf_parser.c
	gcc -o main main.c elf_parser.c
clean:
	rm main
