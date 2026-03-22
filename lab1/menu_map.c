#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
char* map(char *array, int array_length, char (*f) (char)){
    char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
    /* TODO: Complete during task 2.a */
    for (int i = 0; i < array_length; i++) {
        mapped_array[i] = f(array[i]);
    }
    /* */
    return mapped_array;
}
 
char my_get(char c) {
    return fgetc(stdin);
}

char cxprt(char c) {
    if (c >= 0x20 && c <= 0x7E) {
        printf("%c %x\n", c, c);
    }
    else {
        printf(". %x\n", c);
    }
    return c;
}

char encrypt(char c) {
    if (c >= 0x1F && c <= 0x7E) {
        return c + 1;
    }
    return c;
}

char decrypt(char c) {
    if (c >= 0x21 && c <= 0x7F) {
        return c - 1;
    }
    return c;
}

char dprt(char c) {
    printf("%d\n", c);
    return c;
}

/////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv){
    
    struct fun_desc {
    char *name;
    char (*fun)(char);
    };

    struct fun_desc menu[] = { 
        { "my_get", my_get }, 
        { "cxprt", cxprt },
        { "encrypt", encrypt },
        { "decrypt", decrypt },
        { "dprt", dprt }, 
        { NULL, NULL } 
    };

    int menuSize = 0;

    for (int i = 0; menu[i].name != NULL; i++) {
        menuSize++;
    }
    
    char* carray = calloc(5, sizeof(char));

    while (1) {
        
        char input [5];
        for (int i = 0; i < menuSize; i++) {
        printf("%d) %s\n", i, menu[i].name);
        }
        
        printf("Select operation from the following menu:\n");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        int choosen = atoi(input);

        if (choosen >= 0 && choosen < menuSize) {
            printf("Within bounds\n");
        }
        else {
            printf("Not within bounds\n");
            break;
        }

        char* tempArr = malloc(5);
        tempArr = map(carray, 5, menu[choosen].fun);
        free(carray);
        carray = tempArr;
    }
}