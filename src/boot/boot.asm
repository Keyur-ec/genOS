ORG 0x7c00
BITS 16

CODE_SEL equ gdt_code - gdt_start
DATA_SEL equ gdt_data - gdt_start

jmp short start
nop

; FAT16 header
OEMIdentifier       db 'KEYUR_OS'
BytesPerSector      dw 0x200
SectorsPerCluster   db 0x80
ReserverdSectors    dw 200
FATCopies           db 0x02
RootDirEntries      dw 0x40
NumSectors          dw 0x00
MediaType           db 0xF8
SectorsPerFAT       dw 0x100
SectorsPerTrack     dw 0x20
NumberOfHeads       dw 0x40
HiddenSectors       dd 0x00
SectorsBig          dd 0x773594

; Extended BPB (Dos 4.0)
DriveNumber         db 0x80
WinNTBit            db 0x00
Signature           db 0x29
VolumeID            dd 0xD105
VolumeIDString      db 'KEYUROSBOOT'
SystemIDString      db 'FAT16   '

start:
    jmp 0:main

main:
    cli             ; clearing the interrupts
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti             ; setting the interrupts
.load_protected:
    cli
    lgdt[gdt_descriptor]
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax
    jmp CODE_SEL:load32

; Global Descriptor Table (GDT)
gdt_start:

gdt_null:
    dd 0x00
    dd 0x00

; offset 0x08
gdt_code:
    dw 0xffff       ; segment limit first 0-15 bits
    dw 0x00         ; base 0-15 bits
    db 0x00         ; base 16-23 bits
    db 0x9a         ; access byte
    db 11001111b    ; high 4 bit falg and low bit flags
    db 0            ; base 24-31 bits     

; offset 0x10
gdt_data:
    dw 0xffff       ; segment limit first 0-15 bits
    dw 0x00         ; base 0-15 bits
    db 0x00         ; base 16-23 bits
    db 0x92         ; access byte
    db 11001111b    ; high 4 bit falg and low bit flags
    db 0            ; base 24-31 bits     

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

BITS 32
load32:
    mov eax, 0x01       ; starting sector number to load
    mov ecx, 100        ; total number of sector to load
    mov edi, 0x0100000  ; address we want to load them into
    call ata_lba_read
    jmp CODE_SEL:0x0100000

ata_lba_read:
    mov ebx, eax    ; backup the LBA
    
    ; send highest 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0xE0    ; select the master drive
    mov dx, 0x1F6
    out dx, al
    ; finish sending the highest 8 bits of the lba to hard disk controller

    ; send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; finish aending the total sectors to read

    ; send more bits of the lba
    mov eax, ebx
    mov dx, 0x1F3
    out dx, al
    ; finish sending more bits of the lba

    ; send more bits of the lba
    mov dx, 0x1F4
    mov eax, ebx    ; restore the backup lba
    shr eax, 8
    out dx, al
    ; finish sending more bits of the lba

    ; send upper 16 bits of the lba
    mov dx, 0x1F5
    mov eax, ebx    ; restore the backup lba
    shr eax, 16
    out dx, al
    ; fininsh sending upper 16 bits of the lba

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

; read all sectors into memory
.next_sector:
    push ecx

; checking if we need to read
.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .try_again

; we need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next_sector
; end of reading sectors into memory
    ret

times 510-($- $$) db 0
dw 0xAA55


; target remote | qemu-system-x86_64 -hda ./boot.bin -S -gdb stdio
; target remote | qemu-system-x86_64 -hda ./os.bin -S -gdb stdio
; target remote | qemu-system-i386 -hda ./os.bin -S -gdb stdio