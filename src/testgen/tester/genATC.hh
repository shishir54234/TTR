#ifndef GENATC_HH
#define GENATC_HH

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../language/ast.hh"
#include "../language/env.hh"
#include "../language/typemap.hh"

using namespace std;

/**
 * ATCGenerator: Generates Abstract Test Cases from API specifications
 * 
 * Algorithm (from design notes):
 * function genATC(spec, ts, σ)
 *   init := genInit(spec)
 *   atc.append(init)
 *   for bn in ts do
 *     b := genBlock(spec, bn, σ)
 *     atc.append(b)
 *   done
 *   return atc
 * end function
 */
class ATCGenerator {
private:
    const Spec* spec;
    TypeMap typeMap;
    
    /**
     * Generate initialization block from spec.global
     * Creates a sequence of statements initializing global variables
     */
    vector<unique_ptr<Stmt>> genInit(const Spec* spec);
    
    /**
     * Generate a block for a single API call
     * 
     * Algorithm (from design notes):
     * function genBlock(spec, bn, σ)
     *   seq := []
     *   for b in spec.blocks s.t. b.name = bn do
     *     args := b.apicall.args
     *     for a in args do
     *       μ[a] = new variablename()
     *     done
     *     σ_b := σ[bn]
     *     for a in args do
     *       i := 'μ[a] := input(σ_b[a])()'
     *       seq.append[i]
     *     done
     *     seq.append('assume(genPred(σ_b))')
     *   done
     *   return seq
     * end function
     */
    vector<unique_ptr<Stmt>> genBlock(const Spec* spec, const API* block, 
                                      SymbolTable* blockSymTable, int blockIndex);
    
    /**
     * Convert expression by renaming variables according to block index
     * Variables in local scope get suffix (e.g., uid → uid0)
     * Global variables remain unchanged
     */
    unique_ptr<Expr> convertExpr(const unique_ptr<Expr>& expr, 
                                  SymbolTable* symTable, 
                                  const string& suffix);
    
    /**
     * Extract variables with prime notation (') from postcondition
     * These represent "next state" variables (e.g., U' means U_next)
     */
    void extractPrimedVars(const unique_ptr<Expr>& expr, set<string>& primedVars);
    
    /**
     * Remove prime notation from expression
     * Converts: U' → U, and unprimed globals → U_old
     */
    unique_ptr<Expr> removePrimeNotation(const unique_ptr<Expr>& expr, 
                                         const set<string>& primedVars, 
                                         bool insidePrime = false);
    
    /**
     * Collect input variables from expression
     * Input variables are those in the local symbol table
     */
    void collectInputVars(const unique_ptr<Expr>& expr, 
                         vector<unique_ptr<Expr>>& inputVars,
                         const string& suffix,
                         SymbolTable* symTable,
                         TypeMap& localTypeMap);
    
    /**
     * Create input statement: var := input()
     */
    unique_ptr<Stmt> makeInputStmt(unique_ptr<Expr> varExpr);

public:
    ATCGenerator(const Spec* spec, TypeMap typeMap);
    
    /**
     * Main entry point: Generate ATC from specification and test string
     * 
     * @param spec The API specification
     * @param globalSymTable Global symbol table with children for each block
     * @return Program representing the Abstract Test Case
     */
    Program generate(const Spec* spec, 
                    SymbolTable* globalSymTable, vector<string> testString);
};

#endif // GENATC_HH
