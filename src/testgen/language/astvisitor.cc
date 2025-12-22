#include "astvisitor.hh"
#include "ast.hh"

// Centralized dispatch for TypeExpr nodes
void ASTVisitor::visit(const TypeExpr* node) {
    if (!node)
        throw std::runtime_error("Null TypeExpr node in visitor");

    if (node->typeExprType == TypeExprType::TYPE_CONST) {
        visitTypeConst(*dynamic_cast<const TypeConst*>(node));
    }
    else if (node->typeExprType == TypeExprType::FUNC_TYPE) {
        visitFuncType(*dynamic_cast<const FuncType*>(node));
    }
    else if (node->typeExprType == TypeExprType::MAP_TYPE) {
        visitMapType(*dynamic_cast<const MapType*>(node));
    }
    else if (node->typeExprType == TypeExprType::TUPLE_TYPE) {
        visitTupleType(*dynamic_cast<const TupleType*>(node));
    }
    else if (node->typeExprType == TypeExprType::SET_TYPE) {
        visitSetType(*dynamic_cast<const SetType*>(node));
    }
    else {
        throw std::runtime_error("Unknown TypeExpr type in visitor");
    }
}

// Centralized dispatch for Expr nodes
void ASTVisitor::visit(const Expr* node) {
    if (!node)
        throw std::runtime_error("Null Expr node in visitor");

    if (node->exprType == ExprType::VAR) {
        visitVar(*dynamic_cast<const Var*>(node));
    }
    else if (node->exprType == ExprType::FUNCCALL) {
        visitFuncCall(*dynamic_cast<const FuncCall*>(node));
    }
    else if (node->exprType == ExprType::NUM) {
        visitNum(*dynamic_cast<const Num*>(node));
    }
    else if (node->exprType == ExprType::STRING) {
        visitString(*dynamic_cast<const String*>(node));
    }
    else if (node->exprType == ExprType::SET) {
        visitSet(*dynamic_cast<const Set*>(node));
    }
    else if (node->exprType == ExprType::MAP) {
        visitMap(*dynamic_cast<const Map*>(node));
    }
    else if (node->exprType == ExprType::TUPLE) {
        visitTuple(*dynamic_cast<const Tuple*>(node));
    }
    else {
        throw std::runtime_error("Unknown Expr type in visitor");
    }
}

// Centralized dispatch for Stmt nodes
void ASTVisitor::visit(const Stmt* node) {
    if (!node)
        throw std::runtime_error("Null Stmt node in visitor");

    if (node->statementType == StmtType::ASSIGN) {
        visitAssign(*dynamic_cast<const Assign*>(node));
    }
    else if (node->statementType == StmtType::ASSUME) {
        visitAssume(*dynamic_cast<const Assume*>(node));
    }
    else {
        throw std::runtime_error("Unknown Stmt type in visitor");
    }
}
