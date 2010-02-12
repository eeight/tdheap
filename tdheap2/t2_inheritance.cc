#include "t2_inheritance.h"

extern "C" {
#include "pub_tool_libcassert.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
}

#include "m_stl/tr1/functional"

CallSites *g_callSites;
VTables *g_vtables;

// FIXME Memory occupied by g_callSites is never freed.
// FIXME Memory occupied by g_vtables is never freed.

namespace {

std::string int2string(int n) {
    char buffer[1024];
    VG_(snprintf)((Char *)buffer, 1024, "%d", n);
    return buffer;
}

template <class Iterator>
Iterator next(Iterator i) {
    return ++i;
}

template <class Iterator>
Iterator prior(Iterator i) {
    return --i;
}

bool DoIntersect(const VTableSet &lhs, const VTableSet &rhs) {
    for (VTableSet::const_iterator i = lhs.begin();
            i != lhs.end(); ++i) {
        if (rhs.count(*i) > 0) {
            return true;
        }
    }

    return false;
}

/**
 * Propagates callees.
 * Suppose we have two call sites intersecting by used vtables with
 * function numbers n and m, n < m. The we say that everythig used
 * through the second one might be used through the first one.
 */
void PropagateCallees() {
    bool has_been_propagaged;

    do { 
        has_been_propagaged = false;
        for (CallSites::iterator i = g_callSites->begin();
                next(i) != g_callSites->end(); ++i) {
            for (CallSites::iterator ii = next(i);
                    ii != g_callSites->end(); ++ii) {
                CallSite *a = i->second;
                CallSite *b = ii->second;
                if (a->functionNumber() > b->functionNumber()) {
                    std::swap(a, b);
                }
                if (DoIntersect(a->callees(), b->callees())) {
                    if (a->copyCalleesFrom(b)) {
                        has_been_propagaged = true;
                    }
                }
            }
        }
    } while (has_been_propagaged);
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

    next_iter:
    for (CallSites::iterator i = g_callSites->begin();
            next(i) != g_callSites->end(); ++i) {
        for (CallSites::iterator ii = next(i);
                ii != g_callSites->end(); ++ii) {
            if (i->second->functionNumber() ==
                    ii->second->functionNumber() &&
                    DoIntersect(i->second->callees(), ii->second->callees())) {
                i->second->mergeWith(ii->second);
                g_callSites->erase(ii);
                goto next_iter;
            }
        }
    }
}

/**
 * Sorts call sites' interfaces.
 * Suppose we have two call sites used with same vtable.
 * First of them refers to nth furction, second refers to mth function and n < m.
 * Then we say second call site interface inherits the fist call site interface.
 */
void SortInterfaces() {
    if (g_callSites->size() < 2) {
        return;
    }

    for (CallSites::iterator i = g_callSites->begin();
            next(i) != g_callSites->end(); ++i) {
        for (CallSites::iterator ii = next(i);
                ii != g_callSites->end(); ++ii) {
            CallSite *a = i->second;
            CallSite *b = ii->second;
            if (DoIntersect(a->callees(), b->callees())) {
                if (a->functionNumber() == b->functionNumber()) {
                    VG_(tool_panic)((Char *)"Call MergeCallSites before calling SortInterfaces #1");
                }
                if (a->functionNumber() < b->functionNumber()) {
                    std::swap(a, b);
                }
                if (a->parent() != 0 &&
                        a->parent()->functionNumber() == b->functionNumber()) {
                    VG_(printf)("a=%p, a->parent=%p, b=%p\n", a, a->parent(), b);
                    VG_(printf)("Intersect: %d %d %d",
                            (int)DoIntersect(a->callees(), b->callees()),
                            (int)DoIntersect(a->parent()->callees(), a->callees()),
                            (int)DoIntersect(a->parent()->callees(), b->callees())
                            );
                    VG_(tool_panic)((Char *)"Call MergeCallSites before calling SortInterfaces #2");
                }
                if (a->parent() == 0 ||
                        (a->parent()->functionNumber() < b->functionNumber())) {
                    a->setParent(b);
                }
            }
        }
    }
}

/**
 * Collapses chains of inheritance.
 * A -> B -> C
 */
void CollapseChains() {
    next_iter:
    for (CallSites::iterator i = g_callSites->begin();
            i != g_callSites->end(); ++i) {
        CallSite *call_site = i->second;

        if (call_site->parent() != 0 &&
                call_site->parent()->timesInherited() == 1) {
            call_site->parent()->mergeWithChild(call_site);
            // Update parent pointers pointing to the call site being
            // removed.
            if (call_site->timesInherited() > 0) {
                for (CallSites::iterator ii = g_callSites->begin();
                        ii != g_callSites->end(); ++ii) {
                    if (ii->second->parent() == call_site) {
                        ii->second->setParent(call_site->parent());
                    }
                }
            }
            call_site->setParent(0);
            g_callSites->erase(i);
            goto next_iter;
        }
    }
}

std::string codeReference(Addr addr) {
    char objname[1024];
    char buffer[1024];

    if (!VG_(get_objname)(addr, (Char *)objname, 1024)) {
        VG_(strcpy)((Char *)objname, (Char *)"unknown");
    }

    VG_(snprintf)((Char *)buffer, 1024, "%p@%s", addr, objname);
    return buffer;
}

} // namespace

Addr VTable::start() const {
    if (parent_ != 0) {
        return parent_->start_;
    } else {
        return start_;
    }
}

void VTable::addChild(VTable *child) {
    children_.insert(child);
    child->parent_ = this;
}

std::string VTable::label() const {
    if (parent_ != 0) {
        return codeReference(parent_->start_);
    } else {
        return codeReference(start_);
    }
}

void CallSite::mergeWith(const CallSite *site) {
    if (function_number_ != site->function_number_) {
        VG_(tool_panic)((Char *)"CallSite::mergeWith: function numbers don't match");
    }
    copyCalleesFrom(site);
}

void CallSite::mergeWithChild(const CallSite *site) {
    if (function_number_ >= site->function_number_) {
        VG_(tool_panic)((Char *)"CallSite::mergeWithChild: function numbers don't match");
    }
    copyCalleesFrom(site);
    function_number_ = site->function_number_;
}

bool CallSite::copyCalleesFrom(const CallSite *site) {
    size_t old_size = callees_.size();

    std::copy(site->callees_.begin(), site->callees_.end(),
            std::inserter(callees_, callees_.begin()));

    return callees_.size() > old_size;
}
void CallSite::addCallee(VTable *vtable) {
    callees_.insert(vtable);
}

void CallSite::setParent(CallSite *parent) {
    if (parent_ != 0) {
        --parent_->timesInherited_;
    }
    parent_ = parent;
    if (parent != 0) {
        ++parent->timesInherited_;
    } }

std::string CallSite::label() const { return codeReference(ip_) + "\\n// Functions count: " + int2string(function_number_ + 1);
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

VTable *getVtable(Addr vtable) {
    if (g_vtables->find(vtable) != g_vtables->end()) {
        return (*g_vtables)[vtable];
    } else {
        VTable *result = new VTable(vtable);
        g_vtables->insert(std::make_pair(vtable, result));
        return result;
    }
}

void GenerateVtablesLayout() {
    PropagateCallees();
    MergeCallSites();
    SortInterfaces();
    CollapseChains();

    VG_(printf)("digraph hierarchy {\n");
    for (CallSites::const_iterator call_site = g_callSites->begin();
            call_site != g_callSites->end(); ++call_site) {
        const VTableSet &callees = call_site->second->callees();
        for (VTableSet::const_iterator vtable = callees.begin();
                vtable != callees.end(); ++vtable) {
            if (call_site->second->timesInherited() == 0) {
                VG_(printf)("\"%p\" -> \"%p\";\n",
                        (*vtable)->start(), call_site->first);
            }
        }

        VG_(printf)("\"%p\" [ label=\"%s\", shape=rectangle ];\n",
                call_site->first, call_site->second->label().c_str());

        if (call_site->second->parent() != 0) {
            VG_(printf)("\"%p\" -> \"%p\";\n",
                    call_site->first,
                    call_site->second->parent()->ip());
        }
    }
    for (VTables::const_iterator vtable = g_vtables->begin();
            vtable != g_vtables->end(); ++vtable) {
        if (vtable->second->parent() == 0) {
            VG_(printf)("\"%p\" [ label=\"%s\" ];\n",
                    vtable->second->start(), vtable->second->label().c_str());
        }
    }
    VG_(printf)("}\n");
}

void InitInheritanceTracker() {
    g_callSites = new CallSites();
    g_vtables = new VTables();
}

void ShutdownInheritanceTracker() {
    delete g_callSites;
    delete g_vtables;
}
