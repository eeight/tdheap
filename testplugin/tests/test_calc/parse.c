#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

#include "parse.h"

#define UNGET_BUF_LEN 2

int UngetBuffer[UNGET_BUF_LEN];
int LastLineEnd;
int UngetIndex = -1;

Token_t CurToken;
int CurLineNo = 0, CurPosition = 0;
int IsCharReturned = 0;
int IsAsteriskCached = 0;

char *TOKEN_TABLE[] = 
{
	NULL, // unused
	"*",
	"*",
	")",
	"(",
	"^",
	"number",
	"<internal>::dummy",
	"end-of-file",
	";",
	"=",
	"-",
	"<internal>::nothing",
	"%",
	"+",
	"/",
	"variable name",
	"mononial"	
};


int GetChar(void)
{
	int c;
	int a;
	
	if (UngetIndex < 0)
	{
		c = getc(stdin);
		a = 1;
	}
	else
	{
		c = UngetBuffer[UngetIndex--];
		a = 0;
	}
	if (c == '\n')
	{
		CurLineNo++;
		LastLineEnd = CurPosition;
		CurPosition = 0;
		if (a == 1)
		{
			PrintInvitation();
		}
	}
	else
	{
		CurPosition++;
	}
	return c;
}

void UngetChar(int c)
{
	if (UngetIndex < UNGET_BUF_LEN - 1)
	{
		UngetBuffer[++UngetIndex] = c;
		if (c == '\n')
		{
			CurPosition = LastLineEnd;
			CurLineNo--;
		}
		else
		{
			CurPosition--;
		}
	}
	else
	{
		// This should never happen
		exit(1);
	}
}

void SkipWhitespace(void)
{
	int c;
	
	do
	{
		c = GetChar();
	} while (isspace(c));
	UngetChar(c);
}

void SkipUntilNewline(void)
{
	int c;
	
	do
	{
		c = GetChar();
	} while (c != '\n' && c != EOF);
	
	if (c == EOF)
	{
		WriteCurToken(ttEOF);
	}
}

void PrintInvitation(void)
{
	fprintf(stderr, "%-4d>>", CurLineNo + 1);
}

void Error(char *Format, ...)
{
	va_list List;
	
	if (cToken != ttNone)
	{
		fprintf(stderr, "Parse error at %d:%d:\t", CurToken.LineNo + 1, CurToken.Position);
	}
	else
	{
		fprintf(stderr, "Parse error at %d:%d:\t", CurLineNo + 1, CurPosition + 1);
	}
	
	va_start(List, Format);
	vfprintf(stderr, Format, List);
	fprintf(stderr, "\n");
	va_end(List);
}

void WriteCurToken(int Token)
{
	CurToken.Token = Token;
	CurToken.LineNo = CurLineNo;
	CurToken.Position = CurPosition;
}

/*
	_NextToken
	INPUT:	None
	OUTPUT:	Next token in variable CurToken
	RETURN:	Non-null if token read, null on failure
	
	Reads next token from standard input, controls some errors
*/
int _NextToken(void)
{
	int c;
	int i;
	
	if (IsAsteriskCached)
	{
		IsAsteriskCached = 0;
#ifdef SOLID_MONS
		WriteCurToken(ttAsteriskSolid);
#else
		WriteCurToken(ttAsterisk);
#endif
		return 1;
	}
	SkipWhitespace();
	c = GetChar();
	
	if (isalpha(c))
	{
		// Write current line number and position into token
		WriteCurToken(ttNone);
		// There must be variable name
		for (i = 0; isalnum(c) && i < MAX_STRING_LENGTH; i++)
		{
			CurToken.VariableName[i] = c;
			c = GetChar();
		}
		
		if (i == MAX_STRING_LENGTH)
		{
			Error("Variable name too long");
			return 0;
		}
		CurToken.VariableName[i] = 0;
		if (CurToken.VariableName[0] == 'x' && CurToken.VariableName[1] == '\0')
		{
			CurToken.Token = ttX;
		}
		else
		{
			// Variable name readed succesfully
			CurToken.Token = ttVariable;
		}
		// Return last char to stdin
		UngetChar(c);
		return 1;
	}
	if (isdigit(c))
	{
		// There must be constant
		unsigned long long int Result;
		
		Result = 0;
		while (isdigit(c))
		{
			Result = Result*10 + c - '0';
			c = GetChar();
			if (Result > UINT_MAX)
			{
				break;
			}
		}
		if (Result > UINT_MAX)
		{
			Error("Too big number");
			return 0;
		}
		if (c == 'x')
		{
			// We must determine whether is x of just a variable with name starting with x
			int d;
			
			// Look one symbol forward
			d = GetChar();
			if (!isalnum(d))
			{
				// Really x
				// Cache '*'
				IsAsteriskCached = 1;
			}
			// Put this symbol back
			UngetChar(d);			
		}
		WriteCurToken(ttConst);
		CurToken.ConstValue = Result;
		UngetChar(c);
		return 1;
	}
	switch (c)
	{
		case EOF:
			WriteCurToken(ttEOF);
			break;
			
		case ';':
			WriteCurToken(ttEOS);
			break;
			
		case 'x':
			WriteCurToken(ttX);
			break;
			
		case '(':
			WriteCurToken(ttBraceOpen);
			break;
			
		case ')':
			WriteCurToken(ttBraceClose);
			break;
			
		case '=':
			WriteCurToken(ttEqual);
			break;
			
		case '+':
			WriteCurToken(ttPlus);
			break;
			
		case '-':
			WriteCurToken(ttMinus);
			break;
			
		case '*':
			WriteCurToken(ttAsterisk);
			break;
			
		case '/':
			WriteCurToken(ttSlash);
			break;
			
		case '%':
			WriteCurToken(ttPercent);
			break;
			
		case '^':
			WriteCurToken(ttHat);
			break;
			
		default:
			WriteCurToken(ttNone);
			Error("Unexpected '%c' [%d] symbol", c, c);
			return 0;
			break;
	}
	return 1;
}

int NextToken(void)
{
	int Result;
	
	Result = _NextToken();
	if (Result == 0)
	{
		SkipUntilNewline();
	}
	return Result;
}

