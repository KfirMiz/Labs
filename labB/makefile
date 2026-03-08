all: clean compile link

compile:
	gcc -Wall -g -m32 -c AntiVirus.c -o AntiVirus.o

link:
	gcc -Wall -g -m32 AntiVirus.o -o AntiVirus
	rm -f AntiVirus.o

clean:
	rm -f AntiVirus *.o