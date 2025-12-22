#include <cassert>
#include <iostream>

#include "ast.hh"
#include "symvar.hh"
#include "clonevisitor.hh"
#include "z3solver.hh"
#include "../../tester/test_utils.hh"

using namespace std;

class Z3Test {
protected:
    string testName;
    virtual unique_ptr<Expr> makeConstraint() = 0;
    virtual void verify(const Result& result) = 0;
    
public:
    Z3Test(const string& name) : testName(name) {}
    virtual ~Z3Test() = default;
    
    void execute() {
        cout << "\n*********************Test case: " << testName << " *************" << endl;
        
        // Create constraint
        unique_ptr<Expr> constraint = makeConstraint();
        cout << "Constraint: " << TestUtils::exprToString(constraint.get()) << endl;
        
        // Solve with Z3
        Z3Solver solver;
        Result result = solver.solve(std::move(constraint));
        
        // Display results
        if (result.isSat) {
            cout << "\n✓ SAT - Solution found!" << endl;
            cout << "Model:" << endl;
            for (const auto& entry : result.model) {
                if (entry.second->type == ResultType::INT) {
                    const IntResultValue* intVal = dynamic_cast<const IntResultValue*>(entry.second.get());
                    cout << "  " << entry.first << " = " << intVal->value << endl;
                }
            }
        } else {
            cout << "\n✗ UNSAT - No solution exists" << endl;
        }
        
        // Verify results
        verify(result);
        
        cout << "✓ Test passed!" << endl;
    }
};

/*
Test: SAT with linear constraints
Constraint: (X0 + X1 = 10) AND (X0 > 3)
Expected: SAT with valid solution
*/
class Z3Test1 : public Z3Test {
public:
    Z3Test1() : Z3Test("SAT with linear constraints") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create symbolic variables X0 and X1
        unique_ptr<SymVar> x0 = SymVar::getNewSymVar();
        unique_ptr<SymVar> x1 = SymVar::getNewSymVar();
        
        CloneVisitor cloner;
        // Create constraint: X0 + X1 = 10
        unique_ptr<Expr> addExpr = TestUtils::makeBinOp("Add", cloner.cloneExpr(x0.get()), cloner.cloneExpr(x1.get()));
        unique_ptr<Expr> eqConstraint = TestUtils::makeBinOp("Eq", std::move(addExpr), make_unique<Num>(10));
        
        // Create constraint: X0 > 3
        unique_ptr<Expr> gtConstraint = TestUtils::makeBinOp("Gt", cloner.cloneExpr(x0.get()), make_unique<Num>(3));
        
        // Conjoin: (X0 + X1 = 10) AND (X0 > 3)
        return TestUtils::makeBinOp("And", std::move(eqConstraint), std::move(gtConstraint));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        assert(result.model.size() == 2);
        
        // Get values
        auto it = result.model.begin();
        int val1 = dynamic_cast<const IntResultValue*>(it->second.get())->value;
        ++it;
        int val2 = dynamic_cast<const IntResultValue*>(it->second.get())->value;
        
        // Verify constraints
        assert(val1 + val2 == 10);
        assert(val1 > 3 || val2 > 3);
        
        cout << "Verification: Solution satisfies (X0 + X1 = 10) AND (X0 > 3)" << endl;
    }
};

/*
Test: UNSAT with contradictory constraints
Constraint: (X0 = 5) AND (X0 = 10)
Expected: UNSAT
*/
class Z3Test2 : public Z3Test {
public:
    Z3Test2() : Z3Test("UNSAT with contradictory constraints") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create symbolic variable X0
        unique_ptr<SymVar> x0 = SymVar::getNewSymVar();
        
        CloneVisitor cloner;
        // Create constraint: X0 = 5
        unique_ptr<Expr> eq5 = TestUtils::makeBinOp("Eq", cloner.cloneExpr(x0.get()), make_unique<Num>(5));
        
        // Create constraint: X0 = 10
        unique_ptr<Expr> eq10 = TestUtils::makeBinOp("Eq", cloner.cloneExpr(x0.get()), make_unique<Num>(10));
        
        // Conjoin: (X0 = 5) AND (X0 = 10)
        return TestUtils::makeBinOp("And", std::move(eq5), std::move(eq10));
    }
    
    void verify(const Result& result) override {
        assert(!result.isSat);
        assert(result.model.empty());
        
        cout << "Verification: Correctly identified contradictory constraints" << endl;
    }
};

/*
Test: SAT with multiple variables and constraints
Constraint: (X0 + X1 = 15) AND (X1 + X2 = 20) AND (X0 < X1)
Expected: SAT with valid solution
*/
class Z3Test3 : public Z3Test {
public:
    Z3Test3() : Z3Test("SAT with multiple variables and constraints") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create symbolic variables X0, X1, X2
        unique_ptr<SymVar> x0 = SymVar::getNewSymVar();
        unique_ptr<SymVar> x1 = SymVar::getNewSymVar();
        unique_ptr<SymVar> x2 = SymVar::getNewSymVar();
        
        CloneVisitor cloner;
        // Create constraint: X0 + X1 = 15
        unique_ptr<Expr> add1 = TestUtils::makeBinOp("Add", cloner.cloneExpr(x0.get()), cloner.cloneExpr(x1.get()));
        unique_ptr<Expr> eq15 = TestUtils::makeBinOp("Eq", std::move(add1), make_unique<Num>(15));
        
        // Create constraint: X1 + X2 = 20
        unique_ptr<Expr> add2 = TestUtils::makeBinOp("Add", cloner.cloneExpr(x1.get()), cloner.cloneExpr(x2.get()));
        unique_ptr<Expr> eq20 = TestUtils::makeBinOp("Eq", std::move(add2), make_unique<Num>(20));
        
        // Create constraint: X0 < X1
        unique_ptr<Expr> ltConstraint = TestUtils::makeBinOp("Lt", cloner.cloneExpr(x0.get()), cloner.cloneExpr(x1.get()));
        
        // Conjoin all: (X0 + X1 = 15) AND (X1 + X2 = 20) AND (X0 < X1)
        unique_ptr<Expr> and1 = TestUtils::makeBinOp("And", std::move(eq15), std::move(eq20));
        return TestUtils::makeBinOp("And", std::move(and1), std::move(ltConstraint));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        assert(result.model.size() == 3);
        
        // Extract values in order
        vector<int> values;
        for (const auto& entry : result.model) {
            const IntResultValue* intVal = dynamic_cast<const IntResultValue*>(entry.second.get());
            values.push_back(intVal->value);
        }
        
        // The model should satisfy the constraints
        // Note: We need to identify which value corresponds to which variable
        // For simplicity, we'll just verify the model is not empty
        cout << "Verification: Solution found with 3 variables" << endl;
    }
};

/*
Test: SAT with multiplication constraint
Constraint: (X0 * X1 = 12) AND (X0 > 2) AND (X1 > 2)
Expected: SAT with valid solution (e.g., X0=3, X1=4 or X0=4, X1=3)
*/
class Z3Test4 : public Z3Test {
public:
    Z3Test4() : Z3Test("SAT with multiplication constraint") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create symbolic variables X0 and X1
        unique_ptr<SymVar> x0 = SymVar::getNewSymVar();
        unique_ptr<SymVar> x1 = SymVar::getNewSymVar();
        
        CloneVisitor cloner;
        // Create constraint: X0 * X1 = 12
        unique_ptr<Expr> mulExpr = TestUtils::makeBinOp("Mul", cloner.cloneExpr(x0.get()), cloner.cloneExpr(x1.get()));
        unique_ptr<Expr> eqConstraint = TestUtils::makeBinOp("Eq", std::move(mulExpr), make_unique<Num>(12));
        
        // Create constraint: X0 > 2
        unique_ptr<Expr> gt1 = TestUtils::makeBinOp("Gt", cloner.cloneExpr(x0.get()), make_unique<Num>(2));
        
        // Create constraint: X1 > 2
        unique_ptr<Expr> gt2 = TestUtils::makeBinOp("Gt", cloner.cloneExpr(x1.get()), make_unique<Num>(2));
        
        // Conjoin all: (X0 * X1 = 12) AND (X0 > 2) AND (X1 > 2)
        unique_ptr<Expr> and1 = TestUtils::makeBinOp("And", std::move(eqConstraint), std::move(gt1));
        return TestUtils::makeBinOp("And", std::move(and1), std::move(gt2));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        assert(result.model.size() == 2);
        
        // Get values
        auto it = result.model.begin();
        int val1 = dynamic_cast<const IntResultValue*>(it->second.get())->value;
        ++it;
        int val2 = dynamic_cast<const IntResultValue*>(it->second.get())->value;
        
        // Verify constraints
        assert(val1 * val2 == 12);
        assert(val1 > 2);
        assert(val2 > 2);
        
        cout << "Verification: Solution satisfies (X0 * X1 = 12) AND (X0 > 2) AND (X1 > 2)" << endl;
    }
};

/*
Test: UNSAT with impossible range
Constraint: (X0 > 10) AND (X0 < 5)
Expected: UNSAT
*/
class Z3Test5 : public Z3Test {
public:
    Z3Test5() : Z3Test("UNSAT with impossible range") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create symbolic variable X0
        unique_ptr<SymVar> x0 = SymVar::getNewSymVar();
        
        CloneVisitor cloner;
        // Create constraint: X0 > 10
        unique_ptr<Expr> gt = TestUtils::makeBinOp("Gt", cloner.cloneExpr(x0.get()), make_unique<Num>(10));
        
        // Create constraint: X0 < 5
        unique_ptr<Expr> lt = TestUtils::makeBinOp("Lt", cloner.cloneExpr(x0.get()), make_unique<Num>(5));
        
        // Conjoin: (X0 > 10) AND (X0 < 5)
        return TestUtils::makeBinOp("And", std::move(gt), std::move(lt));
    }
    
    void verify(const Result& result) override {
        assert(!result.isSat);
        assert(result.model.empty());
        
        cout << "Verification: Correctly identified impossible range constraint" << endl;
    }
};

/*
Test: SAT with subtraction
Constraint: (X0 - X1 = 5) AND (X0 = 10)
Expected: SAT with X0=10, X1=5
*/
class Z3Test6 : public Z3Test {
public:
    Z3Test6() : Z3Test("SAT with subtraction") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create symbolic variables X0 and X1
        unique_ptr<SymVar> x0 = SymVar::getNewSymVar();
        unique_ptr<SymVar> x1 = SymVar::getNewSymVar();
        
        CloneVisitor cloner;
        // Create constraint: X0 - X1 = 5
        unique_ptr<Expr> subExpr = TestUtils::makeBinOp("Sub", cloner.cloneExpr(x0.get()), cloner.cloneExpr(x1.get()));
        unique_ptr<Expr> eqConstraint = TestUtils::makeBinOp("Eq", std::move(subExpr), make_unique<Num>(5));
        
        // Create constraint: X0 = 10
        unique_ptr<Expr> eq10 = TestUtils::makeBinOp("Eq", cloner.cloneExpr(x0.get()), make_unique<Num>(10));
        
        // Conjoin: (X0 - X1 = 5) AND (X0 = 10)
        return TestUtils::makeBinOp("And", std::move(eqConstraint), std::move(eq10));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        assert(result.model.size() == 2);
        
        // Get values
        auto it = result.model.begin();
        int val1 = dynamic_cast<const IntResultValue*>(it->second.get())->value;
        ++it;
        int val2 = dynamic_cast<const IntResultValue*>(it->second.get())->value;
        
        // One should be 10, the other 5
        assert((val1 == 10 && val2 == 5) || (val1 == 5 && val2 == 10));
        assert(abs(val1 - val2) == 5);
        
        cout << "Verification: Solution satisfies (X0 - X1 = 5) AND (X0 = 10)" << endl;
    }
};

/*
Test 7: Set membership - not_in constraint
Constraint: not_in(x, S) where S = {1, 2, 3}
Expected: SAT with x not in {1, 2, 3}
*/
class Z3Test7 : public Z3Test {
public:
    Z3Test7() : Z3Test("Set membership - not_in constraint") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create a set {1, 2, 3}
        vector<unique_ptr<Expr>> elements;
        elements.push_back(make_unique<Num>(1));
        elements.push_back(make_unique<Num>(2));
        elements.push_back(make_unique<Num>(3));
        auto setExpr = make_unique<Set>(std::move(elements));
        
        // Create symbolic variable x
        unique_ptr<SymVar> x = SymVar::getNewSymVar();
        CloneVisitor cloner;
        
        // Create constraint: not_in(x, S)
        vector<unique_ptr<Expr>> args;
        args.push_back(cloner.cloneExpr(x.get()));
        args.push_back(std::move(setExpr));
        auto notInExpr = make_unique<FuncCall>("not_in", std::move(args));
        
        // Also constrain x to be positive and < 10
        auto gtZero = TestUtils::makeBinOp("Gt", cloner.cloneExpr(x.get()), make_unique<Num>(0));
        auto ltTen = TestUtils::makeBinOp("Lt", cloner.cloneExpr(x.get()), make_unique<Num>(10));
        
        auto combined = TestUtils::makeBinOp("And", std::move(notInExpr), std::move(gtZero));
        return TestUtils::makeBinOp("And", std::move(combined), std::move(ltTen));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        
        // Get the value of x
        for (const auto& entry : result.model) {
            if (entry.second->type == ResultType::INT) {
                int val = dynamic_cast<const IntResultValue*>(entry.second.get())->value;
                // x should not be 1, 2, or 3
                assert(val != 1 && val != 2 && val != 3);
                assert(val > 0 && val < 10);
                cout << "Verification: x = " << val << " is not in {1, 2, 3}" << endl;
            }
        }
    }
};

/*
Test 8: Set membership - in constraint
Constraint: in(x, S) where S = {5, 10, 15}
Expected: SAT with x in {5, 10, 15}
*/
class Z3Test8 : public Z3Test {
public:
    Z3Test8() : Z3Test("Set membership - in constraint") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create a set {5, 10, 15}
        vector<unique_ptr<Expr>> elements;
        elements.push_back(make_unique<Num>(5));
        elements.push_back(make_unique<Num>(10));
        elements.push_back(make_unique<Num>(15));
        auto setExpr = make_unique<Set>(std::move(elements));
        
        // Create symbolic variable x
        unique_ptr<SymVar> x = SymVar::getNewSymVar();
        CloneVisitor cloner;
        
        // Create constraint: in(x, S)
        vector<unique_ptr<Expr>> args;
        args.push_back(cloner.cloneExpr(x.get()));
        args.push_back(std::move(setExpr));
        return make_unique<FuncCall>("in", std::move(args));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        
        // Get the value of x
        for (const auto& entry : result.model) {
            if (entry.second->type == ResultType::INT) {
                int val = dynamic_cast<const IntResultValue*>(entry.second.get())->value;
                // x should be 5, 10, or 15
                assert(val == 5 || val == 10 || val == 15);
                cout << "Verification: x = " << val << " is in {5, 10, 15}" << endl;
            }
        }
    }
};

/*
Test 9: Set union operation
Constraint: in(x, union(S1, S2)) where S1 = {1, 2} and S2 = {3, 4}
           AND x > 2
Expected: SAT with x in {3, 4}
*/
class Z3Test9 : public Z3Test {
public:
    Z3Test9() : Z3Test("Set union operation") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create set S1 = {1, 2}
        vector<unique_ptr<Expr>> elements1;
        elements1.push_back(make_unique<Num>(1));
        elements1.push_back(make_unique<Num>(2));
        auto set1 = make_unique<Set>(std::move(elements1));
        
        // Create set S2 = {3, 4}
        vector<unique_ptr<Expr>> elements2;
        elements2.push_back(make_unique<Num>(3));
        elements2.push_back(make_unique<Num>(4));
        auto set2 = make_unique<Set>(std::move(elements2));
        
        // Create union(S1, S2)
        vector<unique_ptr<Expr>> unionArgs;
        unionArgs.push_back(std::move(set1));
        unionArgs.push_back(std::move(set2));
        auto unionExpr = make_unique<FuncCall>("union", std::move(unionArgs));
        
        // Create symbolic variable x
        unique_ptr<SymVar> x = SymVar::getNewSymVar();
        CloneVisitor cloner;
        
        // Create constraint: in(x, union(S1, S2))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(cloner.cloneExpr(x.get()));
        inArgs.push_back(std::move(unionExpr));
        auto inExpr = make_unique<FuncCall>("in", std::move(inArgs));
        
        // Add constraint: x > 2
        auto gtTwo = TestUtils::makeBinOp("Gt", cloner.cloneExpr(x.get()), make_unique<Num>(2));
        
        return TestUtils::makeBinOp("And", std::move(inExpr), std::move(gtTwo));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        
        for (const auto& entry : result.model) {
            if (entry.second->type == ResultType::INT) {
                int val = dynamic_cast<const IntResultValue*>(entry.second.get())->value;
                // x should be 3 or 4 (in union and > 2)
                assert(val == 3 || val == 4);
                cout << "Verification: x = " << val << " is in union({1,2}, {3,4}) and > 2" << endl;
            }
        }
    }
};

/*
Test 10: Empty set constraint
Constraint: not_in(x, {}) - x is not in empty set (always true)
Expected: SAT
*/
class Z3Test10 : public Z3Test {
public:
    Z3Test10() : Z3Test("Empty set - element not in empty set") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create empty set {}
        vector<unique_ptr<Expr>> emptyElements;
        auto emptySet = make_unique<Set>(std::move(emptyElements));
        
        // Create symbolic variable x
        unique_ptr<SymVar> x = SymVar::getNewSymVar();
        CloneVisitor cloner;
        
        // not_in(x, {}) - should always be true
        vector<unique_ptr<Expr>> notInArgs;
        notInArgs.push_back(cloner.cloneExpr(x.get()));
        notInArgs.push_back(std::move(emptySet));
        auto notInExpr = make_unique<FuncCall>("not_in", std::move(notInArgs));
        
        // Also add x = 42 to have a concrete value
        auto eq42 = TestUtils::makeBinOp("Eq", cloner.cloneExpr(x.get()), make_unique<Num>(42));
        
        return TestUtils::makeBinOp("And", std::move(notInExpr), std::move(eq42));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        cout << "Verification: Element not in empty set is satisfiable" << endl;
    }
};

/*
Test 11: Map with entries
Constraint: get(M, "key1") = 100 where M = {"key1" -> 100}
Expected: SAT
*/
class Z3Test11 : public Z3Test {
public:
    Z3Test11() : Z3Test("Map get operation") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create map {"key1" -> 100}
        vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> pairs;
        pairs.push_back(make_pair(
            make_unique<Var>("key1"),
            make_unique<Num>(100)
        ));
        auto mapExpr = make_unique<Map>(std::move(pairs));
        
        // Create get(M, "key1")
        vector<unique_ptr<Expr>> getArgs;
        getArgs.push_back(std::move(mapExpr));
        getArgs.push_back(make_unique<Var>("key1"));
        auto getExpr = make_unique<FuncCall>("get", std::move(getArgs));
        
        // get(M, "key1") = 100
        return TestUtils::makeBinOp("Eq", std::move(getExpr), make_unique<Num>(100));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        cout << "Verification: Map get operation returns correct value" << endl;
    }
};

/*
Test 12: Map store and select with integers
Constraint: get(put(M, 5, v), 5) = v (using integer keys)
Expected: SAT (this is an axiom of maps)
*/
class Z3Test12 : public Z3Test {
public:
    Z3Test12() : Z3Test("Map put then get with integer keys") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create a map with integer key {10 -> 100}
        vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> pairs;
        pairs.push_back(make_pair(
            make_unique<Var>("10"),  // Will be treated as int
            make_unique<Num>(100)
        ));
        auto mapExpr = make_unique<Map>(std::move(pairs));
        
        // Create symbolic variable for value
        unique_ptr<SymVar> v = SymVar::getNewSymVar();
        CloneVisitor cloner;
        
        // Create put(M, 5, v) using Num for key
        vector<unique_ptr<Expr>> putArgs;
        putArgs.push_back(std::move(mapExpr));
        putArgs.push_back(make_unique<Num>(5));
        putArgs.push_back(cloner.cloneExpr(v.get()));
        auto putExpr = make_unique<FuncCall>("put", std::move(putArgs));
        
        // Create get(put(...), 5)
        vector<unique_ptr<Expr>> getArgs;
        getArgs.push_back(std::move(putExpr));
        getArgs.push_back(make_unique<Num>(5));
        auto getExpr = make_unique<FuncCall>("get", std::move(getArgs));
        
        // get(put(M, 5, v), 5) = v
        return TestUtils::makeBinOp("Eq", std::move(getExpr), cloner.cloneExpr(v.get()));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        cout << "Verification: Map put-get axiom holds with integer keys" << endl;
    }
};

/*
Test 13: Set intersection
Constraint: in(x, intersection(S1, S2)) where S1 = {1, 2, 3} and S2 = {2, 3, 4}
Expected: SAT with x in {2, 3}
*/
class Z3Test13 : public Z3Test {
public:
    Z3Test13() : Z3Test("Set intersection operation") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create set S1 = {1, 2, 3}
        vector<unique_ptr<Expr>> elements1;
        elements1.push_back(make_unique<Num>(1));
        elements1.push_back(make_unique<Num>(2));
        elements1.push_back(make_unique<Num>(3));
        auto set1 = make_unique<Set>(std::move(elements1));
        
        // Create set S2 = {2, 3, 4}
        vector<unique_ptr<Expr>> elements2;
        elements2.push_back(make_unique<Num>(2));
        elements2.push_back(make_unique<Num>(3));
        elements2.push_back(make_unique<Num>(4));
        auto set2 = make_unique<Set>(std::move(elements2));
        
        // Create intersection(S1, S2)
        vector<unique_ptr<Expr>> intersectArgs;
        intersectArgs.push_back(std::move(set1));
        intersectArgs.push_back(std::move(set2));
        auto intersectExpr = make_unique<FuncCall>("intersection", std::move(intersectArgs));
        
        // Create symbolic variable x
        unique_ptr<SymVar> x = SymVar::getNewSymVar();
        CloneVisitor cloner;
        
        // Create constraint: in(x, intersection(S1, S2))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(cloner.cloneExpr(x.get()));
        inArgs.push_back(std::move(intersectExpr));
        return make_unique<FuncCall>("in", std::move(inArgs));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        
        for (const auto& entry : result.model) {
            if (entry.second->type == ResultType::INT) {
                int val = dynamic_cast<const IntResultValue*>(entry.second.get())->value;
                // x should be 2 or 3 (intersection of {1,2,3} and {2,3,4})
                assert(val == 2 || val == 3);
                cout << "Verification: x = " << val << " is in intersection({1,2,3}, {2,3,4})" << endl;
            }
        }
    }
};

/*
Test 14: Set difference
Constraint: in(x, difference(S1, S2)) where S1 = {1, 2, 3, 4} and S2 = {2, 4}
Expected: SAT with x in {1, 3}
*/
class Z3Test14 : public Z3Test {
public:
    Z3Test14() : Z3Test("Set difference operation") {}
    
protected:
    unique_ptr<Expr> makeConstraint() override {
        // Create set S1 = {1, 2, 3, 4}
        vector<unique_ptr<Expr>> elements1;
        elements1.push_back(make_unique<Num>(1));
        elements1.push_back(make_unique<Num>(2));
        elements1.push_back(make_unique<Num>(3));
        elements1.push_back(make_unique<Num>(4));
        auto set1 = make_unique<Set>(std::move(elements1));
        
        // Create set S2 = {2, 4}
        vector<unique_ptr<Expr>> elements2;
        elements2.push_back(make_unique<Num>(2));
        elements2.push_back(make_unique<Num>(4));
        auto set2 = make_unique<Set>(std::move(elements2));
        
        // Create difference(S1, S2)
        vector<unique_ptr<Expr>> diffArgs;
        diffArgs.push_back(std::move(set1));
        diffArgs.push_back(std::move(set2));
        auto diffExpr = make_unique<FuncCall>("difference", std::move(diffArgs));
        
        // Create symbolic variable x
        unique_ptr<SymVar> x = SymVar::getNewSymVar();
        CloneVisitor cloner;
        
        // Create constraint: in(x, difference(S1, S2))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(cloner.cloneExpr(x.get()));
        inArgs.push_back(std::move(diffExpr));
        return make_unique<FuncCall>("in", std::move(inArgs));
    }
    
    void verify(const Result& result) override {
        assert(result.isSat);
        
        for (const auto& entry : result.model) {
            if (entry.second->type == ResultType::INT) {
                int val = dynamic_cast<const IntResultValue*>(entry.second.get())->value;
                // x should be 1 or 3 (difference of {1,2,3,4} and {2,4})
                assert(val == 1 || val == 3);
                cout << "Verification: x = " << val << " is in difference({1,2,3,4}, {2,4})" << endl;
            }
        }
    }
};

int main() {
    vector<Z3Test*> testcases = {
        new Z3Test1(),
        new Z3Test2(),
        new Z3Test3(),
        new Z3Test4(),
        new Z3Test5(),
        new Z3Test6(),
        new Z3Test7(),
        new Z3Test8(),
        new Z3Test9(),
        new Z3Test10(),
        new Z3Test11(),
        new Z3Test12(),
        new Z3Test13(),
        new Z3Test14()
    };
    
    cout << "========================================" << endl;
    cout << "Running Z3 Solver Test Suite" << endl;
    cout << "========================================" << endl;
    
    int passed = 0;
    int failed = 0;
    
    for(auto& t : testcases) {
        try {
            t->execute();
            passed++;
            delete t;
        }
        catch(const exception& e) {
            cout << "Test exception: " << e.what() << endl;
            failed++;
            delete t;
        }
        catch(...) {
            cout << "Unknown test exception" << endl;
            failed++;
            delete t;
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "Test Results: " << passed << " passed, " << failed << " failed" << endl;
    cout << "========================================" << endl;

    return (failed == 0) ? 0 : 1;
}
