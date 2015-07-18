//
//  ast_nodes.cpp
//  x86Emulator
//
//  Created by Félix on 2015-07-03.
//  Copyright © 2015 Félix Cloutier. All rights reserved.
//

#include "ast_nodes.h"

SILENCE_LLVM_WARNINGS_BEGIN()
#include <llvm/Support/raw_os_ostream.h>
SILENCE_LLVM_WARNINGS_END()

#include <iostream>
#include <string>

using namespace llvm;
using namespace std;

namespace
{
	template<typename T, size_t N>
	constexpr size_t countof(const T (&)[N])
	{
		return N;
	}
	
	inline string indent(unsigned times)
	{
		return string(times, '\t');
	}
	
	string unaryOperators[] = {
		[UnaryOperatorExpression::LogicalNegate] = "!",
	};
	
	string binaryOperators[] = {
		[NAryOperatorExpression::ShortCircuitAnd] = "&&",
		[NAryOperatorExpression::ShortCircuitOr] = "||",
		[NAryOperatorExpression::Equality] = "==",
	};
	
	unsigned operatorPrecedence[] = {
		[NAryOperatorExpression::Equality] = 7,
		[NAryOperatorExpression::ShortCircuitAnd] = 11,
		[NAryOperatorExpression::ShortCircuitOr] = 12,
	};
	
	static_assert(countof(unaryOperators) == UnaryOperatorExpression::Max, "Incorrect number of strings for unary operators");
	static_assert(countof(binaryOperators) == NAryOperatorExpression::Max, "Incorrect number of strings for binary operators");
	static_assert(countof(operatorPrecedence) == NAryOperatorExpression::Max, "Incorrect number of operator precedences for binary operators");
	
	constexpr char nl = '\n';
	
	BreakNode breakNode;
	TokenExpression trueExpression("true");
	TokenExpression falseExpression("false");
}

void Statement::dump() const
{
	raw_os_ostream rerr(cerr);
	print(rerr);
}

void SequenceNode::print(llvm::raw_ostream &os, unsigned int indent) const
{
	os << ::indent(indent) << '{' << nl;
	for (size_t i = 0; i < statements.size(); i++)
	{
		statements[i]->print(os, indent + 1);
	}
	os << ::indent(indent) << '}' << nl;
}

void IfElseNode::print(llvm::raw_ostream &os, unsigned int indent) const
{
	os << ::indent(indent) << "if ";
	condition->print(os);
	os << nl;
	
	ifBody->print(os, indent + !isa<SequenceNode>(ifBody));
	if (elseBody != nullptr)
	{
		os << ::indent(indent) << "else" << nl;
		elseBody->print(os, indent + !isa<SequenceNode>(elseBody));
	}
}

LoopNode::LoopNode(Statement* body)
: condition(TokenExpression::trueExpression), position(LoopNode::PreTested), loopBody(body)
{
}

void LoopNode::print(llvm::raw_ostream &os, unsigned int indent) const
{
	if (position == PreTested)
	{
		os << ::indent(indent) << "while ";
		condition->print(os);
		os << nl;
		loopBody->print(os, indent + !isa<SequenceNode>(loopBody));
	}
	else
	{
		assert(position == PostTested);
		os << ::indent(indent) << "do" << nl;
		loopBody->print(os, indent + !isa<SequenceNode>(loopBody));
		os << ::indent(indent) << "while ";
		condition->print(os);
		os << nl;
	}
}

BreakNode* BreakNode::breakNode = &::breakNode;

void BreakNode::print(llvm::raw_ostream &os, unsigned int indent) const
{
	os << ::indent(indent) << "break;" << nl;
}

void ExpressionNode::print(llvm::raw_ostream &os, unsigned int indent) const
{
	os << ::indent(indent);
	if (auto valueNode = dyn_cast<ValueExpression>(expression))
	{
		valueNode->value->print(os);
	}
	else
	{
		expression->print(os);
	}
	os << nl;
}

void ValueExpression::print(llvm::raw_ostream &os) const
{
	os << '(';
	value->printAsOperand(os);
	os << ')';
}

void UnaryOperatorExpression::print(llvm::raw_ostream &os) const
{
	os << (type < Max ? unaryOperators[type] : "<bad unary>");
	operand->print(os);
}

void NAryOperatorExpression::addOperand(Expression *expression)
{
	if (auto asNAry = dyn_cast<NAryOperatorExpression>(expression))
	{
		if (asNAry->type == type)
		{
			operands.push_back(asNAry->operands.begin(), asNAry->operands.end());
			return;
		}
	}
	operands.push_back(expression);
}

void NAryOperatorExpression::print(llvm::raw_ostream &os) const
{
	assert(operands.size() > 1);
	
	auto iter = operands.begin();
	print(os, *iter);
	++iter;
	for (; iter != operands.end(); ++iter)
	{
		os << ' ' << (type < Max ? binaryOperators[type] : "<bad operator>") << ' ';
		print(os, *iter);
	}
}

void NAryOperatorExpression::print(raw_ostream& os, Expression* expr) const
{
	bool parenthesize = false;
	if (auto asNAry = dyn_cast<NAryOperatorExpression>(expr))
	{
		parenthesize = operatorPrecedence[asNAry->type] > operatorPrecedence[type];
	}
	
	if (parenthesize) os << '(';
	expr->print(os);
	if (parenthesize) os << ')';
}

TokenExpression* TokenExpression::trueExpression = &::trueExpression;
TokenExpression* TokenExpression::falseExpression = &::falseExpression;

void TokenExpression::print(llvm::raw_ostream &os) const
{
	os << token;
}
