#ifndef TYPEMAP_HH
#define TYPEMAP_HH

#include <map>
#include <memory>
#include <string>
#include "ast.hh"
#include "env.hh"

using namespace std;

/**
 * TypeMap: Maps variable names to their type expressions
 * Used during type checking and ATC generation to track variable types
 */
class TypeMap : public Env<string , TypeExpr>{
    public:
        TypeMap(TypeMap *parent = nullptr);
        virtual void print();
        virtual string keyToString(string *);

        // Value environment methods
        void setValue(const string& varName, TypeExpr* value);
        TypeExpr* getValue(const string& varName);
        bool hasValue(const string& varName);
        map<string, TypeExpr*>& getTable() { return table; }
};

#endif // TYPEMAP_HH
