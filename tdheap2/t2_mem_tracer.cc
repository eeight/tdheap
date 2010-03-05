#include "t2_mem_tracer.h"

extern "C" {
#include "pub_tool_replacemalloc.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_libcassert.h"
}

namespace {

const Addr kBucketSize = 128;
const Addr kBucketMask = ~(kBucketSize - 1);

} // namespace

MemTracer *g_memTracer;

void initMemTracer() {
    g_memTracer = new MemTracer();
}

void shutdownMemTracer() {
    delete g_memTracer;
}

MemTracer::MemTracer()
{}

MemTracer::~MemTracer()
{}

void MemTracer::RegisterMemoryBlock(Addr addr, SizeT size) {
    InsertInMemTable_(MemoryBlock(addr, size));
}

void MemTracer::UnregisterMemoryBlock(Addr addr) {
    const MemoryBlock *block = FindBlockByAddress(addr);

    if (block == 0) {
        VG_(tool_panic)((Char *)"Cannot unregister not registered block");
    }
    RemoveFromMemTable_(*block);
}

void MemTracer::HandleRealloc(const MemoryBlock &block,
        Addr new_start_addr, SizeT new_size) {
    RemoveFromMemTable_(block);
    RegisterMemoryBlock(new_start_addr, new_size);
}

const MemoryBlock *MemTracer::FindBlockByAddress(Addr addr) {
    Addr bucket_number = addr & kBucketMask;

    MemoryTable::const_iterator entry = memory_table_.find(bucket_number);

    if (entry != memory_table_.end()) {
        for (MemoryBlockSet::const_iterator i = entry->second.begin();
                i != entry->second.end(); ++i) {
#if 0
            VG_(printf)("FindBlockByAddress p=%p, size=%d\n",
                    i->start_addr(), i->size());
#endif
            if (i->DoesContainAddress(addr)) {
#if 0
                VG_(printf)("Block found. start=%p, size=%d\n",
                        i->start_addr(), i->size());
#endif
                return &*i;
            }
        }
    } else {
        return 0;
    }

    return 0;
}

void VG_REGPARM(4) AddUsedFrom(const MemoryBlock &block, Addr addr,
        Addr offset) {
    // TODO
}

void VG_REGPARM(4) MemTracer::TraceMemWrite8(Addr addr, UWord val) {
    // TODO
}

void VG_REGPARM(4) MemTracer::TraceMemWrite16(Addr addr, UWord val) {
    // TODO
}

void VG_REGPARM(4) MemTracer::TraceMemWrite32(Addr add, UWord val) {
    // TODO
}

void VG_REGPARM(3) MemTracer::TraceMemWrite64(Addr addr, ULong val) {
    // TODO
}


void MemTracer::InsertInMemTable_(const MemoryBlock &block) {
#if 0
    VG_(printf)("InsertInMemTable p=%p, size=%d\n",
            block.start_addr(), block.size());
#endif
    Addr begin_bucket = block.start_addr() & kBucketMask;
    Addr end_bucket = block.end_addr() & kBucketMask;

    for (Addr bucket = begin_bucket; bucket <= end_bucket;
            bucket += kBucketSize) {
#if 0
        VG_(printf)("Bucket = %p, bucket_size=%d\n",
                bucket, memory_table_[bucket].size());
#endif
        if (!memory_table_[bucket].insert(block).second) {
            VG_(tool_panic)((Char *)"InsertInMemTable_: cannot insert block");
        }
    }
}

void MemTracer::RemoveFromMemTable_(const MemoryBlock &block) {
#if 0
    VG_(printf)("RemoveFromMemTable p=%p, size=%d\n",
            block.start_addr(), block.size());
#endif
    // We have to make a copy of block.
    // When the block gets erased from the first bucket
    // it belongs, the block becomes invalid.
    MemoryBlock copy(block);
    Addr begin_bucket = block.start_addr() & kBucketMask;
    Addr end_bucket = block.end_addr() & kBucketMask;

    for (Addr bucket = begin_bucket; bucket <= end_bucket;
            bucket += kBucketSize) {
#if 0
        VG_(printf)("Bucket = %p, bucket_size=%d\n",
                bucket, memory_table_[bucket].size());
#endif
        if (memory_table_[bucket].erase(copy) != 1) {
#if 0
            for (MemoryBlockSet::const_iterator i =
                    memory_table_[bucket].begin();
                    i != memory_table_[bucket].end(); ++i) {
                VG_(printf)("Block: p=%p, size=%d, equal=%d\n",
                        i->start_addr(), i->size(), (int)(*i == copy));
            }
#endif
            VG_(tool_panic)((Char *)"RemoveFromMemTable_: cannot erase block");
        }
    }
}
