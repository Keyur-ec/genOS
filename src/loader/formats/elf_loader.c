#include "elf_loader.h"
#include "fs/file.h"
#include "status.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "kernel.h"
#include "config.h"
#include <stdbool.h>

const char elf_signature[] = { 0x7F, 'E', 'L', 'F' };

static bool elf_valid_signature( void *signature )
{
    return memcmp( signature, ( void * ) elf_signature, sizeof( elf_signature ) ) == 0;
}

static bool elf_valid_class( struct elf_header *header )
{
    /* we only support 32 bit binaries */
    return header->e_ident[ EI_CLASS ] == ELFCLASSNONE || header->e_ident[ EI_CLASS ] == ELFCLASS32;
}

static bool elf_valid_encoding( struct elf_header *header )
{
    return header->e_ident[ EI_DATA ] == ELFDATANONE || header->e_ident[ EI_DATA ] == ELFDATA2LSB;
}

static bool elf_is_executable( struct elf_header *header )
{
    return header->e_type == ET_EXEC && header->e_entry >= OS_PROGRAM_VIRTUAL_ADDRESS;
}

static bool elf_has_program_header( struct elf_header *header )
{
    return header->e_phoff != 0;
}

void *elf_memory( struct elf_file *file )
{
    return file->elf_memory;
}

struct elf_header *elf_header( struct elf_file *file )
{
    return file->elf_memory;
}

struct elf32_shdr *elf_sheader( struct elf_header *header )
{
    return ( struct elf32_shdr * ) ( ( int ) header + header->e_shoff );
}

struct elf32_phdr *elf_pheader( struct elf_header *header )
{
    if( header->e_phoff == 0 )
    {
        return 0;
    }

    return ( struct elf32_phdr * ) ( ( int ) header + header->e_phoff );
}

struct elf32_phdr *elf_program_header( struct elf_header *header,
                                       int index )
{
    return &elf_pheader( header )[ index ];
}

struct elf32_shdr *elf_section( struct elf_header *header,
                                int index )
{
    return &elf_sheader( header )[ index ];
}

char *elf_str_table( struct elf_header *header )
{
    return ( char * ) header + elf_section( header, header->e_shstrndx )->sh_offset;
}

void *elf_virtual_base( struct elf_file *file )
{
    return file->virtual_base_address;
}

void *elf_virtual_end( struct elf_file *file )
{
    return file->virtual_end_address;
}

void *elf_physical_base( struct elf_file *file )
{
    return file->physical_base_address;
}

void *elf_physical_end( struct elf_file *file )
{
    return file->physical_end_address;
}

int elf_validate_loaded( struct elf_header *header )
{
    return ( elf_valid_signature( header ) && elf_valid_class( header ) && elf_valid_encoding( header ) && elf_has_program_header( header ) ) ? OS_OK : -INVALID_FORMAT_ERROR;
}

int elf_process_phdr_pt_load( struct elf_file *elf_file,
                              struct elf32_phdr *phdr )
{
    if( ( elf_file->virtual_base_address >= ( void * ) phdr->p_paddr ) || ( elf_file->virtual_base_address == 0x00 ) )
    {
        elf_file->virtual_base_address  = ( void * ) phdr->p_vaddr;
        elf_file->physical_base_address = elf_memory( elf_file ) + phdr->p_offset;
    }

    unsigned int end_virtual_address = phdr->p_vaddr + phdr->p_filesz;

    if( ( elf_file->virtual_end_address <= ( void * ) end_virtual_address ) || ( elf_file->virtual_end_address == 0x00 ) )
    {
        elf_file->virtual_end_address  = ( void * ) end_virtual_address;
        elf_file->physical_end_address = elf_memory( elf_file ) + phdr->p_offset + phdr->p_filesz;
    }

    return OS_OK;
}

int elf_process_pheader( struct elf_file *elf_file,
                         struct elf32_phdr *phdr )
{
    int res = OS_OK;

    switch( phdr->p_type )
    {
        case PT_LOAD:
            res = elf_process_phdr_pt_load( elf_file, phdr );
            break;
    }

    return res;
}

int elf_process_pheaders( struct elf_file *elf_file )
{
    int res = OS_OK;
    struct elf_header *header = elf_header( elf_file );

    for( int idx = 0; idx < header->e_phnum; idx++ )
    {
        struct elf32_phdr *phdr = elf_program_header( header, idx );
        res = elf_process_pheader( elf_file, phdr );

        if( res < 0 )
        {
            break;
        }
    }

    return res;
}

int elf_process_loaded( struct elf_file *elf_file )
{
    int res = OS_OK;
    struct elf_header *header = elf_header( elf_file );

    res = elf_validate_loaded( header );

    if( res < 0 )
    {
        return res;
    }

    res = elf_process_pheaders( elf_file );

    if( res < 0 )
    {
        return res;
    }

    return res;
}

int elf_load( const char *filename,
              struct elf_file **file_out )
{
    struct elf_file *elf_file = kzalloc( sizeof( elf_file ) );
    int res = OS_OK;
    int fd  = 0;

    res = fopen( filename, "r" );

    if( res <= 0 )
    {
        res = -IO_ERROR;
        fclose( fd );
        return res;
    }

    fd = res;

    struct file_stat stat;

    res = fstat( fd, &stat );

    if( res < 0 )
    {
        fclose( fd );
        return res;
    }

    elf_file->elf_memory = kzalloc( stat.filesize );
    res = fread( elf_file->elf_memory, stat.filesize, 1, fd );

    if( res < 0 )
    {
        fclose( fd );
        return res;
    }

    res = elf_process_loaded( elf_file );

    if( res < 0 )
    {
        fclose( fd );
        return res;
    }

    *file_out = elf_file;

    fclose( fd );
    return res;
}

void elf_close( struct elf_file *file )
{
    if( !file )
    {
        return;
    }

    kfree( file->elf_memory );
    kfree( file );
}

void *elf_phdr_physical_address( struct elf_file *file,
                                 struct elf32_phdr *phdr )
{
    return elf_memory( file ) + phdr->p_offset;
}
