#ifndef IDT_H_
#define IDT_H_

#include <stdint.h>

#define IDT_TYPE_TASK_GATE_32BIT         0x5
#define IDT_TYPE_INTERRUPT_GATE_16BIT    0x6
#define IDT_TYPE_TRAP_GATE_16BIT         0x7
#define IDT_TYPE_INTERRUPT_GATE_32BIT    0xE
#define IDT_TYPE_TRAP_GATE_32BIT         0xF

#define LOWER_OFFSET_ADDRESS_MASK        0x0000FFFF
#define UNUSED_FIELD                     0x00
#define PRIVILEGE_LEVEL                  3 /* user space */
#define INTERRUPT_AND_TRAP               0
#define UNUSED                           0

struct idt_desc
{
    uint16_t offset_1;   /* offset bits 0 - 15 */
    uint16_t selector;   /* selector thats in our GDT */
    uint8_t zero;        /* done nothing, unused set to zero */
    uint8_t type_attr;   /* descriptor type and attributes */
    /* uint8_t type_attr : 4;   / * descriptor type and attributes * / */
    /* uint8_t storage_seg : 1; / * storage segment * / */
    /* uint8_t dpl : 2;         / * descriptor privilege level * / */
    /* uint8_t present : 1;     / * present * / */
    uint16_t offset_2;       /* offset bits 16 - 31 */
}
__attribute__( ( packed ) );

struct idtr_desc
{
    uint16_t limit; /* size of descriptor table -1 */
    uint32_t base;  /* base addr. of the start of the interrupt descriptor table */
}
__attribute__( ( packed ) );

struct interrupt_frame
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
}
__attribute__( ( packed ) );

typedef void *( *ISR80H_COMMAND )( struct interrupt_frame *frame );
typedef void ( *INTERRUPT_CALLBACK_FUNCTION )();

void idt_init();
void enable_interrupts();
void disable_interrupts();
void isr80h_register_command( int command_id,
                              ISR80H_COMMAND command );
int idt_register_interrupt_callback( int interrupt,
                                     INTERRUPT_CALLBACK_FUNCTION interrupt_callback );
void idt_clock();

#endif /* IDT_H_ */
