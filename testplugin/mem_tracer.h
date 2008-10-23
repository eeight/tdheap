#ifndef _MEM_TRACER_H_
#define _MEM_TRACER_H_

#include "pub_tool_basics.h"
#include "pub_tool_oset.h"
#include "pub_tool_xarray.h"

// NB: first two fields must be the same as in VGHashNode
typedef struct _MemNode {
    struct _MemNode *do_not_use; // this field is managed by HT_* functions
    UWord key;
    OSet *blocks;
} MemNode;

typedef struct _MemBlock {
    Addr start_addr; // Addr is always same size with UWord
    SizeT size;
    OSet *used_from;
    ULong reads_count, writes_count;
} MemBlock;

typedef struct _MemCluster {
    OSet *used_from;
    XArray *blocks;
} MemCluster;

extern XArray *blocks_allocated;
extern XArray *blocks_clusters;

MemBlock *NewMemBlock(Addr start_addr, SizeT size);
MemNode *NewMemNode(ULong key, MemBlock *block);

void InitMemTracer(void);
void ShutdownMemTracer(void);
void RegisterMemoryBlock(Addr addr, SizeT size);
void InsertInMemTable(MemBlock *block);
void UnregisterMemoryBlock(Addr addr);
MemBlock *FindBlockByAddress(Addr addr);
void VG_REGPARM(2) AddUsedFrom(MemBlock *block, Addr addr);

void ClusterizeMemBlocks(void);
void PrettyPrintClusterFingerprint(UInt cluster_index);

#endif
