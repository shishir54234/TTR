#include <iostream>
#include <cassert>
#include "ast.hh"
#include "env.hh"
#include "symvar.hh"
#include "see.hh"
#include "z3solver.hh"
#include "../../tester/test_utils.hh"
#include "../../apps/app1/app1.hh"
using namespace std;

class SEETest {
protected:
    string testName;
    virtual Program makeProgram() = 0;
    virtual void verify(SEE& see, map<string, int>& model, bool isSat) = 0;
    
public:
    SEETest(const string& name) : testName(name) {}
    virtual ~SEETest() = default;
    
    void execute() {
        cout << "\n*********************Test case: " << testName << " *************" << endl;
        
        Program program = makeProgram();
        SymbolTable st(nullptr);
        FunctionFactory* functionFactory = new App1FunctionFactory();
        
        SEE see(move(functionFactory));
        
        // Execute the program
        see.execute(program, st);
        
        // Display results
        TestUtils::executeAndDisplay(see);
        
        // Solve with Z3
        map<string, int> model;
        bool isSat = TestUtils::solveAndDisplay(see, model);
        
        // Verify results
        verify(see, model, isSat);
        
        cout << "✓ Test passed!" << endl;
    }
};

/*
Test 1: Basic symbolic execution with UNSAT constraints
Program:
    x := input
    y := input  
    z := x+y
    assume(x*y=3)
    z := z+2
    assume(x=5)
Expected: UNSAT (no integer solution for x*y=3 and x=5)
*/
class SEETest1 : public SEETest {
public:
    SEETest1() : SEETest("Basic symbolic execution with UNSAT constraints") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(TestUtils::makeInputAssign("y"));
        
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("z"),
            TestUtils::makeBinOp("Add", make_unique<Var>("x"), make_unique<Var>("y"))
        ));
        
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Eq", 
                TestUtils::makeBinOp("Mul", make_unique<Var>("x"), make_unique<Var>("y")),
                make_unique<Num>(3)
            )
        ));
        
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("z"),
            TestUtils::makeBinOp("Add", make_unique<Var>("z"), make_unique<Num>(2))
        ));
        
        statements.push_back(TestUtils::makeAssumeEq(make_unique<Var>("x"), make_unique<Num>(5)));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        ValueEnvironment& sigma = see.getSigma();
        
        assert(sigma.hasValue("x"));
        assert(sigma.hasValue("y"));
        assert(sigma.hasValue("z"));
        
        assert(sigma.getValue("x")->exprType == ExprType::SYMVAR);
        assert(sigma.getValue("y")->exprType == ExprType::SYMVAR);
        assert(sigma.getValue("z")->exprType == ExprType::FUNCCALL);
        
        vector<Expr*>& pathConstraint = see.getPathConstraint();
        assert(pathConstraint.size() == 2);
        
        assert(!isSat);
        assert(model.empty());
    }
};

/*
Test 2: Simple SAT constraint
Program:
    x := input
    assume(x > 5)
Expected: SAT with x > 5
*/
class SEETest2 : public SEETest {
public:
    SEETest2() : SEETest("Simple SAT constraint") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(5))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        ValueEnvironment& sigma = see.getSigma();
        assert(sigma.hasValue("x"));
        
        vector<Expr*>& pathConstraint = see.getPathConstraint();
        assert(pathConstraint.size() == 1);
        
        assert(isSat);
        assert(model.size() == 1);
        
        int x_val = model.begin()->second;
        assert(x_val > 5);
    }
};

/*
Test 3: Multiple variables with linear constraints
Program:
    x := input
    y := input
    assume(x + y = 10)
    assume(x > 3)
Expected: SAT with valid solution
*/
class SEETest3 : public SEETest {
public:
    SEETest3() : SEETest("Multiple variables with linear constraints") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(TestUtils::makeInputAssign("y"));
        
        statements.push_back(TestUtils::makeAssumeEq(
            TestUtils::makeBinOp("Add", make_unique<Var>("x"), make_unique<Var>("y")),
            make_unique<Num>(10)
        ));
        
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(3))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        ValueEnvironment& sigma = see.getSigma();
        assert(sigma.hasValue("x"));
        assert(sigma.hasValue("y"));
        
        vector<Expr*>& pathConstraint = see.getPathConstraint();
        assert(pathConstraint.size() == 2);
        
        assert(isSat);
        assert(model.size() == 2);
        
        auto it = model.begin();
        int val1 = it->second;
        ++it;
        int val2 = it->second;
        
        assert(val1 + val2 == 10);
        assert(val1 > 3 || val2 > 3);
    }
};

/*
Test 4: Set membership with not_in constraint
Program:
    u := input
    U := {1, 2, 3}
    assume(not_in(u, U))
    assume(u > 0)
    assume(u < 10)
Expected: SAT with u not in {1, 2, 3} and 0 < u < 10
*/
class SEETest4 : public SEETest {
public:
    SEETest4() : SEETest("Set membership - not_in constraint") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // u := input
        statements.push_back(TestUtils::makeInputAssign("u"));
        
        // U := {1, 2, 3}
        vector<unique_ptr<Expr>> elements;
        elements.push_back(make_unique<Num>(1));
        elements.push_back(make_unique<Num>(2));
        elements.push_back(make_unique<Num>(3));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("U"),
            make_unique<Set>(std::move(elements))
        ));
        
        // assume(not_in(u, U))
        vector<unique_ptr<Expr>> notInArgs;
        notInArgs.push_back(make_unique<Var>("u"));
        notInArgs.push_back(make_unique<Var>("U"));
        statements.push_back(make_unique<Assume>(
            make_unique<FuncCall>("not_in", std::move(notInArgs))
        ));
        
        // assume(u > 0)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("u"), make_unique<Num>(0))
        ));
        
        // assume(u < 10)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Lt", make_unique<Var>("u"), make_unique<Num>(10))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        ValueEnvironment& sigma = see.getSigma();
        assert(sigma.hasValue("u"));
        assert(sigma.hasValue("U"));
        
        assert(sigma.getValue("U")->exprType == ExprType::SET);
        
        vector<Expr*>& pathConstraint = see.getPathConstraint();
        assert(pathConstraint.size() == 3);
        
        assert(isSat);
        
        for (const auto& entry : model) {
            int val = entry.second;
            assert(val != 1 && val != 2 && val != 3);
            assert(val > 0 && val < 10);
            cout << "  u = " << val << " (not in {1,2,3})" << endl;
        }
    }
};

/*
Test 5: Set membership with in constraint
Program:
    x := input
    S := {10, 20, 30}
    assume(in(x, S))
Expected: SAT with x in {10, 20, 30}
*/
class SEETest5 : public SEETest {
public:
    SEETest5() : SEETest("Set membership - in constraint") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // x := input
        statements.push_back(TestUtils::makeInputAssign("x"));
        
        // S := {10, 20, 30}
        vector<unique_ptr<Expr>> elements;
        elements.push_back(make_unique<Num>(10));
        elements.push_back(make_unique<Num>(20));
        elements.push_back(make_unique<Num>(30));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S"),
            make_unique<Set>(std::move(elements))
        ));
        
        // assume(in(x, S))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(make_unique<Var>("x"));
        inArgs.push_back(make_unique<Var>("S"));
        statements.push_back(make_unique<Assume>(
            make_unique<FuncCall>("in", std::move(inArgs))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        ValueEnvironment& sigma = see.getSigma();
        assert(sigma.hasValue("x"));
        assert(sigma.hasValue("S"));
        
        assert(isSat);
        
        for (const auto& entry : model) {
            int val = entry.second;
            assert(val == 10 || val == 20 || val == 30);
            cout << "  x = " << val << " (in {10,20,30})" << endl;
        }
    }
};

/*
Test 6: Set union operation
Program:
    x := input
    S1 := {1, 2}
    S2 := {3, 4}
    S3 := union(S1, S2)
    assume(in(x, S3))
    assume(x > 2)
Expected: SAT with x in {3, 4}
*/
class SEETest6 : public SEETest {
public:
    SEETest6() : SEETest("Set union operation") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // x := input
        statements.push_back(TestUtils::makeInputAssign("x"));
        
        // S1 := {1, 2}
        vector<unique_ptr<Expr>> elements1;
        elements1.push_back(make_unique<Num>(1));
        elements1.push_back(make_unique<Num>(2));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S1"),
            make_unique<Set>(std::move(elements1))
        ));
        
        // S2 := {3, 4}
        vector<unique_ptr<Expr>> elements2;
        elements2.push_back(make_unique<Num>(3));
        elements2.push_back(make_unique<Num>(4));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S2"),
            make_unique<Set>(std::move(elements2))
        ));
        
        // S3 := union(S1, S2)
        vector<unique_ptr<Expr>> unionArgs;
        unionArgs.push_back(make_unique<Var>("S1"));
        unionArgs.push_back(make_unique<Var>("S2"));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S3"),
            make_unique<FuncCall>("union", std::move(unionArgs))
        ));
        
        // assume(in(x, S3))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(make_unique<Var>("x"));
        inArgs.push_back(make_unique<Var>("S3"));
        statements.push_back(make_unique<Assume>(
            make_unique<FuncCall>("in", std::move(inArgs))
        ));
        
        // assume(x > 2)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(2))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        ValueEnvironment& sigma = see.getSigma();
        assert(sigma.hasValue("x"));
        assert(sigma.hasValue("S1"));
        assert(sigma.hasValue("S2"));
        assert(sigma.hasValue("S3"));
        
        assert(isSat);
        
        for (const auto& entry : model) {
            int val = entry.second;
            assert(val == 3 || val == 4);
            cout << "  x = " << val << " (in union({1,2}, {3,4}) and > 2)" << endl;
        }
    }
};

/*
Test 7: Set intersection operation
Program:
    x := input
    S1 := {1, 2, 3}
    S2 := {2, 3, 4}
    S3 := intersection(S1, S2)
    assume(in(x, S3))
Expected: SAT with x in {2, 3}
*/
class SEETest7 : public SEETest {
public:
    SEETest7() : SEETest("Set intersection operation") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // x := input
        statements.push_back(TestUtils::makeInputAssign("x"));
        
        // S1 := {1, 2, 3}
        vector<unique_ptr<Expr>> elements1;
        elements1.push_back(make_unique<Num>(1));
        elements1.push_back(make_unique<Num>(2));
        elements1.push_back(make_unique<Num>(3));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S1"),
            make_unique<Set>(std::move(elements1))
        ));
        
        // S2 := {2, 3, 4}
        vector<unique_ptr<Expr>> elements2;
        elements2.push_back(make_unique<Num>(2));
        elements2.push_back(make_unique<Num>(3));
        elements2.push_back(make_unique<Num>(4));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S2"),
            make_unique<Set>(std::move(elements2))
        ));
        
        // S3 := intersection(S1, S2)
        vector<unique_ptr<Expr>> intersectArgs;
        intersectArgs.push_back(make_unique<Var>("S1"));
        intersectArgs.push_back(make_unique<Var>("S2"));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S3"),
            make_unique<FuncCall>("intersection", std::move(intersectArgs))
        ));
        
        // assume(in(x, S3))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(make_unique<Var>("x"));
        inArgs.push_back(make_unique<Var>("S3"));
        statements.push_back(make_unique<Assume>(
            make_unique<FuncCall>("in", std::move(inArgs))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        assert(isSat);
        
        for (const auto& entry : model) {
            int val = entry.second;
            assert(val == 2 || val == 3);
            cout << "  x = " << val << " (in intersection({1,2,3}, {2,3,4}))" << endl;
        }
    }
};

/*
Test 8: Set difference operation
Program:
    x := input
    S1 := {1, 2, 3, 4}
    S2 := {2, 4}
    S3 := difference(S1, S2)
    assume(in(x, S3))
Expected: SAT with x in {1, 3}
*/
class SEETest8 : public SEETest {
public:
    SEETest8() : SEETest("Set difference operation") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // x := input
        statements.push_back(TestUtils::makeInputAssign("x"));
        
        // S1 := {1, 2, 3, 4}
        vector<unique_ptr<Expr>> elements1;
        elements1.push_back(make_unique<Num>(1));
        elements1.push_back(make_unique<Num>(2));
        elements1.push_back(make_unique<Num>(3));
        elements1.push_back(make_unique<Num>(4));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S1"),
            make_unique<Set>(std::move(elements1))
        ));
        
        // S2 := {2, 4}
        vector<unique_ptr<Expr>> elements2;
        elements2.push_back(make_unique<Num>(2));
        elements2.push_back(make_unique<Num>(4));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S2"),
            make_unique<Set>(std::move(elements2))
        ));
        
        // S3 := difference(S1, S2)
        vector<unique_ptr<Expr>> diffArgs;
        diffArgs.push_back(make_unique<Var>("S1"));
        diffArgs.push_back(make_unique<Var>("S2"));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S3"),
            make_unique<FuncCall>("difference", std::move(diffArgs))
        ));
        
        // assume(in(x, S3))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(make_unique<Var>("x"));
        inArgs.push_back(make_unique<Var>("S3"));
        statements.push_back(make_unique<Assume>(
            make_unique<FuncCall>("in", std::move(inArgs))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        assert(isSat);
        
        for (const auto& entry : model) {
            int val = entry.second;
            assert(val == 1 || val == 3);
            cout << "  x = " << val << " (in difference({1,2,3,4}, {2,4}))" << endl;
        }
    }
};

/*
Test 9: Tuple with concrete values
Program:
    t := (10, 20, 30)
    x := input
    assume(x > 5)
    assume(x < 15)
Expected: SAT with valid solution
*/
class SEETest9 : public SEETest {
public:
    SEETest9() : SEETest("Tuple with concrete values") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // t := (10, 20, 30)
        vector<unique_ptr<Expr>> tupleExprs;
        tupleExprs.push_back(make_unique<Num>(10));
        tupleExprs.push_back(make_unique<Num>(20));
        tupleExprs.push_back(make_unique<Num>(30));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("t"),
            make_unique<Tuple>(std::move(tupleExprs))
        ));
        
        // x := input
        statements.push_back(TestUtils::makeInputAssign("x"));
        
        // assume(x > 5)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(5))
        ));
        
        // assume(x < 15)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Lt", make_unique<Var>("x"), make_unique<Num>(15))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        ValueEnvironment& sigma = see.getSigma();
        assert(sigma.hasValue("t"));
        assert(sigma.hasValue("x"));
        
        assert(sigma.getValue("t")->exprType == ExprType::TUPLE);
        
        assert(isSat);
        
        for (const auto& entry : model) {
            int val = entry.second;
            assert(val > 5 && val < 15);
            cout << "  x = " << val << " (5 < x < 15)" << endl;
        }
    }
};

/*
Test 10: UNSAT with set membership contradiction
Program:
    x := input
    S := {1, 2, 3}
    assume(in(x, S))
    assume(x > 10)
Expected: UNSAT (x must be in {1,2,3} but also > 10)
*/
class SEETest10 : public SEETest {
public:
    SEETest10() : SEETest("UNSAT with set membership contradiction") {}
    
protected:
    Program makeProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // x := input
        statements.push_back(TestUtils::makeInputAssign("x"));
        
        // S := {1, 2, 3}
        vector<unique_ptr<Expr>> elements;
        elements.push_back(make_unique<Num>(1));
        elements.push_back(make_unique<Num>(2));
        elements.push_back(make_unique<Num>(3));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("S"),
            make_unique<Set>(std::move(elements))
        ));
        
        // assume(in(x, S))
        vector<unique_ptr<Expr>> inArgs;
        inArgs.push_back(make_unique<Var>("x"));
        inArgs.push_back(make_unique<Var>("S"));
        statements.push_back(make_unique<Assume>(
            make_unique<FuncCall>("in", std::move(inArgs))
        ));
        
        // assume(x > 10)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(10))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(SEE& see, map<string, int>& model, bool isSat) override {
        assert(!isSat);
        assert(model.empty());
        cout << "  Correctly identified UNSAT: x in {1,2,3} AND x > 10" << endl;
    }
};

int main() {
    vector<SEETest*> testcases = {
        new SEETest1(),
        new SEETest2(),
        new SEETest3(),
        new SEETest4(),
        new SEETest5(),
        new SEETest6(),
        new SEETest7(),
        new SEETest8(),
        new SEETest9(),
        new SEETest10()
    };
    
    cout << "========================================" << endl;
    cout << "Running SEE Test Suite" << endl;
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
    cout << "SEE Test Results: " << passed << " passed, " << failed << " failed" << endl;
    cout << "========================================" << endl;
    
    return (failed == 0) ? 0 : 1;
}
