main: main.c elf_parser.c bitpat.c log.c
	gcc -o $@ $^
clean:
	rm main
test:
	./test.sh

.PHONY: clean test
