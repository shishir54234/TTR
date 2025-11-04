#ifndef SEE_HH
#define SEE_HH

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ast.hh"

using namespace std;

class SEE {

    private:
        unique_ptr<Expr> computePathConstraint(vector<Expr&>);
        // If the statement is a call to an API function, then none of its parameters
        // should be variables whose values are symbolic expression.
        bool isReady(Stmt&, SymbolTable&);

        // If the expression is a variable and is not mapped to a symbolic expression
        // then the expression is ready. For any other type of expression, if it has
        // a variable in it, e.g. a function call with a variable being passed as an
        // argument, then all such expression should be ready.
        bool isReady(Expr&, SymbolTable&);
        // If an expression has a symbolic variable as one of its subexpressions, then
        // it is symbolic expression.
        bool isSymbolic(Expr&, SymbolTable&);

	void executeStmt(Stmt&, SymbolTable&);
	void evaluateExpr(Expr&, SymbolTable&);
    public:
        void execute();
};
#endif
