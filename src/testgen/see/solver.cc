#include "z3++.h"

#include "solver.hh"

ResultValue::ResultValue(ResultType t) : type(t) {
}

BoolResultValue::BoolResultValue(bool v) : ResultValue(ResultType::BOOL), value(v) {
}

IntResultValue::IntResultValue(int v) : ResultValue(ResultType::INT), value(v) {
}

StringResultValue::StringResultValue(const string& v) : ResultValue(ResultType::STRING), value(v) {
}

Result::Result(bool tf, map<string, unique_ptr<ResultValue> > m) : isSat(tf), model(std::move(m)) {
}

