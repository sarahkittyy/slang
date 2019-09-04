#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include "parser.hpp"

namespace parser
{

//! TREE NODE DEFINITIONS

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

std::optional<tree_node>
tree_node::find_child(std::function<bool(const tree_node& node)> pred) const
{
	for (auto& child : m_children)
	{
		if (pred(child))
		{
			return child;
		}
	}
	return {};
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
	return m_children[n];
}

const tree_node& tree_node::at(size_t n) const
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

std::string tree_node::str(std::string prefix) const
{
	std::ostringstream ss;
	ss << prefix << m_type << ": " << m_value << "\n";

	for (auto& child : m_children)
	{
		ss << child.str(".." + prefix);
	}

	return ss.str();
}

size_t tree_node::depth() const
{
	size_t depth	  = 0;
	tree_node* parent = m_parent;
	if (parent == nullptr)
	{
		return 0;
	}

	do
	{
		parent = parent->m_parent;
		depth++;
	} while (parent != nullptr);

	return depth;
}

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
						//! Here we retrieve the requested node type and value.
						std::string type;
						std::string value;
						bool negate_value = false;
						type = value = "";
						// Init name.
						if (size_t split_pos = test.find(':'); split_pos != std::string::npos)
						{
							type  = test.substr(0, split_pos);
							value = test.substr(split_pos + 1);
						}
						else if (size_t split_pos = test.find(';'); split_pos != std::string::npos)
						{
							type  = test.substr(0, split_pos);
							value = test.substr(split_pos + 1);
							negate_value = true;
						}
						else
						{
							type = test;
						}
						
						//* Checks if a given node matches the test type/value.
						auto matches = [&type, &value, &negate_value](const tree_node& n) -> bool{
							bool success = n.type() == type;
							if(value == "")
								return success;
							else
								return success && ((n.value() == value) != negate_value);
						};

						if (type.back() == '+') //* Greedy regex operator +
						{
							type.pop_back();

							// If not even one matches, it's false.
							if (!matches(node))
								return false;
								
							// Otherwise, we guarentee a length of one.
							captured_length = 1;

							// We now iterate over all following available nodes,
							// until one doesn't match.
							tree_node* next = node.next();
							do
							{	
								if(!matches(*next)) break;
								
								captured_length++;
							} while ((next = next->next()));
							
							return true;
						}
						else if (type.back() == '*' && type.size() > 1) //* Greedy regex-like operator *
						{
							type.pop_back();
							
							// Unknown how many captures left.
							captured_length = 0;
							
							// Check the first node.
							if(matches(node)) captured_length++;
							
							// Now we go through all following nodes, accumulating matching ones.
							tree_node* next = node.next();
							do
							{
								if(!matches(*next)) break;
								
								captured_length++;
							} while((next = next->next()));
							
							return true; // 0 or more matches will return true always anyway.
						}
						else if (type.back() == '?') //* one or none.
						{
							type.pop_back();
							
							captured_length = (size_t)matches(node);
							
							return true;
						}
						else
						{
							// Types must match, and the value should match only if the value isn't empty.
							if(matches(node))
							{
								captured_length = 1;
								return true;
							}
							
							return false;
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
	/// If there's no match expression, there'll just be a simple regex.
	std::regex regex;
	/// Minimum depth into the parent node to be considered.
	int layer;

	/// Default constructor
	parse_node(std::string type, std::initializer_list<std::string> match_expr, int layer = 0)
		: type(type), match_expr(match_expr), layer(layer)
	{
	}

	parse_node(std::string type, std::string match_expr, int layer = 0)
		: type(type), regex(match_expr), layer(layer)
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
		//* First, check if there's no match expr, and it's just a regex.
		if (match_expr.size() == 0)
		{
			const tree_node& node = program[begin];
			if (std::regex_match(node.value(), regex))
			{
				return {
					true, 1, program.child_slice(begin, begin + 1)
				};
			}
			else
			{
				return { false, -1, {} };
			}
		}   // now it's defs not a regex expression.

		// Iterate over all match criteria
		size_t token_length_sum = 0;
		for (size_t i = 0; i < match_expr.size(); ++i)
		{
			// Get the active match expr.
			std::string expr = match_expr[i];
			// Get active node
			if (token_length_sum + begin >= program.size()) return { false, -1, {} };
			const tree_node& node = program[token_length_sum + begin];

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

/**
 * * Match expressions
 * 
 * Substitutions:
 * name => basic token type
 * name:value => basic token type with value `value`
 * name;value => basic token type except value `value`
 * 
 * | => logical OR
 * \* => any amount
 * + => one or more
 * \? => doesn't have to be there
 * 
 */

/**
 * @brief All available compound expression matchers.
 * 
 */
static std::vector<parse_node> expressions = {
	parse_node("arithmetic", { "identifier|number|string", "operator;=", "identifier|number|string" }),
	parse_node("assignment", { "identifier", "operator:=", "number|string|identifier|expression" }),
	parse_node("nop", { "separator+" }),
	parse_node("expression", { "parens:(", "expression|arithmetic", "parens:)" })
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
	tree_node initial(program);
	// Resulting tree_node.
	tree_node result(program.type(), program.value());

	// For every expression type...
	for (auto& expr : expressions)
	{
		// Iterate over all tokens,
		for (size_t i = 0; i < initial.size(); ++i)
		{
			// Attempt to match the current set of tokens with the current expression.
			auto [success,   // Whether or not it was a successful match.
				  length,	// The length of the successful match
				  toks] =	// The matched tokens, to be moved as a child of the new node.
				expr.try_match(initial, i);

			if (success)
			{
				// Append a new child.
				tree_node& new_node = result.add_child({ expr.type, "" });

				// Iterate over all sub-matched tokens, and add them to the new child.
				for (size_t i = 0; i < length; ++i)
				{
					new_node.add_child(toks[i]);
				}
			}
			else
			{
				result.add_child(initial[i]);
			}
		}
		initial = result;
		result  = tree_node(result.type(), result.value());
	}

	// If nothing changed, we can stop recursing.
	if (initial == program)
	{
		return initial;
	}
	else
	{
		// Otherwise, there's still more to run through, recurse deeper.
		return run_through(initial);
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