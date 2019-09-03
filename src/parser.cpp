#include <algorithm>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <tuple>
#include "parser.hpp"

namespace parser
{

tree_node::tree_node(std::string type, std::string value)
	: m_type(type), m_value(value)
{
}

tree_node& tree_node::add_child(tree_node node, size_t pos)
{
	if (pos == -1)
	{
		m_children.push_back(node);
		return m_children.back();
	}
	else
	{
		return *m_children.insert(m_children.cbegin() + pos, node);
	}
}

tree_node* tree_node::find_child(std::function<bool(const tree_node& node)> pred)
{
	for (auto& child : m_children)
	{
		if (pred(child))
		{
			return &child;
		}
	}
	return nullptr;
}

void tree_node::remove_child(size_t n)
{
	m_children.erase(m_children.begin() + n);
}

tree_node& tree_node::operator[](size_t n)
{
	return m_children[n];
}
const tree_node& tree_node::operator[](size_t n) const
{
	return m_children.at(n);
}

size_t tree_node::size() const
{
	return m_children.size();
}

std::string tree_node::type() const
{
	return m_type;
}

std::string tree_node::value() const
{
	return m_value;
}

const std::vector<tree_node>& tree_node::children() const
{
	return m_children;
}

const std::vector<tree_node> tree_node::child_slice(size_t begin, size_t end) const
{
	std::vector<tree_node> ret;

	for (size_t i = begin; i < end; ++i)
	{
		ret.push_back(m_children.at(i));
	}

	return ret;
}

bool tree_node::operator==(const tree_node& other) const
{
	return m_type == other.m_type &&
		   m_value == other.m_value &&
		   std::equal(m_children.begin(), m_children.end(), other.m_children.begin());
}

/**
 * * Match expressions
 * 
 * Substitutions:
 * name => basic token type
 * name:value => basic token type with value `value`
 * [name] => parse node type, when recursively descending.
 * 
 * | => logical OR
 */

/**
 * @brief Returns true if the given tree node matches the given expression.
 * 
 * @param expr The non-compounded match expression
 * @param node The node to attempt to match against.
 */
bool does_match_expr(const std::string& expr, const tree_node& node)
{
	// First, split expression at all logical or-s
	std::vector<std::string> split = {};
	std::string current			   = "";
	for (size_t i = 0; i < expr.size(); ++i)
	{
		if (expr[i] == '|' && i > 0 && expr[i - 1] != '\\')
		{
			split.push_back(current);
			current = "";
		}
		else
		{
			current += expr[i];
		}
	}
	split.push_back(current);

	// Test for any matching expression.
	return std::any_of(split.begin(),
					   split.end(),
					   [&node](const std::string& test) -> bool {
						   /// Parse node type
						   if (test.front() == '[' && test.back() == ']')
						   {
							   //TODO
							   return false;
						   }
						   else   // Normal token test
						   {
							   std::string type;
							   std::string value;
							   type = value = "";
							   // Init name.
							   if (size_t split_pos = test.find(':'); split_pos != std::string::npos)
							   {
								   type  = test.substr(0, split_pos);
								   value = test.substr(split_pos + 1);
							   }
							   else
							   {
								   type = test;
							   }

							   // Types must match, and the value should match only if the value isn't empty.
							   return type == node.type() && ((value == "") != (node.value() == value));
						   }
					   });
}

/**
 * @brief A parse node, with a type and a match expression,
 * for defining language syntax.
 * 
 */
struct parse_node
{
	/// The parse node type
	std::string type;
	/// The parse node match expression
	std::vector<std::string> match_expr;

	/// Default constructor
	parse_node(std::string type, std::vector<std::string> match_expr)
		: type(type), match_expr(match_expr)
	{
	}

	/**
	 * @brief Attempt to match the next set of tokens with this parse node's
	 * 
	 * @param program The base tree node to find the expression 
	 * @return std::tuple<bool, int>
	 * @param begin Where to begin in the program tree.
	 * 	1 => success
	 * 	2 => length of match, in tokens to consume.
	 * 	3 => all matched nodes
	 */
	std::tuple<bool, int, std::vector<tree_node>> try_match(const tree_node& program, size_t begin = 0) const
	{
		// Iterate over all match criteria
		for (size_t i = 0; i < match_expr.size(); ++i)
		{
			// Get the active match expr.
			std::string expr = match_expr[i];
			// Get active node
			if (i + begin >= program.size()) return { false, -1, {} };
			const tree_node& node = program[i + begin];

			if (!does_match_expr(expr, node))
			{
				return { false, -1, {} };
			}
		}
		// If here, successful match.
		return {
			true, match_expr.size(), program.child_slice(begin, begin + match_expr.size())
		};
	}
};

std::vector<parse_node> expressions = {
	parse_node("assignment", { "identifier", "operator:=", "number|string" })
};

/**
 * @brief Get the expression with the given type.
 * 
 * @param type The expression type.
 * @return parse_node& The parse node associated with that expression type.
 */
parse_node& get_expression(std::string type)
{
	auto found = std::find_if(expressions.begin(),
							  expressions.end(),
							  [&type](const parse_node& pn) {
								  return pn.type == type;
							  });

	if (found != expressions.end())
	{
		return *found;
	}
	else
	{
		throw std::runtime_error("Parse node of type " + type + " not found ;-;");
	}
}

/**
 * @brief Iterate through the whole program recursively until no more changes are made.
 * Updates chains of tokens / parse nodes with higher level parse nodes.
 * 
 * @param program The base program. 
 */
tree_node run_through(const tree_node& program)
{
	tree_node result("entry", "entry");

	size_t token_index = 0;
	while (token_index < program.size())
		for (auto& expr : expressions)
		{
			auto [success, length, toks] = expr.try_match(program, token_index);
			if (success)
			{
				tree_node& new_node = result.add_child({ expr.type, "" });
				for (size_t i = 0; i < length; ++i)
				{
					new_node.add_child(toks[i]);
				}
				token_index += length;
			}
			else
			{
				result.add_child(program[token_index]);
				token_index++;
			}

			if (token_index >= program.size()) break;
		}

	if (result == program)
	{
		return program;
	}
	else
	{
		return run_through(result);
	}
}

tree_node parse(std::vector<lexer::token> tokens)
{
	/// Program's entry point.
	tree_node program("entry", "entry");
	/// Initialize the tree with the initial tokens
	for (auto& tok : tokens)
	{
		program.add_child(tree_node(tok.type, tok.value));
	}

	program = run_through(program);

	return program;
}


}