#ifndef STRING_H_
#define STRING_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

size_t strlen( const char *ptr );
size_t strnlen( const char *ptr,
                int n );
char *strcpy( char *dest,
              const char *src );

char *strncpy( char *dest,
               const char *src,
               int n );
int strncmp( const char *str1,
             const char *str2,
             int n );
int strnlen_terminator( const char *str,
                        int max,
                        char terminator );
char tolower( char chr );
int istrncmp( const char *str1,
              const char *str2,
              int n );
bool isDigit( char chr );
uint8_t toDigit( char chr );

#endif /* STRING_H_ */
