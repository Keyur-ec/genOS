ORG 0
BITS 16

_start:
    jmp short start
    nop

times 33 db 0

start:
    jmp 0x7c0:main

main:
    cli             ; clearing the interrupts
    mov ax, 0x7c0
    mov ds, ax
    mov es, ax
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00
    sti             ; setting the interrupts

    mov ah, 0x02    ; read sector command
    mov al, 0x01    ; one sector to read
    mov ch, 0x00    ; cylinder low eight bit
    mov cl, 0x02    ; read sector two
    mov dh, 0x00    ; head number
    mov bx, buffer
    int 0x13
    jc error

    mov si, buffer
    call print

    jmp $

error:
    mov si, error_message
    call print
    jmp $

print:
    mov bx, 0x00
.loop:
    lodsb
    cmp al, 0x00
    je .done
    call print_char
    jmp .loop
.done:
    ret

print_char:
    mov ah, 0x0e
    int 0x10
    ret

error_message: db 'Failed to load sector', 0

times 510-($- $$) db 0
dw 0xAA55

buffer: