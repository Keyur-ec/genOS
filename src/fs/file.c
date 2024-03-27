#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "kernel.h"
#include "fat/fat16.h"
#include "disk/disk.h"
#include "string/string.h"

struct filesystem *filesystems[ OS_MAX_FILESYSTEMS ];
struct file_descriptor *file_descriptors[ OS_MAX_FILEDISCRIPTORS ];

static struct filesystem **fs_get_free_filesystem()
{
    for( int idx = 0; idx < OS_MAX_FILESYSTEMS; idx++ )
    {
        if( filesystems[ idx ] == 0 )
        {
            return &filesystems[ idx ];
        }
    }

    return 0;
}

void fs_insert_filesystem( struct filesystem *filesystem )
{
    struct filesystem **fs;

    fs = fs_get_free_filesystem();

    if( !fs )
    {
        print( "problem inserting filesystem!" );

        while( 1 )
        {
        }
    }

    *fs = filesystem;
}

static void fs_static_load()
{
    fs_insert_filesystem( fat16_init() );
}

void fs_load()
{
    bzero( filesystems, sizeof( filesystems ) );
    fs_static_load();
}

void fs_init()
{
    bzero( file_descriptors, sizeof( file_descriptors ) );
    fs_load();
}

static void file_free_descriptor( struct file_descriptor *desc )
{
    file_descriptors[ desc->index - 1 ] = 0x00;
    kfree( desc );
}

static int file_new_descriptor( struct file_descriptor **descriptor_out )
{
    int res = -NO_MEMORY_ERROR;

    for( int idx = 0; idx < OS_MAX_FILEDISCRIPTORS; idx++ )
    {
        if( file_descriptors[ idx ] == 0 )
        {
            struct file_descriptor *desc = kzalloc( sizeof( struct file_descriptor ) );
            /* descriptor start at 1 */
            desc->index             = idx + 1;
            file_descriptors[ idx ] = desc;
            *descriptor_out         = desc;
            res = OS_OK;
            break;
        }
    }

    return res;
}

static struct file_descriptor *file_get_descriptor( int fd )
{
    if( ( fd <= 0 ) || ( fd >= OS_MAX_FILEDISCRIPTORS ) )
    {
        return 0;
    }

    /* descriptor start at 1 */
    int index = fd - 1;

    return file_descriptors[ index ];
}

struct filesystem *fs_resolve( struct disk *disk )
{
    struct filesystem *fs = 0;

    for( int idx = 0; idx < OS_MAX_FILESYSTEMS; idx++ )
    {
        if( ( filesystems[ idx ] != 0 ) && ( filesystems[ idx ]->resolve( disk ) == 0 ) )
        {
            fs = filesystems[ idx ];
            break;
        }
    }

    return fs;
}

FILE_MODE file_get_mode_by_string( const char *str )
{
    FILE_MODE mode = FILE_MODE_INVALID;

    if( strncmp( str, "r", 1 ) == 0 )
    {
        mode = FILE_MODE_READ;
    }
    else if( strncmp( str, "w", 1 ) == 0 )
    {
        mode = FILE_MODE_WRITE;
    }
    else if( strncmp( str, "a", 1 ) == 0 )
    {
        mode = FILE_MODE_APPEND;
    }

    return mode;
}

int fopen( const char *filename,
           const char *mode_str )
{
    int res = OS_OK;

    struct path_root *root_path = pathparser_parse( filename, NULL );

    if( !root_path )
    {
        res = -INVALID_ARGUMENT_ERROR;

        /* fopen should not return negative value */
        if( res < 0 )
        {
            res = 0;
        }

        return res;
    }

    /* we cannot have just a root path 0:/ */
    if( !root_path->first )
    {
        res = -INVALID_ARGUMENT_ERROR;

        /* fopen should not return negative value */
        if( res < 0 )
        {
            res = 0;
        }

        return res;
    }

    /* ensuring the disk we are reading from exists */
    struct disk *disk = disk_get( root_path->drive_no );

    if( !disk )
    {
        res = -IO_ERROR;

        /* fopen should not return negative value */
        if( res < 0 )
        {
            res = 0;
        }

        return res;
    }

    if( !disk->filesystem )
    {
        res = -IO_ERROR;

        /* fopen should not return negative value */
        if( res < 0 )
        {
            res = 0;
        }

        return res;
    }

    FILE_MODE mode = file_get_mode_by_string( mode_str );

    if( mode == FILE_MODE_INVALID )
    {
        res = -INVALID_ARGUMENT_ERROR;

        /* fopen should not return negative value */
        if( res < 0 )
        {
            res = 0;
        }

        return res;
    }

    void *descriptor_private_data = disk->filesystem->open( disk, root_path->first, mode );

    if( ISERR( descriptor_private_data ) )
    {
        res = ERROR_I( descriptor_private_data );

        /* fopen should not return negative value */
        if( res < 0 )
        {
            res = 0;
        }

        return res;
    }

    struct file_descriptor *desc = 0;

    res = file_new_descriptor( &desc );

    /* fopen should not return negative value */
    if( res < 0 )
    {
        res = 0;
        return res;
    }

    desc->filesystem = disk->filesystem;
    desc->private    = descriptor_private_data;
    desc->disk       = disk;
    res = desc->index;

    /* fopen should not return negative value */
    if( res < 0 )
    {
        res = 0;
    }

    return res;
}

int fread( void *ptr,
           uint32_t size,
           uint32_t nmemb,
           int fd )
{
    int res = OS_OK;

    if( ( size == 0 ) || ( nmemb == 0 ) || ( fd < 1 ) )
    {
        res = -IO_ERROR;
        return res;
    }

    struct file_descriptor *desc = file_get_descriptor( fd );

    if( !fd )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    res = desc->filesystem->read( desc->disk, desc->private, size, nmemb, ( char * ) ptr );

    return res;
}

int fseek( int fd,
           int offset,
           FILE_SEEK_MODE whence )
{
    int res = OS_OK;
    struct file_descriptor *desc = file_get_descriptor( fd );

    if( !desc )
    {
        res = -IO_ERROR;
        return res;
    }

    res = desc->filesystem->seek( desc->private, offset, whence );

    return res;
}

int fstat( int fd,
           struct file_stat *stat )
{
    int res = OS_OK;
    struct file_descriptor *desc = file_get_descriptor( fd );

    if( !desc )
    {
        res = -IO_ERROR;
        return res;
    }

    res = desc->filesystem->stat( desc->disk, desc->private, stat );

    return res;
}

int fclose( int fd )
{
    int res = OS_OK;
    struct file_descriptor *desc = file_get_descriptor( fd );

    if( !desc )
    {
        res = -IO_ERROR;
        return res;
    }

    res = desc->filesystem->close( desc->private );

    if( res == OS_OK )
    {
        file_free_descriptor( desc );
    }

    return res;
}
