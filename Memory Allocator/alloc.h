#ifndef _ALLOC_H
#define _ALLOC_H

void *kumalloc(size_t size);
void *kucalloc(size_t nmemb, size_t size);
void kufree(void *ptr);
void *kurealloc(void *ptr, size_t size);

#endif
