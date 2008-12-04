#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef FAKE_CRASH
#include <unistd.h>
#endif

#ifndef DEBUG
	#define xmalloc(size) Emalloc(size)
	#define xrealloc(mem, size) Erealloc(mem, size)
#else
	#define xmalloc(size) (printf("%d\n", __LINE__), Emalloc(size))
	#define xrealloc(mem, size) (printf("%d\n", __LINE__), Erealloc(mem, size))
#endif

#include "eval.h"

#define OUT_OF_MEMORY_MSG "Out of memory"

UINT pModulo;
VariableHolder_t *Variables;
UINT VariableCount;

#ifdef FAKE_CRASH
int GotCrash = 0;
#endif

void *Emalloc(size_t size)
{
	void *Result;

	Result = malloc(size);
	if (Result == NULL)
	{
		EvalError(NULL, OUT_OF_MEMORY_MSG);
#ifdef OUT_OF_MEM_HALT
		abort();
#endif
#ifdef FAKE_CRASH
		if (!GotCrash)
		{
			fprintf(stderr, "Abort trap\n");
			fprintf(stderr, "[s02050224@orc11 ~/prog/task1]$ ");
			sleep(5);
			fprintf(stderr, "\nIt's joke! I'm alive\n");
			GotCrash = 1;
		}
#endif
	}
	return Result;
}

void *Erealloc(void *Mem, size_t size)
{
	void *Result;

	Result = realloc(Mem, size);
	if (size != 0 && Result == NULL)
	{
		EvalError(NULL, OUT_OF_MEMORY_MSG);
	}
	return Result;
}

void EvalError(Token_t *Token, char *Format, ...)
{
	va_list List;
	
	if (Token != NULL)
	{
		fprintf(stderr, "Eval error at %d:%d:\t", Token->LineNo +  1, Token->Position);
	}
	else
	{
		fprintf(stderr, "Eval error:\t");
	}
	
	va_start(List, Format);
	vfprintf(stderr, Format, List);
	va_end(List);
	fprintf(stderr, "\n");
}

int AssignVariable(Variable_t *a, Variable_t *b)
{
	DestroyVariable(a);
	a->MonCount = b->MonCount;
	a->Mons = (Monomial_t *)xmalloc(sizeof(Monomial_t)*a->MonCount);
	if (a->Mons == NULL)
	{
		return 0;
	}
	memcpy(a->Mons, b->Mons, sizeof(Monomial_t)*a->MonCount);
	return 1;
}

void DestroyVariable(Variable_t *Variable)
{	
	if (Variable != NULL && Variable->Mons != NULL)
	{
		free((void *)Variable->Mons);
		Variable->Mons = NULL;
	}
	Variable->MonCount = 0;
}

void InitVariables()
{
	VariableCount = 0;
	Variables = NULL;
}

int SaveVariable(char *Name, Variable_t *Variable)
{
	UINT i;
	VariableHolder_t *bak;
	
	for (i = 0; i < VariableCount; i++)
	{
		if (strcmp(Variables[i].VariableName, Name) == 0)
		{
			AssignVariable(&Variables[i].Variable, Variable);
			return 1;
		}
	}
	bak = Variables;
	Variables = (VariableHolder_t *)xrealloc(Variables, sizeof(VariableHolder_t)*(VariableCount + 1));
	if (Variables == NULL)
	{
		// Do not add new variable
		Variables = bak;
		return 0;
	}
	Variables[VariableCount].Variable.MonCount = 0;
	Variables[VariableCount].Variable.Mons = NULL;
	AssignVariable(&Variables[VariableCount].Variable, Variable);
	strcpy(Variables[VariableCount].VariableName, Name);
	VariableCount++;
	return 1;
}

Variable_t *GetVariable(char *Name)
{
	UINT i;
	
	for (i = 0; i < VariableCount; i++)
	{
		if (strcmp(Variables[i].VariableName, Name) == 0)
		{
			return &Variables[i].Variable;
		}
	}
	return NULL;
}

void DestroyVariables()
{
	UINT i;
	
	for (i = 0; i < VariableCount; i++)
	{
		DestroyVariable(&Variables[i].Variable);
	}
	if (Variables != NULL)
	{	
		free((void *)Variables);
	}
}

/*
	Evalute

	INPUT: Root node of tree to evalute, pointer to varaible to store result in
	OUTPUT: None
	RETURN: Non-zero if evaluation successful, zero otherwise

	Evaluates statement stored in syntax tree
*/

int Evalute(TreeNode_t *Node, Variable_t *Result)
{
	#define AVOID(EXPR) if (EXPR) goto EvaluteError
	
	Variable_t a;
	Variable_t *v;
	TreeNode_t *n;
	
	DestroyVariable(Result);
	a.Mons = NULL;
	a.MonCount = 0;
	
	switch (Node->Token.Token)
	{
		case ttPlus:
			AVOID(Evalute(Node->First, Result) == 0);
			AVOID(Evalute(Node->Second, &a) == 0);
			SuperNormalize(Result);
			SuperNormalize(&a);
			AVOID(SuperAdd(Result, &a) == 0);
			break;
		case ttMinus:
			AVOID(Evalute(Node->First, Result) == 0);
			AVOID(Evalute(Node->Second, &a) == 0);
			SuperNormalize(Result);
			SuperNormalize(&a);
			AVOID(SuperSub(Result, &a) == 0);
			break;
		case ttAsterisk:
		case ttAsteriskSolid:
			AVOID(Evalute(Node->First, Result) == 0);
			AVOID(Evalute(Node->Second, &a) == 0);
			SuperNormalize(Result);
			SuperNormalize(&a);
			AVOID(SuperMul(Result, &a) == 0);
			break;
		case ttSlash:
			AVOID(Evalute(Node->First, Result) == 0);
			AVOID(Evalute(Node->Second, &a) == 0);
			SuperNormalize(Result);
			SuperNormalize(&a);
			AVOID(SuperDiv(Result, &a) == 0);
			break;
		case ttPercent:
			AVOID(Evalute(Node->First, Result) == 0);
			AVOID(Evalute(Node->Second, &a) == 0);
			SuperNormalize(Result);
			SuperNormalize(&a);
			AVOID(SuperMod(Result, &a) == 0);
			break;
		case ttHat:
			AVOID(Evalute(Node->First, Result) == 0);
			AVOID(Evalute(Node->Second, &a) == 0);
			SuperNormalize(Result);
			AVOID(SuperPow(Result, &a) == 0);
			break;
		case ttX:
		case ttConst:
			if (Node->Token.Token == ttConst && Node->Token.ConstValue == 0)
			{
				// No monomials needed
				Result->MonCount = 0;
				Result->Mons = NULL;
				break;
			}
			Result->MonCount = 1;
			Result->Mons = (Monomial_t *)xmalloc(sizeof(Monomial_t));
			AVOID(Result->Mons == NULL);
			if (Node->Token.Token == ttX)
			{
				Result->Mons[0].Coeff = 1;
				Result->Mons[0].Power = 1;
			}
			else
			{
				Result->Mons[0].Coeff = Node->Token.ConstValue;
				Result->Mons[0].Power= 0 ;
			}
			break;
		case ttDummy:
			AVOID(Evalute(Node->First, Result) == 0);
			break;
		case ttVariable:
			v = GetVariable(Node->Token.VariableName);
			if (v == NULL)
			{
				EvalError(NULL, "There is no variable '%s'", Node->Token.VariableName);
				goto EvaluteError;
			}
			AVOID(AssignVariable(Result, v) == 0);
			break;
		case ttEqual:
			for (n = Node->First; n->Token.Token == ttDummy; n = n->First);
			if (n->Token.Token != ttVariable)
			{
				EvalError(NULL, "l-value expected in assignment");
				goto EvaluteError;
			}
			AVOID(Evalute(Node->Second, &a) == 0);
			AVOID(SaveVariable(n->Token.VariableName, &a) == 0);
			AVOID(AssignVariable(Result, &a) == 0);
			break;
		default:
			EvalError(NULL, "%s is not supporting at this time", TOKEN_TABLE[Node->Token.Token]);
			goto EvaluteError;
	}
	DestroyVariable(&a);
	
	return 1;
	
	EvaluteError:	
	DestroyVariable(&a);
	DestroyVariable(Result);
	return 0;
	#undef AVOID
}

inline UINT BasicInv(UINT a)
{
	if (a != 0)
	{
		return pModulo - a;
	}
	else
	{
		return 0;
	}
}

inline UINT BasicAdd(UINT a, UINT b)
{
	ULONG t;
	
	t = ((ULONG)a) + b;
	if (t >= pModulo)
	{
		t -= pModulo;
	}
	return (UINT)t;
}

inline UINT BasicPowerAdd(UINT a, UINT b)
{
	ULONG t;

	t = ((ULONG)a) + b;
	while (t > pModulo - 1)
		t -= pModulo - 1;
	return (UINT)t;
}

inline UINT BasicSub(UINT a, UINT b)
{
	ULONG t;
	
	// black magic %)
	t = ((ULONG)a) + pModulo - b;
	while (t >= pModulo)
	{
		t -= pModulo;
	}
	return (UINT)t;
}

inline UINT BasicMul(UINT a, UINT b)
{
	ULONG t;
	
	t = ((ULONG)a)*b;
	
	return (UINT)(t % pModulo);
}

inline UINT BasicPow(UINT a, UINT b)
{
	ULONG t, Result;

	t = a;
	Result = 1;
	while (b != 0)
	{
		if ((b & 1) == 1)
			Result = (Result*t)%pModulo;
		t = (t*t)%pModulo;
		b >>= 1;
	}
	return (UINT)Result;
}

inline UINT BasicDiv(UINT a, UINT b)
{
	return BasicMul(a, BasicPow(b, pModulo - 2)) ;
}

// This is obsolete
#if 0
// A kind of quicksort
void SuperSort(Monomial_t *Mons, UINT l, UINT r)
{
	UINT i, j, x;
	Monomial_t t;
	
	if (r - l < QUICK_SORT_THRESHOLD)
	{
		// Short arrays is better to sort by isertions
		Monomial_t x;

		for (i = l + 1; i <= r; i++)
		{
			x = Mons[i];
			for (j = i; j > l && Mons[j - 1].Power > x.Power; j--)
			{
				Mons[j] = Mons[j - 1];
			}
			Mons[j] = x;
		}
		return;
	}
	i = l;
	j = r;
	x = Mons[(l+r) >> 1].Power;

	do
	{
		while (Mons[i].Power < x) i++;
		while (Mons[j].Power > x) j--;
		if (i <= j)
		{
			t = Mons[i];
			Mons[i] = Mons[j];
			Mons[j] = t;
			i++;
			if (j > 0)
			{
				j--;
			}
		}
	} while (i <= j);
	if (i < r)
	{
		SuperSort(Mons, i, r);
	}
	if (l < j)
	{
		SuperSort(Mons, l, j);
	}
}
#endif

// Insertion sort with guarding element
void SuperSort(Monomial_t *Mons, UINT l, UINT r)
{
	UINT i, j;
	Monomial_t x, Backup;

	// Black magic %)
	Backup = Mons[l];
	Mons[l].Power = 0;

	for (i = l + 1; i <= r; i++)
	{
		x = Mons[i];
		for (j = i; Mons[j - 1].Power > x.Power; j--)
		{
			Mons[j] = Mons[j - 1];
		}
		Mons[j] = x;
	}
	for (i = l + 1; i <= r && Mons[i].Power < Backup.Power; i++)
		Mons[i - 1] = Mons[i];

	Mons[i - 1] = Backup;
}

void SuperNormalize(Variable_t *a)
{
	if (a->MonCount != 1)
		return;
	a->Mons[0].Coeff %= pModulo;
	if (a->Mons[0].Coeff == 0)
	{
		// This is zero
		free(a->Mons);
		a->MonCount = 0;
		a->Mons = NULL;
	}
}

int SuperNormalizeFull(Variable_t *a)
{
	UINT i, j;

	if (a->MonCount <= 1)
	{	
		return 1;
	}
	SuperSort(a->Mons, 0, a->MonCount - 1);
	for (i = 1, j = 0; i < a->MonCount; i++)
	{
		if (a->Mons[i].Power == a->Mons[j].Power)
			a->Mons[j].Coeff = BasicAdd(a->Mons[i].Coeff, a->Mons[j].Coeff);
		else
		{
			if (a->Mons[j].Coeff != 0)
				j++;
			a->Mons[j] = a->Mons[i];
		}
	}
	if (a->Mons[j].Coeff > 0)
		j++;
	if (j != a->MonCount)
	{
		Monomial_t *bak;
	
		bak = a->Mons;
		a->Mons = (Monomial_t *)xrealloc(a->Mons, sizeof(Monomial_t)*j);
		if (a->Mons == NULL)
		{
			// Return old value to a
			a->Mons = bak;
			return 0;
		}
		a->MonCount = j;
		return 1;
	}
	else
	{
		// No realloc needed
		return 1;
	}
}
#define AVOID(EXPR) if (EXPR) goto ERRLABEL

int SuperAdd(Variable_t *a, Variable_t *b)
{
	UINT i, j;
	UINT k;
	Monomial_t *NewMons, *bak;
	
	
	// Allocate a lot of memory
	NewMons = (Monomial_t *)xmalloc((a->MonCount + b->MonCount)*sizeof(Monomial_t));
	if (NewMons == NULL)
	{	
		return 0; 
	}
	for (i = 0, j = 0, k = 0; i < a->MonCount && j < b->MonCount; k++)
	{
		if (a->Mons[i].Power < b->Mons[j].Power)
		{
			// a[i] < b[j]
			NewMons[k] = a->Mons[i];
			i++;
		}
		else if (a->Mons[i].Power > b->Mons[j].Power)
		{
			// b[j] < a[i]
			NewMons[k] = b->Mons[j];
			j++;
		}
		else
		{
			// a[i] == b[j]
			NewMons[k].Power = a->Mons[i].Power;
			NewMons[k].Coeff = BasicAdd(a->Mons[i].Coeff, b->Mons[j].Coeff);
			i++;
			j++;
		}
	}
	for (; i < a->MonCount; k++, i++)
	{
		NewMons[k] = a->Mons[i];
	}
	for (; j < b->MonCount; k++, j++)
	{
		NewMons[k] = b->Mons[j];
	}
	// Now, k holds real number of monomials in answer
	
	// Drop monomials with coeff==0
	for (i = 0, j = 0; j < k; j++)
	{
		if (NewMons[j].Coeff != 0)
		{
			NewMons[i] = NewMons[j];
			i++;
		}
	}
	k = i;
	
	if (k != a->MonCount)
	{
		// Realloc needed
		bak = a->Mons;
		a->Mons = (Monomial_t *)xrealloc(a->Mons, sizeof(Monomial_t)*k);
		if (a->Mons == NULL && k != 0)
		{
			a->Mons = bak;
			return 0;
		}
		a->MonCount = k;
	}
	memcpy(a->Mons, NewMons, sizeof(Monomial_t)*k);
	free((void *)NewMons);
	return 1;
}

int SuperSub(Variable_t *a, Variable_t *b)
{
	// a - b = -(b - a)
	
	UINT i;
	
	// a = -a
	for (i = 0; i < a->MonCount; i++)
	{
		a->Mons[i].Coeff = BasicInv(a->Mons[i].Coeff);
	}
	//a = b - a;
	if (!SuperAdd(a, b))
	{
		return 0;
	}
	// a = -(b - a) == a - b
	for (i = 0; i < a->MonCount; i++)
	{
		a->Mons[i].Coeff = BasicInv(a->Mons[i].Coeff);
	}
	return 1;	
}


int SuperMul(Variable_t *a, Variable_t *b)
{

	if (b->MonCount == 0)
	{
		DestroyVariable(a);
		return 0;
	}
	
	Variable_t c, d;
	UINT i, j;
	
	d.MonCount = 0;
	d.Mons = NULL;
	
	for (i = 0; i < a->MonCount; i++)
	{
		c.MonCount = b->MonCount;
		c.Mons = (Monomial_t *)xmalloc(sizeof(Monomial_t)*b->MonCount);
		if (c.Mons == NULL)
		{
			return 0;
		}
		for (j = 0; j < b->MonCount; j++)
		{
			c.Mons[j].Coeff = BasicMul(a->Mons[i].Coeff, b->Mons[j].Coeff);
			c.Mons[j].Power = BasicPowerAdd(a->Mons[i].Power, b->Mons[j].Power);
		}
		if (SuperNormalizeFull(&c) == 0)
		{
			DestroyVariable(&c);
			return 0;
		}
		if (SuperAdd(&d, &c) == 0)
		{
			DestroyVariable(&d);
			DestroyVariable(&c);
			return 0;
		}
		DestroyVariable(&c);
	}
	DestroyVariable(a);
	free((void *)c.Mons);
	*a = d;
	
	return 1;
}


int SuperDiv(Variable_t *a, Variable_t *b)
{
	Variable_t c, d;

	c.MonCount = 1;
	c.Mons = NULL;
	d.MonCount = 1;
	d.Mons = NULL;
	if (SuperDivMod(a, b, &c, &d) == 0)
	{
		return 0;
	}
	DestroyVariable(a);
	DestroyVariable(&d);
	*a = c;

	return 1;
}
int SuperMod(Variable_t *a, Variable_t *b)
{
	Variable_t c, d;

	c.MonCount = 1;
	c.Mons = NULL;
	d.MonCount = 1;
	d.Mons = NULL;
	if (SuperDivMod(a, b, &c, &d) == 0)
	{
		return 0;
	}
	DestroyVariable(a);
	DestroyVariable(&c);
	*a = d;

	return 1;
}

int SuperDivMod(Variable_t *a, Variable_t *b, Variable_t *c, Variable_t *d)
{
	UINT BPower;
	Variable_t tmp, mul;

	DestroyVariable(c);
	DestroyVariable(d);
	if (b->MonCount == 0)
	{
		EvalError(NULL, "Division by zero");
		return 0;
	}
	tmp.MonCount = 1;
	tmp.Mons = (Monomial_t *)xmalloc(sizeof(Monomial_t));
	if (tmp.Mons == NULL)
	{
		return 0;
	}
	mul.MonCount = 0;
	mul.Mons = NULL;
	AssignVariable(d, a);
	BPower = b->Mons[b->MonCount - 1].Power;
	// While remaining power of polynomial is greater than power of divizor
	while (d->MonCount > 0 && d->Mons[d->MonCount - 1].Power >= BPower)
	{
		// Calculate next monomial in tmp
		tmp.Mons[0].Power = d->Mons[d->MonCount - 1].Power - b->Mons[b->MonCount - 1].Power;
		tmp.Mons[0].Coeff = BasicDiv(d->Mons[d->MonCount - 1].Coeff, b->Mons[b->MonCount - 1].Coeff);
		// Add next monomial to result
		SuperAdd(c, &tmp);
		AssignVariable(&mul, b);
		SuperMul(&mul, &tmp);
		SuperSub(d, &mul);
	}
	DestroyVariable(&tmp);
	DestroyVariable(&mul);

	return 1;
}

int SuperPow(Variable_t *a, Variable_t *b)
{
	#define ERRLABEL SuperPError
	
	if (b->MonCount == 0)
	{
		// a == a^1
		return 1;
	}
	if (b->MonCount > 1 || b->Mons[0].Power > 0)
	{
		EvalError(NULL, "Power must be integer");
		return 0;
	}
	
	UINT p;
	Variable_t c;
	
	c = *a;
	a->MonCount = 1;
	a->Mons = (Monomial_t *)xmalloc(sizeof(Monomial_t));
	if (a->Mons == NULL)
	{
		return 0;
	}
	a->Mons[0].Coeff = 1;
	a->Mons[0].Power = 0;
	p = b->Mons[0].Coeff;
	while (p > 0)
	{
		if ((p & 1) == 1)
		{
			AVOID(SuperMul(a, &c) == 0);
		}
		AVOID(SuperMul(&c, &c) == 0);
		p >>= 1;
	}
	DestroyVariable(&c);
	return 1;
	
	SuperPError:
	DestroyVariable(&c);
	return 0;
	#undef ERRLABEL
}

void PrintVariable(Variable_t *v)
{
	UINT i;
	UINT p, c;
	
	SuperNormalize(v);
	if (v->MonCount > 0)
	{
		i = v->MonCount;
		do
		{
			i--;
			p = v->Mons[i].Power;
			c = v->Mons[i].Coeff;
			if (i < v->MonCount - 1)
			{
				printf("+");
			}
			if (p > 0)
			{
				if (c != 1)
				{
					printf("%u", v->Mons[i].Coeff);
				}
				printf("x");
				if (p > 1)
				{
					printf("^%u", p);
				}
			}
			else
			{
				printf("%u", c);
			}
		} while (i != 0);
	}
	else
	{
		printf("0");
	}
	printf("\n");
}

