#ifndef _MALLOC_REPLACE_H_
#define _MALLOC_REPLACE_H_

#include "pub_tool_basics.h"

extern ULong clo_memreads;
extern ULong clo_memwrites;
extern ULong clo_allocations_count;
extern ULong clo_frees_count;

void *h1_malloc(ThreadId tid, SizeT n);

void *h1_builtin_new(ThreadId tid, SizeT n);

void *h1_builtin_vec_new(ThreadId tid, SizeT n);

void *h1_memalign(ThreadId tid, SizeT align, SizeT n);

void *h1_calloc(ThreadId tid, SizeT nmemb, SizeT size1);

void h1_free(ThreadId tid, void *p);

void h1_builtin_delete(ThreadId tid, void *p);

void h1_builtin_vec_delete(ThreadId tid, void *p);

void *h1_realloc(ThreadId tid, void *p, SizeT new_size);

#endif
