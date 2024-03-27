section .asm

global tss_load

tss_load:
    push ebp            ; saving state of processor
    mov ebp, esp

    mov ax, [ebp+8]     ; TSS segment
    ltr ax

    pop ebp             ; retrive state of processor
    ret
