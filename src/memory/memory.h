#ifndef MEMORY_H_
#define MEMORY_H_

#include <stddef.h>

void *memset( void *ptr,
              int chr,
              size_t size );
void *bzero( void *ptr,
             size_t size );
int memcmp( void *str1,
            void *str2,
            int n );
void *memcpy( void *dest,
              void *src,
              int len );

#endif /* MEMORY_H_ */
