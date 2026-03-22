
int count_digits(char *str) {
    int counter = 0;
   
    while (*str != '\0') {
       
        if (*str >= '0' && *str <= '9') {
            counter++;
        }
        str++; 
    }
    return counter;
}

// compile using gcc -m32 -fno-pie -fno-stack-protector -c count_digits.c -o count_digits.o