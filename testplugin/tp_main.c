
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
   64along with this program; if not, write to the Free Software
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
#include "pub_tool_stacktrace.h"
#include "pub_tool_threadstate.h"
#include "pub_tool_machine.h" // VG_(fnptr_to_fnentry)

#include "mem_tracer.h"

static const UInt kClustersCount = 6;

ULong clo_allocations_count;
ULong clo_frees_count;
ULong clo_memreads;
ULong clo_memwrites;

static void tp_post_clo_init(void)
{
}

// TODO: Is more straightforward implementation possible?
static
inline Addr GetCurrentIp(void) {
    Addr ip, sp, fp;
    ThreadId tid = VG_(get_running_tid)();
    UInt result = VG_(get_StackTrace)(tid, &ip, 1, &sp, &fp, 0);

    if (result < 1) {
        VG_(tool_panic)("Cannot get current ip");
    }

    return ip;
}

static
void VG_REGPARM(2) MemReadHook(Addr addr, SizeT size) {
    MemBlock *block = FindBlockByAddress(addr);
    if (block != NULL) {
        ++block->reads_count;
        AddUsedFrom(block, GetCurrentIp());
    }
    ++clo_memreads;
}

static
void VG_REGPARM(2) MemWriteHook(Addr addr, UWord size) {
    MemBlock *block = FindBlockByAddress(addr);
    if (block != NULL) {
        ++block->writes_count;
        AddUsedFrom(block, GetCurrentIp());
    }
    ++clo_memwrites;
}

static
void VG_REGPARM(2) MemWriteHook8(Addr addr, UWord val) {
    MemWriteHook(addr, 1);
}

static
void VG_REGPARM(2) MemWriteHook16(Addr addr, UWord val) {
    MemWriteHook(addr, 2);
}

static
void VG_REGPARM(2) MemWriteHook32(Addr addr, UWord val) {
    MemWriteHook(addr, 4);
    TraceMemWrite32(addr, val);
}

static
void VG_REGPARM(1) MemWriteHook64(Addr addr, ULong val) {
    MemWriteHook(addr, 8);
}

static
IRExpr *AssignNew(IRSB *code_out, IRTypeEnv *env, IRExpr *expr, HWord size) {
    IRTemp t;

    t = newIRTemp(env, size);
    addStmtToIRSB(code_out, IRStmt_WrTmp(t, expr));
    return IRExpr_RdTmp(t);
}

static
IRExpr *WidenToHostWord(IRSB *code_out, IRExpr *expr, IRTypeEnv *env, HWord size) {
    IRType ty = typeOfIRExpr(env, expr);

    if (size == Ity_I32) {
        switch (ty) {
        case Ity_I32:
            return expr;
            break;
        case Ity_I16:
            return AssignNew(code_out, env, IRExpr_Unop(Iop_16Uto32, expr), size);
            break;
        case Ity_I8:
            return AssignNew(code_out, env, IRExpr_Unop(Iop_8Uto32, expr), size);
            break;
        default:
            // Do nothing
            ty = ty;
        }
    }

    VG_(tool_panic)("WidenToHostWord");
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
                di = unsafeIRDirty_0_N(2, "MemReadHook",
                        VG_(fnptr_to_fnentry)(&MemReadHook), argv);
                addStmtToIRSB(code_out, IRStmt_Dirty(di));
            }
        } else if (st->tag == Ist_Store) {
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
        }
        addStmtToIRSB(code_out, st);
    }
    return code_out;
}

static void tp_fini(Int exitcode)
{
    UInt i, blocks_count, used_blocks_count;
    double addresses_count;
    UInt clusters_count;

    VG_(printf)("mallocs: %lld\nfrees: %lld\n",
           clo_allocations_count, clo_frees_count);
    VG_(printf)("Memory reads: %lld\nMemory writes: %lld\n",
           clo_memreads, clo_memwrites);

    blocks_count = VG_(sizeXA)(blocks_allocated);
    addresses_count = 0.0;
    used_blocks_count = 0;
    for (i = 0; i != blocks_count; ++i) {
        MemBlock *block = *(MemBlock **)VG_(indexXA)(blocks_allocated, i);
        if (block != NULL) {
            if (block->used_from != NULL) {
               addresses_count += VG_(OSetWord_Size)(block->used_from);
               ++used_blocks_count;
            }
       }
    }
    
    if (used_blocks_count != 0) {
        addresses_count /= used_blocks_count;
        
        // VG_(printf) does not support float values :(
        VG_(printf)(
                "Each allocated memory block is accessed "
                "from %llu addresses on average\n", (ULong)addresses_count);
    } else {
        VG_(printf)("No malloc'd blocks used\n");
    }

    VG_(printf)("Clusterizing all allocated memory blocks...\t");
    ClusterizeMemBlocks();
    clusters_count = VG_(sizeXA)(blocks_clusters);
    VG_(printf)("%u clusters\n", clusters_count);
    if (clusters_count > kClustersCount) {
        clusters_count = kClustersCount;
    }
    VG_(printf)("Fingerprints of first %d clusters:\n", clusters_count);
    for (i = 0; i != clusters_count; ++i) {
        PrettyPrintClusterFingerprint(i);
    }

    VG_(printf)("Dot representation:\n\n");
    PrintClustersDot();

    ShutdownMemTracer();
}

static
void *tp_malloc_common(SizeT align, SizeT n, Bool do_register) {
    void *result;

    ++clo_allocations_count;

    if (align != 0) {
      result = VG_(cli_malloc)(align, n);
    } else {
      result = VG_(cli_malloc)(VG_(clo_alignment), n);
    }
    //VG_(printf)("malloc(%lu) = %lu;\n", n, (Addr)result);

    if (n > 0 && do_register) {
        RegisterMemoryBlock((Addr)result, n);
    }

    return result;
}

static
void tp_free_common(void *p) {
   ++clo_frees_count;
   //VG_(printf)("free(%lu);\n", (Addr)p);
   UnregisterMemoryBlock((Addr)p);
   VG_(cli_free)(p);
}

static
void *tp_malloc(ThreadId tid, SizeT n) {
   return tp_malloc_common(0, n, True);
}

static
void *tp_builtin_new(ThreadId tid, SizeT n) {
   return tp_malloc(tid, n);
}

static
void *tp_builtin_vec_new(ThreadId tid, SizeT n) {
   return tp_malloc(tid, n);
}

static
void *tp_memalign(ThreadId tid, SizeT align, SizeT n) {
   return tp_malloc_common(align, n, True);
}

static
void *tp_calloc(ThreadId tid, SizeT nmemb, SizeT size1) {
   void *result = tp_malloc_common(0, nmemb*size1, True);
   VG_(memset)(result, 0, nmemb*size1);
   return result;
}

static
void tp_free(ThreadId tid, void *p) {
   return tp_free_common(p);
}

static
void tp_builtin_delete(ThreadId tid, void *p) {
   return tp_free_common(p);
}

static
void tp_builtin_vec_delete(ThreadId tid, void *p) {
   return tp_free_common(p);
}

static
void *tp_realloc(ThreadId tid, void *p, SizeT new_size) {
    SizeT old_size;

    MemBlock *block = FindBlockByAddress((Addr)p);
    if (block == NULL) {
        VG_(tool_panic)("Cannot realloc not malloc'd block");
    } else {
        old_size = block->size;
    }
    //VG_(printf)("realloc(%lu, %lu);\t old size: %lu\n", (Addr)p, new_size, old_size);

    if (old_size == new_size) {
        return p;
    } else {
        // realloc'd memory block replaces old block, so no need to register it
        void *result = tp_malloc_common(0, new_size, False);
        SizeT copy_size = old_size;

        if (new_size < old_size) {
            copy_size = new_size;
        }
        VG_(memcpy)(result, p, copy_size);
        tp_free_common(p);

        // manually register realloc'd block
        block->start_addr = (Addr)result;
        block->size = new_size;
        InsertInMemTable(block);

        return result;
    }
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
      &tp_malloc,
      &tp_builtin_new,
      &tp_builtin_vec_new,
      &tp_memalign,
      &tp_calloc,
      &tp_free,
      &tp_builtin_delete,
      &tp_builtin_vec_delete,
      &tp_realloc,
      0
   );

   InitMemTracer();
   clo_allocations_count = 0;
   clo_frees_count = 0;
   clo_memreads = 0;
   clo_memwrites = 0;
}

VG_DETERMINE_INTERFACE_VERSION(tp_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
