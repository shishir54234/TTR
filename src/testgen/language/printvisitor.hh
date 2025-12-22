#ifndef PRINTVISITOR_HH
#define PRINTVISITOR_HH

#include <iostream>
#include <string>
#include "ast.hh"
#include "astvisitor.hh"

using namespace std;

/**
 * PrintVisitor: Prints AST nodes in a readable format
 * Useful for debugging and visualizing generated ATCs
 */
class PrintVisitor : public ASTVisitor {
private:
    int indentLevel;
    string indentStr;
    
    void indent() {
        indentLevel++;
        updateIndent();
    }
    
    void dedent() {
        if (indentLevel > 0) indentLevel--;
        updateIndent();
    }
    
    void updateIndent() {
        indentStr = string(indentLevel * 2, ' ');
    }
    
    void printIndent() {
        cout << indentStr;
    }

protected:
    // Type Expression visitors
    void visitTypeConst(const TypeConst &node) override;
    void visitFuncType(const FuncType &node) override;
    void visitMapType(const MapType &node) override;
    void visitTupleType(const TupleType &node) override;
    void visitSetType(const SetType &node) override;

    // Expression visitors
    void visitVar(const Var &node) override;
    void visitFuncCall(const FuncCall &node) override;
    void visitNum(const Num &node) override;
    void visitString(const String &node) override;
    void visitSet(const Set &node) override;
    void visitMap(const Map &node) override;
    void visitTuple(const Tuple &node) override;

    // Statement visitors
    void visitAssign(const Assign &node) override;
    void visitAssume(const Assume &node) override;

public:
    PrintVisitor();
    
    // High-level visitors
    void visitDecl(const Decl &node) override;
    void visitAPIcall(const APIcall &node) override;
    void visitAPI(const API &node) override;
    void visitResponse(const Response &node) override;
    void visitInit(const Init &node) override;
    void visitSpec(const Spec &node) override;
    void visitProgram(const Program &node) override;
    
    // Convenience methods for direct printing
    void printExpr(const Expr* expr);
    void printStmt(const Stmt* stmt);
    void printTypeExpr(const TypeExpr* type);
};

#endif // PRINTVISITOR_HH
