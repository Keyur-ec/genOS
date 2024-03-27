#include "memory.h"

void *memset( void *ptr,
              int chr,
              size_t size )
{
    char *c_ptr = ( char * ) ptr;

    for( int index = 0; index < ( int ) size; index++ )
    {
        c_ptr[ index ] = ( char ) chr;
    }

    return ptr;
}

void *bzero( void *ptr,
             size_t size )
{
    char *c_ptr = ( char * ) ptr;

    for( int index = 0; index < ( int ) size; index++ )
    {
        c_ptr[ index ] = ( char ) 0;
    }

    return ptr;
}

int memcmp( void *str1,
            void *str2,
            int n )
{
    char *pc1 = str1;
    char *pc2 = str2;

    while( n-- > 0 )
    {
        if( *pc1++ != *pc2++ )
        {
            return pc1[ -1 ] < pc2[ -1 ] ? -1 : 1;
        }
    }

    return 0;
}

void *memcpy( void *dest,
              void *src,
              int len )
{
    char *temp1 = dest;
    char *temp2 = src;

    while( len-- )
    {
        *temp1++ = *temp2++;
    }

    return dest;
}
