#include "ast.hh"

TypeExpr::TypeExpr(TypeExprType typeExprType) : typeExprType(typeExprType) {}

FuncDecl::FuncDecl(string name,
    unique_ptr<TypeExpr> param,
    unique_ptr<TypeExpr> outp) : name(std::move(name)),
        params(std::move(param)), outp(std::move(outp)) {
}

TypeConst::TypeConst(string n) : TypeExpr(TypeExprType::TYPE_CONST), name(n) {
}
string TypeConst::toString() {
	return "TYPE_CONST{" + name + "}";
}

/*
void TypeConst::accept(ASTVisitor &visitor) {
}



unique_ptr<TypeExpr> TypeConst::clone() {
    return make_unique<TypeConst>(typeExprType);
}
*/
FuncType::FuncType(vector<unique_ptr<TypeExpr>> params,
    unique_ptr<TypeExpr> returnType)
    : TypeExpr(TypeExprType::FUNC_TYPE), params(std::move(params)),
        returnType(std::move(returnType)) {
}

string FuncType::toString() {
	return "Function type";
}

MapType::MapType(unique_ptr<TypeExpr> domain, unique_ptr<TypeExpr> range)
    : TypeExpr(TypeExprType::MAP_TYPE),
        domain(std::move(domain)), range(std::move(range)) {
}

string MapType::toString() {
	return "Map type";
}

TupleType::TupleType(vector<unique_ptr<TypeExpr>> elements)
    : TypeExpr(TypeExprType::TUPLE_TYPE), elements(std::move(elements)) {}

string TupleType::toString() {
	return "Tuple type";
}

SetType::SetType(unique_ptr<TypeExpr> elementType)
    : TypeExpr(TypeExprType::SET_TYPE), elementType(std::move(elementType)) {}

string SetType::toString() {
	return "Set type";
}

Decl::Decl(string name, unique_ptr<TypeExpr> typeExpr)
    : name(std::move(name)), type(std::move(typeExpr)) {}

Expr::Expr(ExprType exprType) : exprType(exprType) {}

Input::Input() : Expr(ExprType::INPUT) {}

Var::Var(string name) : Expr(ExprType::VAR), name(std::move(name)) {}

bool Var::operator<(const Var &v) const {
    return name < v.name;
}

FuncCall::FuncCall(string name, vector<unique_ptr<Expr>> args)
    : Expr(ExprType::FUNCCALL),
      name(std::move(name)), args(std::move(args)) {
}

Num::Num(int value) : Expr(ExprType::NUM), value(value) {}

String::String(string value) : Expr(ExprType::STRING), value(value) {}

Set::Set(vector<unique_ptr<Expr>> elements)
    : Expr(ExprType::SET), elements(std::move(elements)) {}

Map::Map(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> v)
    : Expr(ExprType::MAP), value(std::move(v)) {}

Tuple::Tuple(vector<unique_ptr<Expr>> exprs) :
    Expr(ExprType::TUPLE), exprs(std::move(exprs)) {}

APIFuncDecl::APIFuncDecl(string name,
         vector<unique_ptr<TypeExpr>> params,
         pair<HTTPResponseCode, vector<unique_ptr<TypeExpr>>> returnType)
    : name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)) {}

Init::Init(string varName, unique_ptr<Expr> expression)
    : varName(std::move(varName)), expr(std::move(expression)) {}

Response::Response(unique_ptr<Expr> expr) {
    // Build ResponseExpr that includes the response code as part of the expression
    // Format: _RESPONSE_CODE_200, _RESPONSE_CODE_201, etc.
    ResponseExpr = std::move(expr);
}

APIcall::APIcall(unique_ptr<FuncCall> Call, Response Response) :
	call(std::move(Call)), response(std::move(Response)) {}

API::API(unique_ptr<Expr> precondition,
    unique_ptr<APIcall> functionCall,
    Response response, string name)
    : pre(std::move(precondition)), call(std::move(functionCall)),
      response(std::move(response)), name(name) {}

Spec::Spec(vector<unique_ptr<Decl>> globals,
     vector<unique_ptr<Init>> init,
     vector<unique_ptr<APIFuncDecl>> functions,
     vector<unique_ptr<API>> blocks)
    : globals(std::move(globals)), init(std::move(init)),
	functions(std::move(functions)), blocks(std::move(blocks)) {
}

Stmt::Stmt(StmtType type) : statementType(type) {}

Assign::Assign(unique_ptr<Expr> left, unique_ptr<Expr> right)
    : Stmt(StmtType::ASSIGN), left(std::move(left)), right(std::move(right)) {}

Assume::Assume(unique_ptr<Expr> e) : Stmt(StmtType::ASSUME), expr(std::move(e)) {}

Assert::Assert(unique_ptr<Expr> e) : Stmt(StmtType::ASSERT), expr(std::move(e)) {}

Program::Program(vector<unique_ptr<Stmt>> Statements)
    : statements(std::move(Statements)) {}
