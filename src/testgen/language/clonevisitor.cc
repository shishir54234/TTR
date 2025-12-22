#include "clonevisitor.hh"
#include "symvar.hh"

// Helper methods to clone vectors
vector<unique_ptr<TypeExpr>> CloneVisitor::cloneTypeExprVector(const vector<unique_ptr<TypeExpr>>& vec) {
    vector<unique_ptr<TypeExpr>> result;
    for (const auto& elem : vec) {
        result.push_back(cloneTypeExpr(elem.get()));
    }
    return result;
}

vector<unique_ptr<Expr>> CloneVisitor::cloneExprVector(const vector<unique_ptr<Expr>>& vec) {
    vector<unique_ptr<Expr>> result;
    for (const auto& elem : vec) {
        result.push_back(cloneExpr(elem.get()));
    }
    return result;
}

// Main entry points
unique_ptr<TypeExpr> CloneVisitor::cloneTypeExpr(const TypeExpr* node) {
    if (!node) return nullptr;
    
    switch (node->typeExprType) {
        case TypeExprType::TYPE_CONST:
            return cloneTypeConst(dynamic_cast<const TypeConst&>(*node));
        case TypeExprType::FUNC_TYPE:
            return cloneFuncType(dynamic_cast<const FuncType&>(*node));
        case TypeExprType::MAP_TYPE:
            return cloneMapType(dynamic_cast<const MapType&>(*node));
        case TypeExprType::TUPLE_TYPE:
            return cloneTupleType(dynamic_cast<const TupleType&>(*node));
        case TypeExprType::SET_TYPE:
            return cloneSetType(dynamic_cast<const SetType&>(*node));
        default:
            throw runtime_error("Unknown TypeExpr type in cloneTypeExpr");
    }
}

unique_ptr<Expr> CloneVisitor::cloneExpr(const Expr* node) {
    if (!node) return nullptr;
    
    switch (node->exprType) {
        case ExprType::VAR:
            return cloneVar(dynamic_cast<const Var&>(*node));
        case ExprType::FUNCCALL:
            return cloneFuncCall(dynamic_cast<const FuncCall&>(*node));
        case ExprType::NUM:
            return cloneNum(dynamic_cast<const Num&>(*node));
        case ExprType::STRING:
            return cloneString(dynamic_cast<const String&>(*node));
        case ExprType::SET:
            return cloneSet(dynamic_cast<const Set&>(*node));
        case ExprType::MAP:
            return cloneMap(dynamic_cast<const Map&>(*node));
        case ExprType::TUPLE:
            return cloneTuple(dynamic_cast<const Tuple&>(*node));
        case ExprType::SYMVAR:
            return cloneSymVar(dynamic_cast<const SymVar&>(*node));
        case ExprType::INPUT:
            return cloneInput(dynamic_cast<const Input&>(*node));
        default:
            throw runtime_error("Unknown Expr type in cloneExpr");
    }
}

unique_ptr<Stmt> CloneVisitor::cloneStmt(const Stmt* node) {
    if (!node) return nullptr;
    
    switch (node->statementType) {
        case StmtType::ASSIGN:
            return cloneAssign(dynamic_cast<const Assign&>(*node));
        case StmtType::ASSUME:
            return cloneAssume(dynamic_cast<const Assume&>(*node));
        default:
            throw runtime_error("Unknown Stmt type in cloneStmt");
    }
}

// TypeExpr cloners
unique_ptr<TypeExpr> CloneVisitor::cloneTypeConst(const TypeConst &node) {
    return make_unique<TypeConst>(node.name);
}

unique_ptr<TypeExpr> CloneVisitor::cloneFuncType(const FuncType &node) {
    vector<unique_ptr<TypeExpr>> clonedParams = cloneTypeExprVector(node.params);
    unique_ptr<TypeExpr> clonedReturn = cloneTypeExpr(node.returnType.get());
    return make_unique<FuncType>(std::move(clonedParams), std::move(clonedReturn));
}

unique_ptr<TypeExpr> CloneVisitor::cloneMapType(const MapType &node) {
    unique_ptr<TypeExpr> clonedDomain = cloneTypeExpr(node.domain.get());
    unique_ptr<TypeExpr> clonedRange = cloneTypeExpr(node.range.get());
    return make_unique<MapType>(std::move(clonedDomain), std::move(clonedRange));
}

unique_ptr<TypeExpr> CloneVisitor::cloneTupleType(const TupleType &node) {
    vector<unique_ptr<TypeExpr>> clonedElements = cloneTypeExprVector(node.elements);
    return make_unique<TupleType>(std::move(clonedElements));
}

unique_ptr<TypeExpr> CloneVisitor::cloneSetType(const SetType &node) {
    unique_ptr<TypeExpr> clonedElement = cloneTypeExpr(node.elementType.get());
    return make_unique<SetType>(std::move(clonedElement));
}

// Expression cloners
unique_ptr<Expr> CloneVisitor::cloneVar(const Var &node) {
    return make_unique<Var>(node.name);
}

unique_ptr<Expr> CloneVisitor::cloneFuncCall(const FuncCall &node) {
    vector<unique_ptr<Expr>> clonedArgs = cloneExprVector(node.args);
    return make_unique<FuncCall>(node.name, std::move(clonedArgs));
}

unique_ptr<Expr> CloneVisitor::cloneNum(const Num &node) {
    return make_unique<Num>(node.value);
}

unique_ptr<Expr> CloneVisitor::cloneString(const String &node) {
    return make_unique<String>(node.value);
}

unique_ptr<Expr> CloneVisitor::cloneSet(const Set &node) {
    vector<unique_ptr<Expr>> clonedElements = cloneExprVector(node.elements);
    return make_unique<Set>(std::move(clonedElements));
}

unique_ptr<Expr> CloneVisitor::cloneMap(const Map &node) {
    vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> clonedValue;
    
    for (const auto& element : node.value) {
        unique_ptr<Expr> clonedKeyExpr = cloneExpr(element.first.get());
        Var* keyPtr = dynamic_cast<Var*>(clonedKeyExpr.get());
        
        if (!keyPtr) {
            throw runtime_error("Map key is not of type Var");
        }
        
        unique_ptr<Var> clonedKey(static_cast<Var*>(clonedKeyExpr.release()));
        unique_ptr<Expr> clonedVal = cloneExpr(element.second.get());
        
        clonedValue.emplace_back(std::move(clonedKey), std::move(clonedVal));
    }
    
    return make_unique<Map>(std::move(clonedValue));
}

unique_ptr<Expr> CloneVisitor::cloneTuple(const Tuple &node) {
    vector<unique_ptr<Expr>> clonedExprs = cloneExprVector(node.exprs);
    return make_unique<Tuple>(std::move(clonedExprs));
}

unique_ptr<Expr> CloneVisitor::cloneSymVar(const SymVar &node) {
    return make_unique<SymVar>(node.getNum());
}

unique_ptr<Expr> CloneVisitor::cloneInput(const Input &node) {
    return make_unique<Input>();
}

// Statement cloners
unique_ptr<Stmt> CloneVisitor::cloneAssign(const Assign &node) {
    unique_ptr<Expr> clonedLeft = cloneExpr(node.left.get());
    unique_ptr<Expr> clonedRight = cloneExpr(node.right.get());
    
    return make_unique<Assign>(std::move(clonedLeft), std::move(clonedRight));
}

unique_ptr<Stmt> CloneVisitor::cloneAssume(const Assume &node) {
    unique_ptr<Expr> clonedExprVal = cloneExpr(node.expr.get());
    return make_unique<Assume>(std::move(clonedExprVal));
}


