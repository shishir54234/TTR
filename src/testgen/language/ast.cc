#include "ast.hh"

TypeExpr::TypeExpr(TypeExprType typeExprType) : typeExprType(typeExprType) {}

FuncDecl::FuncDecl(string name, 
    unique_ptr<TypeExpr> param,
    unique_ptr<TypeExpr> outp) : name(std::move(name)),
        params(std::move(param)), outp(std::move(outp)) {
}
/*
TypeConst::TypeConst(string n) : TypeExpr(TypeExprType::TYPE_CONST), name(n) {
}

void TypeConst::accept(ASTVisitor &visitor) {
}

string TypeConst::toString() {
	return name;
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

void FuncType::accept(ASTVisitor &visitor) {
}

unique_ptr<TypeExpr> FuncType::clone() {
    vector<unique_ptr<TypeExpr>> clonedParams;
    for (auto &param : params)
    {
        clonedParams.push_back(param->clone());
    }
    auto clonedReturnType = returnType ? returnType->clone() : nullptr;
    return make_unique<FuncType>(std::move(clonedParams), std::move(clonedReturnType));
}

string FuncType::toString() {
	return "Function type";
}

MapType::MapType(unique_ptr<TypeExpr> domain, unique_ptr<TypeExpr> range)
    : TypeExpr(TypeExprType::MAP_TYPE),
        domain(std::move(domain)), range(std::move(range)) {
}

void MapType::accept(ASTVisitor &visitor) {
}

unique_ptr<TypeExpr> MapType::clone() {
    auto clonedDomain = domain ? domain->clone() : nullptr;
    auto clonedRange = range ? range->clone() : nullptr;
    return make_unique<MapType>(std::move(clonedDomain), std::move(clonedRange));
}

string MapType::toString() {
	return "Map type";
}

TupleType::TupleType(vector<unique_ptr<TypeExpr>> elements)
    : TypeExpr(TypeExprType::TUPLE_TYPE), elements(std::move(elements)) {}

void TupleType::accept(ASTVisitor &visitor) {
}

// Clone implementation for TupleType
unique_ptr<TypeExpr> TupleType::clone() {
    vector<unique_ptr<TypeExpr>> clonedElements;
    for ( auto &element : elements)
    {
        clonedElements.push_back(element->clone());
    }
    return make_unique<TupleType>(std::move(clonedElements));
}

string TupleType::toString() {
	return "Tuple type";
}

SetType::SetType(unique_ptr<TypeExpr> elementType)
    : TypeExpr(TypeExprType::SET_TYPE), elementType(std::move(elementType)) {}

void SetType::accept(ASTVisitor &visitor) {
}

unique_ptr<TypeExpr> SetType::clone() {
    auto clonedElementType = elementType ? elementType->clone() : nullptr;
    return make_unique<SetType>(std::move(clonedElementType));
}

string SetType::toString() {
	return "Set type";
}

Decl::Decl(string name, unique_ptr<TypeExpr> typeExpr)
    : name(std::move(name)), type(std::move(typeExpr)) {}

void Decl::accept(ASTVisitor &visitor) {
}

// Copy constructor for deep copying.
Decl::Decl( Decl &other) : name(other.name) {
    if (other.type)
    {
        type = other.type->clone();
    }
}

Expr::Expr(ExprType exprType) : exprType(exprType) {}

Var::Var(string name) : Expr(ExprType::VAR), name(std::move(name)) {}

void Var::accept(ASTVisitor &visitor) {
}

bool Var::operator<(const Var &v) const {
    return name < v.name;
}

unique_ptr<Expr> Var::clone() const {
    return make_unique<Var>(name);
}

FuncCall::FuncCall(string name, vector<unique_ptr<Expr>> args)
    : Expr(ExprType::FUNCCALL),
      name(std::move(name)), args(std::move(args)) {
}

void FuncCall::accept(ASTVisitor &visitor) {
}

unique_ptr<Expr> FuncCall::clone() const {
    vector<unique_ptr<Expr>>clonedArgs;
    for(auto &arg:args){
        clonedArgs.push_back(arg->clone());
    }

    return make_unique<FuncCall>(name,std::move(clonedArgs));
}

Num::Num(int value) : Expr(ExprType::NUM), value(value) {}

void Num::accept(ASTVisitor &visitor) {
}

unique_ptr<Expr> Num::clone() const {
    return make_unique<Num>(value);
}

String::String(string value) : Expr(ExprType::STRING), value(value) {}

void String::accept(ASTVisitor &visitor) {
}

unique_ptr<Expr> String::clone() const {
    return make_unique<String>(value);
}

Set::Set(vector<unique_ptr<Expr>> elements)
    : Expr(ExprType::SET), elements(std::move(elements)) {}

void Set::accept(ASTVisitor &visitor) {
}

unique_ptr<Expr> Set::clone() const {
    vector<unique_ptr<Expr>> clonedElements;
    for(auto &element:elements){
        clonedElements.push_back(element->clone());
    }

    return make_unique<Set>(std::move(clonedElements));
}

Map::Map(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> v)
    : Expr(ExprType::MAP), value(std::move(v)) {}

void Map::accept(ASTVisitor &visitor) {
}

unique_ptr<Expr> Map::clone() const {
    vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> clonedValue;

    for (const auto& element : value) {
        auto clonedExpr = element.first->clone();
        auto rawPtr = dynamic_cast<Var*>(clonedExpr.get());

        if (!rawPtr) {
            throw runtime_error("Map key is not of type Var");
        }

        unique_ptr<Var> clonedVar(static_cast<Var*>(clonedExpr.release()));
        // unique_ptr<Var> clonedVar(clonedExpr);
        unique_ptr<Expr> clonedExprValue = element.second->clone();

        clonedValue.emplace_back(std::move(clonedVar), std::move(clonedExprValue));
    }

    return make_unique<Map>(std::move(clonedValue));
}

Tuple::Tuple(vector<unique_ptr<Expr>> exprs) :
    Expr(ExprType::TUPLE), exprs(std::move(exprs)) {}

void Tuple::accept(ASTVisitor &visitor) {
}

unique_ptr<Expr> Tuple::clone() const {
    vector<unique_ptr<Expr>> cloneexprs;
    for(auto &e:exprs){
        cloneexprs.push_back(e->clone());
    }
    return make_unique<Tuple>(std::move(cloneexprs));
}

APIFuncDecl::APIFuncDecl(string name,
         vector<unique_ptr<TypeExpr>> params,
         pair<HTTPResponseCode, vector<unique_ptr<TypeExpr>>> returnType)
    : name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)) {}

void APIFuncDecl::accept(ASTVisitor &visitor) {
}

Init::Init(string varName, unique_ptr<Expr> expression)
    : varName(std::move(varName)), expr(std::move(expression)) {}

void Init::accept(ASTVisitor &visitor) {
}

Response::Response(HTTPResponseCode code, unique_ptr<Expr> expr) :
    code(code), expr(std::move(expr)) {}

void Response::accept(ASTVisitor &visitor) {
}

APIcall::APIcall(unique_ptr<FuncCall> Call, Response Response) :
	call(std::move(Call)), response(std::move(Response)) {}

API::API(unique_ptr<Expr> precondition,
    unique_ptr<APIcall> functionCall,
    Response response)
    : pre(std::move(precondition)), call(std::move(functionCall)),
      response(std::move(response)) {}

void API::accept(ASTVisitor &visitor) {
}

Spec::Spec(vector<unique_ptr<Decl>> globals,
     vector<unique_ptr<Init>> init,
     vector<unique_ptr<APIFuncDecl>> functions,
     vector<unique_ptr<API>> blocks)
    : globals(std::move(globals)), init(std::move(init)),
	functions(std::move(functions)), blocks(std::move(blocks)) {
}

void Spec::accept(ASTVisitor &visitor) {
}

Stmt::Stmt(StmtType type) : statementType(type) {}

Assign::Assign(unique_ptr<Var> left, unique_ptr<Expr> right)
    : Stmt(StmtType::ASSIGN), left(std::move(left)), right(std::move(right)) {}

void Assign::accept(ASTVisitor &visitor) {}

unique_ptr<Stmt> Assign::clone() const {
    // Clone the left-hand side; left->clone() returns a unique_ptr<Expr>.
    unique_ptr<Expr> leftCloneBase = left->clone();
    // Cast the result to a unique_ptr<Var>.
    Var* leftCloneRaw = dynamic_cast<Var*>(leftCloneBase.release());
    if (!leftCloneRaw) {
        throw runtime_error("Clone of left-hand side did not produce a Var instance");
    }
    unique_ptr<Var> clonedLeft(leftCloneRaw);

    // Clone the right-hand side normally (returns a unique_ptr<Expr>).
    unique_ptr<Expr> clonedRight = right->clone();

    // Return a new Assign node constructed with the cloned children.
    return make_unique<Assign>(std::move(clonedLeft), std::move(clonedRight));
}

Assume::Assume(unique_ptr<Expr> e) : Stmt(StmtType::ASSUME), expr(std::move(e)) {}

void Assume::accept(ASTVisitor &visitor) {}

unique_ptr<Stmt> Assume::clone() const {
    // Clone the right-hand side normally (returns a unique_ptr<Expr>).
    unique_ptr<Expr> clonedExpr = expr->clone();

    // Return a new Assign node constructed with the cloned children.
    return make_unique<Assume>(std::move(clonedExpr));
}

FuncCallStmt::FuncCallStmt(unique_ptr<FuncCall> call)
    : Stmt(StmtType::FUNCTIONCALL_STMT), call(std::move(call)) {}

void FuncCallStmt::accept(ASTVisitor &visitor) {
}

unique_ptr<Stmt> FuncCallStmt::clone() const {
    // Clone the function call; call->clone() returns a unique_ptr<Expr>.
    unique_ptr<Expr> callCloneBase = call->clone();
    // Cast the result to a unique_ptr<FuncCall>.
    FuncCall* callCloneRaw = dynamic_cast<FuncCall*>(callCloneBase.release());
    if (!callCloneRaw) {
        throw runtime_error("Clone of FuncCall did not produce a FuncCall instance");
    }
    unique_ptr<FuncCall> clonedCall(callCloneRaw);

    // Return a new FuncCallStmt node with the cloned FuncCall.
    return make_unique<FuncCallStmt>(std::move(clonedCall));
}

Program::Program(vector<unique_ptr<Stmt>> Statements)
    : statements(std::move(Statements)) {}

void Program::accept(ASTVisitor &visitor) {
}
