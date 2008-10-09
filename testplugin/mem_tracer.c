#include "mem_tracer.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_wordfm.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h" 
static const Addr kBucketSize = 1024;

static VgHashTable mem_table;

MemBlock *NewMemBlock(Addr start_addr, SizeT size) {
    MemBlock *result = VG_(malloc)("testplugin.newmemblock",
                                    sizeof(*result));
    result->start_addr = start_addr;
    result->size = size;
    result->reads_count = 0;
    result->writes_count = 0;

    return result;
}

MemNode *NewMemNode(ULong key, MemBlock *block) {
    MemNode *node = VG_(malloc)("testplugin.newmemnode",
                                sizeof(*node));

    node->key = key;
    node->blocks = VG_(newBag)(
            &VG_(malloc),
            "testplugin.newmemnode",
            &VG_(free));
    VG_(addToBag)(node->blocks, (UWord)block);

    return node;
}

void InitMemTracer(void) {
    mem_table = VG_(HT_construct)("testplugin.initmemtracer");
}

void ShutdownMemTracer(void) {
    VG_(HT_destruct)(mem_table);
}

void InsertInMemTable(Addr addr, SizeT size, MemBlock *block) {
    Addr a = addr - (addr%kBucketSize);
    Addr end = addr + size;

    for (; a < end; a += kBucketSize) {
        MemNode *node = VG_(HT_lookup)(mem_table, a);
        if (node != NULL) {
            VG_(addToBag)(node->blocks, (UWord)block);
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

Bool IsAddrInBlock(Addr addr, MemBlock *block) {
    return addr >= block->start_addr && addr < block->start_addr + block->size;
}

MemBlock *FindBlockByAddress(Addr addr) {
    addr = addr - (addr%kBucketSize);

    MemNode *node = VG_(HT_lookup)(mem_table, addr);

    if (node) {
        WordBag *blocks = node->blocks;
        MemBlock *block;
        UWord count;
        VG_(initIterBag)(blocks);
        while (VG_(nextIterBag)(blocks, (UWord *)&block, &count)) {
            tl_assert(count == 1);
            if (IsAddrInBlock(addr, block)) {
                VG_(doneIterBag)(blocks);
                return block;
            }
        }
        VG_(doneIterBag)(blocks);
    }

    return NULL;
}

