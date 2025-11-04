#include "env.hh"

template <typename T1, typename T2> Env<T1, T2>::Env(Env<T1, T2> *p) : parent(p) {}

template <typename T1, typename T2> Env<T1, T2> *Env<T1, T2>::getParent() {
    return parent;
}

template <typename T1, typename T2> void Env<T1, T2>::print() {
}

template <typename T1, typename T2> T2& Env<T1, T2>::get(T1 *key) {

    if(hasKey(key)) {
        return *(table[key]);
    }
    if(parent != NULL) {
        return parent->get(key);
    }
    
    char str[50];
    sprintf(str, "%llx", (long long int)key);
    throw ("Key " + keyToString(key) + " (" + str + ") not found.");
}

template <typename T1, typename T2> bool Env<T1, T2>::hasKey(T1 *key) {
    return table.find(key) != table.end();
}

template <typename T1, typename T2> void Env<T1, T2>::addMapping(T1 *key, T2 *value) {

        if(table.find(key) == table.end()) {
        table[key] = value;
    }
    else {
        string m = "Env::addMapping : repeat declaration for name " + keyToString(key) + ".";
        throw m;
    }
}

template <typename T1, typename T2> Env<T1, T2>::~Env() {}

SymbolTable::SymbolTable(SymbolTable *p = nullptr) : Env(p) {}

string SymbolTable::keyToString(string* key) {
    return *key;
}

void SymbolTable::print() {
    for(auto &d : table) {
        
        cout << keyToString(d.first) << " (" << d.first << "): " << d.second->toString() << endl;
    }
}
