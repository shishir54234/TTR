#ifndef CLONEVISITOR_HH
#define CLONEVISITOR_HH

#include "ast.hh"
#include <memory>
#include <vector>
#include <utility>

using namespace std;

// CloneVisitor creates deep copies of AST nodes
// This is a standalone utility class that doesn't inherit from ASTVisitor
class CloneVisitor
{
public:
    // Main entry points for cloning - these return the cloned objects directly
    unique_ptr<TypeExpr> cloneTypeExpr(const TypeExpr* node);
    unique_ptr<Expr> cloneExpr(const Expr* node);
    unique_ptr<Stmt> cloneStmt(const Stmt* node);

private:
    // Type Expression cloners
    unique_ptr<TypeExpr> cloneTypeConst(const TypeConst &node);
    unique_ptr<TypeExpr> cloneFuncType(const FuncType &node);
    unique_ptr<TypeExpr> cloneMapType(const MapType &node);
    unique_ptr<TypeExpr> cloneTupleType(const TupleType &node);
    unique_ptr<TypeExpr> cloneSetType(const SetType &node);

    // Expression cloners
    unique_ptr<Expr> cloneVar(const Var &node);
    unique_ptr<Expr> cloneFuncCall(const FuncCall &node);
    unique_ptr<Expr> cloneNum(const Num &node);
    unique_ptr<Expr> cloneString(const String &node);
    unique_ptr<Expr> cloneSet(const Set &node);
    unique_ptr<Expr> cloneMap(const Map &node);
    unique_ptr<Expr> cloneTuple(const Tuple &node);
    unique_ptr<Expr> cloneSymVar(const class SymVar &node);
    unique_ptr<Expr> cloneInput(const class Input &node);

    // Statement cloners
    unique_ptr<Stmt> cloneAssign(const Assign &node);
    unique_ptr<Stmt> cloneAssume(const Assume &node);
    unique_ptr<Stmt> cloneAssert(const Assert &node);

    // Helper to clone vectors
    vector<unique_ptr<TypeExpr>> cloneTypeExprVector(const vector<unique_ptr<TypeExpr>>& vec);
    vector<unique_ptr<Expr>> cloneExprVector(const vector<unique_ptr<Expr>>& vec);
};

#endif // CLONEVISITOR_HH
