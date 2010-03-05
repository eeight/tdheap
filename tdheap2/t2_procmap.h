#ifndef _PROCMAP_H_
#define _PROCMAP_H_

extern "C" {
#include "pub_tool_basics.h"
}

void initProcmap();
void shutdownProcmap();
bool isAddressReadable(Addr addr);
bool isAddressWritable(Addr addr);

#endif
