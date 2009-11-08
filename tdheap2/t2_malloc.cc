#include "t2_malloc.h"

extern "C" {
#include "pub_tool_replacemalloc.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_libcassert.h"
}

#include "t2_mem_tracer.h"

ULong clo_memreads;
ULong clo_memwrites;
ULong clo_allocations_count;
ULong clo_frees_count;

namespace {

void *t2_malloc_common(SizeT align, SizeT n) {
    void *result;

    ++clo_allocations_count;

    if (align != 0) {
      result = VG_(cli_malloc)(align, n);
    } else {
      result = VG_(cli_malloc)(VG_(clo_alignment), n);
    }

    if (n > 0) {
        theMemTracer->RegisterMemoryBlock((Addr)result, n);
    }

    //VG_(printf)("Allocated %d bytes\n", n);
    return result;
}

void t2_free_common(void *p) {
   ++clo_frees_count;
   theMemTracer->UnregisterMemoryBlock((Addr)p);
   VG_(cli_free)(p);
}

} // namespace

void *t2_malloc(ThreadId tid, SizeT n) {
   return t2_malloc_common(0, n);
}

void *t2_builtin_new(ThreadId tid, SizeT n) {
   return t2_malloc(tid, n);
}

void *t2_builtin_vec_new(ThreadId tid, SizeT n) {
   return t2_malloc(tid, n);
}

void *t2_memalign(ThreadId tid, SizeT align, SizeT n) {
   return t2_malloc_common(align, n);
}

void *t2_calloc(ThreadId tid, SizeT nmemb, SizeT size1) {
   void *result = t2_malloc_common(0, nmemb*size1);
   VG_(memset)(result, 0, nmemb*size1);
   return result;
}

void t2_free(ThreadId tid, void *p) {
   return t2_free_common(p);
}

void t2_builtin_delete(ThreadId tid, void *p) {
   return t2_free_common(p);
}

void t2_builtin_vec_delete(ThreadId tid, void *p) {
   return t2_free_common(p);
}

void *t2_realloc(ThreadId tid, void *p, SizeT new_size) {
  SizeT old_size;

  MemoryBlockPtr block = theMemTracer->FindBlockByAddress((Addr)p);
  if (block == NULL) {
      VG_(tool_panic)((Char *)"Cannot realloc not malloc'd block");
  } else {
      old_size = block->size();
  }

  if (old_size == new_size) {
    return p;
  } else {
    // realloc'd memory block replaces old block, so no need to register it
    void *result = VG_(cli_malloc)(VG_(clo_alignment), new_size);
    SizeT copy_size = old_size;

    if (new_size < old_size) {
      copy_size = new_size;
    }
    VG_(memcpy)(result, p, copy_size);
    VG_(cli_free)(p);

    theMemTracer->HandleRealloc(block, (Addr)result, new_size);

    return result;
  }
}

SizeT t2_usable_size(ThreadId tid, void *p) {
  MemoryBlockPtr block = theMemTracer->FindBlockByAddress((Addr)p);
  if (block != 0) {
    return block->size();
  } else {
    return 0;
  }
}
