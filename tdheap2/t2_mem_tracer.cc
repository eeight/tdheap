#include "t2_mem_tracer.h"

namespace {

const Addr kBucketSize = 128;
const Addr kBucketMask = (static_cast<Addr>(0) - 1) << 6;

} // namespace

MemTracer *theMemTracer;

void InitMemTracer() {
  theMemTracer = new MemTracer();
}

void ShutdownMemTracer() {
  delete theMemTracer;
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

  if (block != 0) {
    RemoveFromMemTable_(*block);
  }
}

void MemTracer::HandleRealloc(const MemoryBlock &block,
    Addr new_start_addr, SizeT new_size) {
  RemoveFromMemTable_(block);
  RegisterMemoryBlock(new_start_addr, new_size);
}

const MemoryBlock *MemTracer::FindBlockByAddress(Addr addr) {
  Addr bucket_number = addr&kBucketMask;

  MemoryTable::const_iterator entry = memory_table_.find(bucket_number);

  if (entry != memory_table_.end()) {
    for (MemoryBlockSet::const_iterator i = entry->second.begin();
        i != entry->second.end(); ++i) {
      if (i->DoesContainAddress(addr)) {
        return &*i;
      }
    }
  } else {
    return 0;
  }
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
  Addr begin_bucket = block.start_addr() & kBucketMask;
  Addr end_bucket = block.end_addr() & kBucketMask;

  for (Addr bucket = begin_bucket; bucket <= end_bucket;
      bucket += kBucketSize) {
    memory_table_[bucket].insert(block);
  }
}

void MemTracer::RemoveFromMemTable_(const MemoryBlock &block) {
  Addr begin_bucket = block.start_addr() & kBucketMask;
  Addr end_bucket = block.end_addr() & kBucketMask;

  for (Addr bucket = begin_bucket; bucket <= end_bucket;
      bucket += kBucketSize) {
    memory_table_[bucket].erase(block);
  }
}
