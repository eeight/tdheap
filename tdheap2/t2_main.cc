extern "C" {
#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_machine.h" // VG_(fnptr_to_fnentry)
}
#include "pub_tool_cplusplus.h"

#include "m_stl/std/vector"
#include "m_stl/std/string"
#include "m_stl/std/map"
#include "m_stl/std/set"
#include "m_stl/std/algorithm"

#include "t2_malloc.h"
#include "t2_mem_tracer.h"

Addr current_vtable;

static void t2_post_clo_init() {
}

static
void VG_REGPARM(2) MemReadHook(Addr addr, SizeT size) {
  if (size == sizeof(void *)) {
    const MemoryBlock *block = theMemTracer->FindBlockByAddress(addr);

    if (block != 0) {
      // Program something that looks like a pointer located at the beginning of
      // memory block. Probably this is a pointer to vtable.
      if (addr == block->start_addr()) {
        // Save this pointer in variable.
        // TODO: add test not to use values that are not vatable pointers for sure.
        current_vtable = *reinterpret_cast<Addr *>(addr);
        VG_(printf)("Obtained current vtable\n");
      }
    }
  }
}

static
void VG_REGPARM(1) CallHook(Addr addr) {
  if (current_vtable != 0) {
    VG_(printf)("Virtual call(method=%p, vtable=%p)\n", addr, current_vtable);
  }
}

static
void ResetVtable() {
  current_vtable = 0;
}

static
IRSB* t2_instrument(VgCallbackClosure* closure,
                    IRSB* code_in,
                    VexGuestLayout* layout, 
                    VexGuestExtents* vge,
                    IRType gWordTy, IRType hWordTy ) {
  IRSB *code_out;
  IRDirty *di;
  IRType type;
  Int i;

  code_out = deepCopyIRSBExceptStmts(code_in);

  // Reset vtable pointer when entring new block.
  di = unsafeIRDirty_0_N(0, "ResetVtable",
      VG_(fnptr_to_fnentry)(reinterpret_cast<void *>(&ResetVtable)),
      mkIRExprVec_0());
  addStmtToIRSB(code_out, IRStmt_Dirty(di));

  for (i = 0; i < code_in->stmts_used; ++i) {
    IRStmt *st = code_in->stmts[i];
    // If statement writes to tmp variable
    if (st->tag == Ist_WrTmp) {
      IRExpr *expr = st->Ist.WrTmp.data;
      type = typeOfIRExpr(code_out->tyenv, expr);
      tl_assert(type != Ity_INVALID);
      if (expr->tag == Iex_Load) {
        IRExpr **argv;
        argv = mkIRExprVec_2(
            expr->Iex.Load.addr,
            mkIRExpr_HWord((HWord)sizeofIRType(type)));
        di = unsafeIRDirty_0_N(2, "MemReadHook",
            VG_(fnptr_to_fnentry)(reinterpret_cast<void *>(&MemReadHook)),
            argv);
        addStmtToIRSB(code_out, IRStmt_Dirty(di));
      }
    } else if (st->tag == Ist_Store) {
#if 0
      IRExpr **argv;
      type = typeOfIRExpr(code_out->tyenv, st->Ist.Store.data);
      // For these write we want to know the value being written
      if (type == Ity_I8 || type == Ity_I16 || type == Ity_I32 || type == Ity_I64) {
        if (type == Ity_I64) {
          argv = mkIRExprVec_2(st->Ist.Store.addr, st->Ist.Store.data);
        } else {
          argv = mkIRExprVec_2(st->Ist.Store.addr, WidenToHostWord(code_out, st->Ist.Store.data, code_out->tyenv, hWordTy));
        }
        di = NULL;
        switch (type) {
          case Ity_I8:
            di = unsafeIRDirty_0_N(2, "MemWriteHook8",
                VG_(fnptr_to_fnentry)(&MemWriteHook8), argv);
            break;
          case Ity_I16:
            di = unsafeIRDirty_0_N(2, "MemWriteHook16",
                VG_(fnptr_to_fnentry)(&MemWriteHook16), argv);
            break;
          case Ity_I32:
            di = unsafeIRDirty_0_N(2, "MemWriteHook32",
                VG_(fnptr_to_fnentry)(&MemWriteHook32), argv);
            break;
          case Ity_I64:
            di = unsafeIRDirty_0_N(1, "MemWriteHook64",
                VG_(fnptr_to_fnentry)(&MemWriteHook64), argv);
            break;
          default:
            tl_assert(0);
        }
        addStmtToIRSB(code_out, IRStmt_Dirty(di));
      } else {
        // Just the fact of writing.
        argv = mkIRExprVec_2(st->Ist.Store.addr, (HWord)sizeofIRType(type));
        di = unsafeIRDirty_0_N(2, "MemWriteHook",
            VG_(fnptr_to_fnentry)(&MemWriteHook), argv);
      }
#endif
    }
    addStmtToIRSB(code_out, st);
  }

  /*
   * When call to a virtual function happens, argument to call instruction
   * cannot be constant. This kind of call can only end superblock.
   */
  // If this blocks ends with a call.
  if (code_in->jumpkind == Ijk_Call) {
    IRExpr *expr = code_in->next;
    type = typeOfIRExpr(code_out->tyenv, expr);
    tl_assert(type != Ity_INVALID);
    // Looks like virtual call.
    //VG_(printf)("Call target tag = %d\n", expr->tag);
    if (expr->tag == Iex_RdTmp) {
      IRExpr **argv;
      argv = mkIRExprVec_1(expr);
      di = unsafeIRDirty_0_N(1, "CallHook",
          VG_(fnptr_to_fnentry)(reinterpret_cast<void *>(&CallHook)),
          argv);
      addStmtToIRSB(code_out, IRStmt_Dirty(di));
      //VG_(printf)("Processed suspicious IRSB\n");
      //ppIRSB(code_in);
    }
  }
  return code_out;
}

static void t2_fini(Int exitcode) {
  ShutdownMemTracer();
}

static void t2_pre_clo_init() {
  VG_(details_name)            ((Char*)"TdHeap2");
  VG_(details_version)         ((Char*)0);
  VG_(details_description)     ((Char*)"Dynamic class hierarchy reconstructor");
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
  current_vtable = 0;
}

VG_DETERMINE_INTERFACE_VERSION(t2_pre_clo_init)
