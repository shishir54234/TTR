#include "env.hh"

template <typename T1, typename T2> Env<T1, T2>::Env(Env<T1, T2> *p) : parent(p) {}

template <typename T1, typename T2> Env<T1, T2> *Env<T1, T2>::getParent() {
    return parent;
}

template <typename T1, typename T2> void Env<T1, T2>::print() {
}

template <typename T1, typename T2> T2& Env<T1, T2>::get(T1 *key) {

    if(hasKey(key)) {
        return *(table[*key]);
    }
    if(parent != NULL) {
        return parent->get(key);
    }
    
    char str[50];
    sprintf(str, "%llx", (long long int)key);
    throw ("Key " + keyToString(key) + " (" + str + ") not found.");
}

template <typename T1, typename T2> bool Env<T1, T2>::hasKey(T1 *key) {
    return table.find(*key) != table.end();
}

template <typename T1, typename T2> void Env<T1, T2>::addMapping(T1 *key, T2 *value) {

    if(table.find(*key) == table.end()) {
        table[*key] = value;
    }
    else {
        string m = "Env::addMapping : repeat declaration for name " + keyToString(key) + ".";
        throw m;
    }
}

template <typename T1, typename T2> Env<T1, T2>::~Env() {}

SymbolTable::SymbolTable(SymbolTable *p = nullptr) : Env(p) {}

SymbolTable::~SymbolTable() {
    // Don't delete children - they're managed by the caller
    // Just clear the vector
    children.clear();
}

void SymbolTable::addChild(SymbolTable* child) {
    children.push_back(child);
}

SymbolTable* SymbolTable::getChild(size_t index) const {
    if (index < children.size()) {
        return children[index];
    }
    return nullptr;
}

string SymbolTable::keyToString(string* key) {
    return *key;
}

void SymbolTable::print() {
    for(auto &d : table) {
        cout << d.first << " (" << d.first << ")" << endl;
    }
    if (!children.empty()) {
        cout << "  Children: " << children.size() << " symbol tables" << endl;
    }
}

// ValueEnvironment implementation
ValueEnvironment::ValueEnvironment(ValueEnvironment *p) : Env(p) {}

string ValueEnvironment::keyToString(string* key) {
    return *key;
}

void ValueEnvironment::print() {
    cout << "Value Environment:" << endl;
    for(auto &d : table) {
        cout << "  " << d.first << " -> ";
        if (d.second) {
            // Print expression type or value
            if (d.second->exprType == ExprType::NUM) {
                Num* num = dynamic_cast<Num*>(d.second);
                cout << num->value;
            } else if (d.second->exprType == ExprType::SYMVAR) {
                cout << "SymVar";
            } else if (d.second->exprType == ExprType::FUNCCALL) {
                FuncCall* fc = dynamic_cast<FuncCall*>(d.second);
                cout << fc->name << "(...)";
            } else {
                cout << "Expr";
            }
        } else {
            cout << "null";
        }
        cout << endl;
    }
}

void ValueEnvironment::setValue(const string& varName, Expr* value) {
    // For value environment, we allow updating existing values (unlike SymbolTable)
    table[varName] = value;
}

Expr* ValueEnvironment::getValue(const string& varName) {
    if (table.find(varName) != table.end()) {
        return table[varName];
    }
    if (parent != nullptr) {
        ValueEnvironment* parentEnv = dynamic_cast<ValueEnvironment*>(parent);
        if (parentEnv) {
            return parentEnv->getValue(varName);
        }
    }
    return nullptr;
}

bool ValueEnvironment::hasValue(const string& varName) {
    if (table.find(varName) != table.end()) {
        return true;
    }
    if (parent != nullptr) {
        ValueEnvironment* parentEnv = dynamic_cast<ValueEnvironment*>(parent);
        if (parentEnv) {
            return parentEnv->hasValue(varName);
        }
    }
    return false;
}

// ConcValEnv implementation
ConcValEnv::ConcValEnv(ConcValEnv *p) : Env(p) {}

string ConcValEnv::keyToString(string* key) {
    return *key;
}

void ConcValEnv::print() {
    cout << "Value Environment:" << endl;
    for(auto &d : table) {
        cout << "  " << d.first << " -> ";
        if (d.second) {
            // Print expression type or value
            if (d.second->exprType == ExprType::NUM) {
                Num* num = dynamic_cast<Num*>(d.second);
                cout << num->value;
            } else if (d.second->exprType == ExprType::SYMVAR) {
                cout << "SymVar";
            } else if (d.second->exprType == ExprType::FUNCCALL) {
                FuncCall* fc = dynamic_cast<FuncCall*>(d.second);
                cout << fc->name << "(...)";
            } else {
                cout << "Expr";
            }
        } else {
            cout << "null";
        }
        cout << endl;
    }
}

void ConcValEnv::setValue(const string& varName, Expr* value) {
    // For value environment, we allow updating existing values (unlike SymbolTable)
    table[varName] = value;
}

Expr* ConcValEnv::getValue(const string& varName) {
    if (table.find(varName) != table.end()) {
        return table[varName];
    }
    if (parent != nullptr) {
        ConcValEnv* parentEnv = dynamic_cast<ConcValEnv*>(parent);
        if (parentEnv) {
            return parentEnv->getValue(varName);
        }
    }
    return nullptr;
}

bool ConcValEnv::hasValue(const string& varName) {
    if (table.find(varName) != table.end()) {
        return true;
    }
    if (parent != nullptr) {
        ConcValEnv* parentEnv = dynamic_cast<ConcValEnv*>(parent);
        if (parentEnv) {
            return parentEnv->hasValue(varName);
        }
    }
    return false;
}

// Explicit template instantiation for TypeMap
template class Env<string, TypeExpr>;
