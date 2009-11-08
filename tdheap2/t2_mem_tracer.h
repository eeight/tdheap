#ifndef _MEM_TRACER_H_
#define _MEM_TRACER_H_

extern "C" {
#include "pub_tool_basics.h"
}

#include "m_stl/tr1/unordered_map"
#include "m_stl/tr1/unordered_set"
#include "m_stl/tr1/memory"

class MemoryBlock;

typedef std::tr1::unordered_map<SizeT, SizeT> FieldMap;
typedef std::tr1::unordered_set<Addr> UsedFromSet;
typedef std::tr1::shared_ptr<MemoryBlock> MemoryBlockPtr;
typedef std::tr1::unordered_set<MemoryBlockPtr> MemoryBlockSet;
typedef std::tr1::unordered_map<Addr, MemoryBlockSet> MemoryTable;

class MemoryBlock {
 public:
  MemoryBlock(Addr start_addr, SizeT size) :
    start_addr_(start_addr), size_(size), vtable_(0)
  {}

  void Reset(Addr start_addr, SizeT size) {
    start_addr_ = start_addr;
    size_ = size;
  }

  Addr start_addr() const { return start_addr_; }

  Addr end_addr() const { return start_addr_ + size_ - 1; }

  SizeT size() const { return size_; }

  const UsedFromSet &used_from() const { return used_from_; }

  void AddUsedFrom(Addr addr) {
    used_from_.insert(addr);
  }

  bool DoesContainAddress(Addr addr) const {
    return addr >= start_addr_ && addr < start_addr_ + size_;
  } 

  void SetField(SizeT offset, SizeT size) {
    field_map_[offset] = size;
    //TODO: sanity check
  }

  void SetVtable(Addr vtable) {
    vtable_ = vtable;
    //TODO: sanity check
  }

  bool operator ==(const MemoryBlock &other) const {
    return start_addr_ == other.start_addr_;
  }

 private:
  Addr start_addr_;
  SizeT size_;
  std::tr1::unordered_map<SizeT, SizeT> field_map_;
  std::tr1::unordered_set<Addr> used_from_;
  Addr vtable_;
};

namespace std {
namespace tr1 {

template <>
struct hash<MemoryBlockPtr> : public unary_function<MemoryBlockPtr, size_t> {
  size_t operator()(const MemoryBlockPtr &block) const {
    return block->start_addr();
  }
};

} // namespace tr1
} // namespace std

class MemTracer {
 public:
  MemTracer();
  ~MemTracer();

  void RegisterMemoryBlock(Addr addr, SizeT size);
  void UnregisterMemoryBlock(Addr addr);
  void HandleRealloc(const MemoryBlockPtr &block, Addr new_start_addr,
      SizeT new_size);
  void UpdateMemoryBlockSize(const MemoryBlock &block, SizeT new_size);
  MemoryBlockPtr FindBlockByAddress(Addr addr);

  void VG_REGPARM(4) AddUsedFrom(const MemoryBlock &block, Addr addr,
      Addr offset);
  void VG_REGPARM(3) TraceMemWrite8(Addr addr, UWord val);
  void VG_REGPARM(3) TraceMemWrite16(Addr addr, UWord val);
  void VG_REGPARM(3) TraceMemWrite32(Addr add, UWord val);
  void VG_REGPARM(3) TraceMemWrite64(Addr addr, ULong val);

  MemoryTable &memory_table() { return memory_table_; }

 private:
  void VG_REGPARM(3) TraceMemWrite_(Addr addr, UChar size);
  void VG_REGPARM(3) TracePtrWrite_(Addr addr, Addr val);
  void InsertInMemTable_(const MemoryBlockPtr &block);
  void RemoveFromMemTable_(const MemoryBlockPtr &block);

  MemoryTable memory_table_;
};

extern MemTracer *theMemTracer;

void InitMemTracer();
void ShutdownMemTracer();

#endif
