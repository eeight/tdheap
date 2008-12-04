#ifndef _EVAL_H_
#define _EVAL_H_

#include "parse.h"
#include "tree.h"

struct _Monomial
{
	UINT Power;
	UINT Coeff;
};
typedef struct _Monomial Monomial_t;

// Only polynomials supported
struct _Variable
{
	// Count of monomials in polynomial
	UINT MonCount;
	// Monomials sorted by power ascending
	struct _Monomial *Mons;
};
typedef struct _Variable Variable_t;

struct _VariableHolder
{
	char VariableName[MAX_STRING_LENGTH + 1];
	Variable_t Variable;
};
typedef struct _VariableHolder VariableHolder_t;


extern UINT pModulo;

void EvalError(Token_t *Token, char *Format, ...);

int AssignVariable(Variable_t *a, Variable_t *b);
void DestroyVariable(Variable_t *Variable);


void InitVariables();
int SaveVariable(char *Name, Variable_t *Variable);
Variable_t *GetVariable(char *Name);
void DestroyVariables();

// Operations in residue field
UINT BasicInv(UINT a);
UINT BasicAdd(UINT a, UINT b);
UINT BasicPowerAdd(UINT a, UINT b);
UINT BasicSub(UINT a, UINT b);
UINT BasicMul(UINT a, UINT b);
UINT BasicPow(UINT a, UINT b);
UINT BasicDiv(UINT a, UINT b);

int Evalute(TreeNode_t *Node, Variable_t *Result);

// Operations on polynomes

void SuperSort(Monomial_t *Mons, UINT l, UINT r);
void SuperNormalize(Variable_t *a);
int SuperNormalizeFull(Variable_t *a);
int SuperAdd(Variable_t *a, Variable_t *b);
int SuperSub(Variable_t *a, Variable_t *b);
int SuperMul(Variable_t *a, Variable_t *b);
int SuperDiv(Variable_t *a, Variable_t *b);
int SuperMod(Variable_t *a, Variable_t *b);
int SuperDivMod(Variable_t *a, Variable_t *b, Variable_t *c, Variable_t *d);
int SuperPow(Variable_t *a, Variable_t *b);

// Printing
void PrintVariable(Variable_t *v);
#endif

