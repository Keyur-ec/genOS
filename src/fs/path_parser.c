#include "path_parser.h"
#include "config.h"
#include "string/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"
#include "kernel.h"

static int pathparser_path_valid_format( const char *filename )
{
    int len = strnlen( filename, OS_MAX_PATH );

    return( len >= 3 && isDigit( filename[ 0 ] ) && memcmp( ( void * ) &filename[ 1 ], ":/", 2 ) == 0 );
}

static int pathparser_get_drive_by_path( const char **path )
{
    if( !pathparser_path_valid_format( *path ) )
    {
        return BAD_PATH_ERROR;
    }

    int drive_no = toDigit( *path[ 0 ] );

    /* add 3 bytes to skip drive number 0:/ 1:/ 2:/ */
    *path += 3;

    return drive_no;
}

static struct path_root *pathparser_create_root( int drive_no )
{
    struct path_root *path_root = kzalloc( sizeof( struct path_root ) );

    path_root->drive_no = drive_no;
    path_root->first    = 0;

    return path_root;
}

static const char *pathparser_get_path_part( const char **path )
{
    char *result_path_part = kzalloc( OS_MAX_PATH );
    int idx = 0;

    while( **path != '/' && **path != 0x00 )
    {
        result_path_part[ idx ] = **path;
        *path += 1;
        idx++;
    }

    if( **path == '/' )
    {
        /* skip the forward slash to avoid problems */
        *path += 1;
    }

    if( idx == 0 )
    {
        kfree( result_path_part );
        result_path_part = 0;
    }

    return result_path_part;
}

struct path_part *pathparser_parser_path_part( struct path_part *last_part,
                                               const char **path )
{
    const char *path_part_str = pathparser_get_path_part( path );

    if( !path_part_str )
    {
        return 0;
    }

    struct path_part *part = kzalloc( sizeof( struct path_part ) );

    part->part = path_part_str;
    part->next = 0x00;

    if( last_part )
    {
        last_part->next = part;
    }

    return part;
}

void pathparser_free( struct path_root *root )
{
    struct path_part *part = root->first;

    while( part )
    {
        struct path_part *next_part = part->next;
        kfree( ( void * ) part->part );
        kfree( part );
        part = next_part;
    }

    kfree( root );
}

struct path_root *pathparser_parse( const char *path,
                                    const char *current_directory_path )
{
    int res = OS_OK;
    const char *temp_path       = path;
    struct path_root *path_root = 0;

    if( strlen( path ) > OS_MAX_PATH )
    {
        return path_root;
    }

    res = pathparser_get_drive_by_path( &temp_path );

    if( res < 0 )
    {
        return path_root;
    }

    path_root = pathparser_create_root( res );

    if( !path_root )
    {
        return path_root;
    }

    struct path_part *first_part = pathparser_parser_path_part( NULL, &temp_path );

    if( !first_part )
    {
        return path_root;
    }

    path_root->first = first_part;
    struct path_part *part = pathparser_parser_path_part( first_part, &temp_path );

    while( part )
    {
        part = pathparser_parser_path_part( part, &temp_path );
    }

    return path_root;
}
