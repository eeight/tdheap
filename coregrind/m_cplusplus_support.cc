// TODO: license!
extern "C" {
  #include "valgrind.h"
  #include "pub_tool_basics.h"
  #include "pub_tool_mallocfree.h"
  #include "pub_tool_libcbase.h"
  #include "pub_tool_libcassert.h"
  #include "pub_tool_libcprint.h"
}


#include <new>
#include "include/pub_tool_cplusplus.h"


//---------------------- C++ malloc support -------------- {{{1
class MallocCostCenterStack {
 public:
  void Push(const char *cc) {
    tl_assert(size_ < kMaxMallocStackSize);
    tl_assert(cc);
    malloc_cost_centers_[size_++] = cc;
  }
  void Pop() {
    tl_assert(size_ > 0);
    size_--;
  }
  const char *Top() { 
    return size_ ? malloc_cost_centers_[size_ - 1] : "default_cc"; 
  }
 private:
  static const int kMaxMallocStackSize = 100;
  int size_;
  const char *malloc_cost_centers_[kMaxMallocStackSize];
};

// Not thread-safe. Need to make it thread-local once we are multi-threaded.
static MallocCostCenterStack g_malloc_stack; 

void PushMallocCostCenter(const char *cc) { g_malloc_stack.Push(cc); }
void PopMallocCostCenter() { g_malloc_stack.Pop(); }


void *operator new (size_t size) {
//  VG_(printf)("new %ld %s\n", size, g_malloc_stack.Top());
  return VG_(malloc)((HChar*)g_malloc_stack.Top(), size);
}
void *operator new [](size_t size) {
//  VG_(printf)("new [] %ld %s\n", size, g_malloc_stack.Top());
  return VG_(malloc)((HChar*)g_malloc_stack.Top(), size);
}
void operator delete (void *p) {
//  VG_(printf)("delete %p\n", p);
  VG_(free)(p);
}
void operator delete [](void *p) {
//  VG_(printf)("delete [] %p\n", p);
  VG_(free)(p);
}

//---------------------- Memmove and friends -------------- {{{1

// kcc: I am not sure we really need these...

void * memmove ( void * destination, const void * source, size_t num ) {
  return VG_(memmove)(destination, source, num);
}
void * memchr (const void * ptr, int value, size_t num ) {
  char chr_value = (char)value,
       *chr_ptr  = (char*)ptr;
  for (size_t i = 0; i < num; i++)
    if (chr_value == chr_ptr[i])
      return chr_ptr + i;
  return NULL;
}
size_t strlen ( const char * str ) {
  return VG_(strlen)((const Char*)str);
}
namespace std {
  void abort() {
    tl_assert(false);
  }

double ceil(double x) {
  return static_cast<size_t>(x + 0.5);
}

float ceilf(float x) {
  return static_cast<size_t>(x + 0.5f);
}

} // namespace std
