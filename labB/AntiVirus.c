#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define bufferSize 10000

// Structs
typedef struct virus {
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;


typedef struct link link;
struct link {
    link* nextVirus;
    virus* vir;
};


// Globals
int isBigEndian = 0; // 1-yes 0-no
link* SignaturesList = NULL;


int readMagic(FILE* file) {
    char magic[4];
    if (fread(magic, 1, 4, file) != 4) {
        printf("Error reading magic number\n");
        return 0;
    }

    if (memcmp(magic, "VIRB", 4) == 0)
        isBigEndian = 1;
    else if (memcmp(magic, "VIRL", 4) == 0)
        isBigEndian = 0;
    else {
        printf("Error: Not a signature file\n");
        return 0;
    }
    return 1;
}


virus* readSignature(FILE* file) {
    if (!file) return NULL;

    virus* v = malloc(sizeof(virus));
    if (!v) return NULL;

    unsigned char header[18]; // 2 for SigSize, 16 for name
    int bytesRead = fread(header, 1, 18, file);
    if (bytesRead < 18) {
        free(v);
        return NULL;
    }

    // sigSize
    if (isBigEndian)
        v->SigSize = (header[0] << 8) | header[1];
    else
        v->SigSize = (header[1] << 8) | header[0];
    
    // virusName
    memcpy(v->virusName, header + 2, 16);

    v->sig = malloc(v->SigSize);
    if (!v->sig) {
        free(v);
        return NULL;
    }

    // sig
    if (fread(v->sig, 1, v->SigSize, file) != v->SigSize) {
        free(v->sig);
        free(v);
        return NULL;
    }

    return v;
}


void printSignature(virus* v, FILE* output) {
    if (!v || !output) return;

    fprintf(output, "Virus name: %s\n", v->virusName);
    fprintf(output, "Virus signature length: %u\n", v->SigSize);
    fprintf(output, "Signature:\n");
    for (int i = 0; i < v->SigSize; i++) {
        fprintf(output, "%02X ", v->sig[i]);
    }
    fprintf(output, "\n\n");
}


link* list_append(link* virus_list, virus* data) {
    link* newNode = malloc(sizeof(link));
    if (!newNode) return virus_list;

    newNode->vir = data;
    newNode->nextVirus = NULL;

    if (!virus_list) return newNode;  

    link* temp = virus_list;
    while (temp->nextVirus)
        temp = temp->nextVirus;

    temp->nextVirus = newNode;
    return virus_list;
}


void list_print(link* virus_list, FILE* output) {
    if (!virus_list) {
        fprintf(output, "No signatures\n");
        return;
    }

    link* temp = virus_list;
    while (temp) {
        printSignature(temp->vir, output);
        temp = temp->nextVirus;
    }
}


void list_free(link* virus_list) {
    link* temp;
    while (virus_list) {
        temp = virus_list->nextVirus;
        if (virus_list->vir) {
            free(virus_list->vir->sig);
            free(virus_list->vir);
        }
        free(virus_list);
        virus_list = temp;
    }
}


void neutralize_virus(char* fileName, int signatureOffset) {
    FILE* file = fopen(fileName, "r+b");
    if (!file) {
        printf("Error opening file for writing.\n");
        return;
    }
    if (fseek(file, signatureOffset, SEEK_SET) != 0) {
        printf("Error seeking in file.\n");
        fclose(file);
        return;
    }
    unsigned char ret = 0xC3;
    int written = fwrite(&ret, 1, 1, file);
    if (written != 1) {
        printf("Error writing to file.\n");
    }
    fclose(file);

}


// ---------- Menu functions ---------- //

void detect_virus() {
    if (!SignaturesList) {
        printf("No signatures loaded.\n");
        return;
    }

    char filename[256];
    printf("Enter suspected file name: ");
    if (!fgets(filename, sizeof(filename), stdin)) return;
    filename[strcspn(filename, "\n")] = 0;  // remove newline

    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Error opening file %s\n", filename);
        return;
    }

    unsigned char buffer[bufferSize];
    int bytesRead = fread(buffer, 1, bufferSize, f);
    fclose(f);

    if (bytesRead == 0) {
        printf("File is empty or read error.\n");
        return;
    }

    link* current = SignaturesList;
    while (current) {
        virus* v = current->vir;
        for (int i = 0; i <= bytesRead - v->SigSize; i++) {
            if (memcmp(buffer + i, v->sig, v->SigSize) == 0) {
                printf("Virus found at byte %zu\n", i);
                printf("Virus name: %s\n", v->virusName);
                printf("Virus size: %u\n\n", v->SigSize);
            }
        }
        current = current->nextVirus;
    }
}


void fixFile() {
     if (!SignaturesList) {
        printf("No signatures loaded.\n");
        return;
    }

    char filename[256];
    printf("Enter suspected file name: ");
    if (!fgets(filename, sizeof(filename), stdin)) return;
    filename[strcspn(filename, "\n")] = 0;  // remove newline

    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Error opening file %s\n", filename);
        return;
    }

    unsigned char buffer[bufferSize];
    int bytesRead = fread(buffer, 1, bufferSize, f);
    fclose(f);

    if (bytesRead == 0) {
        printf("File is empty or read error.\n");
        return;
    }

    link* current = SignaturesList;
    while (current) {
        virus* v = current->vir;
        for (int i = 0; i <= bytesRead - v->SigSize; i++) {
            if (memcmp(buffer + i, v->sig, v->SigSize) == 0) {
                neutralize_virus(filename, i);
                printf("the virus %s as been removed from the file", v->virusName);
            }
        }
        current = current->nextVirus;
    }
}


void aiAnalysis() {
    printf("Not implemented\n");
}


void quit() {
    if (SignaturesList) list_free(SignaturesList);
    exit(0);
}


void LoadSignatures() {
    char filename[256];
    printf("Enter signatures file name: ");
    if (!fgets(filename, sizeof(filename), stdin)) return;

    filename[strcspn(filename, "\n")] = 0;

    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Error opening file %s\n", filename);
        return;
    }

    if (!readMagic(f)) {
        fclose(f);
        return;
    }

    if (SignaturesList) list_free(SignaturesList);
    SignaturesList = NULL;

    virus* v;
    while ((v = readSignature(f)) != NULL) {
        SignaturesList = list_append(SignaturesList, v);
    }

    fclose(f);
    printf("Signatures loaded successfully.\n");
}


void PrintSignatures() {
    list_print(SignaturesList, stdout);
}


int main(int argc, char **argv) {

    struct fun_desc {
        char *name;
        void (*fun)();
    };

    struct fun_desc menu[] = {
        { "Load signatures",LoadSignatures },
        { "Print signatures", PrintSignatures },
        { "Detect viruses", detect_virus },
        { "Fix file", fixFile },
        { "AI Analysis of file", aiAnalysis },
        { "Quit", quit },
        { NULL, NULL }
    };

    int menuSize = 0;
    while (menu[menuSize].name != NULL) menuSize++;

    while (1) {
        char input[100];
        int choice;

        printf("\n");
        for (int i = 0; i < menuSize; i++) {
            printf("%d) %s\n", i + 1, menu[i].name);
        }
        printf("Select one of the above options:\n");

        if (!fgets(input, sizeof(input), stdin))
            continue;

        if (sscanf(input, "%d", &choice) != 1) {
            printf("Not within bounds\n");
            continue;
        }

        choice -= 1;  

        if (choice >= 0 && choice < menuSize) {
            menu[choice].fun();
        } else {
            printf("Not within bounds\n");
        }
    }
    return 0;
}