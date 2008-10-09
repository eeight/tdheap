#include "mem_tracer.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_oset.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h" 
static const Addr kBucketSize = 1024;

XArray *blocks_allocated;
static VgHashTable mem_table;

MemBlock *NewMemBlock(Addr start_addr, SizeT size) {
    MemBlock block;
    Word block_index;
    
    block.start_addr = start_addr;
    block.size = size;
    block.used_from = NULL;
    block.reads_count = 0;
    block.writes_count = 0;
    
    block_index = VG_(addToXA)(blocks_allocated, &block);
    return VG_(indexXA)(blocks_allocated, block_index);
}

MemNode *NewMemNode(ULong key, MemBlock *block) {
    MemNode *node = VG_(malloc)("testplugin.newmemnode",
                                sizeof(*node));

    node->key = key;
    node->blocks = VG_(OSetWord_Create)(
            &VG_(malloc),
            "testplugin.newmemnode",
            &VG_(free));
    VG_(OSetWord_Insert)(node->blocks, (UWord)block);

    return node;
}

void InitMemTracer(void) {
    mem_table = VG_(HT_construct)("testplugin.initmemtracer.1");
    blocks_allocated = VG_(newXA)(
            &VG_(malloc),
            "testplugin.initmemtracer.2",
            &VG_(free),
            sizeof(MemBlock));
}

void ShutdownMemTracer(void) {
    VG_(HT_destruct)(mem_table);
    VG_(deleteXA)(blocks_allocated);
}

static
void InsertInMemTable(Addr addr, SizeT size, MemBlock *block) {
    Addr a = addr - (addr%kBucketSize);
    Addr end = addr + size;

    for (; a < end; a += kBucketSize) {
        MemNode *node = VG_(HT_lookup)(mem_table, a);
        if (node != NULL) {
            VG_(OSetWord_Insert)(node->blocks, (UWord)block);
        } else {
            node = NewMemNode(a, block);
            VG_(HT_add_node)(mem_table, node);
        }
    }
}

void RegisterMemoryBlock(Addr addr, SizeT size) {
    MemBlock *block = NewMemBlock(addr, size);
    InsertInMemTable(addr, size, block);
}

static
Bool VG_REGPARM(2) IsAddrInBlock(Addr addr, MemBlock *block) {
    return addr >= block->start_addr && addr < block->start_addr + block->size;
}

MemBlock *FindBlockByAddress(Addr addr) {
    MemNode *node = VG_(HT_lookup)(mem_table, addr);
    addr = addr - (addr%kBucketSize);

    if (node) {
        OSet *blocks = node->blocks;
        MemBlock *block;
        VG_(OSetWord_ResetIter)(blocks);
        while (VG_(OSetWord_Next)(blocks, (UWord *)&block)) {
            if (IsAddrInBlock(addr, block)) {
                return block;
            }
        }
    }

    return NULL;
}

void VG_REGPARM(2) AddUsedFrom(MemBlock *block, Addr addr) {
    if (block->used_from == NULL) {
        block->used_from = VG_(OSetWord_Create)(
            &VG_(malloc),
            "testplugin.addusedfrom",
            &VG_(free));
    }

    if (!VG_(OSetWord_Contains)(block->used_from, addr)) {
        VG_(OSetWord_Insert)(block->used_from, addr);
    }
}
