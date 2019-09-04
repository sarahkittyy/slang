#include <algorithm>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <tuple>
#include "parser.hpp"

namespace parser
{

tree_node::tree_node(std::string type, std::string value)
	: m_type(type), m_value(value), m_parent(nullptr), m_index(0)
{
}

tree_node& tree_node::add_child(tree_node node, size_t pos)
{
	node.m_parent = this;
	if (pos == -1)
	{
		node.m_index = m_children.size();
		m_children.push_back(node);
		return m_children.back();
	}
	else
	{
		tree_node& inserted = *m_children.insert(m_children.cbegin() + pos, node);
		for (size_t i = 0; i < m_children.size(); ++i)
		{
			m_children[i].m_index = i;
		}
		return inserted;
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

tree_node* tree_node::next() const
{
	if (m_parent == nullptr)
	{
		throw std::runtime_error("Cannot get sibling of parent node.");
	}

	if (m_parent->size() == m_index + 1)
	{
		return nullptr;
	}
	else
	{
		return &((*m_parent)[m_index + 1]);
	}
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
 * \* => any amount
 * + => one or more
 * 
 */

/**
 * @brief Returns true if the given tree node matches the given expression.
 * 
 * @param expr The non-compounded match expression
 * @param node The node to attempt to match against.
 * 
 * @return std::tuple<bool,int>
 * 	1 => matches or not
 * 	2 => amount of matches captured.
 */
std::tuple<bool, int> does_match_expr(const std::string& expr, const tree_node& node)
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

	// For
	size_t captured_length = 0;

	// clang-format off
	// Test for any matching expression.
	return { 
		std::any_of(split.begin(),
					split.end(),
					[&node, &captured_length](const std::string& test) -> bool {
						captured_length = 0;
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

							if (type.back() == '+')
							{
								type.pop_back();
								// Types must match,
								bool this_matches = type == node.type() && ((value == "") != (node.value() == value));

								if (!this_matches)
									return false;

								//... and now we can count how many operators after this we can go

								captured_length = 1;

								tree_node* next = node.next();
								do
								{
									bool matches = next->type() == type && ((value == "") != (next->value() == value));
									
									if(!matches) break;
									
									captured_length++;
								} while ((next = next->next()));
								
								return true;
							}
							else if (type.back() == '*')
							{
								type.pop_back();
								captured_length = 0;
								
								if(type == node.type() && ((value == "") != (node.value() == value))) captured_length++;
								
								tree_node* next = node.next();
								
								do
								{
									bool matches = next->type() == type && ((value == "") != (next->value() == value));
									
									if(!matches) break;
									
									captured_length++;
								} while((next = next->next()));
								
								return true;
							}
							else
							{
								captured_length = 1;
								// Types must match, and the value should match only if the value isn't empty.
								return type == node.type() && ((value == "") != (node.value() == value));
							}
						}
					}),
			 captured_length };
	// clang-format on
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
		size_t token_length_sum = 0;
		for (size_t i = 0; i < match_expr.size(); ++i)
		{
			// Get the active match expr.
			std::string expr = match_expr[i];
			// Get active node
			if (i + begin >= program.size()) return { false, -1, {} };
			const tree_node& node = program[i + begin];

			auto [success, length] = does_match_expr(expr, node);
			if (!success)
			{
				return { false, -1, {} };
			}
			token_length_sum += length;
		}
		// If here, successful match.
		return {
			true, token_length_sum, program.child_slice(begin, begin + token_length_sum)
		};
	}
};

std::vector<parse_node> expressions = {
	parse_node("assignment", { "identifier", "operator:=", "number|string" }),
	parse_node("nop", { "separator+" })
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
			if (token_index >= program.size()) break;

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