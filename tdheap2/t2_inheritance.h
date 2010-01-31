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
    CallSite(int function_number) : function_number_(function_number)
    {}

    void mergeWith(const CallSite &site);
    void addCallee(Addr addr);

    int functionNumber() const { return function_number_; }
    const AddrSet &callees() const { return callees_; }

private:
    AddrSet callees_;
    int function_number_;
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
