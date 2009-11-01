#ifndef _MEM_TRACER_H_
#define _MEM_TRACER_H_

extern "C" {
#include "pub_tool_basics.h"
}

#include "m_stl/tr1/unordered_map"
#include "m_stl/tr1/unordered_set"

class MemoryBlock {
 public:
  MemoryBlock(Addr start_addr, SizeT size) :
    start_addr_(start_addr), size_(size)
  {}

  Addr start_addr() const { return start_addr_; }
  Addr end_addr() const { return start_addr_ + size_ - 1; }
  SizeT size() const { return size; }
  bool DoesContainAddress(Addr addr) const {
    return addr >= start_addr_ && addr < start_addr_ + size_;
  } 
  bool operator ==(const MemoryBlock &other) {
    return start_addr_ == other.start_addr_;
  }

 private:
  Addr start_addr_;
  SizeT size_;
};

namespace std {
namespace tr1 {

template <>
size_t hash::operator()<MemoryBlock>(const MemoryBlock &block) {
  return block.start_addr();
}

} // namespace tr1
} // namespace std

class MemTracer {
 public:
  MemTracer();
  ~MemTracer();

  void RegisterMemoryBlock(Addr addr, SizeT size);
  void UnregisterMemoryBlock(Addr addr);
  void UnregisterMemoryBlock(const MemoryBlock &block, Addr new_start_addr,
      SizeT new_size);
  void UpdateMemoryBlockSize(const MemoryBlock &block, SizeT new_size);
  const MemoryBlock *FindBlockByAddress(Addr addr);

  void VG_REGPARM(4) AddUsedFrom(const MemoryBlock &block, Addr addr,
      Addr offset);
  void VG_REGPARM(4) TraceMemWrite8(Addr addr, UWord val);
  void VG_REGPARM(4) TraceMemWrite16(Addr addr, UWord val);
  void VG_REGPARM(4) TraceMemWrite32(Addr add, UWord val);
  void VG_REGPARM(3) TraceMemWrite64(Addr addr, ULong val);

 private:
  void InsertInMemTable_(const MemoryBlock &block);
  void RemoveFromMemTable_(const MemoryBlock &block);

  typedef std::tr1::unordered_set<MemoryBlock> MemoryBlockSet;
  typedef std::tr1::unordered_map<Addr, MemoryBlockSet> MemoryTable;
  MemoryTable memory_table_;
};

extern MemTracer theMemTracer;

#endif