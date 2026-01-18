; Isometric tile rendering in assembly
; Optimized for x86-64

section .data
    tile_width equ 64
    tile_height equ 32

section .text
    global draw_iso_tile_asm

; void draw_iso_tile_asm(uint8_t* dest, int x, int y, uint32_t color, int screen_width)
; Arguments: rdi=dest, esi=x, edx=y, ecx=color, r8d=screen_width
draw_iso_tile_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save arguments
    mov r12, rdi        ; framebuffer
    mov r13d, esi       ; x
    mov r14d, edx       ; y
    mov r15d, ecx       ; color (RGBA)
    ; r8d already contains screen_width

    ; Extract and store color components (BGR order for ARGB8888)
    movzx eax, cl
    mov byte [rbp-4], al    ; B component
    mov eax, r15d
    shr eax, 8
    movzx eax, al
    mov byte [rbp-8], al    ; G component
    mov eax, r15d
    shr eax, 16
    movzx eax, al
    mov byte [rbp-12], al   ; R component

    ; Draw diamond (isometric tile)
    xor r9d, r9d        ; y counter
    
.y_loop:
    cmp r9d, tile_height
    jge .done

    ; Calculate distance from center
    mov eax, tile_height
    shr eax, 1          ; half_height = 16
    mov ebx, r9d
    sub ebx, eax
    
    ; Get absolute value of (y - half_height)
    mov eax, ebx
    neg eax
    cmovl eax, ebx      ; eax = abs(y - half_height)
    
    ; Calculate width for this row: tile_width - (dy * 2)
    mov ebx, eax
    shl ebx, 1          ; dy * 2
    mov ecx, tile_width
    sub ecx, ebx        ; width for this row
    
    ; Skip if width is invalid
    test ecx, ecx
    jle .next_y
    
    ; Calculate start_x: x + dy
    mov r10d, r13d      ; x
    add r10d, eax       ; x + dy
    
    ; Calculate pixel y position
    mov r11d, r14d      ; y
    add r11d, r9d       ; y + row
    
    ; Check if y is in bounds
    test r11d, r11d
    js .next_y
    cmp r11d, 600       ; Screen height
    jge .next_y
    
    ; Draw horizontal line
    push rcx            ; Save width
    xor ebx, ebx        ; x counter for this row
    
.x_loop:
    cmp ebx, ecx
    jge .end_row
    
    ; Calculate pixel x position
    mov eax, r10d
    add eax, ebx
    
    ; Check x bounds
    test eax, eax
    js .next_x
    cmp eax, r8d        ; screen_width
    jge .next_x
    
    ; Calculate pixel offset: (y * width + x) * 4
    push rax
    push rbx
    push rcx
    
    mov eax, r11d
    imul eax, r8d       ; y * width
    pop rcx
    pop rbx
    pop rdx             ; x position
    add eax, edx        ; + x
    shl rax, 2          ; * 4 (RGBA)
    
    ; Write pixel (BGRA format for SDL_PIXELFORMAT_ARGB8888)
    lea rdi, [r12 + rax]
    
    mov al, byte [rbp-4]
    mov byte [rdi], al      ; B
    mov al, byte [rbp-8]
    mov byte [rdi+1], al    ; G
    mov al, byte [rbp-12]
    mov byte [rdi+2], al    ; R
    mov byte [rdi+3], 0xFF  ; A
    
.next_x:
    inc ebx
    jmp .x_loop
    
.end_row:
    pop rcx             ; Restore width
    
.next_y:
    inc r9d
    jmp .y_loop
    
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    add rsp, 16
    pop rbp
    ret