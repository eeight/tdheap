#ifndef _INHERITANCE_H_
#define _INHERITANCE_H_

extern "C" {
#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
}

#include "m_stl/tr1/unordered_map"
#include "m_stl/tr1/unordered_set"

typedef std::tr1::unordered_set<Addr> AddrSet;

class CallSite {
public:
    CallSite(Addr ip, int function_number) : 
        ip_(ip), function_number_(function_number), parent_(0),
        isInherited_(false)
    {}

    void mergeWith(const CallSite &site);
    void addCallee(Addr addr);

    Addr ip() const { return ip_; }

    int functionNumber() const { return function_number_; }
    const AddrSet &callees() const { return callees_; }

    CallSite *parent() const { return parent_; }

    void setParent(CallSite *parent) {
        parent_ = parent;
        parent->isInherited_ = true;
    }

    bool isInherited() const { return isInherited_; }

private:
    // Vtables used with this call site.
    AddrSet callees_;
    Addr ip_;
    int function_number_;
    CallSite *parent_;
    bool isInherited_;
};

typedef std::tr1::unordered_map<Addr, CallSite> CallSites;

/*
 * Finds begginning of vtable.
 * If a virtual call happens though pointer to inherited class
 * pointer to virtual table might have a non-zero offset from real vtable.
 *
 * Layout is the following:
 * (See http://www.cse.wustl.edu/~mdeters/seminar/fall2005/mi.html
 * for more details).
 *
 * <vbase_offset>
 * <top_offset>
 * <ptr to typeinfo>
 * first method
 */
Addr FindVtableBeginning(Addr addr);
Addr FindObjectBeginning(Addr addr, Addr real_vtable);

void GenerateVtablesLayout();

extern CallSites *g_callSites;

void InitInheritanceTracker();
void ShutdownInheritanceTracker();

#endif
