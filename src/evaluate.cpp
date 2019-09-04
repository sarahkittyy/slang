#include "evaluate.hpp"

using interpreter::env;
using interpreter::variable;
using parser::tree_node;

namespace eval
{

void assignment(env& state, const tree_node& node)
{
	tree_node lhs = node.at(0);
	tree_node rhs = node.at(2);

	if (lhs.type() != "identifier") throw std::runtime_error("Invalid syntax in assignment.");

	if (rhs.type() == "identifier")
	{
		auto var = state.vars.find(rhs.value());
		if (var == state.vars.end())
		{
			throw std::runtime_error("Identifier " + rhs.value() + " undefined.");
		}
		state.vars[lhs.value()] = state.vars[rhs.value()];
	}
	else
	{
		state.vars[lhs.value()].val  = rhs.value();
		state.vars[lhs.value()].type = rhs.type();
	}
}

}