[BITS 32]

section .asm

global print:function
global os_getkey:function
global os_putchar:function
global os_malloc:function
global os_free:function
global os_process_load_start:function
global os_system:function
global os_process_get_arguments:function
global os_exit:function

; void print(const char* filename)
print:
    push ebp            ; saving state of processor
    mov ebp, esp

    push dword [ebp+8]
    mov eax, 1          ; command print
    int 0x80
    add esp, 4

    pop ebp             ; retrive state of processor
    ret

; int os_getkey()
os_getkey:
    push ebp            ; saving state of processor
    mov ebp, esp

    mov eax, 2          ; command getkey
    int 0x80

    pop ebp             ; retrive state of processor
    ret

; int os_putchar(int chr)
os_putchar:
    push ebp            ; saving state of processor
    mov ebp, esp

    push dword [ebp+8]  ; argument 'chr'
    mov eax, 3          ; command putchar
    int 0x80
    add esp, 4

    pop ebp             ; retrive state of processor
    ret

; void *os_malloc(size_t size)
os_malloc:
    push ebp            ; saving state of processor
    mov ebp, esp

    push dword [ebp+8]  ; argument 'size'
    mov eax, 4          ; command malloc ( allocates memory for the process )
    int 0x80
    add esp, 4

    pop ebp             ; retrive state of processor
    ret

; void os_free(void *ptr)
os_free:
    push ebp            ; saving state of processor
    mov ebp, esp

    push dword [ebp+8]  ; argument 'ptr'
    mov eax, 5          ; command free ( free up allocated memory for the process )
    int 0x80
    add esp, 4

    pop ebp             ; retrive state of processor
    ret

; void os_process_load_start(const char *filename)
os_process_load_start:
    push ebp            ; saving state of processor
    mov ebp, esp

    push dword [ebp+8]  ; argument 'filename'
    mov eax, 6          ; command process load start
    int 0x80
    add esp, 4

    pop ebp             ; retrive state of processor
    ret

; int os_system(struct command_argument* arguments)
os_system:
    push ebp            ; saving state of processor
    mov ebp, esp

    push dword [ebp+8]  ; argument 'arguments'
    mov eax, 7          ; command process_system (runs a system command based on the arguments)
    int 0x80
    add esp, 4

    pop ebp             ; retrive state of processor
    ret    

; void os_process_get_arguments(struct process_arguments* arguments)
os_process_get_arguments:
    push ebp            ; saving state of processor
    mov ebp, esp

    push dword [ebp+8]  ; argument 'arguments'
    mov eax, 8          ; command get process arguments
    int 0x80
    add esp, 4

    pop ebp             ; retrive state of processor
    ret

; void os_exit()
os_exit:
    push ebp            ; saving state of processor
    mov ebp, esp

    mov eax, 9          ; command process exit
    int 0x80

    pop ebp             ; retrive state of processor
    ret
