# GenATC Test Suite Summary

## Overview

Comprehensive test suite for the Abstract Test Case Generator, covering all major functionality from the design notes.

## Test Coverage

### ✅ Initialization (Test 1)
- Global variable initialization
- Empty map/set creation
- Statement generation from Init nodes

### ✅ Input Variables (Test 2)
- Input statement generation: `var := input()`
- Variable collection from API call arguments
- Local vs global variable distinction

### ✅ Variable Renaming (Tests 2, 4)
- Local variables get block suffix: `u` → `u0`, `u1`
- Global variables unchanged: `U` stays `U`
- Proper scoping with SymbolTable

### ✅ Preconditions (Tests 2, 3, 4)
- Assume statement generation
- Expression conversion with renamed variables
- Precondition placement before API call

### ✅ Prime Notation (Test 3)
- Extraction of primed variables: `U'` → identifies `U`
- Old state preservation: `U_old = U`
- Prime removal in postconditions:
  - `U'` → `U`
  - `U` → `U_old` (when `U'` exists)

### ✅ Postconditions (Test 3)
- Assert statement generation
- Expression transformation
- Proper placement after API call

### ✅ API Calls (Tests 2, 3, 4)
- Function call generation
- Argument renaming
- Result variable: `_result0`, `_result1`

### ✅ Multiple Blocks (Test 4)
- Sequential block generation
- Independent variable scopes per block
- Shared global state

## Test Statistics

| Test | Statements | Variables | Blocks | Features Tested |
|------|-----------|-----------|--------|-----------------|
| Test 1 | 1 | 1 global | 0 | Init only |
| Test 2 | 5+ | 2 local, 1 global | 1 | Input, assume, call |
| Test 3 | 7+ | 2 local, 1 global | 1 | Primes, assert |
| Test 4 | 10+ | 4 local, 2 global | 2 | Multiple blocks |

## Algorithm Coverage

From design notes algorithm:

```
function genATC(spec, ts, σ)
  init := genInit(spec)           ✅ Test 1
  atc.append(init)                ✅ All tests
  for bn in ts do                 ✅ Test 4
    b := genBlock(spec, bn, σ)    ✅ Tests 2, 3, 4
    atc.append(b)                 ✅ Tests 2, 3, 4
  done
  return atc                      ✅ All tests
end function
```

```
function genBlock(spec, bn, σ)
  seq := []
  for b in spec.blocks s.t. b.name = bn do
    args := b.apicall.args        ✅ Tests 2, 3, 4
    for a in args do
      μ[a] = new variablename()   ✅ Tests 2, 3, 4 (renaming)
    done
    σ_b := σ[bn]                  ✅ Tests 2, 3, 4 (symbol table)
    for a in args do
      i := 'μ[a] := input(...)'   ✅ Tests 2, 3, 4
      seq.append[i]               ✅ Tests 2, 3, 4
    done
    seq.append('assume(...)')     ✅ Tests 2, 3, 4
  done
  return seq
end function
```

## Expression Types Tested

- ✅ Var
- ✅ FuncCall
- ✅ Num
- ✅ Map
- ✅ Set (in type declarations)
- ✅ Tuple (in type declarations)

## Statement Types Generated

- ✅ Assign (for init, input, old state, API call)
- ✅ Assume (for preconditions)
- ✅ Assert (for postconditions)

## Integration Points Verified

- ✅ AST structure (`ast.hh`)
- ✅ SymbolTable (`env.hh`)
- ✅ TypeMap (`typemap.hh`)
- ✅ Spec, API, Init, Decl nodes
- ✅ Expression hierarchy
- ✅ Statement hierarchy

## Build Integration

- ✅ Makefile rules added
- ✅ Object file dependencies
- ✅ Test binary creation
- ✅ Run target: `make run_test_genATC`
- ✅ Integrated into `make test`

## Next Steps

### Potential Additional Tests

1. **Complex expressions**: Nested function calls, complex maps
2. **Edge cases**: Empty blocks, no preconditions, no postconditions
3. **Multiple primed variables**: `U'` and `T'` in same postcondition
4. **Error handling**: Invalid specs, missing symbol tables
5. **Type tracking**: Verify TypeMap population

### Integration Tests

1. **Parser → GenATC**: Full pipeline from spec file
2. **GenATC → SEE**: Generated ATC through symbolic execution
3. **GenATC → GenCTS**: ATC to concrete test cases

### Performance Tests

1. Large specifications (100+ blocks)
2. Deep expression nesting
3. Many global variables

## Running the Tests

```bash
# Build and run genATC tests only
cd src/testgen
make test_genATC
make run_test_genATC

# Run all tests including genATC
make test

# Clean build artifacts
make clean
```

## Success Criteria

All tests pass with:
- ✅ Correct number of statements generated
- ✅ Proper variable renaming
- ✅ Correct statement types
- ✅ Valid expression structure
- ✅ No memory leaks (proper cleanup)
- ✅ No compilation warnings
