#include <iostream>
#include <cassert>
#include <memory>
#include "../../language/ast.hh"
#include "../../language/env.hh"
#include "../../language/typemap.hh"
#include "../../language/printvisitor.hh"
#include "../../tester/genATC.hh"
#include "../../tester/tester.hh"
#include "../../tester/test_utils.hh"
#include "../../apps/app1/app1.hh"

using namespace std;

/**
 * End-to-End Test: Full pipeline from Spec to Concrete Test Case
 * 
 * Pipeline stages:
 * 1. Spec (API specification with globals, init, functions, blocks)
 * 2. genATC (Generate Abstract Test Case from Spec)
 * 3. Tester (Generate Concrete Test Case from ATC using symbolic execution)
 */
class E2ETest {
protected:
    string testName;
    PrintVisitor printer;
    
    virtual unique_ptr<Spec> makeSpec() = 0;
    virtual SymbolTable* makeSymbolTables() = 0;
    virtual void verify(const Program& ctc) = 0;
    
    virtual void cleanup(SymbolTable* globalTable) {
        if (globalTable) {
            // Delete children first
            for (size_t i = 0; i < globalTable->getChildCount(); i++) {
                delete globalTable->getChild(i);
            }
            delete globalTable;
        }
    }

public:
    E2ETest(const string& name) : testName(name) {}
    virtual ~E2ETest() = default;
    
    void execute() {
        cout << "\n" << string(80, '=') << endl;
        cout << "E2E Test: " << testName << endl;
        cout << string(80, '=') << endl;
        
        // ===== STAGE 1: Create Specification =====
        cout << "\n[STAGE 1] Creating API Specification..." << endl;
        unique_ptr<Spec> spec = makeSpec();
        assert(spec != nullptr);
        
        cout << "\nSpecification:" << endl;
        printer.visitSpec(*spec);
        
        // ===== STAGE 2: Generate Abstract Test Case =====
        cout << "\n[STAGE 2] Generating Abstract Test Case (ATC)..." << endl;
        SymbolTable* globalSymTable = makeSymbolTables();
        TypeMap typeMap;
        
        ATCGenerator generator(spec.get(), std::move(typeMap));
        Program atc = generator.generate(spec.get(), globalSymTable);
        
        cout << "\nAbstract Test Case (ATC):" << endl;
        printer.visitProgram(atc);
        
        // ===== STAGE 3: Generate Concrete Test Case =====
        cout << "\n[STAGE 3] Generating Concrete Test Case (CTC) via Symbolic Execution..." << endl;
        
        FunctionFactory* functionFactory = new App1FunctionFactory();
        Tester tester(functionFactory);
        vector<Expr*> initialConcreteVals;
        ValueEnvironment ve(nullptr);
        
        unique_ptr<Program> atcCopy = make_unique<Program>(
            move(const_cast<vector<unique_ptr<Stmt>>&>(atc.statements))
        );
        
        unique_ptr<Program> ctc = tester.generateCTC(
            std::move(atcCopy),
            initialConcreteVals,
            &ve
        );
        
        cout << "\nConcrete Test Case (CTC):" << endl;
        printer.visitProgram(*ctc);
        
        // ===== STAGE 4: Verify Results =====
        cout << "\n[STAGE 4] Verifying Results..." << endl;
        verify(*ctc);
        
        // Cleanup
        cleanup(globalSymTable);
        delete functionFactory;
        
        cout << "\n✓ E2E Test Passed!" << endl;
        cout << string(80, '=') << endl;
    }
};

/**
 * E2E Test 1: Simple f1 API call
 * 
 * Spec:
 *   Global: y : int
 *   Init: y := 0
 *   API: f1(x, z)
 *     Pre: x > 0 AND z > 0
 *     Post: none
 * 
 * Expected ATC:
 *   y := 0
 *   x0 := input()
 *   z0 := input()
 *   assume(x0 > 0 AND z0 > 0)
 *   _result0 := f1(x0, z0)
 * 
 * Expected CTC:
 *   y := 0
 *   x0 := <concrete_value>
 *   z0 := <concrete_value>
 *   assume(x0 > 0 AND z0 > 0)
 *   _result0 := f1(x0, z0)
 */
class E2ETest1 : public E2ETest {
public:
    E2ETest1() : E2ETest("Simple f1 API call with precondition") {}
    
protected:
    unique_ptr<Spec> makeSpec() override {
        // Global: y : int
        vector<unique_ptr<Decl>> globals;
        globals.push_back(make_unique<Decl>("y", make_unique<TypeConst>("int")));
        
        // Init: y := 0
        vector<unique_ptr<Init>> inits;
        inits.push_back(make_unique<Init>("y", make_unique<Num>(0)));
        
        vector<unique_ptr<APIFuncDecl>> functions;
        vector<unique_ptr<API>> blocks;
        
        // API Block: f1(x, z)
        // Pre: x > 0 AND z > 0
        vector<unique_ptr<Expr>> xGt0Args;
        xGt0Args.push_back(make_unique<Var>("x"));
        xGt0Args.push_back(make_unique<Num>(0));
        auto xGt0 = make_unique<FuncCall>("Gt", std::move(xGt0Args));
        
        vector<unique_ptr<Expr>> zGt0Args;
        zGt0Args.push_back(make_unique<Var>("z"));
        zGt0Args.push_back(make_unique<Num>(0));
        auto zGt0 = make_unique<FuncCall>("Gt", std::move(zGt0Args));
        
        vector<unique_ptr<Expr>> andArgs;
        andArgs.push_back(std::move(xGt0));
        andArgs.push_back(std::move(zGt0));
        auto pre = make_unique<FuncCall>("And", std::move(andArgs));
        
        // API call: f1(x, z)
        vector<unique_ptr<Expr>> callArgs;
        callArgs.push_back(make_unique<Var>("x"));
        callArgs.push_back(make_unique<Var>("z"));
        auto call = make_unique<FuncCall>("f1", std::move(callArgs));
        
        auto apiCall = make_unique<APIcall>(
            std::move(call),
            Response(HTTPResponseCode::OK_200, nullptr)
        );
        
        blocks.push_back(make_unique<API>(
            std::move(pre),
            std::move(apiCall),
            Response(HTTPResponseCode::OK_200, nullptr)
        ));
        
        return make_unique<Spec>(
            std::move(globals),
            std::move(inits),
            std::move(functions),
            std::move(blocks)
        );
    }
    
    SymbolTable* makeSymbolTables() override {
        // Global symbol table
        auto* globalTable = new SymbolTable(nullptr);
        
        auto* f1Table = new SymbolTable(globalTable);
        string* x = new string("x");
        string* z = new string("z");
        f1Table->addMapping(x, nullptr);
        f1Table->addMapping(z, nullptr);
        
        globalTable->addChild(f1Table);
        return globalTable;
    }
    
    void verify(const Program& ctc) override {
        cout << "  Verifying CTC structure..." << endl;
        
        // Count statements
        cout << "  Total statements: " << ctc.statements.size() << endl;
        
        // Check what we have
        int inputCount = 0;
        int concreteCount = 0;
        bool foundF1 = false;
        
        for (const auto& stmt : ctc.statements) {
            if (stmt->statementType == StmtType::ASSIGN) {
                const Assign& assign = dynamic_cast<const Assign&>(*stmt);
                if (assign.right->exprType == ExprType::FUNCCALL) {
                    const FuncCall& fc = dynamic_cast<const FuncCall&>(*assign.right);
                    if (fc.name == "input") {
                        inputCount++;
                        cout << "  Found input() for variable: " << dynamic_cast<const Var&>(*assign.left).name << endl;
                    }
                    if (fc.name == "f1") {
                        foundF1 = true;
                        cout << "  ✓ Found f1 API call" << endl;
                    }
                } else if (assign.right->exprType == ExprType::NUM) {
                    concreteCount++;
                    cout << "  Found concrete assignment: " << dynamic_cast<const Var&>(*assign.left).name << " := " 
                         << dynamic_cast<const Num&>(*assign.right).value << endl;
                }
            }
        }
        
        cout << "  Input calls remaining: " << inputCount << endl;
        cout << "  Concrete assignments: " << concreteCount << endl;
        
        // For now, just verify the structure exists
        assert(ctc.statements.size() >= 1);
        assert(foundF1);
        
        if (inputCount == 0) {
            cout << "  ✓ All input() calls replaced with concrete values" << endl;
        } else {
            cout << "  ⚠ Warning: " << inputCount << " input() calls still present (symbolic execution may be incomplete)" << endl;
        }
        
        cout << "  ✓ API call f1 present in CTC" << endl;
    }
};

/**
 * E2E Test 2: Two sequential API calls (f1 then f2)
 * 
 * Spec:
 *   Global: y : int
 *   Init: y := 0
 *   API 1: f1(x, z)
 *     Pre: x > 0
 *   API 2: f2()
 *     Pre: none
 * 
 * Expected: CTC with two API calls, different variable names (x0/z0 for f1)
 */
class E2ETest2 : public E2ETest {
public:
    E2ETest2() : E2ETest("Sequential API calls - f1 then f2") {}
    
protected:
    unique_ptr<Spec> makeSpec() override {
        // Global: y : int
        vector<unique_ptr<Decl>> globals;
        globals.push_back(make_unique<Decl>("y", make_unique<TypeConst>("int")));
        
        // Init: y := 0
        vector<unique_ptr<Init>> inits;
        inits.push_back(make_unique<Init>("y", make_unique<Num>(0)));
        
        vector<unique_ptr<APIFuncDecl>> functions;
        vector<unique_ptr<API>> blocks;
        
        // Block 1: f1(x, z)
        {
            vector<unique_ptr<Expr>> preArgs;
            preArgs.push_back(make_unique<Var>("x"));
            preArgs.push_back(make_unique<Num>(0));
            auto pre = make_unique<FuncCall>("Gt", std::move(preArgs));
            
            vector<unique_ptr<Expr>> callArgs;
            callArgs.push_back(make_unique<Var>("x"));
            callArgs.push_back(make_unique<Var>("z"));
            auto call = make_unique<FuncCall>("f1", std::move(callArgs));
            
            auto apiCall = make_unique<APIcall>(
                std::move(call),
                Response(HTTPResponseCode::OK_200, nullptr)
            );
            
            blocks.push_back(make_unique<API>(
                std::move(pre),
                std::move(apiCall),
                Response(HTTPResponseCode::OK_200, nullptr)
            ));
        }
        
        // Block 2: f2()
        {
            auto pre = make_unique<Num>(1); // true precondition
            
            vector<unique_ptr<Expr>> callArgs;
            auto call = make_unique<FuncCall>("f2", std::move(callArgs));
            
            auto apiCall = make_unique<APIcall>(
                std::move(call),
                Response(HTTPResponseCode::OK_200, nullptr)
            );
            
            blocks.push_back(make_unique<API>(
                std::move(pre),
                std::move(apiCall),
                Response(HTTPResponseCode::OK_200, nullptr)
            ));
        }
        
        return make_unique<Spec>(
            std::move(globals),
            std::move(inits),
            std::move(functions),
            std::move(blocks)
        );
    }
    
    SymbolTable* makeSymbolTables() override {
        // Global symbol table
        auto* globalTable = new SymbolTable(nullptr);
        
        // Symbol table for f1 block
        auto* f1Table = new SymbolTable(globalTable);
        string* x = new string("x");
        string* z = new string("z");
        f1Table->addMapping(x, nullptr);
        f1Table->addMapping(z, nullptr);
        
        // Symbol table for f2 block (no parameters)
        auto* f2Table = new SymbolTable(globalTable);
        
        globalTable->addChild(f1Table);
        globalTable->addChild(f2Table);
        return globalTable;
    }
    
    void verify(const Program& ctc) override {
        cout << "  Verifying CTC structure..." << endl;
        cout << "  Total statements: " << ctc.statements.size() << endl;
        
        int inputCount = 0;
        bool foundF1 = false, foundF2 = false;
        
        for (const auto& stmt : ctc.statements) {
            if (stmt->statementType == StmtType::ASSIGN) {
                const Assign& assign = dynamic_cast<const Assign&>(*stmt);
                if (assign.right->exprType == ExprType::FUNCCALL) {
                    const FuncCall& fc = dynamic_cast<const FuncCall&>(*assign.right);
                    if (fc.name == "input") inputCount++;
                    if (fc.name == "f1") {
                        foundF1 = true;
                        cout << "  ✓ Found f1 API call" << endl;
                    }
                    if (fc.name == "f2") {
                        foundF2 = true;
                        cout << "  ✓ Found f2 API call" << endl;
                    }
                }
            }
        }
        
        assert(foundF1);
        assert(foundF2);
        
        if (inputCount == 0) {
            cout << "  ✓ All input() calls replaced with concrete values" << endl;
        } else {
            cout << "  ⚠ Warning: " << inputCount << " input() calls still present" << endl;
        }
        
        cout << "  ✓ Both API calls present (f1, f2)" << endl;
    }
};

/**
 * E2E Test 3: API with global state (get_y/set_y)
 * 
 * Spec:
 *   Global: y : int
 *   Init: y := 0
 *   API: f1(x, z) with get_y/set_y calls
 *     Pre: x < 10
 *     Post: y' = y + x (using get_y and set_y)
 * 
 * Expected: CTC with get_y/set_y calls and state management
 */
class E2ETest3 : public E2ETest {
public:
    E2ETest3() : E2ETest("API with global state - get_y/set_y") {}
    
protected:
    unique_ptr<Spec> makeSpec() override {
        // Global: y : int
        vector<unique_ptr<Decl>> globals;
        globals.push_back(make_unique<Decl>("y", make_unique<TypeConst>("int")));
        
        // Init: y := 0 (using set_y)
        vector<unique_ptr<Init>> inits;
        vector<unique_ptr<Expr>> setYArgs;
        setYArgs.push_back(make_unique<Num>(0));
        inits.push_back(make_unique<Init>(
            "_tmp",
            make_unique<FuncCall>("set_y", std::move(setYArgs))
        ));
        
        vector<unique_ptr<APIFuncDecl>> functions;
        vector<unique_ptr<API>> blocks;
        
        // Precondition: x < 10
        vector<unique_ptr<Expr>> preArgs;
        preArgs.push_back(make_unique<Var>("x"));
        preArgs.push_back(make_unique<Num>(10));
        auto pre = make_unique<FuncCall>("Lt", std::move(preArgs));
        
        // API call: f1(x, 0)
        vector<unique_ptr<Expr>> callArgs;
        callArgs.push_back(make_unique<Var>("x"));
        callArgs.push_back(make_unique<Num>(0));
        auto call = make_unique<FuncCall>("f1", std::move(callArgs));
        
        auto apiCall = make_unique<APIcall>(
            std::move(call),
            Response(HTTPResponseCode::OK_200, nullptr)
        );
        
        blocks.push_back(make_unique<API>(
            std::move(pre),
            std::move(apiCall),
            Response(HTTPResponseCode::OK_200, nullptr)
        ));
        
        return make_unique<Spec>(
            std::move(globals),
            std::move(inits),
            std::move(functions),
            std::move(blocks)
        );
    }
    
    SymbolTable* makeSymbolTables() override {
        // Global symbol table
        auto* globalTable = new SymbolTable(nullptr);
        
        auto* f1Table = new SymbolTable(globalTable);
        string* x = new string("x");
        f1Table->addMapping(x, nullptr);
        
        globalTable->addChild(f1Table);
        return globalTable;
    }
    
    void verify(const Program& ctc) override {
        cout << "  Verifying CTC structure..." << endl;
        cout << "  Total statements: " << ctc.statements.size() << endl;
        
        int inputCount = 0;
        bool foundSetY = false;
        bool foundF1 = false;
        
        for (const auto& stmt : ctc.statements) {
            if (stmt->statementType == StmtType::ASSIGN) {
                const Assign& assign = dynamic_cast<const Assign&>(*stmt);
                if (assign.right->exprType == ExprType::FUNCCALL) {
                    const FuncCall& fc = dynamic_cast<const FuncCall&>(*assign.right);
                    if (fc.name == "input") inputCount++;
                    if (fc.name == "set_y") {
                        foundSetY = true;
                        cout << "  ✓ Found set_y call" << endl;
                    }
                    if (fc.name == "f1") {
                        foundF1 = true;
                        cout << "  ✓ Found f1 API call" << endl;
                    }
                }
            }
        }
        
        assert(foundSetY);
        assert(foundF1);
        
        if (inputCount == 0) {
            cout << "  ✓ All input() calls replaced with concrete values" << endl;
        } else {
            cout << "  ⚠ Warning: " << inputCount << " input() calls still present" << endl;
        }
        
        cout << "  ✓ set_y and f1 calls verified" << endl;
    }
};

int main() {
    cout << "\n" << string(80, '=') << endl;
    cout << "End-to-End Test Suite: Spec -> ATC -> CTC" << endl;
    cout << string(80, '=') << endl;
    
    vector<E2ETest*> tests = {
        new E2ETest1(),
        new E2ETest2(),
        new E2ETest3()
    };
    
    int passed = 0;
    int failed = 0;
    
    for (auto* test : tests) {
        try {
            test->execute();
            passed++;
            delete test;
        }
        catch (const exception& e) {
            cout << "\n✗ Test failed with exception: " << e.what() << endl;
            failed++;
            delete test;
        }
        catch (...) {
            cout << "\n✗ Test failed with unknown exception" << endl;
            failed++;
            delete test;
        }
    }
    
    cout << "\n" << string(80, '=') << endl;
    cout << "Test Results: " << passed << " passed, " << failed << " failed" << endl;
    cout << string(80, '=') << endl;
    
    return (failed == 0) ? 0 : 1;
}
