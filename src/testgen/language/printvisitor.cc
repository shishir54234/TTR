#include "printvisitor.hh"
#include "symvar.hh"

PrintVisitor::PrintVisitor() : indentLevel(0) {
    updateIndent();
}

// Type Expression visitors
void PrintVisitor::visitTypeConst(const TypeConst &node) {
    cout << node.name;
}

void PrintVisitor::visitFuncType(const FuncType &node) {
    cout << "(";
    for (size_t i = 0; i < node.params.size(); i++) {
        if (i > 0) cout << ", ";
        visit(node.params[i].get());
    }
    cout << ") -> ";
    visit(node.returnType.get());
}

void PrintVisitor::visitMapType(const MapType &node) {
    cout << "map<";
    visit(node.domain.get());
    cout << ", ";
    visit(node.range.get());
    cout << ">";
}

void PrintVisitor::visitTupleType(const TupleType &node) {
    cout << "(";
    for (size_t i = 0; i < node.elements.size(); i++) {
        if (i > 0) cout << ", ";
        visit(node.elements[i].get());
    }
    cout << ")";
}

void PrintVisitor::visitSetType(const SetType &node) {
    cout << "set<";
    visit(node.elementType.get());
    cout << ">";
}

// Expression visitors
void PrintVisitor::visitVar(const Var &node) {
    cout << node.name;
}

void PrintVisitor::visitFuncCall(const FuncCall &node) {
    cout << node.name << "(";
    for (size_t i = 0; i < node.args.size(); i++) {
        if (i > 0) cout << ", ";
        visit(node.args[i].get());
    }
    cout << ")";
}

void PrintVisitor::visitNum(const Num &node) {
    cout << node.value;
}

void PrintVisitor::visitString(const String &node) {
    cout << "\"" << node.value << "\"";
}

void PrintVisitor::visitSet(const Set &node) {
    cout << "{";
    for (size_t i = 0; i < node.elements.size(); i++) {
        if (i > 0) cout << ", ";
        visit(node.elements[i].get());
    }
    cout << "}";
}

void PrintVisitor::visitMap(const Map &node) {
    cout << "{";
    for (size_t i = 0; i < node.value.size(); i++) {
        if (i > 0) cout << ", ";
        visit(node.value[i].first.get());
        cout << " -> ";
        visit(node.value[i].second.get());
    }
    cout << "}";
}

void PrintVisitor::visitTuple(const Tuple &node) {
    cout << "(";
    for (size_t i = 0; i < node.exprs.size(); i++) {
        if (i > 0) cout << ", ";
        visit(node.exprs[i].get());
    }
    cout << ")";
}

// Statement visitors
void PrintVisitor::visitAssign(const Assign &node) {
    visit(node.left.get());
    cout << " := ";
    visit(node.right.get());
}

void PrintVisitor::visitAssume(const Assume &node) {
    cout << "assume(";
    visit(node.expr.get());
    cout << ")";
}

// High-level visitors
void PrintVisitor::visitDecl(const Decl &node) {
    cout << node.name << ": ";
    visit(node.type.get());
}

void PrintVisitor::visitAPIcall(const APIcall &node) {
    visit(node.call.get());
    cout << " -> ";
    visitResponse(node.response);
}

void PrintVisitor::visitAPI(const API &node) {
    cout << "API {" << endl;
    indent();
    
    printIndent();
    cout << "pre: ";
    visit(node.pre.get());
    cout << endl;
    
    printIndent();
    cout << "call: ";
    visitAPIcall(*node.call);
    cout << endl;
    
    printIndent();
    cout << "post: ";
    visitResponse(node.response);
    cout << endl;
    
    dedent();
    printIndent();
    cout << "}";
}

void PrintVisitor::visitResponse(const Response &node) {
    // Print HTTP response code
    cout << "Response(";
    switch (node.code) {
        case HTTPResponseCode::OK_200:
            cout << "200";
            break;
        case HTTPResponseCode::CREATED_201:
            cout << "201";
            break;
        case HTTPResponseCode::BAD_REQUEST_400:
            cout << "400";
            break;
        default:
            cout << "???";
            break;
    }
    // Print expression if present
    if (node.expr) {
        cout << ", ";
        visit(node.expr.get());
    }
    cout << ")";
}

void PrintVisitor::visitInit(const Init &node) {
    cout << node.varName << " := ";
    visit(node.expr.get());
}

void PrintVisitor::visitSpec(const Spec &node) {
    cout << "=== Spec ===" << endl;
    
    cout << "Globals:" << endl;
    for (const auto& g : node.globals) {
        printIndent();
        visitDecl(*g);
        cout << endl;
    }
    
    cout << "Init:" << endl;
    for (const auto& i : node.init) {
        printIndent();
        visitInit(*i);
        cout << endl;
    }
    
    cout << "Blocks:" << endl;
    for (const auto& b : node.blocks) {
        visitAPI(*b);
        cout << endl;
    }
    
    cout << "=== End Spec ===" << endl;
}

void PrintVisitor::visitProgram(const Program &node) {
    cout << "=== Program ===" << endl;
    for (size_t i = 0; i < node.statements.size(); i++) {
        cout << "Statement " << i << ": ";
        printStmt(node.statements[i].get());
        cout << endl;
    }
    cout << "=== End Program ===" << endl;
}

// Convenience methods
void PrintVisitor::printExpr(const Expr* expr) {
    if (!expr) {
        cout << "null";
        return;
    }
    visit(expr);
}

void PrintVisitor::printStmt(const Stmt* stmt) {
    if (!stmt) {
        cout << "null";
        return;
    }
    
    // Check if it's an Assert (not in base visitor yet)
    // Assert uses ASSUME type but is a different class
    const Assert* assertStmt = dynamic_cast<const Assert*>(stmt);
    if (assertStmt) {
        cout << "assert(";
        if (assertStmt->expr) {
            visit(assertStmt->expr.get());
        }
        cout << ")";
        return;
    }
    
    // Try regular visitor dispatch
    try {
        visit(stmt);
    } catch (const std::runtime_error& e) {
        cout << "UnknownStmt";
    }
}

void PrintVisitor::printTypeExpr(const TypeExpr* type) {
    if (!type) {
        cout << "null";
        return;
    }
    visit(type);
}
