#include "malloc_replace.h"

#include "pub_tool_replacemalloc.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_tooliface.h"

#include "mem_tracer.h"

ULong clo_memreads;
ULong clo_memwrites;
ULong clo_allocations_count;
ULong clo_frees_count;

static
void *h1_malloc_common(SizeT align, SizeT n, Bool do_register) {
    void *result;

    ++clo_allocations_count;

    if (align != 0) {
      result = VG_(cli_malloc)(align, n);
    } else {
      result = VG_(cli_malloc)(VG_(clo_alignment), n);
    }

    if (n > 0 && do_register) {
        RegisterMemoryBlock((Addr)result, n);
    }

    return result;
}

static
void h1_free_common(void *p) {
   ++clo_frees_count;
   UnregisterMemoryBlock((Addr)p);
   VG_(cli_free)(p);
}

void *h1_malloc(ThreadId tid, SizeT n) {
   return h1_malloc_common(0, n, True);
}

void *h1_builtin_new(ThreadId tid, SizeT n) {
   return h1_malloc(tid, n);
}

void *h1_builtin_vec_new(ThreadId tid, SizeT n) {
   return h1_malloc(tid, n);
}

void *h1_memalign(ThreadId tid, SizeT align, SizeT n) {
   return h1_malloc_common(align, n, True);
}

void *h1_calloc(ThreadId tid, SizeT nmemb, SizeT size1) {
   void *result = h1_malloc_common(0, nmemb*size1, True);
   VG_(memset)(result, 0, nmemb*size1);
   return result;
}

void h1_free(ThreadId tid, void *p) {
   return h1_free_common(p);
}

void h1_builtin_delete(ThreadId tid, void *p) {
   return h1_free_common(p);
}

void h1_builtin_vec_delete(ThreadId tid, void *p) {
   return h1_free_common(p);
}

void *h1_realloc(ThreadId tid, void *p, SizeT new_size) {
    SizeT old_size;

    MemBlock *block = FindBlockByAddress((Addr)p);
    if (block == NULL) {
        VG_(tool_panic)("Cannot realloc not malloc'd block");
    } else {
        old_size = block->size;
    }

    if (old_size == new_size) {
        return p;
    } else {
        // realloc'd memory block replaces old block, so no need to register it
        void *result = h1_malloc_common(0, new_size, False);
        SizeT copy_size = old_size;

        if (new_size < old_size) {
            copy_size = new_size;
        }
        VG_(memcpy)(result, p, copy_size);
        h1_free_common(p);

        // manually register realloc'd block
        block->start_addr = (Addr)result;
        block->size = new_size;
        InsertInMemTable(block);

        return result;
    }
}
