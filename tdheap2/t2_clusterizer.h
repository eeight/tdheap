#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include "t2_mem_tracer.h"

#include "m_stl/tr1/unordered_map"
#include "m_stl/tr1/unordered_set"
#include "m_stl/std/vector"

class Cluster {
 public:
  Cluster(const MemoryBlockPtr &block);
  void AddBlock(const MemoryBlockPtr &block);

  bool DoesBlockBelongTo(const MemoryBlockPtr &block);

  void Print();

 private:
  void AddUsedFrom_(const UsedFromSet &used_from);

  UsedFromSet used_from_;
  std::vector<MemoryBlockPtr> blocks_;
};

class Clusterizer {
 public:
  Clusterizer(MemoryTable &mem_table) :
    mem_table_(mem_table)
  {}

  void PrintClusters();

 private:
  MemoryTable mem_table_;
};

#endif

