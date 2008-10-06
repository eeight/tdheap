#include "mem_tracer.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_hashtable.h"

static VgHashTable mem_table;

void InitMemTracer(void) {
    mem_table = VG_(HT_construct)("testplugin.initmemtracer");
}

void ShutdownMemTracer(void) {
    VG_(HT_destruct)(mem_table);
}

void RegisterMemoryBlock(Addr addr, SizeT size) {
    MemNode *node = NewMemNode(addr, size);
    VG_(HT_add_node)(mem_table, node);
}

MemNode *FindNodeByAddress(Addr addr) {
    // TODO: do something useful
    return NULL;
}

MemNode *NewMemNode(Addr start_addr, SizeT size) {
    MemNode *result = VG_(malloc)("testplugin.newmemdescriptor",
                                    sizeof(*result));
    result->start_addr = start_addr;
    result->size = size;
    result->reads_count = 0;
    result->writes_count = 0;
    return result;
}
