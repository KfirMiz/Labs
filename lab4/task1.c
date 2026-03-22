#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 //Global Variables as requested 
char debug_mode = 0;
char file_name[128];
int unit_size = 1;
unsigned char mem_buf[10000];
size_t mem_count = 0;
int MYLIMIT = 12345;
int display_mode = 0; // 0 = Hex, 1 = Decimal

 //Function Prototypes 
void toggle_debug_mode();
void set_file_name();
void set_unit_size();
void load_into_memory();
void toggle_display_mode();
void memory_display();
void save_into_file();
void memory_modify();
void quit();

 //Menu Structure 
struct fun_desc {
    char *name;
    void (*fun)();
};

void toggle_display_mode() {
    if (display_mode == 0) {
        display_mode = 1;
        printf("Decimal display flag now on, decimal representation\n"); 
    } else {
        display_mode = 0;
        printf("Decimal display flag now off, hexadecimal representation\n"); 
    }
}


void memory_display() {
    
    static char* hex_formats[] = {"%#hhx\n", "%#hx\n", "No such unit", "%#x\n"};
    static char* dec_formats[] = {"%#hhd\n", "%#hd\n", "No such unit", "%#d\n"};

    unsigned int addr;
    int u; // number of units
    
    printf("Enter address and length:\n");
    
    char input_buffer[100];
    fgets(input_buffer, sizeof(input_buffer), stdin);
    
    if (sscanf(input_buffer, "%x %d", &addr, &u) != 2) {
        printf("Error: invalid input\n");
        return;
    }

    if (display_mode) {
        printf("Decimal\n=======\n");
    } else {
        printf("Hexadecimal\n===========\n");
    }

    char ** formats = hex_formats;
    if(display_mode){
        formats = dec_formats;
    }
    
    for (int i = 0; i < u; i++) {
       
        unsigned int offset = addr + (i * unit_size);
        
        if (offset + unit_size > sizeof(mem_buf)) {
            printf("Error: offset out of bounds\n");
            break;
        }

        int val = 0;
        unsigned char* ptr = &mem_buf[offset];

        if (unit_size == 1) {
            val = *ptr;
        } else if (unit_size == 2) {
            val = *((unsigned short*)ptr);
        } else if (unit_size == 4) {
            val = *((unsigned int*)ptr);
        }

        printf(formats[unit_size - 1], val);
    }
}
void save_into_file() {
   
    unsigned int source_addr;
    unsigned int target_loc;
    int length;

    printf("Please enter <source-address> <target-location> <length>\n");
    char input_buffer[100];
    
   
    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) return;

    
    if (sscanf(input_buffer, "%x %x %d", &source_addr, &target_loc, &length) != 3) {
        printf("Error: invalid input format\n");
        return;
    }

    if (debug_mode) {
        fprintf(stderr, "Debug: source_addr: 0x%x, target_loc: 0x%x, length: %d\n", 
                source_addr, target_loc, length);
    }

   
   
    FILE *file = fopen(file_name, "rb+");
    if (file == NULL) {
        printf("Error: could not open file '%s' for writing\n", file_name);
        return;
    }

   
    fseek(file, 0, SEEK_END);
   
    long file_size = ftell(file);
    
   
    if (target_loc > file_size) {
        printf("Error: target location 0x%x is beyond file size %ld\n", target_loc, file_size);
        fclose(file); 
        return;
    }


    unsigned char *source_ptr;

    if (source_addr == 0) {
       
        source_ptr = mem_buf;
    } else {
        
        source_ptr = (unsigned char *)source_addr;
    }

   
    fseek(file, target_loc, SEEK_SET);
    
  
    int written = fwrite(source_ptr, unit_size, length, file);
    
    if (written != length) {
        printf("Error: failed to write all units to file\n");
    } else {
        printf("Wrote %d units into file '%s' at offset 0x%x\n", length, file_name, target_loc);
    }

  
    fclose(file);
}
void memory_modify() { 
    unsigned int location;
    unsigned int val; 

    printf("enter <location> and <val>: \n");
    char input_buff[100];
    if(fgets(input_buff,sizeof(input_buff),stdin) == NULL){fprintf(stderr,"invalid input\n");}
    if(sscanf(input_buff,"%x %x", &location, &val) != 2){
        fprintf(stderr,"invalid input");
        return;
    }
    if(debug_mode){
        printf("Debug: location is:0x%x, the val is: 0x%x \n", location,val);
    }
    if(location+unit_size>sizeof(mem_buf)){
        fprintf(stderr,"Error: invalid location\n");
        return;
    }
    unsigned char* ptr;
    ptr = &mem_buf[location];
    switch(unit_size) {
        case 1:
            *ptr = (unsigned char)val;
            break;
        case 2:
            *((unsigned short *)ptr) = (unsigned short)val;
            break;
        case 4:
            *((unsigned int *)ptr) = (unsigned int)val;
            break;
        default:
            printf("Error: invalid unit size\n");
    }

}
void load_into_memory() {
  
    if (strcmp(file_name, "") == 0) {
        printf("Error: file name is empty\n");
        return;
    }

   
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("Error: could not open file '%s'\n", file_name);
        return;
    }

   
    unsigned int location;
    int length;
    
    printf("Please enter <location>in hexadecimal and <length> in decimal\n");
    
    char input_buffer[100];
    fgets(input_buffer, sizeof(input_buffer), stdin);
    

    int parsed = sscanf(input_buffer, "%x %d", &location, &length);
    
    if (parsed != 2) {
        printf("Error: invalid input format\n");
        fclose(file);
        return;
    }

   
    if (debug_mode) {
        fprintf(stderr, "Debug: file_name: %s, location: 0x%x, length: %d\n", 
                file_name, location, length);
    }

   
    if (length * unit_size > sizeof(mem_buf)) {
        printf("Error: request exceeds buffer size\n");
        fclose(file);
        return;
    }

  
    if (fseek(file, location, SEEK_SET) != 0) {
        perror("Error seeking in file"); 
        fclose(file);
        return;
    }

   
    size_t loaded = fread(mem_buf, unit_size, length, file);
    
   
    if (loaded != length) {
        printf("Error: read less units than requested (read %zu out of %d)\n", loaded, length);
    } else {
        printf("Loaded %zu units into memory\n", loaded);
    }

    fclose(file);
}

void toggle_debug_mode() {
    if (debug_mode) {
        debug_mode = 0;
        printf("Debug flag now off\n");
    } else {
        debug_mode = 1;
        printf("Debug flag now on\n");
    }
}

void set_file_name() {
    printf("Please enter <file name>\n");
   
    fgets(file_name, sizeof(file_name), stdin);
    
   
    file_name[strcspn(file_name, "\n")] = 0;
    
    if (debug_mode) {
        fprintf(stderr, "Debug: file name set to '%s'\n", file_name);
    }
}

void set_unit_size() {
    int size;
    printf("Please enter unit size (1, 2, or 4):\n");
    scanf("%d", &size);
   
    fgetc(stdin); 

    if (size == 1 || size == 2 || size == 4) {
        unit_size = size;
        if (debug_mode) {
            fprintf(stderr, "Debug: set size to %d\n", unit_size);
        }
    } else {
        printf("Invalid unit size\n");
    }
}

void quit() {
    if (debug_mode) {
        fprintf(stderr, "quitting\n");
    }
    exit(0);
}

 //Menu Array 
struct fun_desc menu[] = {
    { "Toggle Debug Mode", toggle_debug_mode },
    { "Set File Name", set_file_name },
    { "Set Unit Size", set_unit_size },
    { "Load Into Memory", load_into_memory },
    { "Toggle Display Mode", toggle_display_mode },
    { "Memory Display", memory_display },
    { "Save Into File", save_into_file },
    { "Memory Modify", memory_modify },
    { "Quit", quit },
    { NULL, NULL }
};

int main(int argc, char *argv[]){
   
    char line[100]; 
    
   
    int menu_size = 0;
    while(menu[menu_size].name != NULL) {
        menu_size++;
    }

    while(1){
        if(debug_mode == 1){
          
            fprintf(stderr, "Debug: file_name: %s\n", file_name);
            fprintf(stderr, "Debug: unit_size: %d\n", unit_size);
            fprintf(stderr, "Debug: mem_count: %zu\n", mem_count);
        }

       
        for(int i = 0; i < menu_size; i++){
            printf("%d-%s\n", i, menu[i].name);
        } 
        printf("enter your option: ");

        if(fgets(line, sizeof(line), stdin) == NULL){
            break; 
        }

        int chosen_func = -1;
       
        if(sscanf(line, "%d", &chosen_func) != 1){
             printf("Invalid input\n");
             continue;
        }

       
        if(chosen_func >= 0 && chosen_func < menu_size){
           
            menu[chosen_func].fun(); 
        } else {
            printf("Not within bounds\n");
        }
        
        printf("\n");
    }
    
    return 0;
}
