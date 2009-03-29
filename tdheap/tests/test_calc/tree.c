#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

// Common error handlers
#define NEXT_TOKEN() if (!NextToken()) goto ERRLABEL;
#define AVOID(EXPR) if (EXPR) goto ERRLABEL

TreeNode_t *NewTreeNode(int Token, TreeNode_t *First, TreeNode_t *Second)
{
	TreeNode_t *New;
	
	New = (TreeNode_t *)malloc(sizeof(TreeNode_t));
	
	if (New != NULL)
	{
		New->Token.Token = Token;
		New->First = First;
		New->Second = Second;
	}
	else
	{
		Error("Out of memory");
#ifdef OUT_OF_MEM_HALT
		abort();
#endif
		SkipUntilNewline();
	}
	return New;
}

/*
	BuildSyntaxTree[n]
	
	INPUT:	None
	OUTPUT:	None
	RETURN:	Pointer to root node of created syntax tree
	
	Reads input using NextToken() function and builds syntax tree handling operators with priority n
*/

// Handling = operator, right-associative
TreeNode_t *BuildSyntaxTree0(void)
{
	#define ERRLABEL STError0
	
	TreeNode_t *a, *b, *tmp;
	
	a = b = 0;
	
	a = BuildSyntaxTree1();
	AVOID (a == NULL);
	if (cToken == ttEqual)
	{
		NEXT_TOKEN();
		b = BuildSyntaxTree0();
		AVOID(b == NULL);
		tmp = NewTreeNode(ttEqual, a, b);
		AVOID(tmp == NULL);
		return tmp;
	}
	else
	{
		return a;
	}
	
	// Error handling
	STError0:
	DestroySyntaxTree(a);
	DestroySyntaxTree(b);
	
	return NULL;
	#undef ERRLABEL
}

// Handlind + and - operators
TreeNode_t *BuildSyntaxTree1(void)
{
	#define ERRLABEL STError1
	TreeNode_t *a, *b, *tmp;
	int Op;
	
	a = b = 0;
	
	a = BuildSyntaxTree2();
	AVOID(a == NULL);
	while (cToken == ttPlus || cToken == ttMinus)
	{
		Op = cToken;
		NEXT_TOKEN();
		b = BuildSyntaxTree2();
		AVOID(b == NULL);
		tmp = NewTreeNode(Op, a, b);
		AVOID(tmp == NULL);
		a = tmp;
	}
	return a;
	
	// Error handling
	STError1:
	DestroySyntaxTree(a);
	DestroySyntaxTree(b);
	
	return NULL;	
	#undef ERRLABEL
}

// Handling * / and % operators
TreeNode_t *BuildSyntaxTree2(void)
{
	#define ERRLABEL STError2
	TreeNode_t *a, *b, *tmp;
	
	a = b = 0;
	int Op;
	
#ifdef SOLID_MONS
	a = BuildSyntaxTree3();
#else
	a = BuildSyntaxTree4();
#endif
	AVOID(a == NULL);
	while (cToken == ttAsterisk || cToken == ttSlash || cToken == ttPercent)
	{
		Op = cToken;
		NEXT_TOKEN();
#ifdef SOLID_MONS
		b = BuildSyntaxTree3();
#else
		b = BuildSyntaxTree4();
#endif
		AVOID(b == NULL);
		tmp = NewTreeNode(Op, a, b);
		AVOID(tmp == NULL);
		a = tmp;
	}
	return a;

	// Error handling
	STError2:
	DestroySyntaxTree(a);
	DestroySyntaxTree(b);
	
	return NULL;	
	#undef ERRLABEL
}

// Handling * (2x) operator if SOLID_MONS enabled
TreeNode_t *BuildSyntaxTree3(void)
{
	#define ERRLABEL STError3
	TreeNode_t *a, *b, *tmp;
	
	a = b = 0;
	
	a = BuildSyntaxTree4();
	AVOID(a == NULL);
	while (cToken == ttAsteriskSolid)
	{
		NEXT_TOKEN();
		b = BuildSyntaxTree4();
		AVOID(b == NULL);
		tmp = NewTreeNode(ttAsteriskSolid, a, b);
		AVOID(tmp == NULL);
		a = tmp;
	}
	return a;

	// Error handling
	STError3:
	DestroySyntaxTree(a);
	DestroySyntaxTree(b);
	
	return NULL;	
	#undef ERRLABEL
}

// Handling ^ operator
TreeNode_t *BuildSyntaxTree4(void)
{
	#define ERRLABEL STError4
	TreeNode_t *a, *b, *tmp;
	
	a = b = 0;
	
	a = BuildSyntaxTree5();
	AVOID (a == NULL);
	if (cToken == ttHat)
	{
		NEXT_TOKEN();
		b = BuildSyntaxTree4();
		AVOID(b == NULL);
		tmp = NewTreeNode(ttHat, a, b);
		AVOID(tmp == NULL);
		return tmp;
	}
	else
	{
		return a;
	}

	// Error handling
	STError4:
	DestroySyntaxTree(a);
	DestroySyntaxTree(b);
	
	return NULL;	
	#undef ERRLABEL
}

// Handling () operators, constants and variables
TreeNode_t *BuildSyntaxTree5(void)
{
	#define ERRLABEL STError5
	TreeNode_t *a;
	
	a = NULL;
	
	switch (cToken)
	{
		case ttBraceOpen:
			NEXT_TOKEN();
			a = BuildSyntaxTree0();
			AVOID(a == NULL);
			if (cToken != ttBraceClose)
			{
				Error(") expected, but %s found", TOKEN_TABLE[cToken]);
				goto STError5;
			}
			a = NewTreeNode(ttDummy, a, NULL);
			AVOID(a == NULL);
			NEXT_TOKEN();
			break;
		case ttConst:
			a = NewTreeNode(ttConst, NULL, NULL);
			AVOID(a == NULL);
			a->Token = CurToken;
			NEXT_TOKEN();
			break;
		case ttVariable:
			a = NewTreeNode(ttVariable, NULL, NULL);
			AVOID(a == NULL);
			a->Token = CurToken;
			NEXT_TOKEN();
			break;
		case ttX:
			a = NewTreeNode(ttX, NULL, NULL);
			AVOID(a == NULL);
			a->Token = CurToken;
			NEXT_TOKEN();
			break;
		default:
			Error("Unexpected %s", TOKEN_TABLE[cToken]);
			SkipUntilNewline();
			goto STError5;
			break;
	}
	
	return a;
	// Error handling
	STError5:
	DestroySyntaxTree(a);
	
	return NULL;
	#undef ERRLABEL
}

void PrintTree(TreeNode_t *Root)
{
	if (Root == NULL)
	{
		return;
	} if (Root->First != NULL && Root->Second != NULL)
	{
		fprintf(stderr, "(");
		PrintTree(Root->First);
		fprintf(stderr, "%s", TOKEN_TABLE[Root->Token.Token]);
		PrintTree(Root->Second);
		fprintf(stderr, ")");
	}
	else
	{
		switch (Root->Token.Token)
		{
			case ttVariable:
				fprintf(stderr, "%s", Root->Token.VariableName);
				break;
				
			case ttConst:
				fprintf(stderr, "%u", Root->Token.ConstValue);
				break;
				
			case ttX:
				fprintf(stderr, "x");
				break;
			
			case ttDummy:
				fprintf(stderr, "%s", TOKEN_TABLE[Root->Token.Token]);
				PrintTree(Root->First);
				break;
				
			default:
				fprintf(stderr, "%s", TOKEN_TABLE[Root->Token.Token]);
		}
	}
}

/*
	DestroySyntaxTree
	
	INPUT:	Pointer to root of syntax tree
	OUTPUT:	None
	RETURN:	None
	
	Destroys syntax tree freeing all memory
	Note that Root pointer will be illegal after function call
*/
void DestroySyntaxTree(TreeNode_t *Root)
{
	if (Root == NULL)
	{
		return;
	}
	DestroySyntaxTree(Root->First);
	DestroySyntaxTree(Root->Second);
	free((void *)Root);
}
