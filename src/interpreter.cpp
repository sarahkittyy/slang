#include <iostream>
#include <unordered_map>
#include "evaluate.hpp"
#include "interpreter.hpp"

namespace interpreter
{

/// The singleton persistent state.
env state = env();

env interpret(parser::tree_node& code)
{
	for (auto& node : code.children())
	{
		if (node.type() == "assignment")
		{
			eval::assignment(state, node);
		}
	}

	return state;
}

}