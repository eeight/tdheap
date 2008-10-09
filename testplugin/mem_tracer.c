#include "mem_tracer.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_oset.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h" 
static const Addr kBucketSize = 128;

XArray *blocks_allocated;
static VgHashTable mem_table;

MemBlock *NewMemBlock(Addr start_addr, SizeT size) {
    MemBlock *block = VG_(malloc)("testplugin.newmemblock", sizeof(*block));
    
    block->start_addr = start_addr;
    block->size = size;
    block->used_from = NULL;
    block->reads_count = 0;
    block->writes_count = 0;
    
    VG_(addToXA)(blocks_allocated, &block);

    return block;
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
            sizeof(void *));
}

void ShutdownMemTracer(void) {
    UInt blocks_count = VG_(sizeXA)(blocks_allocated);
    UInt i;
    for (i = 0; i != blocks_count; ++i) {
        MemBlock *block = *(MemBlock **)VG_(indexXA)(blocks_allocated, i);
        VG_(free)(block);
    }
    VG_(deleteXA)(blocks_allocated);
    VG_(HT_destruct)(mem_table);
}

static
void InsertInMemTable(Addr addr, MemBlock *block) {
    SizeT size = block->size;
    Addr a = addr - addr%kBucketSize;
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

static
void RemoveFromMemTable(Addr addr, MemBlock *block) {
    SizeT size = block->size;
    Addr a = addr - addr%kBucketSize;
    Addr end = addr + size;

    for (; a < end; a += kBucketSize) {
        MemNode *node = VG_(HT_lookup)(mem_table, a);
        if (node != NULL) {
            VG_(OSetWord_Remove)(node->blocks, (UWord)block);
        }
    }
}

void RegisterMemoryBlock(Addr addr, SizeT size) {
    MemBlock *block = NewMemBlock(addr, size);
    InsertInMemTable(addr, block);
}

void UnregisterMemoryBlock(Addr addr) {
    MemBlock *block = FindBlockByAddress(addr);
    // if block == NULL this block must be of size 0
    if (block != NULL) {
        RemoveFromMemTable(addr, block);
    }
}

static
Bool VG_REGPARM(2) IsAddrInBlock(Addr addr, MemBlock *block) {
    return addr >= block->start_addr && addr < block->start_addr + block->size;
}

MemBlock *FindBlockByAddress(Addr addr) {
    Addr bucket = addr - addr%kBucketSize;
    MemNode *node = VG_(HT_lookup)(mem_table, bucket);

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
