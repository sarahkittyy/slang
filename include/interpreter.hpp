#pragma once

#include <string>
#include "parser.hpp"

namespace interpreter
{

/**
 * @brief A basic variable structure.
 * 
 */
struct variable
{

	std::string type;
	std::string val;
};

/**
 * @brief The interpreter's persistent state.
 * 
 */
struct env
{
	env()
	{
		vars.clear();
	}

	std::unordered_map<std::string, variable> vars;
};

/**
 * @brief Interpret the lexed and parsed code.
 * 
 */
env interpret(parser::tree_node& parsed_code);

}