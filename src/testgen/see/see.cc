#include "../language/env.hh" // will change this to normal env.hh later
#include "./see.hh"
#include "../language/clonevisitor.hh"
#include "functionfactory.hh"
#include <iostream>
#include <set>
using namespace std;

// Helper function to print expressions (raw pointer version)
static string exprToString(Expr* expr) {
    if (!expr) return "null";
    
    if (expr->exprType == ExprType::SYMVAR) {
        SymVar* sv = dynamic_cast<SymVar*>(expr);
        return "X" + ::to_string(sv->getNum());
    }
    else if (expr->exprType == ExprType::NUM) {
        Num* num = dynamic_cast<Num*>(expr);
        return ::to_string(num->value);
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

// Helper function to print expressions (unique_ptr version)
static string exprToString(const unique_ptr<Expr>& expr) {
    return exprToString(expr.get());
}

unique_ptr<Expr> SEE::computePathConstraint(vector<Expr*> C) {
    if (C.empty()) {
        // No constraints, return true (represented as 1 == 1)
        vector<unique_ptr<Expr>> args;
        args.push_back(make_unique<Num>(1));
        args.push_back(make_unique<Num>(1));
        return make_unique<FuncCall>("Eq", std::move(args));
    }
    
    CloneVisitor cloner;
    if (C.size() == 1) {
        // Single constraint, return it
        return cloner.cloneExpr(C[0]);
    }
    
    // Multiple constraints, conjoin them with AND
    // For now, we'll create a nested structure: C1 AND (C2 AND (C3 AND ...))
    unique_ptr<Expr> result = cloner.cloneExpr(C.back());
    
    for (int i = C.size() - 2; i >= 0; i--) {
        vector<unique_ptr<Expr>> args;
        args.push_back(cloner.cloneExpr(C[i]));
        args.push_back(std::move(result));
        result = make_unique<FuncCall>("And", std::move(args));
    }
    
    return result;
}

unique_ptr<Expr> SEE::computePathConstraint() {
    return computePathConstraint(pathConstraint);
}

bool SEE::isReady(Stmt& s, SymbolTable& st) {
    if(s.statementType == StmtType::ASSIGN) {
        Assign& assign = dynamic_cast<Assign&>(s);
        
        // Check if this is an API call assignment (e.g., r1 := f(x1))
        if(assign.right->exprType == ExprType::FUNCCALL) {
            FuncCall& fc = dynamic_cast<FuncCall&>(*assign.right);
            
            if(isAPI(fc)) {
                // This is an API call - check if arguments are ready
                // If any argument is symbolic, we need to interrupt and solve first
                for(const auto& arg : fc.args) {
                    if(isSymbolic(*arg, st)) {
                        cout << "[SEE] API call '" << fc.name << "' with symbolic arguments - interruption point" << endl;
                        return false; // Not ready - need to solve constraints first
                    }
                }
                
                // All arguments are concrete, API call is ready for execution
                cout << "[SEE] API call '" << fc.name << "' ready for actual execution" << endl;
                return true;
            } else{
                // Built-in function call, always ready
                return true;

            }
        }
        
        // Not an API call, check if right-hand side is ready
        return isReady(*assign.right, st);
    }
    else if(s.statementType == StmtType::ASSUME) {
        Assume& assume = dynamic_cast<Assume&>(s);
        return isReady(*assume.expr, st);
    }
    else if(s.statementType == StmtType::DECL) {
        // Declaration statements are always ready
        return true;
    }
    else {
        return false;
    }
}

bool SEE::isReady(Expr& e, SymbolTable& st) {
    if(e.exprType == ExprType::FUNCCALL) {
        FuncCall& fc = dynamic_cast<FuncCall&>(e);
        
        // Special case: input() with no arguments IS ready for symbolic execution
        // It will create a new symbolic variable
        if(fc.name == "input" && fc.args.size() == 0) {
            return true;
        }
        
        // Check if this is an API call
        if(isAPI(fc)) {
            // API calls need all arguments to be concrete (not symbolic)
            // Check if any argument is symbolic
            for(unsigned int i = 0; i < fc.args.size(); i++) {
                if(isSymbolic(*fc.args[i], st)) {
                    return false; // Not ready - has symbolic arguments
                }
            }
            return true; // All arguments are concrete
        }
        
        // Built-in functions (Add, Gt, etc.) are always ready
        // They can work with symbolic arguments
        return true;
    }
    if(e.exprType == ExprType::MAP) {
        Map& map = dynamic_cast<Map&>(e);
        for(unsigned int i = 0; i < map.value.size(); i++) {
            if(isReady(*map.value[i].second, st) == false) {
                return false;
            }
        }
        return true;
    }
    if(e.exprType == ExprType::NUM) {
        return true;
    }
    if(e.exprType == ExprType::SET) {
        Set& set = dynamic_cast<Set&>(e);
        for(unsigned int i = 0; i < set.elements.size(); i++) {
            if(isReady(*set.elements[i], st) == false) {
                return false;
            }
        }
        return true;
    }
    if(e.exprType == ExprType::STRING) {
        return true;
    }
    if(e.exprType == ExprType::SYMVAR) {
        // Symbolic variables are ready (they're the result of evaluation)
        return false;
    }
    if(e.exprType == ExprType::TUPLE) {
        Tuple& tuple = dynamic_cast<Tuple&>(e);
        for(unsigned int i = 0; i < tuple.exprs.size(); i++) {
            if(isReady(*tuple.exprs[i], st) == false) {
                return false;
            }
        }
        return true;
    }
    if(e.exprType == ExprType::VAR) {
        // Variables are ready if they're bound in sigma
        Var& var = dynamic_cast<Var&>(e);

        if(sigma.hasValue(var.name) == false) {
            return false;
        }else {
            Expr* val = sigma.getValue(var.name);
            if(isSymbolic(*val, st)) {
                return false;
            }
            else{
                return true;
            }
        }
    }
    else {
        return false;
    }
}

bool SEE::isAPI(const FuncCall& fc) {
    // Built-in functions that are NOT API calls
    static const set<string> builtInFunctions = {
        // Arithmetic
        "Add", "Sub", "Mul", "Div",
        // Comparison
        "Eq", "Lt", "Gt", "Le", "Ge", "Neq",
        "=", "==", "!=", "<>", "<", ">", "<=", ">=",
        // Logical
        "And", "Or", "Not", "Implies",
        "and", "or", "not", "&&", "||", "!",
        // Input
        "input",
        // Set operations
        "in", "not_in", "member", "not_member", "contains", "not_contains",
        "union", "intersection", "intersect", "difference", "diff", "minus",
        "subset", "is_subset", "add_to_set", "remove_from_set", "is_empty_set",
        // Map operations
        "get", "put", "lookup", "select", "store", "update",
        "contains_key", "has_key",
        // List/Sequence operations
        "concat", "append_list", "length", "at", "nth",
        "prefix", "suffix", "contains_seq",
        // Prime notation (for postconditions)
        "'"
    };
    
    return builtInFunctions.find(fc.name) == builtInFunctions.end();
}

bool SEE::isSymbolic(Expr& e, SymbolTable& st) {
    if(e.exprType == ExprType::SYMVAR) {
        return true;
    }
    else if(e.exprType == ExprType::FUNCCALL) {
        FuncCall& fc = dynamic_cast<FuncCall&>(e);
        for(unsigned int i = 0; i < fc.args.size(); i++) {
            if(isSymbolic(*fc.args[i], st) == true) {
                return true;
            }
        }
        return false;
    }
    else if(e.exprType == ExprType::MAP) {
        Map& map = dynamic_cast<Map&>(e);
        for(unsigned int i = 0; i < map.value.size(); i++) {
            if(isSymbolic(*map.value[i].second, st) == true) {
                return true;
            }
        }
        return false;
    }
    else if(e.exprType == ExprType::NUM) {
        return false;
    }
    else if(e.exprType == ExprType::SET) {
        Set& set = dynamic_cast<Set&>(e);
        for(unsigned int i = 0; i < set.elements.size(); i++) {
            if(isSymbolic(*set.elements[i], st) == true) {
                return true;
            }
        }
        return false;
    }
    else if(e.exprType == ExprType::STRING) {
        return false;
    }
    else if(e.exprType == ExprType::TUPLE) {
        Tuple& tuple = dynamic_cast<Tuple&>(e);
        for(unsigned int i = 0; i < tuple.exprs.size(); i++) {
            if(isSymbolic(*tuple.exprs[i], st) == true) {
                return true;
            }
        }
        return false;
    }
    else if(e.exprType == ExprType::VAR) {
        Var& var = dynamic_cast<Var&>(e);
        // Look up the variable in sigma to see if its value is symbolic
        if(sigma.hasValue(var.name)) {
            Expr* val = sigma.getValue(var.name);
            return isSymbolic(*val, st);
        }
        return false;
    }
    else {
        return false;
    }
}

// Symbolic Execution function following the algorithm:
// function symex([s1, s2, ..., sn], σ)
//   C ← []
//   for i = 1 to n do
//     if isReady(si) then
//       symexInstr(si, σ, C)
//     else
//       break
//   pc ← computePathConstraint(C)
//   return solve(pc)
void SEE::execute(Program &pg, SymbolTable& st) {
    // C is represented by pathConstraint (already a member variable)
    // σ is represented by sigma (already a member variable)
    
    // Clear previous state
    pathConstraint.clear();
    
    // Iterate through statements
    for (size_t i = 0; i < pg.statements.size(); i++) {
        const auto& stmt = pg.statements[i];
        
        // Check if statement is ready for execution
        if (isReady(*stmt, st)) {
            // Execute the statement (symexInstr)
            executeStmt(*stmt, st);
        } else {
            // Statement not ready (e.g., contains input() that needs concrete value)
            cout << "[SEE] Statement " << i << " not ready, interrupting execution" << endl;
            break;
        }
    }
    
    // Compute path constraint from collected constraints
    auto pc = computePathConstraint();
    cout << "\n[SEE] Path Constraint: " << exprToString(pc) << endl;
    
    // Note: solve(pc) is called externally by the caller (Tester class)
    return;
}

void SEE::executeStmt(Stmt& stmt, SymbolTable& st) {
    // the various if conditions for different statement types

    if(stmt.statementType == StmtType::ASSIGN) {
        Assign& assign = dynamic_cast<Assign&>(stmt);
        
        // Get the variable name from the left-hand side
        string varName;
        if (assign.left->exprType == ExprType::VAR) {
            Var* leftVar = dynamic_cast<Var*>(assign.left.get());
            varName = leftVar->name;
        } else if (assign.left->exprType == ExprType::TUPLE) {
            // Handle tuple assignment - use first element name or placeholder
            varName = "_tuple_result";
        } else {
            varName = "_unknown";
        }
        
        cout << "\n[ASSIGN] Evaluating: " << varName << " := " << exprToString(assign.right.get()) << endl;
        
        // Check if this is an API call assignment (e.g., r1 := f(x1))
        if(assign.right->exprType == ExprType::FUNCCALL) {
            FuncCall& fc = dynamic_cast<FuncCall&>(*assign.right);
            
            if(isAPI(fc)) {
                // This is an API call - execute it
                cout << "[API_CALL] Executing API function: " << fc.name << endl;
                
                // Evaluate all arguments to get concrete values
                vector<Expr*> concreteArgs;
                for(const auto& arg : fc.args) {
                    Expr* evaluatedArg = evaluateExpr(*arg, st);
                    concreteArgs.push_back(evaluatedArg);
                    cout << "  [API_ARG] " << exprToString(evaluatedArg) << endl;
                }
                
                // Execute the actual API function if factory is available
                if(functionFactory != nullptr) {
                    try {
                        // Get the function implementation from the factory
                        cout << "  [API_CALL] Getting function from factory..." << endl;
                        unique_ptr<Function> function = functionFactory->getFunction(fc.name, concreteArgs);
                        
                        // Execute the function with concrete arguments
                        cout << "  [API_CALL] Executing function..." << endl;
                        unique_ptr<Expr> result = function->execute();
                        cout << "  [API_CALL] Function returned: " << exprToString(result) << endl;
                        
                        // Store the return value in sigma
                        cout << "  [API_CALL] Storing result in variable: " << varName << endl;
                        sigma.setValue(varName, result.release());
                        
                        cout << "[ASSIGN] Result: " << varName << " := " << exprToString(sigma.getValue(varName)) << endl;
                        return;
                    } catch(const char* error) {
                        cout << "  [API_CALL] Error: " << error << endl;
                        throw runtime_error(string("Function execution failed: ") + error);
                    } catch(const exception& e) {
                        cout << "  [API_CALL] Error: " << e.what() << endl;
                        throw;
                    }
                } else {
                    // No function factory available - use placeholder behavior
                    cout << "  [API_CALL] Warning: No FunctionFactory set, using placeholder" << endl;
                    cout << "  [API_CALL] Storing placeholder value in: " << varName << endl;
                    throw runtime_error("FunctionFactory not set in SEE");
                    cout << "[ASSIGN] Result: " << varName << " := 1 (placeholder)" << endl;
                    return;
                }
            } else {
                // Built-in function call (input, Add, etc.) - evaluate symbolically
                Expr* rhsExpr = evaluateExpr(*assign.right, st);
                
                cout << "[ASSIGN] Result: " << varName << " := " << exprToString(rhsExpr) << endl;

                // Store the mapping in sigma (value environment)
                sigma.setValue(varName, rhsExpr);
            }
        } else {
            // Not a function call - evaluate normally
            Expr* rhsExpr = evaluateExpr(*assign.right, st);
            
            cout << "[ASSIGN] Result: " << varName << " := " << exprToString(rhsExpr) << endl;

            // Store the mapping in sigma (value environment)
            sigma.setValue(varName, rhsExpr);
        }
    }     
    else if(stmt.statementType == StmtType::ASSUME) {
        Assume& assume = dynamic_cast<Assume&>(stmt);
        
        cout << "\n[ASSUME] Evaluating: " << exprToString(assume.expr.get()) << endl;
        
        // Add the assumption expression to the path constraint
        Expr* constraint = evaluateExpr(*assume.expr, st);
        
        cout << "[ASSUME] Adding constraint: " << exprToString(constraint) << endl;
        
        pathConstraint.push_back(constraint);
    } else if(stmt.statementType == StmtType::DECL) {
        // taking this as the declaration of a symbolic variable or the input statement
        // we need to get the last symbolic variable and add it to sigma with a new symbolic expression
        Decl& decl = dynamic_cast<Decl&>(stmt);
        string varName = decl.name;
        // we need to get the latest symbolic variable
        
        cout << "\n[DECL] Declaring symbolic variable: " << varName << endl;
        
        SymVar* symVarExpr = SymVar::getNewSymVar().release();
        
        cout << "[DECL] Created: " << varName << " := " << exprToString(symVarExpr) << endl;
        
        sigma.setValue(varName, symVarExpr);
    }

}

Expr* SEE::evaluateExpr(Expr& expr, SymbolTable& st) {
    // Evaluate expressions based on their type
    CloneVisitor cloner;
    
    if(expr.exprType == ExprType::FUNCCALL) {
        FuncCall& fc = dynamic_cast<FuncCall&>(expr);
        
        cout << "  [EVAL] FuncCall: " << fc.name << " with " << fc.args.size() << " args" << endl;
        
        // Special case: "input" function with no arguments returns a new symbolic variable
        if(fc.name == "input" && fc.args.size() == 0) {
            SymVar* symVar = SymVar::getNewSymVar().release();
            cout << "    [EVAL] input() returns new symbolic variable: " << exprToString(symVar) << endl;
            return symVar;
        }
        
        // Evaluate all arguments
        vector<unique_ptr<Expr>> evaluatedArgs;
        for (size_t i = 0; i < fc.args.size(); i++) {
            cout << "    [EVAL] Arg[" << i << "]: " << exprToString(fc.args[i]) << endl;
            Expr* argResult = evaluateExpr(*fc.args[i], st);
            cout << "    [EVAL] Arg[" << i << "] result: " << exprToString(argResult) << endl;
            evaluatedArgs.push_back(cloner.cloneExpr(argResult));
        }
        
        FuncCall* result = new FuncCall(fc.name, ::move(evaluatedArgs));
        cout << "    [EVAL] FuncCall result: " << exprToString(result) << endl;
        
        return result;
    }
    else if(expr.exprType == ExprType::NUM) {
        Num* result = new Num(dynamic_cast<Num&>(expr).value);
        cout << "  [EVAL] Num: " << exprToString(result) << endl;
        return result;
    }
    else if(expr.exprType == ExprType::STRING) {
        String* result = new String(dynamic_cast<String&>(expr).value);
        cout << "  [EVAL] String: " << exprToString(result) << endl;
        return result;
    }
    else if(expr.exprType == ExprType::SYMVAR) {
        // Return the symbolic variable as-is
        cout << "  [EVAL] SymVar: " << exprToString(&expr) << endl;
        return &expr;
    }
    else if(expr.exprType == ExprType::VAR) {
        // Look up variable in sigma
        Var& v = dynamic_cast<Var&>(expr);
        cout << "  [EVAL] Var lookup: " << v.name << endl;
        if(sigma.hasValue(v.name)) {
            Expr* value = sigma.getValue(v.name);
            cout << "    [EVAL] Found in sigma: " << exprToString(value) << endl;
            return value;
        }
        cout << "    [EVAL] Not found in sigma, returning as-is" << endl;
        return &expr;
    }
    else if(expr.exprType == ExprType::SET) {
        // Evaluate each element in the set
        Set& set = dynamic_cast<Set&>(expr);
        cout << "  [EVAL] Set with " << set.elements.size() << " elements" << endl;
        
        vector<unique_ptr<Expr>> evaluatedElements;
        for (size_t i = 0; i < set.elements.size(); i++) {
            Expr* elemResult = evaluateExpr(*set.elements[i], st);
            evaluatedElements.push_back(cloner.cloneExpr(elemResult));
        }
        
        Set* result = new Set(::move(evaluatedElements));
        cout << "    [EVAL] Set result: " << exprToString(result) << endl;
        return result;
    }
    else if(expr.exprType == ExprType::MAP) {
        // Evaluate each key-value pair in the map
        Map& map = dynamic_cast<Map&>(expr);
        cout << "  [EVAL] Map with " << map.value.size() << " entries" << endl;
        
        vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> evaluatedPairs;
        for (size_t i = 0; i < map.value.size(); i++) {
            // Clone the key (Var)
            unique_ptr<Var> keyClone = make_unique<Var>(map.value[i].first->name);
            // Evaluate the value
            Expr* valResult = evaluateExpr(*map.value[i].second, st);
            evaluatedPairs.push_back(make_pair(::move(keyClone), cloner.cloneExpr(valResult)));
        }
        
        Map* result = new Map(::move(evaluatedPairs));
        cout << "    [EVAL] Map result: " << exprToString(result) << endl;
        return result;
    }
    else if(expr.exprType == ExprType::TUPLE) {
        // Evaluate each element in the tuple
        Tuple& tuple = dynamic_cast<Tuple&>(expr);
        cout << "  [EVAL] Tuple with " << tuple.exprs.size() << " elements" << endl;
        
        vector<unique_ptr<Expr>> evaluatedExprs;
        for (size_t i = 0; i < tuple.exprs.size(); i++) {
            Expr* elemResult = evaluateExpr(*tuple.exprs[i], st);
            evaluatedExprs.push_back(cloner.cloneExpr(elemResult));
        }
        
        Tuple* result = new Tuple(::move(evaluatedExprs));
        cout << "    [EVAL] Tuple result: " << exprToString(result) << endl;
        return result;
    }
    
    // Default case: return the expression as-is
    cout << "  [EVAL] Unknown type, returning as-is" << endl;
    return &expr;
}
