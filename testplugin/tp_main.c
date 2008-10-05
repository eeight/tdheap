
/*--------------------------------------------------------------------*/
/*--- Testplugin: Test plugin.                           tp_main.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Testplugin, the simplest possible Valgrind tool,
   which does something unclear.

   Copyright (C) 2002-2008 Petr Prokhorenkov
      prokhorenkov@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_replacemalloc.h"
#include "pub_tool_machine.h" // VG_(fnptr_to_fnentry)

ULong clo_allocations_count;
ULong clo_frees_count;
ULong clo_memreads;
ULong clo_memwrites;

static void tp_post_clo_init(void)
{
}

static
void ReadHook(Addr addr, SizeT size) {
    ++clo_memreads;
}

static
void WriteHook(Addr addr, SizeT size) {
    ++clo_memwrites;
}

static
IRSB* tp_instrument ( VgCallbackClosure* closure,
                      IRSB* code_in,
                      VexGuestLayout* layout, 
                      VexGuestExtents* vge,
                      IRType gWordTy, IRType hWordTy )
{
    IRSB *code_out;
    IRDirty *di;
    IRType type;
    Int i;

    code_out = deepCopyIRSBExceptStmts(code_in);

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
                di = unsafeIRDirty_0_N(1, "ReadHook",
                        VG_(fnptr_to_fnentry)(&ReadHook), argv);
                addStmtToIRSB(code_out, IRStmt_Dirty(di));
            }
        } else if (st->tag == Ist_Store) {
            IRExpr **argv;
            type = typeOfIRExpr(code_out->tyenv, st->Ist.Store.data);
            argv = mkIRExprVec_2(st->Ist.Store.addr,
                    mkIRExpr_HWord((HWord)sizeofIRType(type)));
            di = unsafeIRDirty_0_N(1, "WriteHook",
                    VG_(fnptr_to_fnentry)(&WriteHook), argv);
            addStmtToIRSB(code_out, IRStmt_Dirty(di));
        }
        addStmtToIRSB(code_out, st);
    }
    return code_out;
}

static void tp_fini(Int exitcode)
{
   VG_(printf)("mallocs: %lld\nfrees: %lld\n",
           clo_allocations_count, clo_frees_count);
   VG_(printf)("Memory reads: %lld\nMemory writes: %lld\n",
           clo_memreads, clo_memwrites);
}

static
void *tc_malloc_common(SizeT align, SizeT n) {
   ++clo_allocations_count;
   if (align != 0) {
      return VG_(cli_malloc)(align, n);
   } else {
      return VG_(cli_malloc)(VG_(clo_alignment), n);
   }
}

static
void tc_free_common(void *p) {
   ++clo_frees_count;
   VG_(cli_free)(p);
}

static
void *tc_malloc(ThreadId tid, SizeT n) {
   return tc_malloc_common(0, n);
}

static
void *tc_builtin_new(ThreadId tid, SizeT n) {
   return tc_malloc(tid, n);
}

static
void *tc_builtin_vec_new(ThreadId tid, SizeT n) {
   return tc_malloc(tid, n);
}

static
void *tc_memalign(ThreadId tid, SizeT align, SizeT n) {
   return tc_malloc_common(align, n);
}

static
void *tc_calloc(ThreadId tid, SizeT nmemb, SizeT size1) {
   void *result = tc_malloc_common(0, nmemb*size1);
   VG_(memset)(result, 0, nmemb*size1);
   return result;
}

static
void tc_free(ThreadId tid, void *p) {
   return tc_free_common(p);
}

static
void tc_builtin_delete(ThreadId tid, void *p) {
   return tc_free(tid, p);
}

static
void tc_builtin_vec_delete(ThreadId tid, void *p) {
   return tc_free(tid, p);
}

// This is dumb, but what to do?
static
void *tc_realloc(ThreadId tid, void *p, SizeT new_size) {
   tc_free_common(p);
   return tc_malloc_common(0, new_size);
}

static void tp_pre_clo_init(void)
{
   VG_(details_name)            ("Testplugin");
   VG_(details_version)         (NULL);
   VG_(details_description)     ("some unclear tool");
   VG_(details_copyright_author)(
      "Copyright (C) 2002-2008 Petr Prokhorenkov.");
   VG_(details_bug_reports_to)  (VG_BUGS_TO);

   VG_(basic_tool_funcs)        (tp_post_clo_init,
                                 tp_instrument,
                                 tp_fini);
   VG_(needs_malloc_replacement)(
      &tc_malloc,
      &tc_builtin_new,
      &tc_builtin_vec_new,
      &tc_memalign,
      &tc_calloc,
      &tc_free,
      &tc_builtin_delete,
      &tc_builtin_vec_delete,
      &tc_realloc,
      0
   );
   clo_allocations_count = 0;
   clo_frees_count = 0;
   clo_memreads = 0;
   clo_memwrites = 0;
}

VG_DETERMINE_INTERFACE_VERSION(tp_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
