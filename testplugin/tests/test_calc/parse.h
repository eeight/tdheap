#ifndef _PARSE_H_
#define _PARSE_H_

#define MAX_STRING_LENGTH 63

typedef unsigned int UINT;
typedef unsigned long long int ULONG;
typedef long long int LONG;

// Token types
enum 
{
	ttAsterisk = 1,		// *
	ttAsteriskSolid,	// *
	ttBraceClose,		// (
	ttBraceOpen,		// )
	ttHat,				// ^
	ttConst,			// 123
	ttDummy,			// <especially for tree>
	ttEOF,				// ^D <end of file>
	ttEOS,				// ; <end of statement>
	ttEqual,			// =
	ttMinus,			// -
	ttNone,				// <nothing>
	ttPercent,			// %
	ttPlus,				// +
	ttSlash,			// /
	ttVariable,			// var
	ttX					// x
};

// The way to get a token name in a string by token type
extern char *TOKEN_TABLE[];

struct _Token
{
	int Token;
	int LineNo, Position;

	union
	{
		// Defined only if TokenType == ttVariable, holds variable name
		char VariableName[MAX_STRING_LENGTH + 1];
		// Defined only if TokenType == ttConst, holds constant value
		UINT ConstValue;
	};
};

typedef struct _Token Token_t;

// Current token
extern Token_t CurToken;

// Very useful shortcut
#define cToken CurToken.Token

int GetChar(void);
void UngetChar(int c);
void SkipWhitespace(void);
void SkipUntilNewline(void);
void PrintInvitation(void);
void Error(char *Format, ...);
void WriteCurToken(int TokenType);
int NextToken(void);

#endif
