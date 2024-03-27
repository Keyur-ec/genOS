#ifndef ELF_LOADER_H_
#define ELF_LOADER_H_

#include <stdint.h>
#include <stddef.h>
#include "elf.h"
#include "config.h"

struct elf_file
{
    char filename[ OS_MAX_PATH ];
    int in_memory_size;
    /* the physical memory address that this elf file is loaded at */
    void *elf_memory;
    /* the virtual base address of this binary */
    void *virtual_base_address;
    /* the ending virtual address of this binary */
    void *virtual_end_address;
    /* the physical base address of this binary */
    void *physical_base_address;
    /* the ending physical address of this binary */
    void *physical_end_address;
};

struct elf32_shdr *elf_sheader( struct elf_header *header );
struct elf_header *elf_header( struct elf_file *file );
struct elf32_phdr *elf_pheader( struct elf_header *header );
struct elf32_phdr *elf_program_header( struct elf_header *header,
                                       int index );
struct elf32_shdr *elf_section( struct elf_header *header,
                                int index );
void *elf_memory( struct elf_file *file );
int elf_load( const char *filename,
              struct elf_file **file_out );
void elf_close( struct elf_file *file );
void *elf_virtual_base( struct elf_file *file );
void *elf_virtual_end( struct elf_file *file );
void *elf_physical_base( struct elf_file *file );
void *elf_physical_end( struct elf_file *file );
void *elf_phdr_physical_address( struct elf_file *file,
                                 struct elf32_phdr *phdr );
#endif /* ELF_LOADER_H_ */
