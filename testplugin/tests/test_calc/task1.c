#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "eval.h"
#include "parse.h"
#include "tree.h"

char *HelpMessage = 
"This is a small program which provides some calc functions\n\n"
"All calculations are done out of polynomes from x variable\n"
"above the residue field modulo p\n\n"
"Operations supported are:\n"
"=\twith priority 0 which is also right-associative\n"
"+-\twith priority 1\n"
"*/\twith priority 2\n"
"^\twith priority 3 which is also right-associative\n";

char *UsageMessage = 
"Usage:\t%s [modulo] [--help]\n"
"\tmodulo - modulo of residue field, should be prime in range [3, 2**31-1]\n"
"\n"
"Try --help to get a bit of useful information about this program\n";

int SafeNextToken(void);
void SafeSkipUntilNewline(void);

int main(int argc, char **argv)
{
	int GotError;
	TreeNode_t *SyntaxTree;
	unsigned long long int tmp;
	UINT t;
	Variable_t v;
	
	if (argc < 2)
	{
		printf(UsageMessage, argv[0]);
		return 0;
	}
	if (sscanf(argv[1], "%llu", &tmp) != 1 || tmp > UINT_MAX)
	{
		if (strcmp(argv[1], "--help") != 0)
		{
			printf(UsageMessage, argv[0]);
		}
		else
		{
			printf("%s", HelpMessage);
		}
		return 0;
	}
	pModulo = (UINT)tmp;
	t = (UINT)sqrt((double)pModulo);
	for (; t > 1 && pModulo % t != 0; t--);
	if (pModulo % t == 0 && (t != 1 || pModulo == 1))
	{
		fprintf(stderr, "Prime number expected\n");
		return 1;
	}
	if (argc > 2)
	{
		if (strcmp(argv[2], "--help") == 0)
		{
			printf("%s", HelpMessage);
			return 0;			
		}
		else
		{
			printf(UsageMessage, argv[0]);
			return 0;
		}
	}
	
	WriteCurToken(ttNone);
	
	PrintInvitation();
	GotError = 0;
	v.MonCount = 0;
	v.Mons = NULL;
	InitVariables();
	// Read first token
	GotError |= SafeNextToken();
	while (cToken != ttEOF)
	{
		SyntaxTree = BuildSyntaxTree0();
		if (SyntaxTree != NULL)
		{
			if (cToken != ttEOS)
			{
				Error("; expected but '%s' found", TOKEN_TABLE[cToken]);
				GotError = 1;
				SafeSkipUntilNewline();
			}
			else
			{
				// Readed successfully
				if (Evalute(SyntaxTree, &v))
				{
					if (SyntaxTree->Token.Token != ttEqual)
					{
						PrintVariable(&v);
					}
					DestroyVariable(&v);
					GotError |= SafeNextToken();
				}
				else
				{
					GotError = 1;
					SafeNextToken();
				}				
			}
			DestroySyntaxTree(SyntaxTree);
		}
		else
		{
			GotError = 1;
			SafeNextToken();
		}
	}
	
	printf("\n");
	DestroyVariables();
	return GotError;
}

int SafeNextToken(void)
{
	int Result;
	
	Result = 0;
	if (NextToken() == 1)
	{
		return 0;
	}
	while (cToken == ttNone)
	{
		NextToken();
	}
	return 1;
}

void SafeSkipUntilNewline(void)
{
	SkipUntilNewline();
	SafeNextToken();
}


