#pragma once
#include <map>
#include <string>

#include "ast.hh"

using namespace std;
template<typename T1, typename T2> class Env {
    protected:
        map<T1, T2 *> table;
        Env<T1, T2> *parent;
    
    public:
        Env(Env<T1, T2> *parent);
        virtual Env<T1, T2> *getParent();
        virtual T2& get(T1 *);
        virtual bool hasKey(T1 *);
        virtual void addMapping(T1 *, T2 *);
        virtual string keyToString(T1 *) = 0;
        virtual void print() = 0;
        virtual ~Env();
};

class SymbolTable : public Env<string, TypeExpr> {
    private:
        vector<SymbolTable*> children;  // Child symbol tables (one level deep)
    
    public:
        SymbolTable(SymbolTable *parent);
        virtual void print();
        virtual string keyToString(string *);
        
        // Child management
        void addChild(SymbolTable* child);
        const vector<SymbolTable*>& getChildren() const { return children; }
        SymbolTable* getChild(size_t index) const;
        size_t getChildCount() const { return children.size(); }
        
        virtual ~SymbolTable();
};

// ValueEnvironment: maps variable names (strings) to their symbolic/concrete values (Expr*)
// Used during symbolic execution to track the value of each variable
class ValueEnvironment : public Env<string, Expr> {
    public:
        ValueEnvironment(ValueEnvironment *parent = nullptr);
        virtual void print();
        virtual string keyToString(string *);
        
        // Value environment methods
        void setValue(const string& varName, Expr* value);
        Expr* getValue(const string& varName);
        bool hasValue(const string& varName);
        map<string, Expr*>& getTable() { return table; }
};

// ConcValEnv: maps variable names (strings) to their symbolic/concrete values (Expr*)
// Used during symbolic execution to track the value of each variable
class ConcValEnv : public Env<string, Expr> {
    public:
        ConcValEnv(ConcValEnv *parent = nullptr);
        virtual void print();
        virtual string keyToString(string *);
        
        // Value environment methods
        void setValue(const string& varName, Expr* value);
        Expr* getValue(const string& varName);
        bool hasValue(const string& varName);
        map<string, Expr*>& getTable() { return table; }
};

