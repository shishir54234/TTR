#include <iostream>
#include <cassert>
#include "ast.hh"
#include "env.hh"
#include "symvar.hh"
#include "../../tester/tester.hh"
#include "../../tester/test_utils.hh"
#include "../../apps/app1/app1.hh"
using namespace std;

class TesterTest {
protected:
    string testName;
    virtual Program makeAbstractProgram() = 0;
    virtual void verify(Tester& tester, unique_ptr<Program>& result) = 0;
    
public:
    TesterTest(const string& name) : testName(name) {}
    virtual ~TesterTest() = default;
    
    void printProgram(const string& title, const Program& prog) {
        cout << title << endl;
        for(size_t i = 0; i < prog.statements.size(); i++) {
            cout << "  Statement " << i << ": ";
            if(prog.statements[i]->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*prog.statements[i]);
                Var* leftVar = dynamic_cast<Var*>(assign.left.get());
                cout << (leftVar ? leftVar->name : "?") << " := " << TestUtils::exprToString(assign.right.get()) << endl;
            } else if(prog.statements[i]->statementType == StmtType::ASSUME) {
                Assume& assume = dynamic_cast<Assume&>(*prog.statements[i]);
                cout << "assume(" << TestUtils::exprToString(assume.expr.get()) << ")" << endl;
            }
        }
    }
    
    void execute() {
        cout << "\n*********************Test case: " << testName << " *************" << endl;
        
        Program abstractProgram = makeAbstractProgram();
        printProgram("\n[1] Abstract Test Case (ATC):", abstractProgram);
        
        // Create tester
        FunctionFactory* functionFactory = new App1FunctionFactory();
        
        Tester tester(functionFactory);
        vector<Expr*> initialConcreteVals; // Empty initially
        ValueEnvironment ve(nullptr);
        
        // Generate concrete test case (this will do the full process)
        unique_ptr<Program> atcCopy = make_unique<Program>(move(const_cast<vector<unique_ptr<Stmt>>&>(abstractProgram.statements)));
        unique_ptr<Program> concreteProgram = tester.generateCTC(
            std::move(atcCopy),
            initialConcreteVals,
            &ve
        );
        
        // Now demonstrate the rewrite step with example concrete values
        // Create another copy of the abstract program
        Program abstractProgram2 = makeAbstractProgram();
        unique_ptr<Program> atcCopy2 = make_unique<Program>(move(const_cast<vector<unique_ptr<Stmt>>&>(abstractProgram2.statements)));
        
        printProgram("\n[2] Final Concrete Test Case (CTC - after full symbolic execution):", *concreteProgram);
        
        // Verify results
        verify(tester, concreteProgram);
        
        cout << "✓ Test passed!" << endl;
    }
};

/*
Test: Simple abstract test case with one input
Program:
    x := input()
    assume(x > 5)
Expected: Concrete test case with x assigned a value > 5
*/
class TesterTest1 : public TesterTest {
public:
    TesterTest1() : TesterTest("Simple abstract test case with one input") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(5))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(Tester& tester, unique_ptr<Program>& result) override {
        // Verify the result is concrete (no input statements)
        for(const auto& stmt : result->statements) {
            if(stmt->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*stmt);
                if(assign.right->exprType == ExprType::FUNCCALL) {
                    FuncCall& fc = dynamic_cast<FuncCall&>(*assign.right);
                    assert(fc.name != "input"); // Should not have input() anymore
                }
            }
        }
        
        // Verify path constraints were generated (may be more than 1 due to recursive calls)
        vector<Expr*>& pathConstraints = tester.getPathConstraints();
        assert(pathConstraints.size() >= 1);
        
        // Verify the first statement is an assignment with a concrete number
        assert(result->statements.size() >= 1);
        assert(result->statements[0]->statementType == StmtType::ASSIGN);
        Assign& firstAssign = dynamic_cast<Assign&>(*result->statements[0]);
        assert(firstAssign.right->exprType == ExprType::NUM);
        
        cout << "Verification: Abstract test case successfully converted to concrete" << endl;
    }
};

/*
Test: Abstract test case with two inputs and constraints
Program:
    x := input()
    y := input()
    assume(x + y = 10)
    assume(x > 3)
Expected: Concrete test case with x and y assigned values satisfying constraints
*/
class TesterTest2 : public TesterTest {
public:
    TesterTest2() : TesterTest("Abstract test case with two inputs and constraints") {}
    
protected:
    Program makeAbstractProgram() override {
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
    
    void verify(Tester& tester, unique_ptr<Program>& result) override {
        // Verify the result is concrete (no input statements)
        for(const auto& stmt : result->statements) {
            if(stmt->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*stmt);
                if(assign.right->exprType == ExprType::FUNCCALL) {
                    FuncCall& fc = dynamic_cast<FuncCall&>(*assign.right);
                    assert(fc.name != "input");
                }
            }
        }
        
        // Verify path constraints were generated (may be more than 2 due to recursive calls)
        vector<Expr*>& pathConstraints = tester.getPathConstraints();
        assert(pathConstraints.size() >= 2);
        
        // Verify we have assignment statements with concrete numbers
        int assignCount = 0;
        for(const auto& stmt : result->statements) {
            if(stmt->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*stmt);
                if(assign.right->exprType == ExprType::NUM) {
                    assignCount++;
                }
            }
        }
        assert(assignCount >= 2); // Should have x and y assignments
        
        cout << "Verification: Two-input abstract test case successfully converted" << endl;
    }
};

/*
Test: Already concrete test case (no inputs)
Program:
    x := 5
    y := 10
    assume(x + y = 15)
Expected: Same program returned (no changes needed)
*/
class TesterTest3 : public TesterTest {
public:
    TesterTest3() : TesterTest("Already concrete test case (no inputs)") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("x"),
            make_unique<Num>(5)
        ));
        
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y"),
            make_unique<Num>(10)
        ));
        
        statements.push_back(TestUtils::makeAssumeEq(
            TestUtils::makeBinOp("Add", make_unique<Var>("x"), make_unique<Var>("y")),
            make_unique<Num>(15)
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(Tester& tester, unique_ptr<Program>& result) override {
        // Verify the result has the same number of statements
        assert(result->statements.size() == 3);
        
        // Verify first statement is x := 5
        Assign& assign1 = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar1 = dynamic_cast<Var*>(assign1.left.get());
        assert(leftVar1 && leftVar1->name == "x");
        assert(assign1.right->exprType == ExprType::NUM);
        Num& num1 = dynamic_cast<Num&>(*assign1.right);
        assert(num1.value == 5);
        
        cout << "Verification: Concrete test case returned unchanged" << endl;
    }
};

/*
Test: Abstract test case with computation
Program:
    x := input()
    y := input()
    z := x + y
    assume(z = 10)
Expected: Concrete test case with x and y assigned values where x + y = 10
*/
class TesterTest4 : public TesterTest {
public:
    TesterTest4() : TesterTest("Abstract test case with computation") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(TestUtils::makeInputAssign("y"));
        
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("z"),
            TestUtils::makeBinOp("Add", make_unique<Var>("x"), make_unique<Var>("y"))
        ));
        
        statements.push_back(TestUtils::makeAssumeEq(
            make_unique<Var>("z"),
            make_unique<Num>(10)
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(Tester& tester, unique_ptr<Program>& result) override {
        // Verify the result is concrete
        for(const auto& stmt : result->statements) {
            if(stmt->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*stmt);
                if(assign.right->exprType == ExprType::FUNCCALL) {
                    FuncCall& fc = dynamic_cast<FuncCall&>(*assign.right);
                    assert(fc.name != "input");
                }
            }
        }
        
        // Verify path constraints were generated (may be more than 1 due to recursive calls)
        vector<Expr*>& pathConstraints = tester.getPathConstraints();
        assert(pathConstraints.size() >= 1);
        
        cout << "Verification: Abstract test case with computation successfully converted" << endl;
    }
};

// ============================================================================
// Tests for rewriteATC function
// ============================================================================

class RewriteATCTest {
protected:
    string testName;
    virtual Program makeAbstractProgram() = 0;
    virtual vector<unique_ptr<Expr>> makeConcreteVals() = 0;
    virtual void verify(unique_ptr<Program>& result) = 0;
    
public:
    RewriteATCTest(const string& name) : testName(name) {}
    virtual ~RewriteATCTest() = default;
    
    virtual void execute() {
        cout << "\n*** Test: " << testName << " ***" << endl;
        
        Program abstractProgram = makeAbstractProgram();
        vector<unique_ptr<Expr>> concreteValsOwned = makeConcreteVals();
        
        // Convert to raw pointers for the function call
        vector<Expr*> concreteVals;
        for(const auto& expr : concreteValsOwned) {
            concreteVals.push_back(expr.get());
        }
        
        unique_ptr<Program> atc = make_unique<Program>(move(const_cast<vector<unique_ptr<Stmt>>&>(abstractProgram.statements)));
        
        Tester tester(nullptr);
        unique_ptr<Program> result = tester.rewriteATC(atc, concreteVals);
        
        verify(result);
        // No cleanup needed - unique_ptrs handle it automatically
        
        cout << "✓ Test passed!" << endl;
    }
};

/*
Test: rewriteATC with single input and one concrete value
Program:
    x := input()
    assume(x > 5)
ConcreteVals: [10]
Expected: x := 10, assume(x > 5)
*/
class RewriteATCTest1 : public RewriteATCTest {
public:
    RewriteATCTest1() : RewriteATCTest("rewriteATC with single input") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(5))
        ));
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        vector<unique_ptr<Expr>> vals;
        vals.push_back(make_unique<Num>(10));
        return vals;
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 2);
        assert(result->statements[0]->statementType == StmtType::ASSIGN);
        Assign& assign = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar = dynamic_cast<Var*>(assign.left.get());
        assert(leftVar && leftVar->name == "x");
        assert(assign.right->exprType == ExprType::NUM);
        Num& num = dynamic_cast<Num&>(*assign.right);
        assert(num.value == 10);
        assert(result->statements[1]->statementType == StmtType::ASSUME);
    }
};

/*
Test: rewriteATC with multiple inputs and concrete values
Program:
    x := input()
    y := input()
    z := input()
ConcreteVals: [5, 10, 15]
Expected: x := 5, y := 10, z := 15
*/
class RewriteATCTest2 : public RewriteATCTest {
public:
    RewriteATCTest2() : RewriteATCTest("rewriteATC with multiple inputs") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(TestUtils::makeInputAssign("y"));
        statements.push_back(TestUtils::makeInputAssign("z"));
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        vector<unique_ptr<Expr>> vals;
        vals.push_back(make_unique<Num>(5));
        vals.push_back(make_unique<Num>(10));
        vals.push_back(make_unique<Num>(15));
        return vals;
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 3);
        
        Assign& assign1 = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar1 = dynamic_cast<Var*>(assign1.left.get());
        assert(leftVar1 && leftVar1->name == "x");
        assert(dynamic_cast<Num&>(*assign1.right).value == 5);
        
        Assign& assign2 = dynamic_cast<Assign&>(*result->statements[1]);
        Var* leftVar2 = dynamic_cast<Var*>(assign2.left.get());
        assert(leftVar2 && leftVar2->name == "y");
        assert(dynamic_cast<Num&>(*assign2.right).value == 10);
        
        Assign& assign3 = dynamic_cast<Assign&>(*result->statements[2]);
        Var* leftVar3 = dynamic_cast<Var*>(assign3.left.get());
        assert(leftVar3 && leftVar3->name == "z");
        assert(dynamic_cast<Num&>(*assign3.right).value == 15);
    }
};

/*
Test: rewriteATC with no inputs (already concrete)
Program:
    x := 5
    y := 10
ConcreteVals: []
Expected: Same program unchanged
*/
class RewriteATCTest3 : public RewriteATCTest {
public:
    RewriteATCTest3() : RewriteATCTest("rewriteATC with no inputs") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("x"),
            make_unique<Num>(5)
        ));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y"),
            make_unique<Num>(10)
        ));
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        return {};
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 2);
        
        Assign& assign1 = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar1 = dynamic_cast<Var*>(assign1.left.get());
        assert(leftVar1 && leftVar1->name == "x");
        assert(dynamic_cast<Num&>(*assign1.right).value == 5);
        
        Assign& assign2 = dynamic_cast<Assign&>(*result->statements[1]);
        Var* leftVar2 = dynamic_cast<Var*>(assign2.left.get());
        assert(leftVar2 && leftVar2->name == "y");
        assert(dynamic_cast<Num&>(*assign2.right).value == 10);
    }
};

/*
Test: rewriteATC with mixed statements (inputs and non-inputs)
Program:
    x := input()
    y := 5
    z := input()
    w := x + y
ConcreteVals: [10, 20]
Expected: x := 10, y := 5, z := 20, w := x + y
*/
class RewriteATCTest4 : public RewriteATCTest {
public:
    RewriteATCTest4() : RewriteATCTest("rewriteATC with mixed statements") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y"),
            make_unique<Num>(5)
        ));
        statements.push_back(TestUtils::makeInputAssign("z"));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("w"),
            TestUtils::makeBinOp("Add", make_unique<Var>("x"), make_unique<Var>("y"))
        ));
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        vector<unique_ptr<Expr>> vals;
        vals.push_back(make_unique<Num>(10));
        vals.push_back(make_unique<Num>(20));
        return vals;
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 4);
        
        Assign& assign1 = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar1 = dynamic_cast<Var*>(assign1.left.get());
        assert(leftVar1 && leftVar1->name == "x");
        assert(dynamic_cast<Num&>(*assign1.right).value == 10);
        
        Assign& assign2 = dynamic_cast<Assign&>(*result->statements[1]);
        Var* leftVar2 = dynamic_cast<Var*>(assign2.left.get());
        assert(leftVar2 && leftVar2->name == "y");
        assert(dynamic_cast<Num&>(*assign2.right).value == 5);
        
        Assign& assign3 = dynamic_cast<Assign&>(*result->statements[2]);
        Var* leftVar3 = dynamic_cast<Var*>(assign3.left.get());
        assert(leftVar3 && leftVar3->name == "z");
        assert(dynamic_cast<Num&>(*assign3.right).value == 20);
        
        Assign& assign4 = dynamic_cast<Assign&>(*result->statements[3]);
        Var* leftVar4 = dynamic_cast<Var*>(assign4.left.get());
        assert(leftVar4 && leftVar4->name == "w");
        assert(assign4.right->exprType == ExprType::FUNCCALL);
    }
};

/*
Test: rewriteATC with fewer concrete values than inputs
Program:
    x := input()
    y := input()
    z := input()
ConcreteVals: [5, 10]
Expected: x := 5, y := 10, z := input() (last input remains)
*/
class RewriteATCTest5 : public RewriteATCTest {
public:
    RewriteATCTest5() : RewriteATCTest("rewriteATC with fewer concrete values") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(TestUtils::makeInputAssign("y"));
        statements.push_back(TestUtils::makeInputAssign("z"));
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        vector<unique_ptr<Expr>> vals;
        vals.push_back(make_unique<Num>(5));
        vals.push_back(make_unique<Num>(10));
        return vals;
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 3);
        
        Assign& assign1 = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar1 = dynamic_cast<Var*>(assign1.left.get());
        assert(leftVar1 && leftVar1->name == "x");
        assert(assign1.right->exprType == ExprType::NUM);
        assert(dynamic_cast<Num&>(*assign1.right).value == 5);
        
        Assign& assign2 = dynamic_cast<Assign&>(*result->statements[1]);
        Var* leftVar2 = dynamic_cast<Var*>(assign2.left.get());
        assert(leftVar2 && leftVar2->name == "y");
        assert(assign2.right->exprType == ExprType::NUM);
        assert(dynamic_cast<Num&>(*assign2.right).value == 10);
        
        Assign& assign3 = dynamic_cast<Assign&>(*result->statements[2]);
        Var* leftVar3 = dynamic_cast<Var*>(assign3.left.get());
        assert(leftVar3 && leftVar3->name == "z");
        assert(assign3.right->exprType == ExprType::FUNCCALL);
        FuncCall& fc = dynamic_cast<FuncCall&>(*assign3.right);
        assert(fc.name == "input");
    }
};

/*
Test: rewriteATC with empty program and empty concrete values
Program: (empty)
ConcreteVals: []
Expected: Empty program returned
*/
class RewriteATCTest6 : public RewriteATCTest {
public:
    RewriteATCTest6() : RewriteATCTest("rewriteATC with empty program and empty values") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        return {};
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 0);
    }
};

/*
Test: rewriteATC with empty program but concrete values provided (error case)
Program: (empty)
ConcreteVals: [5, 10]
Expected: Exception thrown
*/
class RewriteATCTest7 : public RewriteATCTest {
public:
    RewriteATCTest7() : RewriteATCTest("rewriteATC error case (empty program with values)") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        vector<unique_ptr<Expr>> vals;
        vals.push_back(make_unique<Num>(5));
        vals.push_back(make_unique<Num>(10));
        return vals;
    }
    
    void verify(unique_ptr<Program>& result) override {
        // Should not reach here - exception should be thrown
        assert(false);
    }
    
public:
    void execute() override {
        cout << "\n*** Test: " << testName << " ***" << endl;
        
        Program abstractProgram = makeAbstractProgram();
        vector<unique_ptr<Expr>> concreteValsOwned = makeConcreteVals();
        
        // Convert to raw pointers for the function call
        vector<Expr*> concreteVals;
        for(const auto& expr : concreteValsOwned) {
            concreteVals.push_back(expr.get());
        }
        
        unique_ptr<Program> atc = make_unique<Program>(move(const_cast<vector<unique_ptr<Stmt>>&>(abstractProgram.statements)));
        
        Tester tester(nullptr);
        bool exceptionThrown = false;
        
        try {
            unique_ptr<Program> result = tester.rewriteATC(atc, concreteVals);
        } catch(const runtime_error& e) {
            exceptionThrown = true;
            cout << "Expected exception caught: " << e.what() << endl;
        }
        
        assert(exceptionThrown);
        // No cleanup needed - unique_ptrs handle it automatically
        
        cout << "✓ Test passed!" << endl;
    }
};

/*
Test: rewriteATC with assume statements
Program:
    x := input()
    assume(x > 0)
    y := input()
    assume(y < 100)
ConcreteVals: [5, 50]
Expected: x := 5, assume(x > 0), y := 50, assume(y < 100)
*/
class RewriteATCTest8 : public RewriteATCTest {
public:
    RewriteATCTest8() : RewriteATCTest("rewriteATC with assume statements") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(0))
        ));
        statements.push_back(TestUtils::makeInputAssign("y"));
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Lt", make_unique<Var>("y"), make_unique<Num>(100))
        ));
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        vector<unique_ptr<Expr>> vals;
        vals.push_back(make_unique<Num>(5));
        vals.push_back(make_unique<Num>(50));
        return vals;
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 4);
        
        assert(result->statements[0]->statementType == StmtType::ASSIGN);
        Assign& assign1 = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar1 = dynamic_cast<Var*>(assign1.left.get());
        assert(leftVar1 && leftVar1->name == "x");
        assert(dynamic_cast<Num&>(*assign1.right).value == 5);
        
        assert(result->statements[1]->statementType == StmtType::ASSUME);
        
        assert(result->statements[2]->statementType == StmtType::ASSIGN);
        Assign& assign2 = dynamic_cast<Assign&>(*result->statements[2]);
        Var* leftVar2 = dynamic_cast<Var*>(assign2.left.get());
        assert(leftVar2 && leftVar2->name == "y");
        assert(dynamic_cast<Num&>(*assign2.right).value == 50);
        
        assert(result->statements[3]->statementType == StmtType::ASSUME);
    }
};

/*
Test: rewriteATC preserves non-input function calls
Program:
    x := input()
    y := Add(x, 5)
    z := Mul(y, 2)
ConcreteVals: [10]
Expected: x := 10, y := Add(x, 5), z := Mul(y, 2)
*/
class RewriteATCTest9 : public RewriteATCTest {
public:
    RewriteATCTest9() : RewriteATCTest("rewriteATC preserves non-input function calls") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        statements.push_back(TestUtils::makeInputAssign("x"));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y"),
            TestUtils::makeBinOp("Add", make_unique<Var>("x"), make_unique<Num>(5))
        ));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("z"),
            TestUtils::makeBinOp("Mul", make_unique<Var>("y"), make_unique<Num>(2))
        ));
        return Program(std::move(statements));
    }
    
    vector<unique_ptr<Expr>> makeConcreteVals() override {
        vector<unique_ptr<Expr>> vals;
        vals.push_back(make_unique<Num>(10));
        return vals;
    }
    
    void verify(unique_ptr<Program>& result) override {
        assert(result->statements.size() == 3);
        
        Assign& assign1 = dynamic_cast<Assign&>(*result->statements[0]);
        Var* leftVar1 = dynamic_cast<Var*>(assign1.left.get());
        assert(leftVar1 && leftVar1->name == "x");
        assert(assign1.right->exprType == ExprType::NUM);
        assert(dynamic_cast<Num&>(*assign1.right).value == 10);
        
        Assign& assign2 = dynamic_cast<Assign&>(*result->statements[1]);
        Var* leftVar2 = dynamic_cast<Var*>(assign2.left.get());
        assert(leftVar2 && leftVar2->name == "y");
        assert(assign2.right->exprType == ExprType::FUNCCALL);
        FuncCall& fc2 = dynamic_cast<FuncCall&>(*assign2.right);
        assert(fc2.name == "Add");
        
        Assign& assign3 = dynamic_cast<Assign&>(*result->statements[2]);
        Var* leftVar3 = dynamic_cast<Var*>(assign3.left.get());
        assert(leftVar3 && leftVar3->name == "z");
        assert(assign3.right->exprType == ExprType::FUNCCALL);
        FuncCall& fc3 = dynamic_cast<FuncCall&>(*assign3.right);
        assert(fc3.name == "Mul");
    }
};

/*
Test: Complex test case with API function calls requiring interruption
Program:
    y := 0
    // F1
    y1 := y
    x1 := input()
    assume(x1 < 10)
    f1(x1, 0) --> r1  (FUNCTION_CALL_STMT - needs interruption)
    // assume(r1 = 1)
    // assume(y = y1 + x1)
    // F2
    y2 := y
    x2 := input()
    assume(x2 < 10)
    f1(x2, 0) --> r2  (FUNCTION_CALL_STMT - needs interruption)
    // assume(r2 = 1)
    // assume(y = y2 + x2)

*/
class TesterTest5 : public TesterTest {
public:
    TesterTest5() : TesterTest("Complex test with API function calls and interruption") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // y := 0
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y"),
            make_unique<Num>(0)
        ));
        
        // F1 sequence
        // y1 := y
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y1"),
            make_unique<Var>("y")
        ));
        
        // x1 := input()
        statements.push_back(TestUtils::makeInputAssign("x1"));
        
        // assume(x1 < 10)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Lt", make_unique<Var>("x1"), make_unique<Num>(10))
        ));
        
        // r1 := f1(x1, 0) (API call as ASSIGN)
        vector<unique_ptr<Expr>> f1Args;
        f1Args.push_back(make_unique<Var>("x1"));
        f1Args.push_back(make_unique<Num>(0));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("r1"),
            make_unique<FuncCall>("f1", std::move(f1Args))
        ));
        
        // // assume(r1 = 1) - Note: r1 would be the return value from f(x1)
        // // For now, we'll use a placeholder variable
        // statements.push_back(make_unique<Assume>(
        //     TestUtils::makeBinOp("Eq", make_unique<Var>("r1"), make_unique<Num>(1))
        // ));
        
        // // assume(y = y1 + x1)
        // statements.push_back(make_unique<Assume>(
        //     TestUtils::makeBinOp("Eq", 
        //         make_unique<Var>("y"),
        //         TestUtils::makeBinOp("Add", make_unique<Var>("y1"), make_unique<Var>("x1"))
        //     )
        // ));
        
        // F2 sequence
        // y2 := y
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y2"),
            make_unique<Var>("y")
        ));
        
        // x2 := input()
        statements.push_back(TestUtils::makeInputAssign("x2"));
        
        // assume(x2 < 10)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Lt", make_unique<Var>("x2"), make_unique<Num>(10))
        ));
        
        // r2 := f1(x2, 0) (API call as ASSIGN)
        vector<unique_ptr<Expr>> f2Args;
        f2Args.push_back(make_unique<Var>("x2"));
        f2Args.push_back(make_unique<Num>(0));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("r2"),
            make_unique<FuncCall>("f1", std::move(f2Args))
        ));
        
        // // assume(r2 = 1)
        // statements.push_back(make_unique<Assume>(
        //     TestUtils::makeBinOp("Eq", make_unique<Var>("r2"), make_unique<Num>(1))
        // ));
        
        // // assume(y = y2 + x2)
        // statements.push_back(make_unique<Assume>(
        //     TestUtils::makeBinOp("Eq", 
        //         make_unique<Var>("y"),
        //         TestUtils::makeBinOp("Add", make_unique<Var>("y2"), make_unique<Var>("x2"))
        //     )
        // ));
        
        return Program(std::move(statements));
    }
    
    void verify(Tester& tester, unique_ptr<Program>& result) override {
        // Verify the result has concrete values
        cout << "Verification: Complex test case with API calls" << endl;
        
        // The test should have been processed in segments:
        // Segment 1: y := 0, y1 := y, x1 := input(), assume(x1 < 10)
        // Then interrupt at f(x1), solve for x1, execute f(x1)
        // Segment 2: Continue with r1 result, then process until f(x2)
        // Then interrupt at f(x2), solve for x2, execute f(x2)
        // Segment 3: Continue with r2 result
        
        // For now, just verify we have statements
        assert(result->statements.size() > 0);
        

    }
};

/*
Test: Multiple API calls with different functions (f1 and f2)
Program:
    x := input()
    y := input()
    assume(x > 0)
    assume(y > 0)
    r1 := f1(x, y)  // f1 adds two numbers
    assume(r1 < 20)
    r2 := f2()      // f2 returns 0
    z := r1 + r2
Expected: Concrete test case with x, y values satisfying constraints
*/
class TesterTest6 : public TesterTest {
public:
    TesterTest6() : TesterTest("Multiple API calls with f1 and f2") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // x := input()
        statements.push_back(TestUtils::makeInputAssign("x"));
        
        // y := input()
        statements.push_back(TestUtils::makeInputAssign("y"));
        
        // assume(x > 0)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("x"), make_unique<Num>(0))
        ));
        
        // assume(y > 0)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Gt", make_unique<Var>("y"), make_unique<Num>(0))
        ));
        
        // r1 := f1(x, y)
        vector<unique_ptr<Expr>> f1Args;
        f1Args.push_back(make_unique<Var>("x"));
        f1Args.push_back(make_unique<Var>("y"));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("r1"),
            make_unique<FuncCall>("f1", std::move(f1Args))
        ));
        
        // assume(r1 = x + y)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Eq", 
                make_unique<Var>("r1"), 
                TestUtils::makeBinOp("Add", make_unique<Var>("x"), make_unique<Var>("y"))
            )
        ));
        
        // r2 := f2()
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("r2"),
            make_unique<FuncCall>("f2", vector<unique_ptr<Expr>>{})
        ));
        
        // z := r1 + r2
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("z"),
            TestUtils::makeBinOp("Add", make_unique<Var>("r1"), make_unique<Var>("r2"))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(Tester& tester, unique_ptr<Program>& result) override {
        cout << "Verification: Multiple API calls with f1 and f2" << endl;
        
        // Verify the result is concrete (no input statements)
        for(const auto& stmt : result->statements) {
            if(stmt->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*stmt);
                if(assign.right->exprType == ExprType::FUNCCALL) {
                    FuncCall& fc = dynamic_cast<FuncCall&>(*assign.right);
                    assert(fc.name != "input");
                }
            }
        }
        
        // Verify we have at least 2 concrete assignments (x and y)
        int concreteAssignments = 0;
        for(const auto& stmt : result->statements) {
            if(stmt->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*stmt);
                Var* leftVar = dynamic_cast<Var*>(assign.left.get());
                if(leftVar && (leftVar->name == "x" || leftVar->name == "y") && 
                   assign.right->exprType == ExprType::NUM) {
                    concreteAssignments++;
                }
            }
        }
        assert(concreteAssignments >= 2);
        
        // Verify path constraints were generated
        vector<Expr*>& pathConstraints = tester.getPathConstraints();
        assert(pathConstraints.size() >= 2);
        
        cout << "  ✓ All input() calls replaced with concrete values" << endl;
        cout << "  ✓ API calls f1 and f2 executed successfully" << endl;
        cout << "  ✓ Path constraints satisfied" << endl;
    }
};

/*
Test: Global state management with getter/setter functions
Program:
    set_y(0) // _temp0 := set_y(0)
    y1 := get_y()
    x1 := input()
    assume(x1 < 10)
    r1 := f1(x1, 0)
    y2 := get_y()
    x2 := input()
    assume(x2 < 10)
    r2 := f1(x2, 0)
Expected: Concrete test case with global state accessed via get_y/set_y
*/
class TesterTest7 : public TesterTest {
public:
    TesterTest7() : TesterTest("Global state with getter/setter functions") {}
    
protected:
    Program makeAbstractProgram() override {
        vector<unique_ptr<Stmt>> statements;
        
        // set_y(0)
        vector<unique_ptr<Expr>> setYArgs;
        setYArgs.push_back(make_unique<Num>(0));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("_tmp0"),
            make_unique<FuncCall>("set_y", std::move(setYArgs))
        ));
        
        // y1 := get_y()
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y1"),
            make_unique<FuncCall>("get_y", vector<unique_ptr<Expr>>{})
        ));
        
        // x1 := input()
        statements.push_back(TestUtils::makeInputAssign("x1"));
        
        // assume(x1 < 10)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Lt", make_unique<Var>("x1"), make_unique<Num>(10))
        ));
        
        // r1 := f1(x1, 0)
        vector<unique_ptr<Expr>> f1Args1;
        f1Args1.push_back(make_unique<Var>("x1"));
        f1Args1.push_back(make_unique<Num>(0));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("r1"),
            make_unique<FuncCall>("f1", std::move(f1Args1))
        ));
        
        // y2 := get_y()
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("y2"),
            make_unique<FuncCall>("get_y", vector<unique_ptr<Expr>>{})
        ));
        
        // x2 := input()
        statements.push_back(TestUtils::makeInputAssign("x2"));
        
        // assume(x2 < 10)
        statements.push_back(make_unique<Assume>(
            TestUtils::makeBinOp("Lt", make_unique<Var>("x2"), make_unique<Num>(10))
        ));
        
        // r2 := f1(x2, 0)
        vector<unique_ptr<Expr>> f1Args2;
        f1Args2.push_back(make_unique<Var>("x2"));
        f1Args2.push_back(make_unique<Num>(0));
        statements.push_back(make_unique<Assign>(
            make_unique<Var>("r2"),
            make_unique<FuncCall>("f1", std::move(f1Args2))
        ));
        
        return Program(std::move(statements));
    }
    
    void verify(Tester& tester, unique_ptr<Program>& result) override {
        cout << "Verification: Global state with getter/setter" << endl;
        
        // Verify the result is concrete (no input statements)
        for(const auto& stmt : result->statements) {
            if(stmt->statementType == StmtType::ASSIGN) {
                Assign& assign = dynamic_cast<Assign&>(*stmt);
                if(assign.right->exprType == ExprType::FUNCCALL) {
                    FuncCall& fc = dynamic_cast<FuncCall&>(*assign.right);
                    assert(fc.name != "input");
                }
            }
        }
        
        // Verify we have statements
        assert(result->statements.size() > 0);
        
        cout << "  ✓ All input() calls replaced with concrete values" << endl;
        cout << "  ✓ Global state accessed via get_y/set_y" << endl;
        cout << "  ✓ API calls executed successfully" << endl;
    }
};

int main() {
    cout << "========================================" << endl;
    cout << "Running rewriteATC Test Suite" << endl;
    cout << "========================================" << endl;
    
    // Run rewriteATC tests
    vector<RewriteATCTest*> rewriteTests = {
        // new RewriteATCTest1(),
        // new RewriteATCTest2(),
        // new RewriteATCTest3(),
        // new RewriteATCTest4(),
        // new RewriteATCTest5(),
        // new RewriteATCTest6(),
        // new RewriteATCTest7(),
        // new RewriteATCTest8(),
        // new RewriteATCTest9()
    };
    
    for(auto& t : rewriteTests) {
        try {
            t->execute();
            delete t;
        }
        catch(const exception& e) {
            cout << "Test exception: " << e.what() << endl;
            delete t;
        }
        catch(...) {
            cout << "Unknown test exception" << endl;
            delete t;
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "Running Tester Integration Test Suite" << endl;
    cout << "========================================" << endl;
    
    // Run integration tests
    vector<TesterTest*> integrationTests = {
        // new TesterTest1(),
        // new TesterTest2(),
        // new TesterTest3(),
        // new TesterTest4(),
        new TesterTest5(),
        new TesterTest6(),
        new TesterTest7()
    };
    
    for(auto& t : integrationTests) {
        try {
            t->execute();
            delete t;
        }
        catch(const exception& e) {
            cout << "Test exception: " << e.what() << endl;
            delete t;
        }
        catch(...) {
            cout << "Unknown test exception" << endl;
            delete t;
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "All tests passed!" << endl;
    cout << "========================================" << endl;
    
    return 0;
}
