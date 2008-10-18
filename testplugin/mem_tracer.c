#include "mem_tracer.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_oset.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h" 
#include "pub_tool_debuginfo.h"


static const Addr kBucketSize = 128;

XArray *blocks_allocated;
XArray *blocks_clusters;

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
    blocks_clusters = NULL;
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

void InsertInMemTable(MemBlock *block) {
    SizeT size = block->size;
    Addr addr = block->start_addr;
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
    InsertInMemTable(block);
}

void UnregisterMemoryBlock(Addr addr) {
    MemBlock *block = FindBlockByAddress(addr);
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

static
Bool IsSubset(OSet *a, OSet *b) {
    Word val;

    VG_(OSetWord_ResetIter)(a);
    while (VG_(OSetWord_Next)(a, &val)) {
        if (!VG_(OSetWord_Contains)(b, val)) {
            return False;
        }
    }

    return True;
}

static
Bool AreTwoMemBlocksBelongToSameCluster(MemBlock *a, MemBlock *b) {
    // iff used_from of one block is subset of another
    return IsSubset(a->used_from, b->used_from) ||
           IsSubset(b->used_from, a->used_from);
}

static
void CreateNewMemClusterWithOneElement(MemBlock *a) {
    XArray *new_cluster = VG_(newXA)(
            &VG_(malloc),
            "testplugin.createnewmemclusterwithoneelement",
            &VG_(free),
            sizeof(void *));

    VG_(addToXA)(new_cluster, &a);
    VG_(addToXA)(blocks_clusters, &new_cluster);
}

void ClusterizeMemBlocks(void) {
    if (blocks_allocated != NULL) {
        UInt blocks_count, i;

        blocks_clusters = VG_(newXA)(
                &VG_(malloc),
                "testplugin.clusterizememblocks",
                &VG_(free),
                sizeof(void *));
        blocks_count = VG_(sizeXA)(blocks_allocated);
        for (i = 0; i != blocks_count; ++i) {
            UInt clusters_count, ii;
            Bool create_new_cluster = True;
            MemBlock *current_block = *(MemBlock **)VG_(indexXA)(blocks_allocated, i);

            clusters_count = VG_(sizeXA)(blocks_clusters);
            for (ii = 0; ii != clusters_count; ++ii) {
                XArray *cluster = *(XArray **)VG_(indexXA)(blocks_clusters, ii);
                XArray *cluster_sample;
                tl_assert(VG_(sizeXA)(cluster) > 0);

                cluster_sample = *(XArray **)VG_(indexXA)(cluster, 0);
                if (AreTwoMemBlocksBelongToSameCluster(current_block,
                                                       cluster_sample)) {
                    VG_(addToXA)(cluster, &current_block);
                    create_new_cluster = False;
                    break;
                }
            }

            if (create_new_cluster) {
                CreateNewMemClusterWithOneElement(current_block);
            }
        }
    }
}

void PrettyPrintClusterFingerprint(UInt cluster_index) {
    XArray *cluster = *(XArray **)VG_(indexXA)(blocks_clusters, cluster_index);
    MemBlock *block;
    OSet *used_from = 0;
    UInt i, cluster_size;
    Addr addr;

    cluster_size = VG_(sizeXA)(cluster);
    tl_assert(cluster_size > 0);

    for (i = 0; i != cluster_size; ++i) {
        block = *(MemBlock **)VG_(indexXA)(cluster, i);
        if (used_from == NULL ||
                VG_(OSetWord_Size)(used_from) <
                        VG_(OSetWord_Size)(block->used_from)) {
            used_from = block->used_from;
        }
    }

    tl_assert(used_from != NULL);

    VG_(printf)("Cluster #%u fingerprint:\n", cluster_index);
    VG_(OSetWord_ResetIter)(used_from);
    while (VG_(OSetWord_Next)(used_from, &addr)) {
        Char filename[1024], dirname[1024];
        Bool *dirname_available;
        UInt line_num;

        if (VG_(get_filename_linenum)(addr,
                                      filename, sizeof(filename),
                                      dirname, sizeof(dirname),
                                      &dirname_available,
                                      &line_num)) {
            if (!dirname_available) {
              dirname[0] = 0;
            }
            VG_(printf)("\t%s%s:0x%u\n", dirname, filename, line_num);
        } else {
            VG_(printf)("Address 0x%x: no debug info present\n", addr);
        }
    }
}
