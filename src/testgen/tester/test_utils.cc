#include "test_utils.hh"

string TestUtils::exprToString(Expr* expr) {
    if (!expr) return "null";
    
    if (expr->exprType == ExprType::SYMVAR) {
        SymVar* sv = dynamic_cast<SymVar*>(expr);
        return "X" + to_string(sv->getNum());
    }
    else if (expr->exprType == ExprType::NUM) {
        Num* num = dynamic_cast<Num*>(expr);
        return to_string(num->value);
    }
    else if (expr->exprType == ExprType::VAR) {
        Var* var = dynamic_cast<Var*>(expr);
        return var->name;
    }
    else if (expr->exprType == ExprType::FUNCCALL) {
        FuncCall* fc = dynamic_cast<FuncCall*>(expr);
        string result = fc->name + "(";
        for (size_t i = 0; i < fc->args.size(); i++) {
            if (i > 0) result += ", ";
            result += exprToString(fc->args[i].get());
        }
        result += ")";
        return result;
    }
    else if (expr->exprType == ExprType::STRING) {
        String* str = dynamic_cast<String*>(expr);
        return "\"" + str->value + "\"";
    }
    else if (expr->exprType == ExprType::SET) {
        Set* set = dynamic_cast<Set*>(expr);
        string result = "{";
        for (size_t i = 0; i < set->elements.size(); i++) {
            if (i > 0) result += ", ";
            result += exprToString(set->elements[i].get());
        }
        result += "}";
        return result;
    }
    else if (expr->exprType == ExprType::MAP) {
        Map* map = dynamic_cast<Map*>(expr);
        string result = "{";
        for (size_t i = 0; i < map->value.size(); i++) {
            if (i > 0) result += ", ";
            result += exprToString(map->value[i].first.get());
            result += " -> ";
            result += exprToString(map->value[i].second.get());
        }
        result += "}";
        return result;
    }
    else if (expr->exprType == ExprType::TUPLE) {
        Tuple* tuple = dynamic_cast<Tuple*>(expr);
        string result = "(";
        for (size_t i = 0; i < tuple->exprs.size(); i++) {
            if (i > 0) result += ", ";
            result += exprToString(tuple->exprs[i].get());
        }
        result += ")";
        return result;
    }
    
    return "Unknown";
}

unique_ptr<FuncCall> TestUtils::makeBinOp(string op, unique_ptr<Expr> left, unique_ptr<Expr> right) {
    vector<unique_ptr<Expr>> args;
    args.push_back(std::move(left));
    args.push_back(std::move(right));
    return make_unique<FuncCall>(op, std::move(args));
}

unique_ptr<Assign> TestUtils::makeInputAssign(const string& varName) {
    return make_unique<Assign>(
        make_unique<Var>(varName),
        make_unique<FuncCall>("input", vector<unique_ptr<Expr>>{})
    );
}

unique_ptr<Assume> TestUtils::makeAssumeEq(unique_ptr<Expr> left, unique_ptr<Expr> right) {
    return make_unique<Assume>(makeBinOp("Eq", std::move(left), std::move(right)));
}

void TestUtils::printSigma(ValueEnvironment& sigma) {
    cout << "\nSigma (value environment):" << endl;
    for (auto& entry : sigma.getTable()) {
        cout << "  " << entry.first << " -> " << exprToString(entry.second) << endl;
    }
}

void TestUtils::printPathConstraints(vector<Expr*>& pathConstraint) {
    cout << "\nPath constraints:" << endl;
    for (size_t i = 0; i < pathConstraint.size(); i++) {
        cout << "  C[" << i << "] = " << exprToString(pathConstraint[i]) << endl;
    }
}

void TestUtils::executeAndDisplay(SEE& see) {
    ValueEnvironment& sigma = see.getSigma();
    printSigma(sigma);
    
    vector<Expr*>& pathConstraint = see.getPathConstraint();
    printPathConstraints(pathConstraint);
}

bool TestUtils::solveAndDisplay(SEE& see, map<string, int>& modelOut) {
    cout << "\n=== Solving Path Constraints with Z3 ===" << endl;
    unique_ptr<Expr> formula = see.computePathConstraint();
    cout << "Conjoined formula: " << exprToString(formula.get()) << endl;
    
    Z3Solver solver;
    Result result = solver.solve(std::move(formula));
    
    if (result.isSat) {
        cout << "\n✓ SAT - Solution found!" << endl;
        cout << "Model:" << endl;
        for (const auto& entry : result.model) {
            if (entry.second->type == ResultType::INT) {
                const IntResultValue* intVal = dynamic_cast<const IntResultValue*>(entry.second.get());
                cout << "  " << entry.first << " = " << intVal->value << endl;
                modelOut[entry.first] = intVal->value;
            }
        }
    } else {
        cout << "\n✗ UNSAT - No solution exists" << endl;
    }
    
    return result.isSat;
}
