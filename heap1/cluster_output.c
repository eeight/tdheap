#include "cluster_output.h"

#include "mem_tracer.h"

#include "pub_tool_basics.h"
#include "pub_tool_vki.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcprint.h"

static void FPrintf(Int fd, const char *format, ...) {
  va_list va;
  static char buffer[1024];

  va_start(va, format);
  VG_(vsnprintf)(buffer, 1024, format, va);
  VG_(write)(fd, buffer, VG_(strlen)(buffer));

  va_end(va);
}

void WriteClusters(const char *filename) {
  UInt clusters_count, i, ii;
  SysRes result;
  Int fd;

  result = VG_(open)(filename, VKI_O_CREAT | VKI_O_WRONLY | VKI_O_TRUNC, 0644);

  if (result.isError) {
    VG_(printf)("Cannot open file %s for writing.\n", filename);
  } else {
    UsedFromEntry *used_from_entry;
    fd = result.res;
    // Output cluster definitions.
    FPrintf(fd, "<clusters>\n");
    clusters_count = VG_(sizeXA)(blocks_clusters);
    for (i = 0; i != clusters_count; ++i) {
      UInt size;
      MemCluster *cluster = *(MemCluster **)VG_(indexXA)(blocks_clusters, i);
      FPrintf(fd, "  <cluster name=\"x%x\" is_array=\"%s\">\n", (UWord)(cluster),
          cluster->is_array ? "true" : "false");
      FPrintf(fd, "    <fields>\n");
      for (ii = 0; ii < cluster->size; ii += size) {
        MemClusterMapEntry *entry = VG_(HT_lookup)(cluster->map, ii);
        if (entry == NULL) {
          FPrintf(fd, "      <field size=\"1\" unknown=\"true\" />\n");
          size = 1;
        } else {
          size = entry->size;
          if (entry->cluster != NULL) {
            FPrintf(fd, "      <field size=\"%u\" unknown=\"false\" points_to=\"x%x\" />\n",
                (UWord)entry->size, (UWord)entry->cluster);
          } else {
            FPrintf(fd, "      <field size=\"%u\" unknown=\"false\" />\n", entry->size);
          }
        }
      }
      FPrintf(fd, "    </fields>\n");
      FPrintf(fd, "    <usages>\n");

      // Output cluster fingerprint
      VG_(HT_ResetIter)(cluster->used_from);
      while ((used_from_entry = VG_(HT_Next)(cluster->used_from))) {
        Addr offset;
        VG_(OSetWord_ResetIter)(used_from_entry->offsets);
        while (VG_(OSetWord_Next)(used_from_entry->offsets, &offset)) {
          FPrintf(fd, "      <usage address=\"0x%x\" offset=\"%ld\"/>\n",
              used_from_entry->addr, offset);
        }
      }
      FPrintf(fd, "    </usages>\n");
      FPrintf(fd, "  </cluster>\n");
    }

    FPrintf(fd, "</clusters>\n");

    VG_(close)(fd);
  }
}
