section .data
    buffer: times 610 db 0    ; for user input

    hex_format: db "%02hhx", 0
    newline:    db 10, 0

    x_struct: dd 5
    x_num: db 0xaa, 1,2,0x44,0x4f
    y_struct: dd 6
    y_num: db 0xaa, 1,2,3,0x44,0x4f

    STATE: dw 0xACE1          
    MASK:  dw 0xB400          

section .text
    extern fgets, stdin, malloc, strlen
    extern printf
    global main

getmulti:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    ; user input
    push dword [stdin]    
    push 600              
    push buffer           
    call fgets
    add esp, 12           

    ; remove newline
    push buffer
    call strlen           
    add esp, 4

    mov ecx, eax          
    cmp ecx, 0
    je .error             

    ; if last char is newline, ignore it
    cmp byte [buffer + ecx - 1], 10
    jne .prepare_struct
    dec ecx               ; ecx is now the clean string length

.prepare_struct:
    ; (length + 1) / 2 bytes needed for the new struct
    mov eax, ecx
    inc eax
    shr eax, 1            
    
    mov ebx, eax          
    add eax, 4            ; Add 4 bytes, for the 'size' field

    push ecx    
    push ebx    

    push eax              
    call malloc
    add esp, 4            ; eax = pointer to new struct

    pop ebx     
    pop ecx     

    mov [eax], ebx        ; set struct->size = ebx
    lea edi, [eax + 4]    ; edi = num[0]
    

    mov esi, buffer       ; esi = start of the string
    add esi, ecx          
    dec esi               ; esi = end of the string

    mov edx, 0          ; edx = index 

.loop:
    cmp ecx, 0
    jle .finish           

    movzx ebx, byte [esi] 
    cmp bl, '9'
    jbe .is_digit1
    and bl, 0xDF          ; make uppercase (a -> A)
    sub bl, 'A'-10        ; 'A' evaluates to 10
    jmp .store1
.is_digit1:
    sub bl, '0'           ; '1' evaluates to 1
.store1:
    mov [edi + edx], bl   ; Put in array
    
    dec esi               ; move string pointer left
    dec ecx               ; decrease chars remaining
    jz .finish            ; in case the string is odd


    movzx ebx, byte [esi]
    cmp bl, '9'
    jbe .is_digit2
    and bl, 0xDF          
    sub bl, 'A'-10
    jmp .store2
.is_digit2:
    sub bl, '0'
.store2:
    shl bl, 4             ; Shift to high 4 bits
    or [edi + edx], bl    ; Combine with existing low nibble

    dec esi
    dec ecx
    inc edx               
    jmp .loop

.finish:
    ; eax contains the pointer to the allocated struct
.error:
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret


;-----------------------------------------------------


print_multi:
    push ebp
    mov ebp, esp
    push ebx            
    push esi            

    mov esi, [ebp + 8]  ; esi = pointer to the struct
    mov ebx, [esi]      ; ebx = size 
    lea esi, [esi + 4]  ; esi = pointer to the start of the 'num' array
    
    dec ebx             ; index of the last byte

.print_loop:
    cmp ebx, 0
    jl .doneP            

    movzx eax, byte [esi + ebx]
    push eax
    push hex_format
    call printf
    add esp, 4*2          

    dec ebx             
    jmp .print_loop

.doneP:
    push newline
    call printf
    add esp, 4            

    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret


;-----------------------------------------------------

MaxMin:
    mov ecx, [eax]      ; Get size of first struct 
    mov edx, [ebx]      ; Get size of second struct
    cmp ecx, edx        
    jge .already_sorted 
    
    xchg eax, ebx       
    
.already_sorted:
    ret


;-----------------------------------------------------

add_multi:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    mov eax, [ebp + 8]   ; p
    mov ebx, [ebp + 12]  ; q
    call MaxMin          ; eax = Max, ebx = Min
    mov esi, eax         ; esi = Max pointer
    mov edi, ebx         ; edi = Min pointer

    ; allocate result struct
    mov ecx, [esi]       
    lea eax, [ecx + 5]   ; +1 for the last carry and +4 for the size field
    push ecx 

    push eax             
    call malloc
    add esp, 4

    pop ecx              

    ; initialize header
    mov edx, ecx
    inc edx
    mov [eax], edx       ; new struct size = MaxLen + 1

    ; addition Loop Setup
    push eax             ; save struct pointer for final return
    lea edx, [eax + 4]   ; edx = start of result array
    
    mov ebx, [edi]       ; ebx = MinLen
    mov eax, 0           ; eax = index
    clc                  ; initial carry = 0
    pushfd               ; save carry flag

.loop_common:
    cmp eax, ebx         
    je .loop_tail_setup

    popfd                ; restore carry flag
    mov cl, [esi + eax + 4]
    adc cl, [edi + eax + 4]
    pushfd               ; save carry flag
    
    mov [edx + eax], cl  ; save into the new struct
    inc eax
    jmp .loop_common

.loop_tail_setup:
    mov ebx, [esi]       ; ebx = MaxLen

.loop_tail:
    cmp eax, ebx         
    je .final_carry
    
    popfd                
    mov cl, [esi + eax + 4]
    adc cl, 0
    pushfd               
    
    mov [edx + eax], cl
    inc eax
    jmp .loop_tail

.final_carry:
    popfd                
    mov cl, 0
    adc cl, 0            
    mov [edx + eax], cl

    pop eax              ; restore the pointer to the allocated struct
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret

;-----------------------------------------------------

; PRNG 
rand_num:
    push ebp
    mov ebp, esp
    
    mov ax, [STATE]
    mov bx, [MASK]
    and ax, bx          ; ax = Masked bits

    
    mov dx, ax
    shr dx, 8           ; dx = bits 8-15
    xor al, dl          ; al = bits 0-7 XOR bits 8-15
    
    
    jp .even_parity     
    mov cx, 1           ; odd parity -> bit 15 will be 1
    jmp .do_shift

.even_parity:
    mov cx, 0           ; even parity -> bit 15 will be 0

.do_shift:
    mov ax, [STATE]     
    shr ax, 1           
    shl cx, 15          ; move the parity bit to position 15
    or ax, cx           ; insert the parity bit into the MSB
    mov [STATE], ax     ; new state
    movzx eax, ax       
    
    mov esp, ebp
    pop ebp
    ret


;-----------------------------------------------------

PRmulti:
    push ebp
    mov ebp, esp
    push ebx

.get_size:
    call rand_num
    and al, 0xFF        
    jz .get_size        
    movzx ebx, al
    
    lea eax, [ebx + 4]  
    push ebx
    push eax
    call malloc
    add esp, 4
    pop ebx

    mov [eax], ebx      ; Set struct size
    mov ecx, 0

.fill:
    cmp ecx, ebx
    je .done
    push eax            ; save struct pointer
    push ecx
    push ebx
    call rand_num       
    pop ebx
    pop ecx
    pop edx             ; restore struct pointer
    mov [edx + ecx + 4], al
    mov eax, edx        ; Restore eax for next loop
    inc ecx
    jmp .fill

.done:
    pop ebx
    mov esp, ebp
    pop ebp
    ret


;-----------------------------------------------------


main:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp + 8]      ; argc
    cmp eax, 2
    jl .default             ; no args

    mov ebx, [ebp + 12]     ; argv
    mov ecx, [ebx + 4]      ; argv[1]
    cmp byte [ecx + 0], '-'
    jne .inputError
    cmp byte [ecx + 1], 'I'
    je .input_mode
    cmp byte [ecx + 1], 'R'
    je .random_mode
    jmp .inputError

.default:
    mov esi, x_struct
    mov edi, y_struct
    jmp .do_math

.input_mode:
    call getmulti
    mov esi, eax
    call getmulti
    mov edi, eax
    jmp .do_math

    .random_mode:
    call PRmulti
    mov esi, eax
    call PRmulti
    mov edi, eax

.do_math:
    push esi
    call print_multi
    add esp, 4
    
    push edi
    call print_multi
    add esp, 4

    push edi
    push esi
    call add_multi
    add esp, 8
    
    push eax                ; Result of addition
    call print_multi
    add esp, 4
    
    mov eax, 0
    mov esp, ebp
    pop ebp
    ret

.inputError:
    mov eax, 1
    mov esp, ebp
    pop ebp
    ret