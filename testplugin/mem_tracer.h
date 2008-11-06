#ifndef _MEM_TRACER_H_
#define _MEM_TRACER_H_

#include "pub_tool_basics.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_oset.h"
#include "pub_tool_xarray.h"

// NB: first two fields must be the same as in VGHashNode
typedef struct _MemNode {
    struct _MemNode *do_not_use; // this field is managed by HT_* functions
    UWord key;
    OSet *blocks;
} MemNode;

// NB: first two fields must be the same as in VGHashNode
typedef struct _MemClusterMapEntry {
  struct _MemBlockMapEntry *do_not_use;
  UWord offset;
  UChar size;
  struct _MemCluster *cluster;
} MemClusterMapEntry;

typedef struct _MemCluster {
    OSet *used_from;
    OSet *links_to;
    XArray *blocks;
    UWord size;
    Bool is_array;
    VgHashTable map;
} MemCluster;

// NB: first two fields must be the same as in VGHashNode
typedef struct _MemBlockMapEntry {
  struct _MemBlockMapEntry *do_not_use;
  UWord offset;
  UChar size;
  struct _MemBlock *block;
} MemBlockMapEntry;

typedef struct _MemBlock {
    Addr start_addr; // Addr is always same size with UWord
    SizeT size;
    MemCluster *cluster;
    OSet *used_from;
    OSet *links_to;
    VgHashTable map;
    ULong reads_count, writes_count;
} MemBlock;

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
void VG_REGPARM(2) TraceMemWrite8(Addr addr, UWord val);
void VG_REGPARM(2) TraceMemWrite16(Addr addr, UWord val);
void VG_REGPARM(2) TraceMemWrite32(Addr add, UWord val);
void VG_REGPARM(1) TraceMemWrite64(Addr addr, ULong val);

void ClusterizeMemBlocks(void);
void PrettyPrintClusterFingerprint(UInt cluster_index);
void PrintClustersDot(void);
void PrintClustersStructs(void);

#endif
