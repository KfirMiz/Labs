all: clean compile link

compile: 
	gcc -m32 -fno-pie -fno-stack-protector -c task4.c -o task4.o
	gcc -Wall -g -m32 -c task1.c -o task1.o

link:
	gcc -Wall -g -m32 task1.o -o task1

clean:
	rm -f task1 *.o

.PHONY: clean