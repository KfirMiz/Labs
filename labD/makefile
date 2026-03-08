all: clean compile link

compile:
	nasm -f elf32 multi.s -o multi.o

link: 
	gcc -m32 multi.o -o multi
	rm -f *.o
	
clean:
	rm -f *.o multi