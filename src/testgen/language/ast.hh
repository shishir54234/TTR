#ifndef AST_HH
#define AST_HH

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "astvisitor.hh"

using namespace std;

enum class HTTPResponseCode
{
    OK_200,
    CREATED_201,
    BAD_REQUEST_400,
    // Add other response codes as needed
};

enum class ExprType
{
    INPUT,
    FUNCCALL,
    MAP,
    NUM,
    POLYFUNCCALL,
    SET,
    STRING,
    SYMVAR,
    TUPLE,
    VAR
};

enum class TypeExprType
{
    TYPE_CONST,
    TYPE_VARIABLE,
    FUNC_TYPE,
    MAP_TYPE,
    SET_TYPE,
    TUPLE_TYPE
};

enum class StmtType
{
    ASSIGN,
    ASSUME,
    FUNCTIONCALL_STMT,
    DECL,
};

class TypeExpr
{
public:
    TypeExprType typeExprType;
public:
    virtual ~TypeExpr() = default;
    virtual void accept(ASTVisitor &visitor) = 0;
    virtual unique_ptr<TypeExpr> clone() = 0;
    virtual string toString() = 0;

protected:
    TypeExpr(TypeExprType);
};

class FuncDecl
{
public:
    const string name;
    const unique_ptr<TypeExpr> params;
    const unique_ptr<TypeExpr> outp;

    FuncDecl(string, unique_ptr<TypeExpr>, unique_ptr<TypeExpr>);
};

// Type Expressions
/*
class TypeConst : public TypeExpr
{
public:
    const string name;
public:
    explicit TypeConst(string);
    void accept(ASTVisitor &) override;
    unique_ptr<TypeExpr> clone() override;
    virtual string toString();
};
*/

class FuncType : public TypeExpr
{
public:
    const vector<unique_ptr<TypeExpr>> params;
    const unique_ptr<TypeExpr> returnType;
public:
    FuncType(vector<unique_ptr<TypeExpr>>, unique_ptr<TypeExpr>);
    void accept(ASTVisitor &) override;
    unique_ptr<TypeExpr> clone() override;
    virtual string toString();
};

class MapType : public TypeExpr
{
public:
    const unique_ptr<TypeExpr> domain;
    const unique_ptr<TypeExpr> range;
public:
    MapType(unique_ptr<TypeExpr>, unique_ptr<TypeExpr>);
    void accept(ASTVisitor &) override;
    unique_ptr<TypeExpr> clone() override;
    virtual string toString();
};

class TupleType : public TypeExpr
{
public:
    const vector<unique_ptr<TypeExpr>> elements;
public:
    explicit TupleType(vector<unique_ptr<TypeExpr>>); 
    void accept(ASTVisitor &) override;
    unique_ptr<TypeExpr> clone() override;
    virtual string toString();
};

class SetType : public TypeExpr
{
public:
    unique_ptr<TypeExpr> elementType;
public:
    explicit SetType(unique_ptr<TypeExpr>);
    void accept(ASTVisitor &) override;
    unique_ptr<TypeExpr> clone()  override;
    virtual string toString();
};

class Decl
{
public:
    string name;
    unique_ptr<TypeExpr> type;
public:
    Decl(string, unique_ptr<TypeExpr>);
    virtual ~Decl() = default;
    virtual void accept(ASTVisitor &);
    Decl( Decl &);
//    virtual unique_ptr<Decl> clone();
};

// Expressions
class Expr
{
public:
    ExprType exprType;

protected:
    Expr(ExprType);

public:
    virtual ~Expr() = default;
    virtual void accept(ASTVisitor&) = 0;
    virtual unique_ptr<Expr> clone() const = 0;
};

class Input : public Expr {
public:
    virtual void accept(ASTVisitor&);
    virtual unique_ptr<Expr> clone();
};

class FuncCall : public Expr
{
public:
    const string name;
    const vector<unique_ptr<Expr>> args;
public:
    FuncCall(string, vector<unique_ptr<Expr>>);
    void accept(ASTVisitor &)  override;
    unique_ptr<Expr> clone() const override;
};

class Map : public Expr
{
public:
    const vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> value;
public:
    explicit Map(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>);
    void accept(ASTVisitor &visitor)  override;
    unique_ptr<Expr> clone() const override;
};

class Num : public Expr
{
public:
    const int value;
public:
    explicit Num(int);
    void accept(ASTVisitor &) override;
    unique_ptr<Expr> clone() const override;
};

class Set : public Expr
{
public:
    const vector<unique_ptr<Expr>> elements;
public:
    explicit Set(vector<unique_ptr<Expr>>);
    void accept(ASTVisitor &visitor)  override;
    unique_ptr<Expr> clone() const override;

};

class String : public Expr
{
public:
    const string value;
public:
    explicit String(string);
    void accept(ASTVisitor &) override;
    unique_ptr<Expr> clone() const override;
};

class Tuple : public Expr
{
public:
    const vector<unique_ptr<Expr>> exprs;
public:
    explicit Tuple(vector<unique_ptr<Expr>> exprs);
    void accept(ASTVisitor &visitor)  override;
    unique_ptr<Expr> clone() const override;
};

class Var : public Expr
{
public:
    const string name;
public:
    explicit Var(string);
    void accept(ASTVisitor &)  override;
    bool operator<(const Var &v) const;
    unique_ptr<Expr> clone() const override;
};

// Function Declaration
class APIFuncDecl
{
public:
    const string name;
    const vector<unique_ptr<TypeExpr>> params;
    const pair<HTTPResponseCode, vector<unique_ptr<TypeExpr>>> returnType;
public:
    APIFuncDecl(string,
             vector<unique_ptr<TypeExpr>>,
             pair<HTTPResponseCode, vector<unique_ptr<TypeExpr>>>);
    void accept(ASTVisitor &);
};

// Initialization
class Init
{
public:
    const string varName;
    const unique_ptr<Expr> expr;
public:
    Init(string, unique_ptr<Expr>);
    void accept(ASTVisitor &);
};

class Response
{
public:
    HTTPResponseCode code;
    unique_ptr<Expr> expr;
public:
    Response(HTTPResponseCode, unique_ptr<Expr>);
    void accept(ASTVisitor &);
};

class APIcall
{
public:
    const unique_ptr<FuncCall> call;
    const Response response;
    void accept(ASTVisitor &);

public:
    APIcall(unique_ptr<FuncCall>, Response);
};

// API
class API
{
public:
    unique_ptr<Expr> pre;
    unique_ptr<APIcall> call;
    Response response;
public:
    API(unique_ptr<Expr>, unique_ptr<APIcall>, Response);
    void accept(ASTVisitor &);
};

// Block class (placeholder as it wasn't fully specified in the grammar)
// Top-level Spec class
class Spec
{
public:
    const vector<unique_ptr<Decl>> globals;
    const vector<unique_ptr<Init>> init;
    const vector<unique_ptr<APIFuncDecl>> functions;
    const vector<unique_ptr<API>> blocks;
public:
    Spec(vector<unique_ptr<Decl>>,
         vector<unique_ptr<Init>>,
         vector<unique_ptr<APIFuncDecl>>,
         vector<unique_ptr<API>>);
    void accept(ASTVisitor &);
};

class Stmt
{
public:
    const StmtType statementType;
public:
    virtual ~Stmt() = default;
    virtual void accept(ASTVisitor &) = 0;
    virtual unique_ptr<Stmt> clone() const = 0;
protected:
    Stmt(StmtType);
};

// Assignment statement: l = r
class Assign : public Stmt
{
public:
    const unique_ptr<Var> left;
    const unique_ptr<Expr> right;
public:
    Assign(unique_ptr<Var>, unique_ptr<Expr>);
    void accept(ASTVisitor &visitor)  override;
    unique_ptr<Stmt> clone() const override;
};

// Assume statement: assume(c)
class Assume : public Stmt
{
public:
    const unique_ptr<Expr> expr;
public:
    Assume(unique_ptr<Expr>);
    void accept(ASTVisitor &visitor)  override;
    unique_ptr<Stmt> clone() const override;
};

// Function call statement
class FuncCallStmt : public Stmt
{
public:
    const unique_ptr<FuncCall> call;
public:
    explicit FuncCallStmt(unique_ptr<FuncCall>);
    void accept(ASTVisitor &visitor)  override;
    unique_ptr<Stmt> clone() const override;
};

// Program is the root of our AST
class Program
{
public:
    const vector<unique_ptr<Stmt>> statements;
public:
    explicit Program(vector<unique_ptr<Stmt>>);
    void accept(ASTVisitor &);
};
#endif
