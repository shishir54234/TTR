CC=g++
CCFLAGS= -g
BUILD=build
TEST=test
BIN=bin
INC=-I language
INC_SYM=-I see
LIB=-lz3

# Common object file dependencies
COMMON_OBJS=$(BUILD)/ast.o $(BUILD)/astvisitor.o $(BUILD)/env.o $(BUILD)/symvar.o $(BUILD)/clonevisitor.o $(BUILD)/printvisitor.o 
SEE_OBJS=$(BUILD)/see.o $(BUILD)/z3solver.o $(BUILD)/solver.o
TEST_OBJS=$(BUILD)/test_utils.o
TESTER_OBJS=$(BUILD)/tester.o
GENATC_OBJS=$(BUILD)/genATC.o
APP_OBJS=$(BUILD)/app1.o
# All dependencies for tests
ALL_TEST_DEPS=$(TEST_OBJS) $(SEE_OBJS) $(COMMON_OBJS) $(APP_OBJS) $(BUILD)/typemap.o
# --------------------------------------------------
#  Object build rules (only what is actually used)
# --------------------------------------------------
$(BUILD)/app1.o : apps/app1/app1.cc apps/app1/app1.hh language/ast.hh language/astvisitor.hh language/symvar.hh language/clonevisitor.hh
	$(CC) $(CCFLAGS) -c apps/app1/app1.cc -o $@ $(INC) $(INC_SYM) 


$(BUILD)/astvisitor.o : language/astvisitor.cc language/astvisitor.hh language/ast.hh
	$(CC) $(CCFLAGS) -c language/astvisitor.cc -o $@ $(INC)

$(BUILD)/ast.o : $(BUILD)/astvisitor.o language/ast.cc language/ast.hh language/astvisitor.hh
	$(CC) $(CCFLAGS) -c language/ast.cc -o $@ $(INC)

$(BUILD)/env.o : language/env.cc language/env.hh
	$(CC) $(CCFLAGS) -c language/env.cc -o $@ $(INC)

$(BUILD)/symvar.o : language/symvar.cc language/symvar.hh language/ast.hh language/astvisitor.hh
	$(CC) $(CCFLAGS) -c language/symvar.cc -o $@ $(INC)

$(BUILD)/clonevisitor.o : language/clonevisitor.cc language/clonevisitor.hh language/ast.hh language/astvisitor.hh language/symvar.hh
	$(CC) $(CCFLAGS) -c language/clonevisitor.cc -o $@ $(INC)

$(BUILD)/printvisitor.o : language/printvisitor.cc language/printvisitor.hh language/ast.hh language/astvisitor.hh language/symvar.hh
	$(CC) $(CCFLAGS) -c language/printvisitor.cc -o $@ $(INC)

$(BUILD)/solver.o : see/solver.cc see/solver.hh language/ast.hh
	$(CC) $(CCFLAGS) -c see/solver.cc -o $@ $(INC) $(LIB)

$(BUILD)/see.o : see/see.cc see/see.hh language/ast.hh language/env.hh see/functionfactory.hh language/clonevisitor.hh
	$(CC) $(CCFLAGS) -c see/see.cc -o $@ $(INC)

$(BUILD)/z3solver.o : see/z3solver.cc see/z3solver.hh see/solver.hh language/ast.hh language/symvar.hh
	$(CC) $(CCFLAGS) -c see/z3solver.cc -o $@ $(INC) $(LIB)

$(BUILD)/tester.o : tester/tester.cc tester/tester.hh language/ast.hh language/clonevisitor.hh
	$(CC) $(CCFLAGS) -c tester/tester.cc -o $@ $(INC) $(LIB)

$(BUILD)/test_utils.o : tester/test_utils.cc tester/test_utils.hh see/see.hh see/z3solver.hh
	$(CC) $(CCFLAGS) -c tester/test_utils.cc -o $@ $(INC) $(LIB)

$(BUILD)/typemap.o : language/typemap.cc language/typemap.hh language/ast.hh
	$(CC) $(CCFLAGS) -c language/typemap.cc -o $@ $(INC)

$(BUILD)/genATC.o : tester/genATC.cc tester/genATC.hh language/ast.hh language/env.hh language/typemap.hh
	$(CC) $(CCFLAGS) -c tester/genATC.cc -o $@ $(INC)


# --------------------------------------------------
#  Test object files
# --------------------------------------------------
$(BUILD)/test_see.o : $(TEST)/test_see/test_see.cc tester/test_utils.hh see/see.hh
	$(CC) $(CCFLAGS) -c $(TEST)/test_see/test_see.cc -o $@ $(INC) $(INC_SYM)

$(BUILD)/test_z3solver.o : $(TEST)/test_z3solver/test_z3solver.cc tester/test_utils.hh see/z3solver.hh
	$(CC) $(CCFLAGS) -c $(TEST)/test_z3solver/test_z3solver.cc -o $@ $(INC) $(INC_SYM)

$(BUILD)/test_tester.o : $(TEST)/test_tester/test_tester.cc tester/test_utils.hh tester/tester.hh
	$(CC) $(CCFLAGS) -c $(TEST)/test_tester/test_tester.cc -o $@ $(INC) $(INC_SYM)

$(BUILD)/test_genATC.o : $(TEST)/test_genATC/test_genATC.cc tester/genATC.hh language/typemap.hh
	$(CC) $(CCFLAGS) -c $(TEST)/test_genATC/test_genATC.cc -o $@ $(INC) $(INC_SYM)

$(BUILD)/test_e2e.o : $(TEST)/test_e2e/test_e2e.cc tester/genATC.hh tester/tester.hh tester/test_utils.hh apps/app1/app1.hh
	$(CC) $(CCFLAGS) -c $(TEST)/test_e2e/test_e2e.cc -o $@ $(INC) $(INC_SYM)

# --------------------------------------------------
#  TEST binaries (linking only)
# --------------------------------------------------
test_see: $(BUILD)/test_see.o $(ALL_TEST_DEPS)
	$(CC) $(CCFLAGS) $(BUILD)/test_see.o $(ALL_TEST_DEPS) -o $(BIN)/test_see $(LIB)

test_z3solver: $(BUILD)/test_z3solver.o $(ALL_TEST_DEPS)
	$(CC) $(CCFLAGS) $(BUILD)/test_z3solver.o $(ALL_TEST_DEPS) -o $(BIN)/test_z3solver $(LIB)

test_tester: $(BUILD)/test_tester.o $(ALL_TEST_DEPS) $(TESTER_OBJS)
	$(CC) $(CCFLAGS) $(BUILD)/test_tester.o $(ALL_TEST_DEPS) $(TESTER_OBJS) -o $(BIN)/test_tester $(LIB)

test_genATC: $(BUILD)/test_genATC.o $(COMMON_OBJS) $(GENATC_OBJS) $(BUILD)/typemap.o
	$(CC) $(CCFLAGS) $(BUILD)/test_genATC.o $(COMMON_OBJS) $(GENATC_OBJS) $(BUILD)/typemap.o -o $(BIN)/test_genATC $(LIB)

test_e2e: $(BUILD)/test_e2e.o $(ALL_TEST_DEPS) $(TESTER_OBJS) $(GENATC_OBJS)
	$(CC) $(CCFLAGS) $(BUILD)/test_e2e.o $(ALL_TEST_DEPS) $(TESTER_OBJS) $(GENATC_OBJS) -o $(BIN)/test_e2e $(LIB)

# --------------------------------------------------
#  Test run rules
# --------------------------------------------------
run_test_see: test_see
	./$(BIN)/test_see

run_test_z3solver: test_z3solver
	./$(BIN)/test_z3solver

run_test_tester: test_tester
	./$(BIN)/test_tester

run_test_genATC: test_genATC
	./$(BIN)/test_genATC

run_test_e2e: test_e2e
	./$(BIN)/test_e2e

test: run_test_see run_test_z3solver run_test_tester run_test_genATC run_test_e2e

clean:
	rm -f $(BUILD)/*.o $(BIN)/test_see $(BIN)/test_z3solver $(BIN)/test_tester $(BIN)/test_genATC $(BIN)/test_e2e $(BUILD)/test_see.o $(BUILD)/test_z3solver.o $(BUILD)/test_tester.o $(BUILD)/test_genATC.o $(BUILD)/test_e2e.o
