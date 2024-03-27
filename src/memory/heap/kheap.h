#ifndef KHEAP_H_
#define KHEAP_H_

#include <stddef.h>

void kheap_init();
void *kmalloc( size_t size );
void kfree( void *ptr );
void *kzalloc( size_t size );

#endif /* KHEAP_H_ */
