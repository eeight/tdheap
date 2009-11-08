#include "t2_clusterizer.h"

extern "C" {
#include "pub_tool_libcprint.h"
}

namespace {

const float kSimilarityThreshold = 0.3f;

} // namespace

Cluster::Cluster(const MemoryBlockPtr &block) {
  AddBlock(block);
}

void Cluster::AddBlock(const MemoryBlockPtr &block) {
  blocks_.push_back(block);
  AddUsedFrom_(block->used_from());
}

void Cluster::AddUsedFrom_(const UsedFromSet &used_from) {
  for (UsedFromSet::const_iterator i = used_from.begin();
      i != used_from.end(); ++i) {
    used_from_.insert(*i);
  }
}

bool Cluster::DoesBlockBelongTo(const MemoryBlockPtr &block) {
  const UsedFromSet &block_used_from = block->used_from();

  SizeT count = 0;
  for (UsedFromSet::const_iterator i = block_used_from.begin();
      i != block_used_from.end(); ++i) {
    if (used_from_.count(*i) == 1) {
      ++count;
    }
  }

  return (block_used_from.size()*kSimilarityThreshold < count) &&
    (used_from_.size()*kSimilarityThreshold < count);
}

void Cluster::Print() {
  VG_(printf)("struct x%x {\n", (UWord)this);
  //TODO
  VG_(printf)("}\n");
}

void Clusterizer::PrintClusters() {
  std::vector<Cluster> clusters;

  for (MemoryTable::iterator i = mem_table_.begin();
      i != mem_table_.end(); ++i) {
    for (MemoryBlockSet::iterator ii = i->second.begin();
        ii != i->second.end(); ++ii) {
      MemoryBlockPtr &block = const_cast<MemoryBlockPtr&>(*ii);
      bool create_cluster = true;

      for (size_t cluster = 0; cluster != clusters.size(); ++cluster) {
        if (clusters[cluster].DoesBlockBelongTo(block)) {
          clusters[cluster].AddBlock(block);
          create_cluster = false;
          break;
        }
      }

      if (create_cluster) {
        clusters.push_back(Cluster(block));
      }
    }
  }

  for (size_t cluster = 0; cluster != clusters.size(); ++cluster) {
    clusters[cluster].Print();
    VG_(printf)("\n");
  }
}
