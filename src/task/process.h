#ifndef PROCESS_H_
#define PROCESS_H_

#include "task/task.h"
#include "config.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PROCESS_FILETYPE_ELF       0x00
#define PROCESS_FILETYPE_BINARY    0x01
typedef unsigned char PROCESS_FILETYPE;

struct command_argument
{
    char argument[ 512 ];
    struct command_argument *next;
};

struct process_arguments
{
    int argc;
    char **argv;
};

struct process_allocation
{
    void *ptr;
    size_t size;
};

struct process
{
    /* the process id */
    uint16_t id;

    char filename[ OS_MAX_PATH ];

    /* the main process task */
    struct task *task;

    /* the memory (malloc) allocations of the process */
    struct process_allocation allocations[ OS_MAX_PROGRAMS_ALLOCATIONS ];

    PROCESS_FILETYPE filetype;

    union
    {
        /* the physical pointer to the process memory */
        void *ptr;
        struct elf_file *elf_file;
    };

    /* the physical pointer to the stack memory */
    void *stack;

    /* the size of the data pointed by ptr */
    uint32_t size;

    struct keyboard_buffer
    {
        char buffer[ OS_KEYBOARD_BUFFER_SIZE ];
        int tail;
        int head;
    }
    keyboard;

    /* the arguments of the process */
    struct process_arguments arguments;
};


int process_load( const char *filename,
                  struct process **process );
struct process *process_current();
int process_switch( struct process *process );
int process_load_switch( const char *filename,
                         struct process **process );
void *process_malloc( struct process *process,
                      size_t size );
void process_free( struct process *process,
                   void *ptr );
void process_get_arguments( struct process *process,
                            int *argc,
                            char ***argv );
int process_inject_arguments( struct process *process,
                              struct command_argument *root_argument );
int process_terminate( struct process *process );

#endif /* PROCESS_H_ */
