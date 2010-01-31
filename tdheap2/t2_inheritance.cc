#include "t2_inheritance.h"

extern "C" {
#include "pub_tool_libcassert.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
}

#include "m_stl/tr1/functional"

CallSites *g_callSites;

namespace {

template <class Iterator>
Iterator next(Iterator i) {
    return ++i;
}

template <class Iterator>
Iterator prior(Iterator i) {
    return --i;
}

bool DoIntersect(const AddrSet &lhs, const AddrSet &rhs) {
    for (AddrSet::const_iterator i = lhs.begin();
            i != lhs.end(); ++i) {
        if (rhs.count(*i) > 0) {
            return true;
        }
    }

    return false;
}

/**
 * Merges call sites.
 * Two call sites are merged if they have the same function
 * number and there is a vtable used by both.
 */
void MergeCallSites() {
    if (g_callSites->size() < 2) {
        return;
    }

    do {
        next_iter:
        for (CallSites::iterator i = g_callSites->begin();
                next(i) != g_callSites->end(); ++i) {
            for (CallSites::iterator ii = next(i);
                    ii != g_callSites->end(); ++ii) {
                if (i->second.functionNumber() ==
                        ii->second.functionNumber() &&
                        DoIntersect(i->second.callees(), ii->second.callees())) {
                    i->second.mergeWith(ii->second);
                    g_callSites->erase(ii);
                    goto next_iter;
                }
            }
        }
    } while (false);
}

} // namespace

void CallSite::mergeWith(const CallSite &site) {
    if (function_number_ != site.function_number_) {
        VG_(tool_panic)((Char *)"CallSite::mergeWith: function numbers don't match");
    }
    std::copy(site.callees_.begin(), site.callees_.end(),
            std::inserter(callees_, callees_.begin()));
}

void CallSite::addCallee(Addr addr) {
    callees_.insert(addr);
}

Addr FindVtableBeginning(Addr addr) {
    Addr *x = (Addr *)addr;
    Addr top_offset = x[-2];
    Addr ptr_to_typeinfo = x[-1];

    /*
     * There could be a problem when ptr_to_typeinfo == 0 and
     * there are two consequent null entries in the vtable.
     */
    while (top_offset != 0) {
        /*
         * Move x backwards until we found another pointer
         * to the same typeinfo.
         */
        do {
            --x;
        } while (x[-1] != ptr_to_typeinfo);
        top_offset = x[-2];
    }

    //TODO maybe also return offset from given addr.
    return (Addr)x;
}

Addr FindObjectBeginning(Addr addr, Addr real_vtable) {
    Addr *x = (Addr *)addr;

    while (x[0] != real_vtable) {
        --x;
    }

    return (Addr)x;
}

void GenerateVtablesLayout() {
    char buffer[1024];

    MergeCallSites();

    VG_(printf)("digraph hierarchy {\n");
    for (CallSites::const_iterator call_site = g_callSites->begin();
            call_site != g_callSites->end(); ++call_site) {
        const AddrSet &callees = call_site->second.callees();
        for (AddrSet::const_iterator addr = callees.begin();
                addr != callees.end(); ++addr) {
            if (!VG_(get_objname)(call_site->first, (Char *)buffer, 1024)) {
                VG_(strcpy)((Char *)buffer, (Char *)"unknown");
            }
            VG_(printf)("\"%p\" -> \"%s@%p\";\n",
                    *addr, buffer, call_site->first);
        }
    }
    VG_(printf)("}\n");
}

void InitInheritanceTracker() {
    g_callSites = new CallSites();
}

void ShutdownInheritanceTracker() {
    delete g_callSites;
}

