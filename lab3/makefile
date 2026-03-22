all: clean compile link

compile:
	gcc -m32 -Wall -ansi -c -nostdlib -fno-stack-protector main.c -o main.o
	gcc -m32 -Wall -ansi -c -nostdlib -fno-stack-protector util.c -o util.o
	nasm -f elf32 start.s -o start.o

link: 
	ld -m elf_i386 start.o main.o util.o -o main
	rm -f *.o

clean:
	rm -f *.o main