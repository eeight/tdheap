#ifndef _INHERITANCE_H_
#define _INHERITANCE_H_

extern "C" {
#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
}

#include "m_stl/tr1/unordered_map"
#include "m_stl/tr1/unordered_set"
#include "m_stl/std/string"

class VTable;

typedef std::tr1::unordered_set<VTable *> VTableSet;

class VTable {
public:
    VTable(Addr start) : start_(start), parent_(0)
    {}

    void addChild(VTable *child);
    const VTableSet &children() { return children_; }

    Addr start() const;

    VTable *parent() const { return parent_; }

    std::string label() const;

private:
    Addr start_;
    // FIXME: Add int function_count_;
    VTableSet children_;
    VTable *parent_;
};

class CallSite {
public:
    CallSite(Addr ip, int function_number) : 
        ip_(ip), function_number_(function_number), parent_(0),
        timesInherited_(0)
    {}

    void mergeWith(const CallSite *site);
    void mergeWithChild(const CallSite *site);

    void addCallee(VTable *vtable);
    /**
     * @return true iff at least one new callee has been optained.
     */
    bool copyCalleesFrom(const CallSite *site);

    Addr ip() const { return ip_; }

    int functionNumber() const { return function_number_; }
    const VTableSet &callees() const { return callees_; }

    CallSite *parent() const { return parent_; }

    void setParent(CallSite *parent);

    int timesInherited() const { return timesInherited_; }

    std::string label() const;

private:
    // Vtables used with this call site.
    VTableSet callees_;
    Addr ip_;
    int function_number_;
    CallSite *parent_;
    int timesInherited_;
};

// Ip->CallSite map
typedef std::tr1::unordered_map<Addr, CallSite *> CallSites;
// vtable->VTable map
typedef std::tr1::unordered_map<Addr, VTable *> VTables;

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

VTable *getVtable(Addr vtable);

void GenerateVtablesLayout();

extern CallSites *g_callSites;
extern VTables *g_vtables;

void InitInheritanceTracker();
void ShutdownInheritanceTracker();

#endif
