#include "string.h"

size_t strlen( const char *ptr )
{
    size_t len = 0;

    while( *ptr != 0 )
    {
        len++;
        ptr += 1;
    }

    return len;
}

size_t strnlen( const char *ptr,
                int n )
{
    size_t len = 0;

    for( int idx = 0; idx < n; idx++ )
    {
        if( ptr[ idx ] == 0 )
        {
            break;
        }

        len++;
    }

    return len;
}

char *strcpy( char *dest,
              const char *src )
{
    char *res = dest;

    while( *src != 0 )
    {
        *dest = *src;
        src  += 1;
        dest += 1;
    }

    *dest = 0x00;

    return res;
}

char *strncpy( char *dest,
               const char *src,
               int n )
{
    int idx = 0;

    for( idx = 0; idx < n - 1; idx++ )
    {
        if( src[ idx ] == 0x00 )
        {
            break;
        }

        dest[ idx ] = src[ idx ];
    }

    dest[ idx ] = 0x00;

    return dest;
}

int strncmp( const char *str1,
             const char *str2,
             int n )
{
    unsigned char tmp1;
    unsigned char tmp2;

    while( n-- > 0 )
    {
        tmp1 = ( unsigned char ) *str1++;
        tmp2 = ( unsigned char ) *str2++;

        if( tmp1 != tmp2 )
        {
            return tmp1 - tmp2;
        }

        if( tmp1 == '\0' )
        {
            return 0;
        }
    }

    return 0;
}

int strnlen_terminator( const char *str,
                        int max,
                        char terminator )
{
    int idx = 0;

    for( idx = 0; idx < max; idx++ )
    {
        if( ( str[ idx ] == '\0' ) || ( str[ idx ] == terminator ) )
        {
            break;
        }
    }

    return idx;
}

char tolower( char chr )
{
    if( ( chr >= 65 ) && ( chr <= 90 ) )
    {
        chr += 32;
    }

    return chr;
}

int istrncmp( const char *str1,
              const char *str2,
              int n )
{
    unsigned char tmp1;
    unsigned char tmp2;

    while( n-- > 0 )
    {
        tmp1 = ( unsigned char ) *str1++;
        tmp2 = ( unsigned char ) *str2++;

        if( ( tmp1 != tmp2 ) && ( tolower( tmp1 ) != tolower( tmp2 ) ) )
        {
            return tmp1 - tmp2;
        }

        if( tmp1 == '\0' )
        {
            return 0;
        }
    }

    return 0;
}

bool isDigit( char chr )
{
    return ( ( int ) chr >= 48 ) && ( ( int ) chr <= 57 );
}

int toDigit( char chr )
{
    return ( int ) ( chr - ( ( char ) '0' ) );
}

char *sp = 0;
char *strtok( char *str,
              const char *delimiters )
{
    int idx = 0;
    int len = strlen( delimiters );

    if( !str && !sp )
    {
        return 0;
    }

    if( str && !sp )
    {
        sp = str;
    }

    char *p_start = sp;

    while( 1 )
    {
        for( idx = 0; idx < len; idx++ )
        {
            if( *p_start == delimiters[ idx ] )
            {
                p_start++;
                break;
            }
        }

        if( idx == len )
        {
            sp = p_start;
            break;
        }
    }

    if( *sp == '\0' )
    {
        sp = 0;
        return sp;
    }

    /* find end of substring */
    while( *sp != '\0' )
    {
        for( idx = 0; idx < len; idx++ )
        {
            if( *sp == delimiters[ idx ] )
            {
                *sp = '\0';
                break;
            }
        }

        sp++;

        if( idx < len )
        {
            break;
        }
    }

    return p_start;
}
