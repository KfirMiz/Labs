#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define PAGE_SIZE 0x1000
#define PAGE_MASK 0xfffff000


extern int startup(int argc, char **argv, void (*start)());


int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0) return -1; 

    Elf32_Phdr *phdr_table = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);
    int i;
    for (i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr_table[i], arg);
    }
    return 0;
}


void load_phdr(Elf32_Phdr *phdr, int fd) {
    if (phdr->p_type != PT_LOAD) return;

    int prot_flags = 0;
    if (phdr->p_flags & PF_R) prot_flags |= PROT_READ;
    if (phdr->p_flags & PF_W) prot_flags |= PROT_WRITE;
    if (phdr->p_flags & PF_X) prot_flags |= PROT_EXEC;

    int map_flags = MAP_PRIVATE | MAP_FIXED;

    Elf32_Addr vaddr_aligned = phdr->p_vaddr & PAGE_MASK;
    Elf32_Off offset_aligned = phdr->p_offset & PAGE_MASK;
    int padding = phdr->p_vaddr & (PAGE_SIZE - 1);
    size_t length = phdr->p_memsz + padding;

    printf("Mapping segment: VirtAddr=0x%x (aligned 0x%x), Offset=0x%x (aligned 0x%x), Size=0x%x\n",
           phdr->p_vaddr, vaddr_aligned, phdr->p_offset, offset_aligned, phdr->p_memsz);

    void *map_res = mmap((void *)vaddr_aligned, length, prot_flags, map_flags, fd, offset_aligned);
    
    if (map_res == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <executable> [args...]\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    fstat(fd, &st);
    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap header");
        close(fd);
        return 1;
    }

    foreach_phdr(map_start, load_phdr, fd);

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Addr entry_point = ehdr->e_entry;

    munmap(map_start, st.st_size);

    printf("Starting program at entry point: 0x%x\n", entry_point);    
    startup(argc - 1, argv + 1, (void *)entry_point);

    close(fd);
    return 0;
}