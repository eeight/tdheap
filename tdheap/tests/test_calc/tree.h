#ifndef _TREE_H_
#define _TREE_H_

#include "parse.h"

struct _TreeNode
{
	Token_t Token;
	
	// Two child nodes
	struct _TreeNode *First, *Second;
};
typedef struct _TreeNode TreeNode_t;

TreeNode_t *BuildSyntaxTree0(void);
TreeNode_t *BuildSyntaxTree1(void);
TreeNode_t *BuildSyntaxTree2(void);
TreeNode_t *BuildSyntaxTree3(void);
TreeNode_t *BuildSyntaxTree4(void);
TreeNode_t *BuildSyntaxTree5(void);

// For debug use only
void PrintTree(TreeNode_t *Root);

void DestroySyntaxTree(TreeNode_t *Root);

#endif
