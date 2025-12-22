#ifndef Z3SOLVER_HH
#define Z3SOLVER_HH

#include<memory>
#include <stack>
#include<string>

#include "solver.hh"
#include "z3++.h"
#include "../language/astvisitor.hh"
#include "../language/typemap.hh"

using namespace std;

class Z3InputMaker : public ASTVisitor {
    private:
        z3::context ctx;
        stack<z3::expr> theStack;
        vector<z3::expr> variables;
        map<unsigned int, z3::expr*> symVarMap; // Map SymVar numbers to Z3 variables
        map<string, z3::expr*> namedVarMap;     // Map named variables to Z3 expressions
        TypeMap* typeMap;                        // Type information for variables
        
        // Z3 sorts for custom types
        z3::sort getStringSort();
        z3::sort getIntSort();
        z3::sort getSetSort(z3::sort elementSort);
        z3::sort getMapSort(z3::sort keySort, z3::sort valueSort);
        z3::sort getListSort(z3::sort elementSort);
        
        // Helper to get Z3 sort from TypeExpr
        z3::sort typeExprToSort(TypeExpr* type);
        
        // Helper to create empty set/map
        z3::expr makeEmptySet(z3::sort elementSort);
        z3::expr makeEmptyMap(z3::sort keySort, z3::sort valueSort);

    public:
        Z3InputMaker(TypeMap* typeMap = nullptr);
        ~Z3InputMaker();
        z3::expr makeZ3Input(unique_ptr<Expr>& expr);
        z3::expr makeZ3Input(Expr* expr);
	    vector<z3::expr> getVariables();
        z3::context& getContext() { return ctx; }

    protected:
        // Expression visitor methods (protected, called by base class)
        void visitVar(const Var &node) override;
        void visitFuncCall(const FuncCall &node) override;
        void visitNum(const Num &node) override;
        void visitString(const String &node) override;
        void visitSet(const Set &node) override;
        void visitMap(const Map &node) override;
        void visitTuple(const Tuple &node) override;

        // Type expression visitor methods
        void visitTypeConst(const TypeConst &node) override;
        void visitFuncType(const FuncType &node) override;
        void visitMapType(const MapType &node) override;
        void visitTupleType(const TupleType &node) override;
        void visitSetType(const SetType &node) override;

        // Statement visitor methods
        void visitAssign(const Assign &node) override;
        void visitAssume(const Assume &node) override;


        // Declaration/High-level visitor methods
        z3::expr convertArg(const unique_ptr<Expr>& arg);

    public:
        // High-level visitor methods
        void visitDecl(const Decl &node) override;
        void visitAPIcall(const APIcall &node) override;
        void visitAPI(const API &node) override;
        void visitResponse(const Response &node) override;
        void visitInit(const Init &node) override;
        void visitSpec(const Spec &node) override;
        void visitProgram(const Program &node) override;
};

class Z3Solver : public Solver {
    private:
        TypeMap* typeMap;
    public:
        Z3Solver(TypeMap* typeMap = nullptr);
        Result solve(unique_ptr<Expr>) const;
};
#endif
