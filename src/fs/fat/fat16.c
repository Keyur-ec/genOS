#include "fat16.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "kernel.h"
#include "config.h"

struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open    = fat16_open,
    .read    = fat16_read,
    .seek    = fat16_seek,
    .stat    = fat16_stat,
    .close   = fat16_close
};

struct filesystem *fat16_init()
{
    strcpy( fat16_fs.name, "FAT16" );
    return &fat16_fs;
}

static void fat16_init_private( struct disk *disk,
                                struct fat_private *private )
{
    bzero( private, sizeof( struct fat_private ) );
    private->cluster_read_stream = diskstreamer_new( disk->id );
    private->fat_read_stream     = diskstreamer_new( disk->id );
    private->directory_stream    = diskstreamer_new( disk->id );
}

int fat16_sector_to_absolute( struct disk *disk,
                              int sector )
{
    return sector * disk->sector_size;
}

int fat16_get_total_items_for_directory( struct disk *disk,
                                         uint32_t directory_start_sector )
{
    struct fat_directory_item item;
    struct fat_directory_item empty_item;

    bzero( &empty_item, sizeof( empty_item ) );

    struct fat_private *fat_private = disk->fs_private;

    int res = OS_OK;
    int idx = 0;
    int directory_start_pos    = directory_start_sector * disk->sector_size;
    struct disk_stream *stream = fat_private->directory_stream;

    if( diskstreamer_seek( stream, directory_start_pos ) != OS_OK )
    {
        res = -IO_ERROR;
        return res;
    }

    while( 1 )
    {
        if( diskstreamer_read( stream, &item, sizeof( item ) ) != OS_OK )
        {
            res = -IO_ERROR;
            return res;
        }

        if( item.filename[ 0 ] == 0x00 )
        {
            /* we are done */
            break;
        }

        /* is item is unused */
        if( item.filename[ 0 ] == OS_DIRECTORY_ENTRY_IS_FREE )
        {
            continue;
        }

        idx++;
    }

    res = idx;

    return res;
}

int fat16_get_root_directory( struct disk *disk,
                              struct fat_private *fat_private,
                              struct fat_directory *directory )
{
    int res = OS_OK;
    struct fat_directory_item *dir    = 0;
    struct fat_header *primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos           = ( primary_header->fat_copies * primary_header->sectors_per_fat ) + primary_header->reserved_sectors;
    int root_dir_entires = fat_private->header.primary_header.root_dir_entries;
    int root_dir_size    = ( root_dir_entires * sizeof( struct fat_directory_item ) );
    int total_sectors    = root_dir_size / disk->sector_size;

    if( root_dir_size % disk->sector_size )
    {
        total_sectors += 1;
    }

    int total_items = fat16_get_total_items_for_directory( disk, root_dir_sector_pos );

    dir = kzalloc( root_dir_size );

    if( !dir )
    {
        res = -NO_MEMORY_ERROR;

        if( dir )
        {
            kfree( dir );
        }

        return res;
    }

    struct disk_stream *stream = fat_private->directory_stream;

    if( diskstreamer_seek( stream, fat16_sector_to_absolute( disk, root_dir_sector_pos ) ) != OS_OK )
    {
        res = -IO_ERROR;

        if( dir )
        {
            kfree( dir );
        }

        return res;
    }

    if( diskstreamer_read( stream, dir, root_dir_size ) != OS_OK )
    {
        res = -IO_ERROR;

        if( dir )
        {
            kfree( dir );
        }

        return res;
    }

    directory->item = dir;
    directory->total_number_of_items = total_items;
    directory->sector_pos            = root_dir_sector_pos;
    directory->ending_sector_pos     = root_dir_sector_pos + ( root_dir_size / disk->sector_size );

    return res;
}

int fat16_resolve( struct disk *disk )
{
    int res = OS_OK;
    struct fat_private *fat_private = kzalloc( sizeof( struct fat_private ) );

    fat16_init_private( disk, fat_private );

    disk->fs_private = fat_private;
    disk->filesystem = &fat16_fs;

    struct disk_stream *stream = diskstreamer_new( disk->id );

    if( !stream )
    {
        res = -NO_MEMORY_ERROR;

        if( stream )
        {
            diskstreamer_close( stream );
        }

        if( res < 0 )
        {
            kfree( fat_private );
            disk->fs_private = 0;
        }

        return res;
    }

    if( diskstreamer_read( stream, &fat_private->header, sizeof( fat_private->header ) ) != OS_OK )
    {
        res = -IO_ERROR;

        if( stream )
        {
            diskstreamer_close( stream );
        }

        if( res < 0 )
        {
            kfree( fat_private );
            disk->fs_private = 0;
        }

        return res;
    }

    if( fat_private->header.shared.extended_header.signature != OS_FAT16_SIGNATURE )
    {
        res = -FS_NOT_US_ERROR;

        if( stream )
        {
            diskstreamer_close( stream );
        }

        if( res < 0 )
        {
            kfree( fat_private );
            disk->fs_private = 0;
        }

        return res;
    }

    if( fat16_get_root_directory( disk, fat_private, &fat_private->root_directory ) != OS_OK )
    {
        res = -IO_ERROR;

        if( stream )
        {
            diskstreamer_close( stream );
        }

        if( res < 0 )
        {
            kfree( fat_private );
            disk->fs_private = 0;
        }

        return res;
    }

    if( stream )
    {
        diskstreamer_close( stream );
    }

    if( res < 0 )
    {
        kfree( fat_private );
        disk->fs_private = 0;
    }

    return res;
}

void fat16_to_proper_string( char **out,
                             const char *in,
                             size_t size )
{
    int i = 0;

    while( *in != 0x00 && *in != 0x20 )
    {
        **out = *in;
        *out += 1;
        in   += 1;

        /* we can't process anymore since we have exceeded the input buffer size */
        if( i >= size - 1 )
        {
            break;
        }

        i++;
    }

    **out = 0x00;
}

void fat16_get_full_relative_filename( struct fat_directory_item *item,
                                       char *out,
                                       int max_len )
{
    bzero( out, max_len );
    char *out_temp = out;

    fat16_to_proper_string( &out_temp, ( const char * ) item->filename, sizeof( item->filename ) );

    if( ( item->ext[ 0 ] != 0x00 ) && ( item->ext[ 0 ] != 0x20 ) )
    {
        *out_temp++ = '.';
        fat16_to_proper_string( &out_temp, ( const char * ) item->ext, sizeof( item->ext ) );
    }
}

struct fat_directory_item *fat16_clone_directory_item( struct fat_directory_item *item,
                                                       int size )
{
    struct fat_directory_item *item_copy = 0;

    if( size < sizeof( struct fat_directory_item ) )
    {
        return 0;
    }

    item_copy = kzalloc( size );

    if( !item_copy )
    {
        return 0;
    }

    memcpy( item_copy, item, size );

    return item_copy;
}

static uint32_t fat16_get_first_cluster( struct fat_directory_item *item )
{
    return ( item->high_16_bits_first_cluster ) | item->low_16_bits_first_cluster;
}

static int fat16_cluster_to_sector( struct fat_private *private,
                                    int cluster )
{
    return private->root_directory.ending_sector_pos + ( ( cluster - 2 ) *private->header.primary_header.sectors_per_cluster );
}

static uint32_t fat16_get_first_fat_sector( struct fat_private *private )
{
    return private->header.primary_header.reserved_sectors;
}

static int fat16_get_fat_entry( struct disk *disk,
                                int cluster )
{
    int res = -1;
    struct fat_private *private = disk->fs_private;
    struct disk_stream *stream = private->fat_read_stream;

    if( !stream )
    {
        return res;
    }

    uint32_t fat_table_position = fat16_get_first_fat_sector( private ) * disk->sector_size;

    res = diskstreamer_seek( stream, fat_table_position * ( cluster * OS_FAT16_FAT_ENTRY_SIZE ) );

    if( res < 0 )
    {
        return res;
    }

    uint16_t result = 0;

    res = diskstreamer_read( stream, &result, sizeof( result ) );

    if( res < 0 )
    {
        return res;
    }

    res = result;

    return res;
}

/* get the correct cluster to use based on the starting cluster and the offset */
static int fat16_get_cluster_for_offset( struct disk *disk,
                                         int starting_cluster,
                                         int offset )
{
    int res = OS_OK;
    struct fat_private *private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use        = starting_cluster;
    int cluster_ahead         = offset / size_of_cluster_bytes;

    for( int idx = 0; idx < cluster_ahead; idx++ )
    {
        int entry = fat16_get_fat_entry( disk, cluster_to_use );

        if( ( entry == 0xFF8 ) || ( entry == 0xFFF ) )
        {
            /* we are at the last entry in the file */
            res = -IO_ERROR;
            return res;
        }

        /* sector is marked as bad? */
        if( entry == OS_FAT16_BAD_SECTOR )
        {
            res = -IO_ERROR;
            return res;
        }

        /* reserved sectors? */
        if( ( entry == 0xFF0 ) || ( entry == 0xFF6 ) )
        {
            res = -IO_ERROR;
            return res;
        }

        if( entry == 0x00 )
        {
            res = -IO_ERROR;
            return res;
        }

        cluster_to_use = entry;
    }

    res = cluster_to_use;

    return res;
}

static int fat16_read_internal_from_stream( struct disk *disk,
                                            struct disk_stream *stream,
                                            int cluster,
                                            int offset,
                                            int total_bytes_to_read,
                                            void *out )
{
    int res = OS_OK;
    struct fat_private *private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use        = fat16_get_cluster_for_offset( disk, cluster, offset );

    if( cluster_to_use < 0 )
    {
        res = cluster_to_use;
        return res;
    }

    int offset_from_cluster = offset % size_of_cluster_bytes;
    int starting_sector     = fat16_cluster_to_sector( private, cluster_to_use );
    int starting_pos        = ( starting_sector * disk->sector_size ) + offset_from_cluster;
    int total_to_read       = total_bytes_to_read > size_of_cluster_bytes ? size_of_cluster_bytes : total_bytes_to_read;

    res = diskstreamer_seek( stream, starting_pos );

    if( res != OS_OK )
    {
        return res;
    }

    res = diskstreamer_read( stream, out, total_to_read );

    if( res != OS_OK )
    {
        return res;
    }

    total_bytes_to_read -= total_to_read;

    if( total_bytes_to_read > 0 )
    {
        /* we still have to read */
        res = fat16_read_internal_from_stream( disk, stream, cluster, offset + total_to_read, total_bytes_to_read, out + total_to_read );
    }

    return res;
}

static int fat16_read_internal( struct disk *disk,
                                int starting_cluster,
                                int offset,
                                int total,
                                void *out )
{
    struct fat_private *fs_private = disk->fs_private;
    struct disk_stream *stream     = fs_private->cluster_read_stream;

    return fat16_read_internal_from_stream( disk, stream, starting_cluster, offset, total, out );
}

void fat16_free_directory( struct fat_directory *directory )
{
    if( !directory )
    {
        return;
    }

    if( directory->item )
    {
        kfree( directory->item );
    }

    kfree( directory );
}

void fat16_fat_item_free( struct fat_item *item )
{
    if( item->type == FAT_ITEM_TYPE_DIRECTORY )
    {
        fat16_free_directory( item->directory );
    }
    else if( item->type == FAT_ITEM_TYPE_FILE )
    {
        kfree( item->item );
    }

    kfree( item );
}

struct fat_directory *fat16_load_fat_directory( struct disk *disk,
                                                struct fat_directory_item *item )
{
    int res = OS_OK;
    struct fat_directory *directory = 0;
    struct fat_private *fat_private = disk->fs_private;

    if( !( item->attribute & FAT_FILE_SUBDIRECTORY ) )
    {
        res = -INVALID_ARGUMENT_ERROR;

        if( res != OS_OK )
        {
            fat16_free_directory( directory );
        }

        return directory;
    }

    directory = kzalloc( sizeof( struct fat_directory ) );

    if( !directory )
    {
        res = -NO_MEMORY_ERROR;

        if( res != OS_OK )
        {
            fat16_free_directory( directory );
        }

        return directory;
    }

    int cluster        = fat16_get_first_cluster( item );
    int cluster_sector = fat16_cluster_to_sector( fat_private, cluster );
    int total_items    = fat16_get_total_items_for_directory( disk, cluster_sector );

    directory->total_number_of_items = total_items;
    int directory_size = directory->total_number_of_items * sizeof( struct fat_directory_item );

    directory->item = kzalloc( directory_size );

    if( !directory->item )
    {
        res = -NO_MEMORY_ERROR;

        if( res != OS_OK )
        {
            fat16_free_directory( directory );
        }

        return directory;
    }

    res = fat16_read_internal( disk, cluster, 0x00, directory_size, directory->item );

    if( res != OS_OK )
    {
        fat16_free_directory( directory );
        return directory;
    }

    return directory;
}

struct fat_item *fat16_new_fat_item_for_directory_item( struct disk *disk,
                                                        struct fat_directory_item *item )
{
    struct fat_item *f_item = kzalloc( sizeof( struct fat_item ) );

    if( !f_item )
    {
        return 0;
    }

    if( item->attribute & FAT_FILE_SUBDIRECTORY )
    {
        f_item->directory = fat16_load_fat_directory( disk, item );
        f_item->type      = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_directory_item( item, sizeof( struct fat_directory_item ) );

    return f_item;
}

struct fat_item *fat16_find_item_in_directory( struct disk *disk,
                                               struct fat_directory *directory,
                                               const char *name )
{
    struct fat_item *f_item = 0;
    char temp_filename[ OS_MAX_PATH ];

    for( int idx = 0; idx < directory->total_number_of_items; idx++ )
    {
        fat16_get_full_relative_filename( &directory->item[ idx ], temp_filename, sizeof( temp_filename ) );

        if( istrncmp( temp_filename, name, sizeof( temp_filename ) ) == 0 )
        {
            /* found it lets create a new fat_item */
            f_item = fat16_new_fat_item_for_directory_item( disk, &directory->item[ idx ] );
        }
    }

    return f_item;
}

struct fat_item *fat16_get_directory_entry( struct disk *disk,
                                            struct path_part *path )
{
    struct fat_private *fat_private = disk->fs_private;
    struct fat_item *current_item   = 0;
    struct fat_item *root_item      = fat16_find_item_in_directory( disk, &fat_private->root_directory, path->part );

    if( !root_item )
    {
        return current_item;
    }

    struct path_part *next_part = path->next;

    current_item = root_item;

    while( next_part != 0 )
    {
        if( current_item->type != FAT_ITEM_TYPE_DIRECTORY )
        {
            current_item = 0;
            break;
        }

        struct fat_item *temp_item = fat16_find_item_in_directory( disk, current_item->directory, next_part->part );
        fat16_fat_item_free( current_item );
        current_item = temp_item;
        next_part    = next_part->next;
    }

    return current_item;
}

void *fat16_open( struct disk *disk,
                  struct path_part *path,
                  FILE_MODE mode )
{
    struct fat_file_descriptor *descriptor = 0;
    int res = 0;

    if( mode != FILE_MODE_READ )
    {
        res = -READ_ONLY_ERROR;

        if( descriptor )
        {
            kfree( descriptor );
        }

        return ERROR( res );
    }

    descriptor = kzalloc( sizeof( struct fat_file_descriptor ) );

    if( !descriptor )
    {
        res = -NO_MEMORY_ERROR;

        if( descriptor )
        {
            kfree( descriptor );
        }

        return ERROR( res );
    }

    descriptor->item = fat16_get_directory_entry( disk, path );

    if( !descriptor->item )
    {
        res = -IO_ERROR;

        if( descriptor )
        {
            kfree( descriptor );
        }

        return ERROR( res );
    }

    descriptor->pos = 0;

    return descriptor;
}

int fat16_read( struct disk *disk,
                void *descriptor,
                uint32_t size,
                uint32_t nmemb,
                char *out_ptr )
{
    int res = OS_OK;
    struct fat_file_descriptor *fat_desc = descriptor;
    struct fat_directory_item *item      = fat_desc->item->item;
    int offset = fat_desc->pos;

    for( uint32_t idx = 0; idx < nmemb; idx++ )
    {
        res = fat16_read_internal( disk, fat16_get_first_cluster( item ), offset, size, out_ptr );

        if( ISERR( res ) )
        {
            return res;
        }

        out_ptr += size;
        offset  += size;
    }

    res = nmemb;

    return res;
}

int fat16_seek( void *private,
                uint32_t offset,
                FILE_SEEK_MODE seek_mode )
{
    int res = OS_OK;
    struct fat_file_descriptor *desc = private;
    struct fat_item *desc_item       = desc->item;

    if( desc_item->type != FAT_ITEM_TYPE_FILE )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    struct fat_directory_item *ritem = desc_item->item;

    if( offset >= ritem->filesize )
    {
        res = -IO_ERROR;
        return res;
    }

    switch( seek_mode )
    {
        case SEEK_SET:
            desc->pos = offset;
            break;

        case SEEK_END:
            res = -UNIMPLIMENTED_ERROR;
            break;

        case SEEK_CUR:
            desc->pos += offset;
            break;

        default:
            res = -INVALID_ARGUMENT_ERROR;
            break;
    }

    return res;
}

int fat16_stat( struct disk *disk,
                void *private,
                struct file_stat *stat )
{
    int res = OS_OK;
    struct fat_file_descriptor *desc = ( struct fat_file_descriptor * ) private;
    struct fat_item *desc_item       = desc->item;

    if( desc_item->type != FAT_ITEM_TYPE_FILE )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    struct fat_directory_item *ritem = desc_item->item;

    stat->filesize = ritem->filesize;
    stat->flags    = 0x00;

    if( ritem->attribute & FAT_FILE_READ_ONLY )
    {
        stat->flags |= FILE_STAT_READ_ONLY;
    }

    return res;
}

static void fat16_free_file_descriptor( struct fat_file_descriptor *desc )
{
    fat16_fat_item_free( desc->item );
    kfree( desc );
}

int fat16_close( void *private )
{
    fat16_free_file_descriptor( ( struct fat_file_descriptor * ) private );
    return OS_OK;
}
