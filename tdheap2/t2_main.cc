extern "C" {
#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
}
#include "pub_tool_cplusplus.h"

#include "m_stl/std/vector"
#include "m_stl/std/string"
#include "m_stl/std/map"
#include "m_stl/std/set"
#include "m_stl/std/algorithm"

#include "t2_malloc.h"
#include "t2_mem_tracer.h"

static void t2_post_clo_init() {
}

static
IRSB* t2_instrument(VgCallbackClosure* closure,
                    IRSB* bb,
                    VexGuestLayout* layout, 
                    VexGuestExtents* vge,
                    IRType gWordTy, IRType hWordTy ) {
  return bb;
}

static void t2_fini(Int exitcode) {
  ShutdownMemTracer();
}

static void t2_pre_clo_init() {
  VG_(details_name)            ((Char*)"TdHeap2");
  VG_(details_version)         ((Char*)0);
  VG_(details_description)     ((Char*)"Dynamic class hierarcy deconstructor");
  VG_(details_copyright_author)((Char*) "Petr Prokhorenkov");
  VG_(details_bug_reports_to)  ((Char*)VG_BUGS_TO);

  VG_(basic_tool_funcs)        (t2_post_clo_init,
                               t2_instrument,
                               t2_fini);

  VG_(needs_malloc_replacement)(&t2_malloc,
                                &t2_builtin_new,
                                &t2_builtin_vec_new,
                                &t2_memalign,
                                &t2_calloc,
                                &t2_free,
                                &t2_builtin_delete,
                                &t2_builtin_vec_delete,
                                &t2_realloc,
                                &t2_usable_size,
                                0);
  InitMemTracer();
}

VG_DETERMINE_INTERFACE_VERSION(t2_pre_clo_init)
