#include "evaluate.hpp"

using interpreter::env;
using interpreter::variable;
using parser::tree_node;

namespace eval
{

std::optional<variable> get_variable(env& state, std::string name)
{
	try
	{
		return state.vars.at("name");
	}
	catch (std::out_of_range e)
	{
		return {};
	}
}

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
	else if (rhs.type() == "expression")
	{
		state.vars[lhs.value()] = expression(state, rhs);
	}
	else
	{
		state.vars[lhs.value()].val  = rhs.value();
		state.vars[lhs.value()].type = rhs.type();
	}
}

variable expression(env& state, const tree_node& node)
{
	variable ret;

	std::optional<tree_node> math_node =
		node.find_child([](const parser::tree_node& n) -> bool {
			return n.type() == "arithmetic";
		});
	if (math_node.has_value())
	{
		ret = arithmetic(state, math_node.value());
	}

	return ret;
}

variable arithmetic(interpreter::env& state, const parser::tree_node& node)
{
	tree_node lhs  = node.at(0);
	tree_node oper = node.at(1);
	tree_node rhs  = node.at(2);

	variable ret;

	ret.type = "number";
	ret.val  = "5";

	return ret;
}

}