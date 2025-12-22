#include <cstddef>
#include <iostream>
#include <cassert>
#include <memory>
#include "../../language/ast.hh"
#include "../../language/env.hh"
#include "../../language/typemap.hh"
#include "../../language/printvisitor.hh"
#include "../../tester/genATC.hh"

using namespace std;

/**
 * Base class for genATC tests
 */
class GenATCTest {
protected:
    string testName;

    virtual unique_ptr<Spec> makeSpec() = 0;
    virtual SymbolTable* makeSymbolTables() = 0;
    virtual TypeMap makeTypeMap() = 0;
    virtual void verify(const Program& atc) = 0;
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
    GenATCTest(const string& name) : testName(name) {}
    virtual ~GenATCTest() = default;

    void execute() {
        cout << "\n==================== Test: " << testName << " ====================" << endl;
        PrintVisitor printer;
        // Create specification
        unique_ptr<Spec> spec = makeSpec();
        // the pointer should be checked for null
        assert(spec!=nullptr);

        const Spec& spec_obj = *spec;
        cout << " Specification in this TestCase" << endl;

        printer.visitSpec(spec_obj);
        // Create symbol tables
        SymbolTable* globalSymTable = makeSymbolTables();

        // Create type map with variable types
        TypeMap typeMap = makeTypeMap();
        cout << "\nType Map:" << endl;
        typeMap.print();

        // Generate ATC
        ATCGenerator generator(spec.get(), std::move(typeMap));
        Program atc = generator.generate(spec.get(), globalSymTable);

        // Print the generated ATC
        cout << "\nGenerated ATC:" << endl;

        printer.visitProgram(atc);
        cout << endl;

        // Verify results
        verify(atc);

        // Cleanup
        cleanup(globalSymTable);

        cout << "✓ Test passed!" << endl;
    }
};

/**
 * Test 1: Simple initialization only
 * Tests genInit() functionality
 */
class GenATCTest1 : public GenATCTest {
public:
    GenATCTest1() : GenATCTest("Simple initialization - genInit()") {}

protected:
    unique_ptr<Spec> makeSpec() override {
        // Create global declarations
        vector<unique_ptr<Decl>> globals;
        globals.push_back(make_unique<Decl>(
            "U",
            make_unique<MapType>(
                make_unique<TypeConst>("string"),
                make_unique<TypeConst>("string")
            )
        ));

        // Create initialization: U := {}
        vector<unique_ptr<Init>> inits;
        inits.push_back(make_unique<Init>(
            "U",
            make_unique<Map>(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>())
        ));

        // No API functions or blocks
        vector<unique_ptr<APIFuncDecl>> functions;
        vector<unique_ptr<API>> blocks;

        return make_unique<Spec>(
            std::move(globals),
            std::move(inits),
            std::move(functions),
            std::move(blocks)
        );
    }

    SymbolTable* makeSymbolTables() override {
        // No blocks, so just return global symbol table with no children
        return new SymbolTable(nullptr);
    }

    TypeMap makeTypeMap() override {
        TypeMap typeMap;
        // U: map<string, string>
        typeMap.setValue("U", new MapType(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        ));
        return typeMap;
    }

    void verify(const Program& atc) override {
        // Should have 1 initialization statement
        assert(atc.statements.size() == 1);

        // First statement should be assignment: U = {}
        const Stmt* stmt = atc.statements[0].get();
        assert(stmt->statementType == StmtType::ASSIGN);

        const Assign* assign = dynamic_cast<const Assign*>(stmt);
        assert(assign != nullptr);
        assert(assign->left->exprType == ExprType::VAR);
        const Var* leftVar = dynamic_cast<const Var*>(assign->left.get());
        assert(leftVar != nullptr);
        assert(leftVar->name == "U");
        assert(assign->right->exprType == ExprType::MAP);

        cout << "  ✓ Generated 1 initialization statement" << endl;
        cout << "  ✓ U = {} assignment verified" << endl;
    }
};

/**
 * Test 2: Single API block with simple precondition
 * Tests genBlock() with input variables and precondition
 */
class GenATCTest2 : public GenATCTest {
public:
    GenATCTest2() : GenATCTest("Single API block - signup with precondition") {}

protected:
    unique_ptr<Spec> makeSpec() override {
        // Global: U
        vector<unique_ptr<Decl>> globals;
        globals.push_back(make_unique<Decl>("U", make_unique<MapType>(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        )));

        // Init: U := {}
        vector<unique_ptr<Init>> inits;
        inits.push_back(make_unique<Init>(
            "U",
            make_unique<Map>(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>())
        ));

        vector<unique_ptr<APIFuncDecl>> functions;

        // API Block: signup(u, p)
        // Pre: not_in(u, U)
        // Post: none for simplicity
        vector<unique_ptr<API>> blocks;

        // Precondition: not_in(u, U)
        vector<unique_ptr<Expr>> preArgs;
        preArgs.push_back(make_unique<Var>("u"));
        preArgs.push_back(make_unique<Var>("U"));
        auto pre = make_unique<FuncCall>("not_in", std::move(preArgs));

        // API call: signup(u, p)
        vector<unique_ptr<Expr>> callArgs;
        callArgs.push_back(make_unique<Var>("u"));
        callArgs.push_back(make_unique<Var>("p"));
        auto call = make_unique<FuncCall>("signup", std::move(callArgs));

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
        
        // Symbol table for signup block with local variables u, p
        auto* signupTable = new SymbolTable(globalTable);
        string* u = new string("u");
        string* p = new string("p");
        signupTable->addMapping(u, new TypeConst("string"));
        signupTable->addMapping(p, new TypeConst("string"));
        
        globalTable->addChild(signupTable);
        return globalTable;
    }

    TypeMap makeTypeMap() override {
        TypeMap typeMap;
        // Global: U: map<string, string>
        typeMap.setValue("U", new MapType(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        ));
        // Local variables: u, p: string
        typeMap.setValue("u", new TypeConst("string"));
        typeMap.setValue("p", new TypeConst("string"));
        return typeMap;
    }

    void verify(const Program& atc) override {
        // Expected statements:
        // 1. U = {}                    (init)
        // 2. u0 := input()            (input for u)
        // 3. p0 := input()            (input for p)
        // 4. assume(not_in(u0, U))    (precondition)
        // 5. _result0 = signup(u0, p0) (API call) - LHS is Var with tuple type in TypeMap

        cout << "  Generated " << atc.statements.size() << " statements" << endl;
        assert(atc.statements.size() >= 5);

        // Check init
        const Assign* init = dynamic_cast<const Assign*>(atc.statements[0].get());
        assert(init != nullptr);
        assert(init->left->exprType == ExprType::VAR);
        const Var* initLeftVar = dynamic_cast<const Var*>(init->left.get());
        assert(initLeftVar != nullptr && initLeftVar->name == "U");
        cout << "  ✓ Initialization verified" << endl;

        // Check input statements (u0, p0)
        const Assign* input1 = dynamic_cast<const Assign*>(atc.statements[1].get());
        assert(input1 != nullptr);
        assert(input1->left->exprType == ExprType::VAR);
        const Var* input1LeftVar = dynamic_cast<const Var*>(input1->left.get());
        assert(input1LeftVar != nullptr && input1LeftVar->name == "u0");
        assert(input1->right->exprType == ExprType::FUNCCALL);

        const Assign* input2 = dynamic_cast<const Assign*>(atc.statements[2].get());
        assert(input2 != nullptr);
        assert(input2->left->exprType == ExprType::VAR);
        const Var* input2LeftVar = dynamic_cast<const Var*>(input2->left.get());
        assert(input2LeftVar != nullptr && input2LeftVar->name == "p0");
        cout << "  ✓ Input statements verified (u0, p0)" << endl;

        // Check assume statement
        const Assume* assume = dynamic_cast<const Assume*>(atc.statements[3].get());
        assert(assume != nullptr);
        assert(assume->expr->exprType == ExprType::FUNCCALL);
        const FuncCall* assumeCall = dynamic_cast<const FuncCall*>(assume->expr.get());
        assert(assumeCall->name == "not_in");
        cout << "  ✓ Precondition assume() verified" << endl;

        // Check API call - LHS is a Var (type is tuple in TypeMap)
        const Assign* apiCall = dynamic_cast<const Assign*>(atc.statements[4].get());
        assert(apiCall != nullptr);
        assert(apiCall->left->exprType == ExprType::VAR);
        const Var* resultVar = dynamic_cast<const Var*>(apiCall->left.get());
        assert(resultVar != nullptr);
        assert(resultVar->name == "_result0");
        assert(apiCall->right->exprType == ExprType::FUNCCALL);
        const FuncCall* callFunc = dynamic_cast<const FuncCall*>(apiCall->right.get());
        assert(callFunc->name == "signup");
        assert(callFunc->args.size() == 2);
        cout << "  ✓ API call verified" << endl;
    }
};

/**
 * Test 3: API block with primed variables (state transition)
 * Tests prime notation handling: U' = U union {u -> p}
 */
class GenATCTest3 : public GenATCTest {
public:
    GenATCTest3() : GenATCTest("API block with primed variables - state transition") {}

protected:
    unique_ptr<Spec> makeSpec() override {
        // Global: U
        vector<unique_ptr<Decl>> globals;
        globals.push_back(make_unique<Decl>("U", make_unique<MapType>(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        )));

        // Init: U := {}
        vector<unique_ptr<Init>> inits;
        inits.push_back(make_unique<Init>(
            "U",
            make_unique<Map>(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>())
        ));

        vector<unique_ptr<APIFuncDecl>> functions;

        // API Block: signup(u, p)
        // Pre: not_in(u, U)
        // Post: U' = U union {u -> p}
        vector<unique_ptr<API>> blocks;

        // Precondition
        vector<unique_ptr<Expr>> preArgs;
        preArgs.push_back(make_unique<Var>("u"));
        preArgs.push_back(make_unique<Var>("U"));
        auto pre = make_unique<FuncCall>("not_in", std::move(preArgs));

        // API call
        vector<unique_ptr<Expr>> callArgs;
        callArgs.push_back(make_unique<Var>("u"));
        callArgs.push_back(make_unique<Var>("p"));
        auto call = make_unique<FuncCall>("signup", std::move(callArgs));

        // Postcondition: U' = U union {u -> p}
        // Create U'
        vector<unique_ptr<Expr>> primeArgs;
        primeArgs.push_back(make_unique<Var>("U"));
        auto uprimeExpr = make_unique<FuncCall>("'", std::move(primeArgs));

        // Create U union {u -> p}
        vector<unique_ptr<Expr>> unionArgs;
        unionArgs.push_back(make_unique<Var>("U"));

        vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> mapEntries;
        mapEntries.push_back(make_pair(
            make_unique<Var>("u"),
            make_unique<Var>("p")
        ));
        unionArgs.push_back(make_unique<Map>(std::move(mapEntries)));
        auto unionExpr = make_unique<FuncCall>("union", std::move(unionArgs));

        // U' = ...
        vector<unique_ptr<Expr>> eqArgs;
        eqArgs.push_back(std::move(uprimeExpr));
        eqArgs.push_back(std::move(unionExpr));
        auto post = make_unique<FuncCall>("=", std::move(eqArgs));

        auto apiCall = make_unique<APIcall>(
            std::move(call),
            Response(HTTPResponseCode::OK_200, nullptr)
        );

        blocks.push_back(make_unique<API>(
            std::move(pre),
            std::move(apiCall),
            Response(HTTPResponseCode::OK_200, std::move(post))
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
        
        auto* signupTable = new SymbolTable(globalTable);
        string* u = new string("u");
        string* p = new string("p");
        signupTable->addMapping(u, new TypeConst("string"));
        signupTable->addMapping(p, new TypeConst("string"));
        
        globalTable->addChild(signupTable);
        return globalTable;
    }

    TypeMap makeTypeMap() override {
        TypeMap typeMap;
        // Global: U: map<string, string>
        typeMap.setValue("U", new MapType(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        ));
        // Local variables: u, p: string
        typeMap.setValue("u", new TypeConst("string"));
        typeMap.setValue("p", new TypeConst("string"));
        return typeMap;
    }

    void verify(const Program& atc) override {
        // Expected statements:
        // 1. U = {}                    (init)
        // 2. u0 := input()            (input)
        // 3. p0 := input()            (input)
        // 4. assume(not_in(u0, U))    (precondition)
        // 5. U_old = U                (save old state for primed variable)
        // 6. _result0 = signup(u0, p0) (API call)
        // 7. assert(U = U_old union {u0 -> p0}) (postcondition with primes removed)

        cout << "  Generated " << atc.statements.size() << " statements" << endl;
        assert(atc.statements.size() >= 7);

        // Check U_old assignment (statement 4 or 5 depending on implementation)
        bool foundUold = false;
        for (size_t i = 3; i < atc.statements.size(); i++) {
            const Assign* assign = dynamic_cast<const Assign*>(atc.statements[i].get());
            if (assign && assign->left->exprType == ExprType::VAR) {
                const Var* leftVar = dynamic_cast<const Var*>(assign->left.get());
                if (leftVar && leftVar->name == "U_old") {
                    foundUold = true;
                    assert(assign->right->exprType == ExprType::VAR);
                    const Var* rightVar = dynamic_cast<const Var*>(assign->right.get());
                    assert(rightVar->name == "U");
                    cout << "  ✓ U_old = U assignment verified" << endl;
                    break;
                }
            }
        }
        assert(foundUold);

        // Check assert statement exists
        bool foundAssert = false;
        for (size_t i = 0; i < atc.statements.size(); i++) {
            const Assert* assertStmt = dynamic_cast<const Assert*>(atc.statements[i].get());
            if (assertStmt) {
                foundAssert = true;
                cout << "  ✓ Assert statement found" << endl;
                break;
            }
        }
        assert(foundAssert);

        cout << "  ✓ Prime notation handling verified" << endl;
    }
};

/**
 * Test 4: Multiple API blocks (test string)
 * Tests sequential block generation: signup then login
 */
class GenATCTest4 : public GenATCTest {
public:
    GenATCTest4() : GenATCTest("Multiple API blocks - signup then login") {}

protected:
    unique_ptr<Spec> makeSpec() override {
        // Globals
        vector<unique_ptr<Decl>> globals;
        globals.push_back(make_unique<Decl>("U", make_unique<MapType>(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        )));
        globals.push_back(make_unique<Decl>("T", make_unique<MapType>(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        )));

        // Init
        vector<unique_ptr<Init>> inits;
        inits.push_back(make_unique<Init>(
            "U",
            make_unique<Map>(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>())
        ));
        inits.push_back(make_unique<Init>(
            "T",
            make_unique<Map>(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>())
        ));

        vector<unique_ptr<APIFuncDecl>> functions;
        vector<unique_ptr<API>> blocks;

        // Block 1: signup(u, p)
        {
            vector<unique_ptr<Expr>> preArgs;
            preArgs.push_back(make_unique<Var>("u"));
            preArgs.push_back(make_unique<Var>("U"));
            auto pre = make_unique<FuncCall>("not_in", std::move(preArgs));

            vector<unique_ptr<Expr>> callArgs;
            callArgs.push_back(make_unique<Var>("u"));
            callArgs.push_back(make_unique<Var>("p"));
            auto call = make_unique<FuncCall>("signup", std::move(callArgs));

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

        // Block 2: login(u, p)
        {
            vector<unique_ptr<Expr>> preArgs;
            preArgs.push_back(make_unique<Var>("u"));
            preArgs.push_back(make_unique<Var>("U"));
            auto pre = make_unique<FuncCall>("in", std::move(preArgs));

            vector<unique_ptr<Expr>> callArgs;
            callArgs.push_back(make_unique<Var>("u"));
            callArgs.push_back(make_unique<Var>("p"));
            auto call = make_unique<FuncCall>("login", std::move(callArgs));

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
        
        // Symbol table for signup block
        auto* signupTable = new SymbolTable(globalTable);
        string* u1 = new string("u");
        string* p1 = new string("p");
        signupTable->addMapping(u1, new TypeConst("string"));
        signupTable->addMapping(p1, new TypeConst("string"));

        // Symbol table for login block
        auto* loginTable = new SymbolTable(globalTable);
        string* u2 = new string("u");
        string* p2 = new string("p");
        loginTable->addMapping(u2, new TypeConst("string"));
        loginTable->addMapping(p2, new TypeConst("string"));

        globalTable->addChild(signupTable);
        globalTable->addChild(loginTable);
        return globalTable;
    }

    TypeMap makeTypeMap() override {
        TypeMap typeMap;
        // Globals: U, T: map<string, string>
        typeMap.setValue("U", new MapType(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        ));
        typeMap.setValue("T", new MapType(
            make_unique<TypeConst>("string"),
            make_unique<TypeConst>("string")
        ));
        // Local variables: u, p: string
        typeMap.setValue("u", new TypeConst("string"));
        typeMap.setValue("p", new TypeConst("string"));
        return typeMap;
    }

    void verify(const Program& atc) override {
        // Expected:
        // Init: U = {}, T = {}
        // Block 0: u0, p0 inputs, assume, _result0 = signup(u0, p0)
        // Block 1: u1, p1 inputs, assume, _result1 = login(u1, p1)

        cout << "  Generated " << atc.statements.size() << " statements" << endl;
        assert(atc.statements.size() >= 10);

        // Check for u0 and u1 (different suffixes for different blocks)
        bool foundU0 = false, foundU1 = false;
        for (const auto& stmt : atc.statements) {
            const Assign* assign = dynamic_cast<const Assign*>(stmt.get());
            if (assign && assign->left->exprType == ExprType::VAR) {
                const Var* leftVar = dynamic_cast<const Var*>(assign->left.get());
                if (leftVar) {
                    if (leftVar->name == "u0") foundU0 = true;
                    if (leftVar->name == "u1") foundU1 = true;
                }
            }
        }
        assert(foundU0);
        assert(foundU1);
        cout << "  ✓ Variable renaming verified (u0, u1)" << endl;

        // Check for both API calls (LHS is now a Var with tuple type in TypeMap)
        bool foundSignup = false, foundLogin = false;
        for (const auto& stmt : atc.statements) {
            const Assign* assign = dynamic_cast<const Assign*>(stmt.get());
            if (assign && assign->right->exprType == ExprType::FUNCCALL) {
                const FuncCall* call = dynamic_cast<const FuncCall*>(assign->right.get());
                if (call->name == "signup") {
                    foundSignup = true;
                    // Verify Var LHS structure
                    assert(assign->left->exprType == ExprType::VAR);
                }
                if (call->name == "login") {
                    foundLogin = true;
                    // Verify Var LHS structure
                    assert(assign->left->exprType == ExprType::VAR);
                }
            }
        }
        assert(foundSignup);
        assert(foundLogin);
        cout << "  ✓ Both API calls verified (signup, login)" << endl;
    }
};

/**
 * Main test runner
 */
int main() {
    vector<GenATCTest*> testcases = {
        new GenATCTest1(),
        new GenATCTest2(),
        new GenATCTest3(),
        new GenATCTest4()
    };

    cout << "\n========================================" << endl;
    cout << "Running GenATC Test Suite" << endl;
    cout << "========================================" << endl;

    int passed = 0;
    int failed = 0;

    for (auto* test : testcases) {
        try {
            test->execute();
            passed++;
            delete test;
        }
        catch (const exception& e) {
            cout << "✗ Test failed with exception: " << e.what() << endl;
            failed++;
            delete test;
        }
        catch (...) {
            cout << "✗ Test failed with unknown exception" << endl;
            failed++;
            delete test;
        }
    }

    cout << "\n========================================" << endl;
    cout << "Test Results: " << passed << " passed, " << failed << " failed" << endl;
    cout << "========================================" << endl;

    return (failed == 0) ? 0 : 1;
}
