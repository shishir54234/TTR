# Test Generator (TestGen)

This module is responsible for generating automated tests for APIs by bridging the gap between high-level specifications and concrete test executions. It uses a pipeline approach involving Abstract Test Case (ATC) generation and Symbolic Execution (SEE) to produce Concrete Test Cases (CTC).

## 1. Code Structure

The codebase is organized into modular components, each handling a specific stage of the test generation pipeline:

*   **`language/`**: Defines the core data structures and syntax.
    *   `ast.hh/cc`: The Abstract Syntax Tree nodes (Expr, Stmt, FuncCall, Decl, etc.).
    *   `astvisitor.hh`: Visitor pattern interface for traversing the AST.
    *   `typemap.hh/cc`: Manages type information for variables.
    *   `env.hh/cc`: Environment management for variable state.
    *   `symvar.hh/cc`: Symbolic variables used during symbolic execution.

*   **`see/`**: The Symbolic Execution Engine.
    *   `see.hh/cc`: Core logic for symbolic execution, state exploration, and path constraint tracking.
    *   `solver.hh`: Abstract interface for constraint solvers.
    *   `z3solver.hh/cc`: Implementation of the solver using the Z3 Theorem Prover. Handles translation of internal expressions to Z3 formulas.

*   **`tester/`**: The testing orchestration logic.
    *   `genATC.hh/cc`: **ATC Generator**. Converts a high-level `Spec` into an Abstract Test Case (ATC) program. This involves adding `input()` placeholders and handling global/local state initialization.
    *   `tester.hh/cc`: **Tester**. Takes the ATC and uses the SEE to resolve `input()` calls into concrete values, producing a Concrete Test Case (CTC).

*   **`apps/`**: Application-specific definitions.
    *   Contains specific function factories or API definitions for the applications being tested (e.g., `app1`).

*   **`test/`**: Test suites for the generator itself.
    *   `test_genATC/`: Unit tests for ATC generation.
    *   `test_see/`: Unit tests for the Symbolic Execution Engine.
    *   `test_e2e/`: End-to-End tests verifying the full pipeline (Spec → ATC → CTC).

*   **`test_utils.hh/cc`**: Utilities for creating mock ASTs and helping with assertions in tests.

## 2. Testing Strategy

We ensure the reliability of the TestGen tool through multiple levels of testing:

1.  **Unit Testing**:
    *   Each component (`SEE`, `Z3Solver`, `ATCGenerator`) is tested in isolation.
    *   We use a custom testing framework (similar to JUnit/GoogleTest conceptual models) integrated into `test_utils.hh`.
    *   We verify that AST transformations (like variable renaming in `genATC`) are correct.
    *   We veryify that the Z3 solver correctly satisfies logical formulas.

2.  **Integration Testing**:
    *   We test the interaction between `SEE` and `Z3Solver` to ensure path constraints are correctly collected and solved.

3.  **End-to-End (E2E) Testing**:
    *   Located in `test/test_e2e/`.
    *   These tests define a full API Specification in C++ code.
    *   They run the entire pipeline: `Spec` → `ATCGenerator` → `Tester (SEE+Z3)` → `CTC`.
    *   We verify the final output structure: checking for correct `assume` (preconditions), `assert` (postconditions), and that abstract `input()` calls are replaced with concrete values (e.g., `x := 9`).

## 3. Steps to Test a New Application

To generate tests for a new application (e.g., "App2"), follow these steps:

1.  **Define Application Logic**:
    *   Create a directory `apps/app2/`.
    *   Implement necessary helper functions or a `FunctionFactory` if your app uses custom internal functions that needs evaluation during symbolic execution.

2.  **Write the Specification**:
    *   The specification is currently defined programmatically using AST nodes (see `makeSpec()` in E2E examples).
    *   Define Global Variables: `Decl("y", ...)`
    *   Define Initialization: `Init("y", 0)`
    *   Define API Blocks:
        *   **Precondition**: Define logic constraints (e.g., `x > 0`). Use `Any(z)` to declare an input variable without constraints.
        *   **API Call**: The function to call (e.g., `my_api(x)`).
        *   **Postcondition**: The expected result (e.g., `result == x + 1`).

3.  **Create Test Runner**:
    *   Create a test class inheriting from `E2ETest` (similar to `E2ETest1`).
    *   Implement `makeSpec()` to return your specification.
    *   Implement `makeSymbolTables()` to define the variable scopes.
    *   Implement `verify()` to assert the generated CTC is correct.
    *   Add your test to the `main()` function in the test runner.

4.  **Build and Run**:
    *   Update `Makefile` to include your new app objects.
    *   Run `make run_test_e2e` to execute the generation and verification.

## 4. End-to-End Testing Examples

The `test/test_e2e/test_e2e.cc` file contains comprehensive examples demonstrating different testing scenarios.

### E2E Test 1: Simple API Call
*   **Scenario**: A stateless API `f1(x, z)`.
*   **Spec**:
    *   Pre: `x > 0` AND `z > 0`.
    *   Post: `r = x + z`.
*   **Flow**:
    1.  **ATC Gen**: Creates `x0 := input()`, `z0 := input()`, `assume(x0>0 && z0>0)`.
    2.  **SEE/Solver**: Finds integer values satisfying the assumption (e.g., `x=1`, `z=1`).
    3.  **CTC**: Generates `x0 := 1`, `z0 := 1`, calls `f1`, and asserts the result.

### E2E Test 2: Sequential API Calls
*   **Scenario**: Calling `f1(x, z)` followed by `f2()`.
*   **Spec**:
    *   Defines two distinct blocks.
    *   Each block is processed in sequence.
*   **Result**: The generated test case contains a sequence of calls, managing variable renaming automatically (e.g., handling scope for `x` and `z`).

### E2E Test 3: API with Global State & `Any()`
*   **Scenario**: An API that interacts with a global variable `y`.
*   **Spec**:
    *   Global `y` initialized to `0`.
    *   Pre: `x < 10` AND `Any(z)`.
    *   `Any(z)` explicitly marks `z` as an input variable without imposing value constraints.
*   **Flow**:
    1.  **ATC Gen**: Identifies `z` as input due to `Any(z)` precondition.
    2.  **SEE**: Solves constraints. `x` must be `< 10` (e.g., `9`). `z` can be anything (e.g., `0`).
    3.  **CTC**: Sets up global state (`set_y`), executes logic with concrete values.
