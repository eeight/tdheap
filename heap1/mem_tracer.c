#include "mem_tracer.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_oset.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h" 
#include "pub_tool_debuginfo.h"


static const Addr kBucketSize = 128;
static const float kSimilarityThreshold = 0.3f;

XArray *blocks_allocated;
XArray *blocks_clusters;

static VgHashTable mem_table;

MemBlock *NewMemBlock(Addr start_addr, SizeT size) {
  MemBlock *block = VG_(malloc)(sizeof(*block));

  block->start_addr = start_addr;
  block->size = size;
  block->used_from = VG_(HT_construct)("testplugin.newmemblock.2");
  block->reads_count = 0;
  block->writes_count = 0;
  block->links_to = VG_(OSetWord_Create)(
      &VG_(malloc),
      &VG_(free));

  block->map = VG_(HT_construct)("testplugin.newmemblock.3");

  VG_(addToXA)(blocks_allocated, &block);

  return block;
}

MemNode *NewMemNode(ULong key, MemBlock *block) {
  MemNode *node = VG_(malloc)(sizeof(*node));

  node->key = key;
  node->blocks = VG_(OSetWord_Create)(
      &VG_(malloc),
      &VG_(free));
  VG_(OSetWord_Insert)(node->blocks, (UWord)block);

  return node;
}

void InitMemTracer(void) {
  mem_table = VG_(HT_construct)("testplugin.initmemtracer.1");
  blocks_allocated = VG_(newXA)(
      &VG_(malloc),
      &VG_(free),
      sizeof(void *));
  blocks_clusters = NULL;
}

void ShutdownMemTracer(void) {
  UInt blocks_count = VG_(sizeXA)(blocks_allocated);
  UInt i;
  for (i = 0; i != blocks_count; ++i) {
    MemBlock *block = *(MemBlock **)VG_(indexXA)(blocks_allocated, i);
    VG_(free)(block);
  }
  VG_(deleteXA)(blocks_allocated);
  VG_(HT_destruct)(mem_table);
}

void InsertInMemTable(MemBlock *block) {
  SizeT size = block->size;
  Addr addr = block->start_addr;
  Addr end = addr + size;

  // Stupid and slow, but works right.
  for (; addr < end; ++addr) {
    Addr a = addr - addr%kBucketSize;
    MemNode *node = VG_(HT_lookup)(mem_table, a);
    if (node != NULL) {
      if (!VG_(OSetWord_Contains)(node->blocks, (UWord)block)) {
        VG_(OSetWord_Insert)(node->blocks, (UWord)block);
      }
    } else {
      node = NewMemNode(a, block);
      VG_(HT_add_node)(mem_table, node);
    }
  }

  //sanity check
  for (addr = block->start_addr; addr != end; ++addr) {
    MemBlock *b = FindBlockByAddress(addr);
    tl_assert(b == block);
  }
}

static
void RemoveFromMemTable(Addr addr, MemBlock *block) {
  SizeT size = block->size;
  Addr end = addr + size;

  for (; addr < end; ++addr) {
    Addr a = addr - addr%kBucketSize;
    MemNode *node = VG_(HT_lookup)(mem_table, a);
    if (node != NULL) {
      if (VG_(OSetWord_Contains)(node->blocks, (UWord)block)) {
        VG_(OSetWord_Remove)(node->blocks, (UWord)block);
      }
    }
  }
}

void RegisterMemoryBlock(Addr addr, SizeT size) {
  MemBlock *block = NewMemBlock(addr, size);
  InsertInMemTable(block);
}

void UnregisterMemoryBlock(Addr addr) {
  MemBlock *block = FindBlockByAddress(addr);
  if (block != NULL) {
    RemoveFromMemTable(addr, block);
  }
}

static
Bool VG_REGPARM(2) IsAddrInBlock(Addr addr, MemBlock *block) {
  return addr >= block->start_addr && addr < block->start_addr + block->size;
}

MemBlock *FindBlockByAddress(Addr addr) {
  Addr bucket = addr - addr%kBucketSize;
  MemNode *node = VG_(HT_lookup)(mem_table, bucket);

  if (node) {
    OSet *blocks = node->blocks;
    MemBlock *block;
    VG_(OSetWord_ResetIter)(blocks);
    while (VG_(OSetWord_Next)(blocks, (UWord *)&block)) {
      if (IsAddrInBlock(addr, block)) {
        return block;
      }
    }
  }

  return NULL;
}

static
UsedFromEntry *NewUsedFromEntry(Addr addr) {
  UsedFromEntry *entry = VG_(malloc)(sizeof(*entry));
  entry->offsets = VG_(OSetWord_Create)(
      &VG_(malloc),
      &VG_(free));
  entry->addr = addr;
  return entry;
}

static
void DestroyUsedFromEntry(UsedFromEntry *entry) {
  VG_(OSetWord_Destroy)(entry->offsets);
}

void VG_REGPARM(2) AddUsedFrom(MemBlock *block, Addr addr, Addr offset) {
  UsedFromEntry *entry = VG_(HT_lookup)(block->used_from, addr);
  if (entry == NULL) {
    entry = NewUsedFromEntry(addr);
    VG_(HT_add_node)(block->used_from, entry);
  }

  if (!VG_(OSetWord_Contains)(entry->offsets, offset)) {
    VG_(OSetWord_Insert)(entry->offsets, offset);
  }
}

static
void SanityFail(const char *msg) {
  VG_(printf)("Sanity check failed: %s\n", msg);
}

static
MemBlockMapEntry *NewMemBlockMapEntry(UWord offset, UChar size) {
  MemBlockMapEntry *entry = VG_(malloc)(sizeof(*entry));
  entry->offset = offset;
  entry->size = size;
  entry->block = NULL;

  return entry;
}

static
void VG_REGPARM(2) TraceMemWrite(Addr addr, UChar size) {
  MemBlock *dst = FindBlockByAddress(addr);
  if (dst != NULL) {
    // Update this memory block usage info.
    Addr i, i_end;

    MemBlockMapEntry *entry = VG_(HT_lookup)(dst->map, addr - dst->start_addr);
    // This field was already used.
    if (entry != NULL) {
      if (entry->size != size) {
        VG_(printf)("%d vs %d\n", entry->size, size);
        SanityFail("Size does not match");
        entry->size = size;
      }
    } else {
      // Create new field.
      entry = NewMemBlockMapEntry(addr - dst->start_addr, size);
      VG_(HT_add_node)(dst->map, entry);
    }

    // Sanity checks:
    // Check that written value doesn't overlap with any other.
    for (i = addr, i_end = addr + size; i != i_end; ++i) {
      if (VG_(HT_lookup)(dst->map, i) != NULL) {
        SanityFail("Inconsistent read/write");
      }
    }

    // Check that written value doesn't touch memory after the memory block.
    if (addr - dst->start_addr + size > dst->size) {
      SanityFail("Write beyond end of memory block");
    }
  }
}

// Addr and ULong always have same size
static
void VG_REGPARM(2) TraceAddressWrite(Addr addr, Addr val) {
  MemBlock *dst = FindBlockByAddress(addr);
  if (dst != NULL) {
    MemBlock *link = FindBlockByAddress(val);

    if (link != NULL) {
      MemBlockMapEntry *entry = VG_(HT_lookup)(dst->map, addr - dst->start_addr);
      if (entry != NULL) {
        entry->block = link;
      }

      if (!VG_(OSetWord_Contains)(dst->links_to, (UWord)link)) {
        VG_(OSetWord_Insert)(dst->links_to, (UWord)link);
      }
    }
  }
}

void VG_REGPARM(2) TraceMemWrite8(Addr addr, UWord val) {
  TraceMemWrite(addr, 1);
}

void VG_REGPARM(2) TraceMemWrite16(Addr addr, UWord val) {
  TraceMemWrite(addr, 2);
}

void VG_REGPARM(2) TraceMemWrite32(Addr addr, UWord val) {
  TraceMemWrite(addr, 4);
  // I wish I could write #if instead.
  if (sizeof(void *) == 4) {
    TraceAddressWrite(addr, val);
  }
}

void VG_REGPARM(1) TraceMemWrite64(Addr addr, ULong val) {
  TraceMemWrite(addr, 8);
  // I wish I could write #if instead.
  if (sizeof(void *) == 8) {
    TraceAddressWrite(addr, val);
  }
}

static
UInt CommonItemsCount(VgHashTable a, VgHashTable b) {
  UInt result = 0;
  UsedFromEntry *entry;

  VG_(HT_ResetIter)(a);
  while ((entry = VG_(HT_Next)(a))) {
    if (VG_(HT_lookup(b, entry->addr))) {
      ++result;
    }
  }

  return result;
}

static
Bool AreTwoUsesBelongToSameCluster(VgHashTable a, VgHashTable b) {
  UInt a_size, b_size, common_size;

  a_size = VG_(HT_count_nodes)(a);
  b_size = VG_(HT_count_nodes)(b);
  common_size = CommonItemsCount(a, b);
  if (a_size*kSimilarityThreshold < common_size || 
      b_size*kSimilarityThreshold < common_size) {
    return True;
  } else {
    return False;
  }
}

static
void SetAddHashKeys(OSet *to, VgHashTable from) {
  UsedFromEntry *entry;

  VG_(HT_ResetIter)(from);
  while ((entry = VG_(HT_Next)(from))) {
    if (!VG_(OSetWord_Contains)(to, entry->addr)) {
      VG_(OSetWord_Insert)(to, entry->addr);
    }
  }
}

static
void SetAdd(OSet *to, OSet *from) {
  Word val;

  VG_(OSetWord_ResetIter)(from);
  while (VG_(OSetWord_Next)(from, &val)) {
    if (!VG_(OSetWord_Contains)(to, val)) {
      VG_(OSetWord_Insert)(to, val);
    }
  }
}

static
void HashAdd(VgHashTable to, VgHashTable from) {
  UsedFromEntry *entry;

  VG_(HT_ResetIter)(from);
  while ((entry = VG_(HT_Next)(from))) {
    UsedFromEntry *old_entry = VG_(HT_lookup)(to, entry->addr);
    if (!old_entry) {
      old_entry = NewUsedFromEntry(entry->addr);
      VG_(HT_add_node)(to, old_entry);
    }

    SetAdd(old_entry->offsets, entry->offsets);
  }
}

static
void CreateNewMemClusterWithOneElement(MemBlock *a) {
  MemCluster *cluster = VG_(malloc)(sizeof(*cluster));
  cluster->used_from = VG_(HT_construct)(
      "heap1.createnewmemclusterwithoneelement.1");
  cluster->links_to = VG_(OSetWord_Create)(
      &VG_(malloc),
      &VG_(free));
  cluster->blocks = VG_(newXA)(
      &VG_(malloc),
      &VG_(free),
      sizeof(void *));
  cluster->map = VG_(HT_construct)(
      "heap1.createnewmemclusterwithoneelement.2");

  HashAdd(cluster->used_from, a->used_from);

  VG_(addToXA)(cluster->blocks, &a);
  VG_(addToXA)(blocks_clusters, &cluster);
}

static
void MergeTwoClusters(MemCluster *to, MemCluster *from) {
  UInt i, blocks_count;
  void **val;

  HashAdd(to->used_from, from->used_from);
  // TODO propely clear memory used for offsets members in hash table.
  VG_(HT_destruct)(from->used_from);
  from->used_from = NULL;

  blocks_count = VG_(sizeXA)(from->blocks);
  for (i = 0; i != blocks_count; ++i) {
    val = VG_(indexXA)(from->blocks, i);
    VG_(addToXA)(to->blocks, val);
  }
  VG_(deleteXA)(from->blocks);
  from->blocks = NULL;
}

static
void MergeClusters(void) {
  UInt i, ii, clusters_count;
  XArray *new_clusters;
  Bool merged;

  clusters_count = VG_(sizeXA)(blocks_clusters);
  do {
    merged = False;
    for (i = 0; i != clusters_count; ++i) {
      MemCluster *a = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);
      if (a->used_from == NULL) {
        continue;
      }
      for (ii = i + 1; ii != clusters_count; ++ii) {
        MemCluster *b = *(MemCluster **)VG_(indexXA)(blocks_clusters, ii);
        if (b->used_from == NULL) {
          continue;
        }
        if (AreTwoUsesBelongToSameCluster(a->used_from, b->used_from)) {
          MergeTwoClusters(a, b);
          merged = True;
        }
      }
    }
  } while (merged);
  new_clusters = VG_(newXA)(
      &VG_(malloc),
      &VG_(free),
      sizeof(void *));
  clusters_count = VG_(sizeXA)(blocks_clusters);
  for (i = 0; i != clusters_count; ++i) {
    MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);
    if (cluster->used_from != NULL) {
      VG_(addToXA)(new_clusters, &cluster);
    }
  }

  VG_(deleteXA)(blocks_clusters);
  blocks_clusters = new_clusters;
}

static
void AddToMemCluster(MemCluster *cluster, MemBlock *block) {
  VG_(addToXA)(cluster->blocks, &block);
  HashAdd(cluster->used_from, block->used_from);
}

static
UWord Gcd(UWord a, UWord b) {
  if (a == 0) {
    return b;
  } else if (b == 0) {
    return a;
  } else {
    return Gcd(b, a%b);
  }
}

static
MemClusterMapEntry *NewMemClusterMapEntry(MemBlockMapEntry *block_entry) {
  MemClusterMapEntry *entry = VG_(malloc)(sizeof(*entry));

  entry->offset = block_entry->offset;
  entry->size = block_entry->size;
  if (block_entry->block != NULL) {
    entry->cluster = block_entry->block->cluster;
  } else {
    entry->cluster = NULL;
  }

  return entry;
}

static
void PostProcessClusters(void) {
  UInt clusters_count = VG_(sizeXA)(blocks_clusters);
  UInt i;

  // Firstly, set links from memory blocks to clusters they are belong to.
  for (i = 0; i != clusters_count; ++i) {
    UInt blocks_count, ii;
    MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);
    blocks_count = VG_(sizeXA)(cluster->blocks);

    for (ii = 0; ii != blocks_count; ++ii) {
      MemBlock *block = *(MemBlock **)VG_(indexXA)(cluster->blocks, ii);
      block->cluster = cluster;
    }
  }

  for (i = 0; i != clusters_count; ++i) {
    UInt blocks_count, ii;
    MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);
    UWord size = 0;
    Bool is_array = False;
    blocks_count = VG_(sizeXA)(cluster->blocks);

    for (ii = 0; ii != blocks_count; ++ii) {
      MemBlock *block = *(MemBlock **)VG_(indexXA)(cluster->blocks, ii);
      MemBlock *link_to;
      MemBlockMapEntry *block_entry;
      MemClusterMapEntry *cluster_entry;

      if (size != 0 && size != block->size) {
        is_array = True;
      }
      size = Gcd(size, block->size);

      // deal with pointers from this block
      VG_(OSetWord_ResetIter)(block->links_to);
      while (VG_(OSetWord_Next)(block->links_to, (UWord *)&link_to)) {
        MemCluster *link_to_cluster = link_to->cluster;
        if (!VG_(OSetWord_Contains)(cluster->links_to, (UWord)link_to_cluster)) {
          VG_(OSetWord_Insert)(cluster->links_to, (UWord)link_to_cluster);
        }
      }

      // usage maps
      VG_(HT_ResetIter)(block->map);
      while ((block_entry = VG_(HT_Next)(block->map))) {
        tl_assert(block_entry->size <= 8);
        cluster_entry = VG_(HT_lookup)(cluster->map, block_entry->offset);

        if (cluster_entry != NULL) {
          if (block_entry->block != NULL) {
            if (cluster_entry->cluster == NULL) {
              cluster_entry->cluster = block_entry->block->cluster;
            } else if (cluster_entry->cluster != block_entry->block->cluster) {
              SanityFail("Inconsistent clusterization");
            }
          }
        } else {
          cluster_entry = NewMemClusterMapEntry(block_entry);
          VG_(HT_add_node)(cluster->map, cluster_entry);
        }
      }
    }

    cluster->size = size;
    cluster->is_array = is_array;
  }
}

void ClusterizeMemBlocks(void) {
  if (blocks_allocated != NULL) {
    UInt blocks_count, i;

    blocks_clusters = VG_(newXA)(
        &VG_(malloc),
        &VG_(free),
        sizeof(void *));
    blocks_count = VG_(sizeXA)(blocks_allocated);
    for (i = 0; i != blocks_count; ++i) {
      UInt clusters_count, ii;
      Bool create_new_cluster = True;
      MemBlock *current_block = *(MemBlock **)VG_(indexXA)(blocks_allocated, i);

      clusters_count = VG_(sizeXA)(blocks_clusters);
      for (ii = 0; ii != clusters_count; ++ii) {
        MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, ii);
        MemBlock *cluster_sample;
        tl_assert(VG_(sizeXA)(cluster->blocks) > 0);

        cluster_sample = *(MemBlock **)VG_(indexXA)(cluster->blocks, 0);
        if (AreTwoUsesBelongToSameCluster(current_block->used_from,
              cluster_sample->used_from)) {
          AddToMemCluster(cluster, current_block);
          create_new_cluster = False;
          break;
        }
      }

      if (create_new_cluster) {
        CreateNewMemClusterWithOneElement(current_block);
      }
    }
  }
  MergeClusters();
  PostProcessClusters();
}

void PrettyPrintClusterFingerprint(UInt cluster_index) {
  MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, cluster_index);
  MemBlock *block;
  VgHashTable used_from = 0;
  UInt i, cluster_size;
  UsedFromEntry *entry;

  cluster_size = VG_(sizeXA)(cluster->blocks);
  tl_assert(cluster_size > 0);

  for (i = 0; i != cluster_size; ++i) {
    block = *(MemBlock **)VG_(indexXA)(cluster->blocks, i);
    if (used_from == NULL ||
        VG_(HT_count_nodes)(used_from) <
        VG_(HT_count_nodes)(block->used_from)) {
      used_from = block->used_from;
    }
  }

  tl_assert(used_from != NULL);

  VG_(printf)("Cluster #%u fingerprint:\n", cluster_index);
  if (cluster->is_array) {
    VG_(printf)("\trepresents array with element ");
  } else {
    VG_(printf)("\trepresents single structure of ");
  }
  VG_(printf)("size %lu\n", cluster->size);
  VG_(printf)("\tLinks to %lu clusters\n", VG_(OSetWord_Size)(cluster->links_to));
  VG_(HT_ResetIter)(used_from);
  while ((entry = VG_(HT_Next)(used_from))) {
    Char filename[1024], dirname[1024];
    Bool dirname_available;
    UInt line_num;

    if (VG_(get_filename_linenum)(entry->addr,
          filename, sizeof(filename),
          dirname, sizeof(dirname),
          &dirname_available,
          &line_num)) {
      if (!dirname_available) {
        dirname[0] = 0;
      }
      VG_(printf)("\t%s/%s:%u\n", dirname, filename, line_num);
    } else {
      VG_(printf)("\tAddress 0x%x: no debug info present\n", entry->addr);
    }
  }
}

void PrintClustersDot(void) {
  UInt clusters_count = VG_(sizeXA)(blocks_clusters);
  UInt i;

  VG_(printf)("digraph A {\n");
  for (i = 0; i != clusters_count; ++i) {
    MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);
    MemCluster *link_to;

    VG_(printf)("x%x;\n", (UWord)cluster);

    VG_(OSetWord_ResetIter)(cluster->links_to);
    while (VG_(OSetWord_Next)(cluster->links_to, (UWord *)&link_to)) {
      VG_(printf)("x%x -> x%x;\n", (UWord)cluster, (UWord)link_to);
    }
  }

  VG_(printf)("}\n");
}
void PrintClustersDotStructs(void) {
  UInt clusters_count = VG_(sizeXA)(blocks_clusters);
  UInt i, ii;

  VG_(printf)("digraph A {\n");
  for (i = 0; i != clusters_count; ++i) {
    UWord size;
    MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);
    MemCluster *link_to;

    VG_(printf)("x%x [ label=\"", (UWord)cluster);

    VG_(printf)("struct x%x {\\n", (UWord)(cluster));
    for (ii = 0; ii < cluster->size; ii += size) {
      MemClusterMapEntry *entry = VG_(HT_lookup)(cluster->map, ii);
      if (entry == NULL) {
        size = 1;
        VG_(printf)("?: type8;\\n");
      } else {
        size = entry->size;
        VG_(printf)("value: ");
        if (entry->cluster != NULL) {
          VG_(printf)("*x%x", entry->cluster);
        } else {
          VG_(printf)("type%d", entry->size*8);
        }
        VG_(printf)(";\\n");
      }
    }
    VG_(printf)("};\"];\n");

    VG_(OSetWord_ResetIter)(cluster->links_to);
    while (VG_(OSetWord_Next)(cluster->links_to, (UWord *)&link_to)) {
      VG_(printf)("x%x -> x%x;\n", (UWord)cluster, (UWord)link_to);
    }
  }

  VG_(printf)("}\n");
}

void PrintClustersStructs(void) {
  UInt clusters_count = VG_(sizeXA)(blocks_clusters);
  UInt i, ii;

  for (i = 0; i != clusters_count; ++i) {
    UWord size;
    MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);

    VG_(printf)("struct x%x {\n", (UWord)(cluster));
    for (ii = 0; ii < cluster->size; ii += size) {
      MemClusterMapEntry *entry = VG_(HT_lookup)(cluster->map, ii);
      if (entry == NULL) {
        size = 1;
        VG_(printf)("?: type8;\n");
      } else {
        size = entry->size;
        VG_(printf)("value: ");
        if (entry->cluster != NULL) {
          VG_(printf)("*x%x", entry->cluster);
        } else {
          VG_(printf)("type%d", entry->size*8);
        }
        VG_(printf)(";\n");
      }
    }
    VG_(printf)("}\n\n");
  }
}
