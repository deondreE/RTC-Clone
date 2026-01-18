; Sprite blitting routines with transparency
; Optimized for x86-64

section .text
    global blit_sprite_asm

; void blit_sprite_asm(uint8_t* dest, const uint8_t* sprite, int width, int height, int dest_pitch)
; Arguments: rdi=dest, rsi=sprite, rdx=width, rcx=height, r8=dest_pitch
; Sprite format: RGBA, color (0,0,0,0) is transparent
blit_sprite_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov r12, rdx        ; width
    mov r13, rcx        ; height

    ; Convert dest_pitch to bytes (it's in pixels, we need bytes)
    shl r8, 2           ; multiply by 4 for RGBA

.row_loop:
    test r13, r13
    jz .done

    mov rbx, r12        ; Reset width counter

.pixel_loop:
    test rbx, rbx
    jz .next_row

    ; Load sprite pixel (RGBA)
    lodsd               ; Load 4 bytes from [rsi] to eax, increment rsi by 4

    ; Check if transparent (alpha = 0)
    test eax, 0xFF000000
    jz .skip_pixel

    ; Write pixel to destination
    stosd               ; Store eax to [rdi], increment rdi by 4
    dec rbx
    jmp .pixel_loop

.skip_pixel:
    add rdi, 4          ; Skip destination pixel
    dec rbx
    jmp .pixel_loop

.next_row:
    ; Move to next row
    ; dest += (dest_pitch - width * 4)
    mov rax, r12
    shl rax, 2          ; width * 4
    add rdi, r8         ; add pitch
    sub rdi, rax        ; subtract width

    dec r13
    jmp .row_loop

.done:
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret


; Fast horizontal line drawing
; void draw_hline_asm(uint8_t* dest, int x, int y, int width, uint32_t color, int screen_width)
    global draw_hline_asm
draw_hline_asm:
    ; rdi=dest, esi=x, edx=y, ecx=width, r8d=color, r9d=screen_width
    push rbp
    mov rbp, rsp

    ; Calculate offset: (y * screen_width + x) * 4
    mov eax, edx
    imul eax, r9d       ; y * screen_width
    add eax, esi        ; + x
    shl eax, 2          ; * 4
    add rdi, rax        ; dest += offset

    ; Prepare color in eax
    mov eax, r8d

    ; Draw pixels
.loop:
    test ecx, ecx
    jz .done
    stosd               ; Write color, advance rdi
    dec ecx
    jmp .loop

.done:
    pop rbp
    ret


; Fast vertical line drawing
; void draw_vline_asm(uint8_t* dest, int x, int y, int height, uint32_t color, int screen_width)
global draw_vline_asm
draw_vline_asm:
; rdi=dest, esi=x, edx=y, ecx=height, r8d=color, r9d=screen_width
push rbp
mov rbp, rsp

; Calculate initial offset
mov eax, edx
imul eax, r9d
add eax, esi
shl eax, 2
add rdi, rax

; Calculate row stride
shl r9d, 2          ; screen_width * 4

mov eax, r8d        ; color

.loop:
test ecx, ecx
jz .done
mov dword [rdi], eax
add rdi, r9         ; Move to next row
dec ecx
jmp .loop

.done:
pop rbp
ret


; Fill rectangle with bounds checking
; void fill_rect_asm(uint8_t* dest, int x, int y, int width, int height, uint32_t color, int screen_width)
global fill_rect_asm
fill_rect_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Get screen_width from stack
    mov r12d, dword [rbp+16]

    ; Validate inputs
    test rdi, rdi
    jz .done                  ; NULL check

    test ecx, ecx
    jle .done                 ; width <= 0

    test r8d, r8d
    jle .done                 ; height <= 0

    ; Clip x to screen bounds
    test esi, esi
    jns .x_ok
    xor esi, esi              ; x = 0 if negative
.x_ok:
    cmp esi, r12d
    jl .x_in_bounds
    jmp .done                 ; x >= screen_width, nothing to draw
.x_in_bounds:

    ; Clip y to screen bounds
    test edx, edx
    jns .y_ok
    xor edx, edx              ; y = 0 if negative
.y_ok:
    mov r14d, 600             ; SCREEN_HEIGHT
    cmp edx, r14d
    jl .y_in_bounds
    jmp .done                 ; y >= screen_height
.y_in_bounds:

    ; Clip width to fit screen
    mov r13d, ecx             ; requested width
    mov eax, esi
    add eax, ecx              ; x + width
    cmp eax, r12d
    jle .width_ok
    ; Width extends past screen, clip it
    mov r13d, r12d
    sub r13d, esi             ; width = screen_width - x
.width_ok:

    ; Clip height to fit screen
    mov r15d, r8d             ; requested height
    mov eax, edx
    add eax, r8d              ; y + height
    cmp eax, r14d
    jle .height_ok
    ; Height extends past screen, clip it
    mov r15d, r14d
    sub r15d, edx             ; height = screen_height - y
.height_ok:

    ; Final validation
    test r13d, r13d
    jle .done
    test r15d, r15d
    jle .done

    ; Calculate starting address: dest + (y * screen_width + x) * 4
    mov eax, edx              ; y
    imul eax, r12d            ; y * screen_width
    add eax, esi              ; + x
    movsxd rax, eax
    shl rax, 2                ; * 4
    add rdi, rax

    ; Calculate bytes per row: screen_width * 4
    mov r8d, r12d
    shl r8d, 2

    ; Calculate bytes to advance after each row
    mov ebx, r13d
    shl ebx, 2                ; width * 4
    mov r14d, r8d
    sub r14d, ebx             ; row_advance = (screen_width - width) * 4

.row_loop:
    test r15d, r15d
    jz .done

    ; Fill one row
    mov ecx, r13d             ; width
    mov eax, r9d              ; color

.pixel_loop:
    test ecx, ecx
    jz .next_row
    stosd
    dec ecx
    jmp .pixel_loop

.next_row:
    add rdi, r14              ; advance to next row
    dec r15d
    jmp .row_loop

.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
