#include "typemap.hh"
#include "ast.hh"

TypeMap::TypeMap(TypeMap *p) : Env(p) {}

string TypeMap::keyToString(string* key) {
    return *key;
}

void TypeMap::print() {
    cout << "TypeMap:" << endl;
    for(auto &d : table) {
        cout << "  " << d.first << " : ";
        if (d.second) {
            // Print type expression
            switch (d.second->typeExprType) {
                case TypeExprType::TYPE_CONST: {
                    TypeConst* tc = dynamic_cast<TypeConst*>(d.second);
                    cout << tc->name;
                    break;
                }
                case TypeExprType::MAP_TYPE: {
                    MapType* mt = dynamic_cast<MapType*>(d.second);
                    cout << "map<...>";
                    break;
                }
                case TypeExprType::SET_TYPE: {
                    cout << "set<...>";
                    break;
                }
                case TypeExprType::TUPLE_TYPE: {
                    cout << "tuple<...>";
                    break;
                }
                case TypeExprType::FUNC_TYPE: {
                    cout << "func<...>";
                    break;
                }
                default:
                    cout << "unknown";
            }
        } else {
            cout << "null";
        }
        cout << endl;
    }
}

void TypeMap::setValue(const string& varName, TypeExpr* value) {
    // For value environment, we allow updating existing values (unlike SymbolTable)
    table[varName] = value;
}

TypeExpr* TypeMap::getValue(const string& varName) {
    if (table.find(varName) != table.end()) {
        return table[varName];
    }
    if (parent != nullptr) {
        TypeMap* parentEnv = dynamic_cast<TypeMap*>(parent);
        if (parentEnv) {
            return parentEnv->getValue(varName);
        }
    }
    return nullptr;
}

bool TypeMap::hasValue(const string& varName) {
    if (table.find(varName) != table.end()) {
        return true;
    }
    if (parent != nullptr) {
        TypeMap* parentEnv = dynamic_cast<TypeMap*>(parent);
        if (parentEnv) {
            return parentEnv->hasValue(varName);
        }
    }
    return false;
}
