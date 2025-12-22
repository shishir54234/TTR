# GenATC Test Suite

Comprehensive tests for the Abstract Test Case Generator.

## Test Cases

### Test 1: Simple Initialization
Tests `genInit()` functionality with global variable initialization.

**Spec:**
```
global U: map<string, string>
init U := {}
```

**Expected ATC:**
```
U = {}
```

**Verifies:**
- Initialization statement generation
- Proper assignment structure

---

### Test 2: Single API Block with Precondition
Tests `genBlock()` with input variables and precondition.

**Spec:**
```
global U: map<string, string>
init U := {}

signup(u: string, p: string):
  pre: not_in(u, U)
```

**Expected ATC:**
```
U = {}
u0 := input()
p0 := input()
assume(not_in(u0, U))
_result0 = signup(u0, p0)
```

**Verifies:**
- Input variable generation
- Variable renaming (u → u0, p → p0)
- Precondition assume statement
- API call generation

---

### Test 3: API Block with Primed Variables
Tests prime notation handling for state transitions.

**Spec:**
```
global U: map<string, string>
init U := {}

signup(u: string, p: string):
  pre: not_in(u, U)
  post: U' = U union {u -> p}
```

**Expected ATC:**
```
U = {}
u0 := input()
p0 := input()
assume(not_in(u0, U))
U_old = U
_result0 = signup(u0, p0)
assert(U = U_old union {u0 -> p0})
```

**Verifies:**
- Prime notation extraction (U')
- Old state preservation (U_old = U)
- Prime removal in postcondition (U' → U, U → U_old)
- Assert statement generation

---

### Test 4: Multiple API Blocks
Tests sequential block generation with different variable scopes.

**Spec:**
```
global U, T: map<string, string>
init U := {}, T := {}

signup(u: string, p: string):
  pre: not_in(u, U)

login(u: string, p: string):
  pre: in(u, U)
```

**Expected ATC:**
```
U = {}
T = {}
u0 := input()
p0 := input()
assume(not_in(u0, U))
_result0 = signup(u0, p0)
u1 := input()
p1 := input()
assume(in(u1, U))
_result1 = login(u1, p1)
```

**Verifies:**
- Multiple block generation
- Variable renaming per block (u0, p0 vs u1, p1)
- Global variables remain unchanged across blocks
- Sequential execution order

## Building and Running

### Build the test:
```bash
cd src/testgen
make test_genATC
```

### Run the test:
```bash
make run_test_genATC
```

Or directly:
```bash
./bin/test_genATC
```

### Run all tests:
```bash
make test
```

## Test Output

Successful test output:
```
========================================
Running GenATC Test Suite
========================================

==================== Test: Simple initialization - genInit() ====================
  Generated 1 statements
  ✓ Generated 1 initialization statement
  ✓ U = {} assignment verified
✓ Test passed!

==================== Test: Single API block - signup with precondition ====================
  Generated 5 statements
  ✓ Initialization verified
  ✓ Input statements verified (u0, p0)
  ✓ Precondition assume() verified
  ✓ API call verified
✓ Test passed!

...

========================================
Test Results: 4 passed, 0 failed
========================================
```

## Test Structure

Each test inherits from `GenATCTest` base class:

```cpp
class GenATCTest {
protected:
    virtual unique_ptr<Spec> makeSpec() = 0;
    virtual vector<SymbolTable*> makeSymbolTables() = 0;
    virtual void verify(const Program& atc) = 0;
    
public:
    void execute();
};
```

Tests follow the pattern:
1. **makeSpec()** - Build the API specification
2. **makeSymbolTables()** - Create symbol tables for each block
3. **execute()** - Run the ATC generator
4. **verify()** - Assert expected properties of generated ATC

## Adding New Tests

To add a new test:

1. Create a new class inheriting from `GenATCTest`
2. Implement `makeSpec()`, `makeSymbolTables()`, and `verify()`
3. Add to the test vector in `main()`

Example:
```cpp
class GenATCTest5 : public GenATCTest {
public:
    GenATCTest5() : GenATCTest("Your test description") {}
    
protected:
    unique_ptr<Spec> makeSpec() override {
        // Build your spec
    }
    
    vector<SymbolTable*> makeSymbolTables() override {
        // Create symbol tables
    }
    
    void verify(const Program& atc) override {
        // Assert expected behavior
    }
};

// In main():
vector<GenATCTest*> testcases = {
    // ... existing tests ...
    new GenATCTest5()
};
```
