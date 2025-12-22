#pragma once
#include <iostream>
#include <stdexcept>
#include <string>

// Forward declarations
class TypeConst;
class FuncType;
class MapType;
class TupleType;
class SetType;
class Expr;
class TypeExpr;
class Var;
class FuncCall;
class Num;
class String;
class Set;
class Map;
class Tuple;
class PolymorphicFuncCall;
class Decl;
class FuncDecl;
class APIcall;
class Response;
class API;
class Init;
class Spec;

class Stmt;
class Assign;
class Assume;

class Program;

// Base visitor with centralized dispatch logic (similar to Java Visitor pattern)
class ASTVisitor
{
public:
    virtual ~ASTVisitor() = default;

    // Centralized visit method that dispatches to specific handlers
    // This is the main entry point for visiting any node
    void visit(const TypeExpr* node);
    void visit(const Expr* node);
    void visit(const Stmt* node);

protected:
    // Type Expression visitors - to be implemented by concrete visitors
    virtual void visitTypeConst(const TypeConst &node) = 0;
    virtual void visitFuncType(const FuncType &node) = 0;
    virtual void visitMapType(const MapType &node) = 0;
    virtual void visitTupleType(const TupleType &node) = 0;
    virtual void visitSetType(const SetType &node) = 0;

    // Expression visitors - to be implemented by concrete visitors
    virtual void visitVar(const Var &node) = 0;
    virtual void visitFuncCall(const FuncCall &node) = 0;
    virtual void visitNum(const Num &node) = 0;
    virtual void visitString(const String &node) = 0;
    virtual void visitSet(const Set &node) = 0;
    virtual void visitMap(const Map &node) = 0;
    virtual void visitTuple(const Tuple &node) = 0;

    // Statement visitors - to be implemented by concrete visitors
    virtual void visitAssign(const Assign &node) = 0;
    virtual void visitAssume(const Assume &node) = 0;

public:
    // High-level visitors for complex structures
    virtual void visitDecl(const Decl &node) = 0;
    virtual void visitAPIcall(const APIcall &node) = 0;
    virtual void visitAPI(const API &node) = 0;
    virtual void visitResponse(const Response &node) = 0;
    virtual void visitInit(const Init &node) = 0;
    virtual void visitSpec(const Spec &node) = 0;
    virtual void visitProgram(const Program &node) = 0;
};
