all: clean compile link

clean:
	rm -f *.o exe

compile:
	gcc -g -m32 -Wall -c -o menu_map.o menu_map.c

link:
	gcc -g -m32 -Wall -o exe menu_map.o