CC=g++ -g
BUILD=build

INC=-I language

LIB=-lz3

$(BUILD)/ast.o : language/ast.cc language/ast.hh language/astvisitor.hh
	$(CC) -c language/ast.cc -o $(BUILD)/ast.o $(INC)

$(BUILD)/env.o : language/env.cc language/env.hh
	$(CC) -c language/env.cc -o $(BUILD)/env.o $(INC)

$(BUILD)/see.o : see/see.cc see/see.hh language/ast.hh language/env.hh see/functionfactory.hh
	$(CC) -c see/see.cc -o $(BUILD)/see.o $(INC)

$(BUILD)/solver.o : see/solver.cc see/solver.hh language/ast.hh
	$(CC) -c see/solver.cc -o $(BUILD)/solver.o $(INC) $(LIB)

$(BUILD)/symvar.o : language/symvar.cc language/symvar.hh language/ast.hh language/astvisitor.hh
	$(CC) -c language/symvar.cc -o $(BUILD)/symvar.o $(INC)

$(BUILD)/tester.o : tester/tester.cc tester/tester.hh language/ast.hh
	$(CC) -c tester/tester.cc -o $(BUILD)/tester.o $(INC) $(LIB)

$(BUILD)/z3solver.o : see/z3solver.cc see/z3solver.hh see/solver.hh language/ast.hh
	$(CC) -c see/z3solver.cc -o $(BUILD)/z3solver.o $(INC) $(LIB)

