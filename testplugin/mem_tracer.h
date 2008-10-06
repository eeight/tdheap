#ifndef _MEM_TRACER_H_
#define _MEM_TRACER_H_

#include "pub_tool_basics.h"
#include "pub_tool_wordfm.h"

// NB: first two fields must be the same as in VGHashNode
typedef struct _MemNode {
    struct _MemNode *do_not_use; // this field is managed by HT_* functions
    UWord key;
    WordBag *blocks;
} MemNode;

typedef struct _MemBlock {
    Addr start_addr; // Addr is always same size with UWord
    SizeT size;
    ULong reads_count, writes_count;
} MemBlock;

MemBlock *NewMemBlock(Addr start_addr, SizeT size);
MemNode *NewMemNode(ULong key, MemBlock *block);

void InitMemTracer(void);
void ShutdownMemTracer(void);
void RegisterMemoryBlock(Addr addr, SizeT size);
MemBlock *FindBlockByAddress(Addr addr);

#endif
