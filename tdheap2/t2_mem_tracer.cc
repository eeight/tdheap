#include "t2_mem_tracer.h"

namespace {

const Addr kBucketSize = 128;
const Addr kBucketMask = (static_cast<Addr>(0) - 1) << 6;

} // namespace

MemTracer *theMemTracer;

void InitMemTracer() {
  theMemTracer = new MemTracer();
} void ShutdownMemTracer() {
  delete theMemTracer;
}

MemTracer::MemTracer()
{}

MemTracer::~MemTracer()
{}

void MemTracer::RegisterMemoryBlock(Addr addr, SizeT size) {
  InsertInMemTable_(MemoryBlockPtr(new MemoryBlock(addr, size)));
}

void MemTracer::UnregisterMemoryBlock(Addr addr) {
  MemoryBlockPtr block = FindBlockByAddress(addr);

  if (block != 0) {
    RemoveFromMemTable_(block);
  }
}

void MemTracer::HandleRealloc(const MemoryBlockPtr &block,
    Addr new_start_addr, SizeT new_size) {
  // TODO save all usage data
  RemoveFromMemTable_(block);
  RegisterMemoryBlock(new_start_addr, new_size);
}

MemoryBlockPtr MemTracer::FindBlockByAddress(Addr addr) {
  Addr bucket_number = addr&kBucketMask;

  MemoryTable::iterator entry = memory_table_.find(bucket_number);

  if (entry != memory_table_.end()) {
    for (MemoryBlockSet::iterator i = entry->second.begin();
        i != entry->second.end(); ++i) {
      if ((*i)->DoesContainAddress(addr)) {
        return *i;
      }
    }
  } else {
    return MemoryBlockPtr();
  }
}

void VG_REGPARM(4) AddUsedFrom(MemoryBlock *block, Addr addr,
    Addr offset) {
  block->AddUsedFrom(addr);
  // TODO
}

void VG_REGPARM(3) MemTracer::TraceMemWrite8(Addr addr, UWord val) {
  TraceMemWrite_(addr, 1);
}

void VG_REGPARM(3) MemTracer::TraceMemWrite16(Addr addr, UWord val) {
  TraceMemWrite_(addr, 2);
}

void VG_REGPARM(3) MemTracer::TraceMemWrite32(Addr addr, UWord val) {
  TraceMemWrite_(addr, 4);
  if (sizeof(void *) == 4) {
    TracePtrWrite_(addr, val);
  }
}

void VG_REGPARM(3) MemTracer::TraceMemWrite64(Addr addr, ULong val) {
  TraceMemWrite_(addr, 8);
  if (sizeof(void *) == 8) {
    TracePtrWrite_(addr, val);
  }
}

void VG_REGPARM(3) MemTracer::TraceMemWrite_(Addr addr, UChar size) {
  MemoryBlockPtr block = FindBlockByAddress(addr);

  if (block != 0) {
    block->SetField(addr - block->start_addr(), size);
  }
}

void VG_REGPARM(3) MemTracer::TracePtrWrite_(Addr addr, Addr val) {
}

void MemTracer::InsertInMemTable_(const MemoryBlockPtr &block) {
  Addr begin_bucket = block->start_addr() & kBucketMask;
  Addr end_bucket = block->end_addr() & kBucketMask;

  for (Addr bucket = begin_bucket; bucket <= end_bucket;
      bucket += kBucketSize) {
    memory_table_[bucket].insert(block);
  }
}

void MemTracer::RemoveFromMemTable_(const MemoryBlockPtr &block) {
  Addr begin_bucket = block->start_addr() & kBucketMask;
  Addr end_bucket = block->end_addr() & kBucketMask;

  for (Addr bucket = begin_bucket; bucket <= end_bucket;
      bucket += kBucketSize) {
    memory_table_[bucket].erase(block);
  }
}
