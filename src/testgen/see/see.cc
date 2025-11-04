#include "env.hh"
#include "see.hh"
#include "functionfactory.hh"

unique_ptr<Expr> SEE::computePathConstraint(vector<Expr&> C) {
    return make_unique<Num>(0);
}

bool SEE::isReady(Stmt& s, SymbolTable& st) {

    if(s.statementType == StmtType::ASSIGN) {
        Assign& assign = dynamic_cast<Assign>(s);
        return isReady(assign.right, st);
    }
    else if(s.statementType == StmtType::FUNCCALL) {
        FuncCallStmt& fcstmt = dynamic_cast<FuncCallStmt>(s);
        return isReady(fcstmt.call, st);
    }
    else {
    return false;
    }
}

bool SEE::isReady(Expr& e, SymbolTable& st) {
    if(e.exprType == ExprType::FUNCCALL) {
        FuncCall& fcstmt = dynamic_cast<FuncCallStmt>(s);
        for(unsigned int i = 0; i < fcstmt.args.size(); i++) {
            if(isReady(fcstmt.args[i], st) == false) {
                return false;
            }
        }
        return true;
    }
    if(e.exprType == ExprType::MAP) {
        Map& fcstmt = dynamic_cast<FuncCallStmt>(s);
    }
    if(e.exprType == ExprType::NUM) {
        return true;
    }
    if(e.exprType == ExprType::SET) {
        Set& set = dynamic_cast<Set>(e);
        for(unsigned int i = 0; i < set.elements.size(); i++) {
            if(isReady(set.elements[i], st) == false) {
                return false;
            }
        }
        return true;
    }
    if(e.exprType == ExprType::STRING) {
        return true;
    }
    if(e.exprType == ExprType::TUPLE) {
        Tuple> tuple = dynamic_cast<Tuple>(e);
        for(unsigned int i = 0; i < tuple.exprs.size(); i++) {
            if(isReady(tuple.exprs[i], st) == false) {
                return false;
            }
        }
        return true;
    }
    if(e.exprType == ExprType::VAR) {
        Expr& val = st.get(e);
	return !isSymbolic(val);
        if(val.exprType == ExprType::SYMVAR) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
}

bool SEE::isSymbolic(Expr> e) {
     if(e.exprType == ExprType::FUNCCALL) {
        FuncCall> fcstmt = dynamic_cast<FuncCallStmt>(s);
        for(unsigned int i = 0; i < fcstmt.args.size(); i++) {
            if(isSymbolic(fcstmt.args[i]) == true) {
                return true;
            }
        }
        return false;
    }
    if(e.exprType == ExprType::MAP) {
        Map& fcstmt = dynamic_cast<FuncCallStmt>(s);
    }
    if(e.exprType == ExprType::NUM) {
        return false;
    }
    if(e.exprType == ExprType::SET) {
        Set& set = dynamic_cast<Set>(e);
        for(unsigned int i = 0; i < set.elements.size(); i++) {
            if(isSymbolic(set.elements[i], st) == true) {
                return true;
            }
        }
        return false;
    }
    if(e.exprType == ExprType::STRING) {
        return false;
    }
    if(e.exprType == ExprType::TUPLE) {
        Tuple& tuple = dynamic_cast<Tuple>(e);
        for(unsigned int i = 0; i < tuple.exprs.size(); i++) {
            if(isSymbolic(tuple.exprs[i], st) == true) {
                return true;
            }
        }
        return false;
    }
    else {
        return true;
    }
}

void SEE::execute() {
	SymbolTable st;
}

void SEE::executeStmt(Stmt& stmt, SymbolTable& st) {

}

void SEE::evaluateExpr(Expr&, SymbolTable& st) {

}
