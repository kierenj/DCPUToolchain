/**

	File:		NExpressionStatement.h

	Project:	DCPU-16 Tools
	Component:	LibDCPU-ci-lang-c

	Authors:	James Rhodes

	Description:	Declares the NExpressionStatement AST class.

**/

#ifndef __DCPU_COMP_NODES_EXPRESSION_STATEMENT_H
#define __DCPU_COMP_NODES_EXPRESSION_STATEMENT_H

#include "NStatement.h"
#include "NExpression.h"

class NExpressionStatement : public NStatement
{
	public:
		NExpression& expression;
		int line_number;
		NExpressionStatement(int line_num, NExpression& expression) :
			line_number(line_num), expression(expression), NStatement("expression") { }
		virtual AsmBlock* compile(AsmGenerator& context);
		virtual AsmBlock* reference(AsmGenerator& context);
};

#endif
