#include <stdio.h>
#include <stdlib.h>


// Global variables - start //
unsigned char password[] = "1972";   
int debug_mode = 1; // (1 = on | 0 = off)
FILE* infile;
FILE* outfile;
char encode_key[100] = "0";          
int encode_sign = 1; // (1 = +E | -1 = -E)              
int encode_key_index = 0;                   
// Global variables - end //



// Helper functions - start //
int my_strlen(const char* s) {
    int len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

void my_strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
}

int password_matches(const char* s) {
    int i = 0;
    while (password[i] != '\0' && s[i] != '\0') {
        if (password[i] != s[i]) {
            return 0;
        }
        i++;
    }
    return (password[i] == '\0' && s[i] == '\0');
}
// Helper functions - end //



// Encoding function - start //
int encode(int c) {
    char encode_digit = encode_key[encode_key_index];
    int encode_digit_value = encode_digit - '0';
    
    encode_key_index++;
    if (encode_key[encode_key_index] == '\0') {
        encode_key_index = 0;
    }
    
    if (c >= 'A' && c <= 'Z') {
        int offset = c - 'A';
        offset = (offset + encode_sign * encode_digit_value + 26) % 26;
        c = 'A' + offset;
    }
    else if (c >= '0' && c <= '9') {
        int offset = c - '0';
        offset = (offset + encode_sign * encode_digit_value + 10) % 10;
        c = '0' + offset;
    }

    return c;
}
// Encoding function - end //



int main(int argc, char *argv[]) {
    infile = stdin;
    outfile = stdout;

    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'D' && argv[i][2] == '\0') {
            debug_mode = 0;
        }
        else if (argv[i][0] == '+' && argv[i][1] == 'D') {
            if (password_matches(argv[i] + 2)) {
                debug_mode = 1;
            }
        }
        else if (argv[i][0] == '+' && argv[i][1] == 'E') {
            encode_sign = 1;
            my_strcpy(encode_key, argv[i] + 2);
            encode_key_index = 0;
        }
        else if (argv[i][0] == '-' && argv[i][1] == 'E') {
            encode_sign = -1;
            my_strcpy(encode_key, argv[i] + 2);
            encode_key_index = 0;
        }
        else if (argv[i][0] == '-' && argv[i][1] == 'i') {
            infile = fopen(argv[i] + 2, "r");
            if (infile == NULL) {
                fprintf(stderr, "Error: cannot open input file %s\n", argv[i] + 2);
                exit(1);
            }
        }
        else if (argv[i][0] == '-' && argv[i][1] == 'o') {
            outfile = fopen(argv[i] + 2, "w");
            if (outfile == NULL) {
                fprintf(stderr, "Error: cannot open output file %s\n", argv[i] + 2);
                exit(1);
            }
        }
        if (debug_mode && argv[i][1] != 'D') {
            fprintf(stderr, "Arg %d: %s\n", i, argv[i]);
        }
    }

    // encoding
    int c;
    while ((c = fgetc(infile)) != EOF) {
        c = encode(c);
        fputc(c, outfile);
    }

    // closing 
    if (infile != stdin)
        fclose(infile);

    if (outfile != stdout)
        fclose(outfile);

    return 0;
}