extern "C" {
#include "pub_tool_basics.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_machine.h" // VG_(fnptr_to_fnentry)
#include "pub_tool_stacktrace.h"
#include "pub_tool_threadstate.h"
}
#include "pub_tool_cplusplus.h"

#include "m_stl/std/vector"
#include "m_stl/std/string"
#include "m_stl/std/map"
#include "m_stl/std/set"
#include "m_stl/std/algorithm"
#include "m_stl/tr1/unordered_map"
#include "m_stl/tr1/unordered_set"

#include "t2_malloc.h"
#include "t2_mem_tracer.h"
#include "t2_inheritance.h"

namespace {

const IRType kPointerType = sizeof(void *) == 8 ? Ity_I64 : Ity_I32;
const IROp kPointerAddOp = sizeof(void *) == 8 ? Iop_Add64 : Iop_Add32;

Addr GetCurrentIp() {
    Addr ip, sp, fp;

    VG_(get_StackTrace)(VG_(get_running_tid)(), &ip, 1, &sp, &fp, 0);

    return ip;
}

} // namespace

static void t2_post_clo_init() {
}

#if 0
static
ULong IRConstToULong(IRConst *c) {
  switch (c->tag) {
    case ICo_U1:
      return c->Ico.U1;
    case ICo_U8:
      return c->Ico.U8;
    case ICo_U16:
      return c->Ico.U16;
    case ICo_U32:
      return c->Ico.U32;
    case ICo_U64:
      return c->Ico.U64;
    default:
      VG_(tool_panic)((Char *)"IRCosntToULong: Integer constant expected");
  }
}
#endif

static
void VG_REGPARM(3) VCallHook(Addr addr, Addr vtable, Addr offset) {
    Addr real_vtable = FindVtableBeginning(vtable);
    Addr real_addr = FindObjectBeginning(addr, real_vtable);
    Addr ip = GetCurrentIp();
    int function_number = offset/sizeof(void *);
    VTable *vt = getVtable(vtable);
    VTable *rvt = getVtable(real_vtable);

    g_callSites->insert(std::make_pair(
                ip, new CallSite(ip, function_number))).
        first->second->addCallee(vt);

    if (vt != rvt) {
        rvt->addChild(vt);
    }
    
#if 0
    VG_(printf)("Virtual call(ip=%p, object=%p, real_object=%p, vtable=%p, real_vtable=%p, offset=%ld)\n",
            ip, addr, real_addr, vtable, real_vtable, offset/sizeof(void *));
#endif
}

static
IRSB* t2_instrument(VgCallbackClosure* closure,
                    IRSB* code_in,
                    VexGuestLayout* layout, 
                    VexGuestExtents* vge,
                    IRType gWordTy, IRType hWordTy ) {
  /*
   * When call to a virtual function happens, argument to call instruction
   * cannot be constant. This kind of call can only end superblock.
   */
  if (code_in->jumpkind != Ijk_Call ||
      code_in->next->tag != Iex_RdTmp) {
    // Doesn't look like virtual call.
    return code_in;
  }

  IRSB *code_out;
  std::tr1::unordered_map<IRTemp, IRExpr *> tmp_values;

  code_out = deepCopyIRSBExceptStmts(code_in);

  for (Int i = 0; i < code_in->stmts_used; ++i) {
    IRStmt *st = code_in->stmts[i];

    if (st->tag == Ist_WrTmp) {
      tmp_values[st->Ist.WrTmp.tmp] = st->Ist.WrTmp.data;
    }

    addStmtToIRSB(code_out, st);
  }

  // Now we have collected all info about tmp registers assginments.
  //
  // Pattern for virtual call looks like following:
  //
  // t10 = Add64(t11,0xFFFFFFFFFFFFFFD8:I64)  # &a
  // t12 = LDle:I64(t10)                      # a
  // t13 = LDle:I64(t12)                      # a->vtable == &a->vtable[0]
  // t2 = Add64(t13,0x8:I64)                  # &a->vtable[1]
  // t14 = LDle:I64(t2)                       # a->vtable[1] 
  // goto {Call} t14                          # call a->vtable[1]
  //
  //
  // We need to extract a and a->vtable

  IRExpr *expr = code_in->next;
  IRTemp a_vtable_tmp = expr->Iex.RdTmp.tmp;
  IRConst *offset = 0;

  if (IRExpr *a_vtable_tmp_value = tmp_values[a_vtable_tmp]) {
    if (a_vtable_tmp_value->tag == Iex_Load &&
        a_vtable_tmp_value->Iex.Load.ty == kPointerType &&
        a_vtable_tmp_value->Iex.Load.addr->tag == Iex_RdTmp) {
      IRTemp a_vtable_address = a_vtable_tmp_value->Iex.Load.addr->Iex.RdTmp.tmp;

      if (IRExpr *a_vtable_address_value = tmp_values[a_vtable_address]) {
        // This must be shift in vtable if called function is not first.
        if (a_vtable_address_value->tag == Iex_Binop &&
            a_vtable_address_value->Iex.Binop.op == kPointerAddOp) {
          IRExpr *left = a_vtable_address_value->Iex.Binop.arg1;
          IRExpr *right = a_vtable_address_value->Iex.Binop.arg2;

          // TODO: maybe add sanity check: const is small positive number
          if (right->tag == Iex_Const &&
              left->tag == Iex_RdTmp) {
            a_vtable_address = left->Iex.RdTmp.tmp;
            a_vtable_address_value = tmp_values[left->Iex.RdTmp.tmp];
            
            offset = right->Iex.Const.con;
          }
        }

        if (a_vtable_address_value->tag == Iex_Load &&
            a_vtable_address_value->Iex.Load.ty == kPointerType &&
            a_vtable_address_value->Iex.Load.addr->tag == Iex_RdTmp) {
          IRTemp a = a_vtable_address_value->Iex.Load.addr->Iex.RdTmp.tmp;

          // All parts have been matched.
          IRExpr **argv;
          IRDirty *di;

          if (offset == 0) {
            if (sizeof(void *) == 4) {
              offset = IRConst_U32(0);
            } else {
              offset = IRConst_U64(0);
            }
          }

          argv = mkIRExprVec_3(
              IRExpr_RdTmp(a),
              IRExpr_RdTmp(a_vtable_address),
              IRExpr_Const(offset)
              );
          di = unsafeIRDirty_0_N(3, (HChar *)"VCallHook",
              VG_(fnptr_to_fnentry)(reinterpret_cast<void *>(&VCallHook)),
              argv);
          addStmtToIRSB(code_out, IRStmt_Dirty(di));
        }
      }
    }
  }

  //ppIRSB(code_out);

  return code_out;
}

static void t2_fini(Int exitcode) {
    GenerateVtablesLayout();
    ShutdownInheritanceTracker();
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
  InitInheritanceTracker();
}

VG_DETERMINE_INTERFACE_VERSION(t2_pre_clo_init)
