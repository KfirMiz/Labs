all: clean compile link

compile: 
	gcc -Wall -g -m32 -c LineParser.c -o LineParser.o
	gcc -Wall -g -m32 -c myshell.c -o myshell.o
	gcc -Wall -g -m32 -c mypipeline.c -o mypipeline.o

link: 
	gcc -Wall -g -m32 myshell.o LineParser.o -o myshell
	gcc -Wall -g -m32 mypipeline.o -o mypipeline
	rm -f *.o

clean:
	rm -f myshell mypipeline *.o