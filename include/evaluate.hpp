#pragma once

#include "interpreter.hpp"
#include "parser.hpp"

namespace eval
{

void assignment(interpreter::env& state, const parser::tree_node& node);

}