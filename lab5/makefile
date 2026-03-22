all: compile link

compile:
	nasm -f elf32 start.s -o start.o
	nasm -f elf32 startup.s -o startup.o
	gcc -m32 -c task2c_d.c -o task2c_d.o -fno-stack-protector

link:
	ld -o task2c_d task2c_d.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

clean:
	rm -f task2c_d *.o

.PHONY: clean all compile link