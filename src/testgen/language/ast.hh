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
    DECL
};

class TypeExpr
{
public:
    TypeExprType typeExprType;
public:
    virtual ~TypeExpr() = default;
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
class TypeConst : public TypeExpr
{
public:
    const string name;
public:
    TypeConst(string name);
    virtual string toString();
};


class FuncType : public TypeExpr
{
public:
    const vector<unique_ptr<TypeExpr>> params;
    const unique_ptr<TypeExpr> returnType;
public:
    FuncType(vector<unique_ptr<TypeExpr>>, unique_ptr<TypeExpr>);
    virtual string toString();
};

class MapType : public TypeExpr
{
public:
    const unique_ptr<TypeExpr> domain;
    const unique_ptr<TypeExpr> range;
public:
    MapType(unique_ptr<TypeExpr>, unique_ptr<TypeExpr>);
    virtual string toString();
};

class TupleType : public TypeExpr
{
public:
    const vector<unique_ptr<TypeExpr>> elements;
public:
    explicit TupleType(vector<unique_ptr<TypeExpr>>);
    virtual string toString();
};

class SetType : public TypeExpr
{
public:
    unique_ptr<TypeExpr> elementType;
public:
    explicit SetType(unique_ptr<TypeExpr>);
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
};

class Input : public Expr {
public:
    Input();
};

class FuncCall : public Expr
{
public:
    const string name;
    const vector<unique_ptr<Expr>> args;
public:
    FuncCall(string, vector<unique_ptr<Expr>>);
};

class Map : public Expr
{
public:
    const vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> value;
public:
    explicit Map(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>);
};

class Num : public Expr
{
public:
    const int value;
public:
    explicit Num(int);
};

class Set : public Expr
{
public:
    const vector<unique_ptr<Expr>> elements;
public:
    explicit Set(vector<unique_ptr<Expr>>);
};

class String : public Expr
{
public:
    const string value;
public:
    explicit String(string);
};

class Tuple : public Expr
{
public:
    const vector<unique_ptr<Expr>> exprs;
public:
    explicit Tuple(vector<unique_ptr<Expr>> exprs);
};

class Var : public Expr
{
public:
    string name;
public:
    explicit Var(string);
    bool operator<(const Var &v) const;
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
};

// Initialization
class Init
{
public:
    const string varName;
    const unique_ptr<Expr> expr;
public:
    Init(string, unique_ptr<Expr>);
};

class Response
{
public:
    HTTPResponseCode code;
    unique_ptr<Expr> expr;
public:
    Response(HTTPResponseCode, unique_ptr<Expr>);
};

class APIcall
{
public:
    const unique_ptr<FuncCall> call;
    const Response response;

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
};

class Stmt
{
public:
    const StmtType statementType;
public:
    virtual ~Stmt() = default;
protected:
    Stmt(StmtType);
};

// Assignment statement: l = r
class Assign : public Stmt
{
public:
    const unique_ptr<Expr> left;  // Changed from Var to Expr to support tuples
    const unique_ptr<Expr> right;
public:
    Assign(unique_ptr<Expr>, unique_ptr<Expr>);
};

// Assume statement: assume(c)
class Assume : public Stmt
{
public:
    const unique_ptr<Expr> expr;
public:
    Assume(unique_ptr<Expr>);
};

// Assert statement: assert(c)
class Assert : public Stmt
{
public:
    const unique_ptr<Expr> expr;
public:
    Assert(unique_ptr<Expr>);
};

//    is the root of our AST
class Program
{
public:
    const vector<unique_ptr<Stmt>> statements;
public:
    explicit Program(vector<unique_ptr<Stmt>>);
};
#endif
