#ifndef _MALLOC_H_
#define _MALLOC_H_

extern "C" {
#include "pub_tool_basics.h"
}

extern ULong clo_memreads;
extern ULong clo_memwrites;
extern ULong clo_allocations_count;
extern ULong clo_frees_count;

void *t2_malloc(ThreadId tid, SizeT n);
void *t2_builtin_new(ThreadId tid, SizeT n);
void *t2_builtin_vec_new(ThreadId tid, SizeT n);
void *t2_memalign(ThreadId tid, SizeT align, SizeT n);
void *t2_calloc(ThreadId tid, SizeT nmemb, SizeT size1);
void t2_free(ThreadId tid, void *p);
void t2_builtin_delete(ThreadId tid, void *p);
void t2_builtin_vec_delete(ThreadId tid, void *p);
void *t2_realloc(ThreadId tid, void *p, SizeT new_size);
SizeT t2_usable_size(ThreadId tid, void* p); 

#endif
