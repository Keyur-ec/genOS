#ifndef FILE_H_
#define FILE_H_

#include "path_parser.h"
#include <stdint.h>

struct disk;

typedef unsigned int   FILE_SEEK_MODE;
typedef unsigned int   FILE_MODE;
typedef unsigned int   FILE_STAT_FLAGS;

enum
{
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

enum
{
    FILE_STAT_READ_ONLY = 0b00000001
};

struct file_descriptor
{
    /* the descriptor index */
    int index;
    struct filesystem *filesystem;
    /* private data for internal file descriptor */
    void *private;
    /* the disk that the file descriptor should be used on */
    struct disk *disk;
};

struct file_stat
{
    FILE_STAT_FLAGS flags;
    uint32_t filesize;
};

typedef void *(*FS_OPEN_FUNCTION)( struct disk *disk,
                                   struct path_part *path,
                                   FILE_MODE mode );
typedef int (*FS_READ_FUNCTION)( struct disk *disk,
                                 void *private,
                                 uint32_t size,
                                 uint32_t nmemb,
                                 char *out );
typedef int (*FS_SEEK_FUNCTION)( void *private,
                                 uint32_t offset,
                                 FILE_SEEK_MODE );
typedef int (*FS_RESOLVE_FUNCTION)( struct disk *disk );
typedef int (*FS_STAT_FUNCTION)( struct disk *disk,
                                 void *private,
                                 struct file_stat *stat );
typedef int (*FS_CLOSE_FUNCTION)( void *private );

struct filesystem
{
    /* filesystem should return zero from resolve if the provideed disk is using it's filesystem */
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;

    char name[ 20 ];
};

void fs_init();
int fopen( const char *filename,
           const char *mode_str );
int fread( void *ptr,
           uint32_t size,
           uint32_t nmemb,
           int fd );
int fseek( int fd,
           int offset,
           FILE_SEEK_MODE whence );
int fstat( int fd,
           struct file_stat *stat );
int fclose( int fd );
void fs_insert_filesystem( struct filesystem *filesystem );
struct filesystem *fs_resolve( struct disk *disk );

#endif /* FILE_H_ */
