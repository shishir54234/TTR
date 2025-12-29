#include "tester.hh"
#include "../language/clonevisitor.hh"
#include <iostream>

void Tester::generateTest() {}

// Check if a statement is an Input statement (x := input())
bool isInputStmt(const Stmt& stmt) {
    if(stmt.statementType == StmtType::ASSIGN) {
        const Assign* assign = dynamic_cast<const Assign*>(&stmt);
        if(assign && assign->right->exprType == ExprType::FUNCCALL) {
            const FuncCall* fc = dynamic_cast<const FuncCall*>(assign->right.get());
            if(fc) {
                return (fc->name == "input" && fc->args.size() == 0);
            }
        }
    }
    return false;
}

// Check if the test case has at least one input statement
bool isAbstract(const Program& prog) {
    for(const auto& stmt : prog.statements) {
        if(isInputStmt(*stmt)) {
            return true;
        }
    }
    return false;
}

// Generate Concrete Test Case (genCTC)
// function genCTC(t, L, σ)
//   if ¬isAbstract(t) then return t
//   else
//     t' ← rewriteATC(t, L)
//     L' ← symex(t', σ)
//     return getCTC(t', L', σ)
unique_ptr<Program> Tester::generateCTC(unique_ptr<Program> atc, vector<Expr*> ConcreteVals, ValueEnvironment* ve) {
    cout << "\n========================================" << endl;
    cout << ">>> generateCTC: Starting iteration" << endl;
    cout << "========================================" << endl;
    
    // If not abstract (no input statements), return as-is
    if(!isAbstract(*atc)) {
        cout << ">>> generateCTC: Program is concrete, returning" << endl;
        return atc;
    }
    
    cout << ">>> generateCTC: Program is abstract, needs concretization" << endl;
    cout << ">>> generateCTC: Concrete values provided: " << ConcreteVals.size() << endl;
    
    // Rewrite the abstract test case by replacing Input statements with concrete values
    cout << "\n>>> generateCTC: STEP 1 - Rewriting ATC with concrete values" << endl;
    unique_ptr<Program> rewritten = rewriteATC(atc, ConcreteVals);
    
    // // Check if rewriting made progress (i.e., the rewritten program is now concrete)
    // if(!isAbstract(*rewritten)) {
    //     // All input() calls have been replaced, return the concrete program
    //     return rewritten;
    // }
    
    // Run symbolic execution on the rewritten test case using class member
    cout << "\n>>> generateCTC: STEP 2 - Running symbolic execution" << endl;
    SymbolTable st(nullptr);
    see.execute(*rewritten, st);
    
    // Get the path constraints from symbolic execution and store in class member
    pathConstraints = see.getPathConstraint();
    unique_ptr<Expr> pathConstraint = see.computePathConstraint();
    
    // Solve the path constraints to get new concrete values using class member
    cout << "\n>>> generateCTC: STEP 3 - Solving path constraints with Z3" << endl;
    Result result = solver.solve(std::move(pathConstraint));
    
    // Extract concrete values from the solver result
    vector<Expr*> newConcreteVals;
    if(result.isSat) {
        cout << ">>> generateCTC: SAT - Extracting " << result.model.size() << " concrete values" << endl;
        // Extract values from the model in order (X0, X1, X2, ...)
        for(const auto& entry : result.model) {
            if(entry.second->type == ResultType::INT) {
                const IntResultValue* intVal = dynamic_cast<const IntResultValue*>(entry.second.get());
                cout << "    " << entry.first << " = " << intVal->value << endl;
                newConcreteVals.push_back(new Num(intVal->value));
            }
        }
    } else {
        cout << ">>> generateCTC: UNSAT - No solution found, cannot continue" << endl;
    }
    
    // If we didn't get any new concrete values, we can't make progress
    if(newConcreteVals.empty()) {
        cout << ">>> generateCTC: No new concrete values, returning partially rewritten program" << endl;
        // Return the partially rewritten program
        return rewritten;
    }
    
    // Recursively generate CTC with the new concrete values
    cout << "\n>>> generateCTC: STEP 4 - Recursing with " << newConcreteVals.size() << " new concrete values" << endl;
    return generateCTC(std::move(rewritten), newConcreteVals, ve);
}

// Generate Abstract Test Case from specification
unique_ptr<Program> Tester::generateATC(unique_ptr<Spec> spec, vector<string> ts) {
    vector<unique_ptr<Stmt>> stmts;
    // TODO: Implement ATC generation from spec
    // This would parse the test sequence and generate appropriate statements
    return make_unique<Program>(std::move(stmts));
}

// Rewrite Abstract Test Case (rewriteATC)
// function rewriteATC(t, L)
//   if |t| = 0 ∧ |L| ≠ 0 then raise Error
//   match s₁ with
//   | case Input(x) ⇒
//       s'₁ ← Assign(x, v₁)
//       return s'₁ :: rewriteATC([s₂; ...; sₙ] [v₂; ...; vₘ])
//   | _ ⇒ return s₁ :: rewriteATC([s₂; ...; sₙ] [v₁; ...; vₘ])
unique_ptr<Program> Tester::rewriteATC(unique_ptr<Program>& atc, vector<Expr*> ConcreteVals) {
    // Check error condition: empty test case but concrete values provided
    if(atc->statements.size() == 0 && ConcreteVals.size() != 0) {
        throw runtime_error("Empty test case but concrete values provided");
    }
    
    // Create a new program with rewritten statements
    vector<unique_ptr<Stmt>> newStmts;
    int concreteValIndex = 0;
    CloneVisitor cloner;
    
    for(int i = 0; i < atc->statements.size(); i++) {
        const auto& stmt = atc->statements[i];

        // Check if this is an Input statement (x := input())
        if(stmt->statementType == StmtType::ASSIGN) {
            Assign* assign = dynamic_cast<Assign*>(stmt.get());
            
            if(assign && assign->right->exprType == ExprType::FUNCCALL) {
                FuncCall* fc = dynamic_cast<FuncCall*>(assign->right.get());
                
                // If it's input() and we have concrete values, replace it
                if(fc && fc->name == "input" && fc->args.size() == 0) {
                    if(concreteValIndex < ConcreteVals.size()) {
                        // Create new assignment: x := concreteValue
                        Var* leftVarPtr = dynamic_cast<Var*>(assign->left.get());
                        if (!leftVarPtr) {
                            throw runtime_error("Expected Var on left side of input assignment");
                        }
                        unique_ptr<Var> leftVar = make_unique<Var>(leftVarPtr->name);
                        unique_ptr<Expr> rightExpr = cloner.cloneExpr(ConcreteVals[concreteValIndex]);
                        
                        newStmts.push_back(make_unique<Assign>(move(leftVar), move(rightExpr)));
                        concreteValIndex++;
                    } else {
                        // No concrete value available yet, keep the input() statement
                        newStmts.push_back(cloner.cloneStmt(stmt.get()));
                    }
                    continue;
                }
            }
        }
        
        // For all other statements, clone them as-is
        newStmts.push_back(cloner.cloneStmt(stmt.get()));
    }
    
    return make_unique<Program>(move(newStmts));
}
