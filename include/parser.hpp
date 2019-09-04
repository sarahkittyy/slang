#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "lexer.hpp"

namespace parser
{

class tree_node
{
public:
	tree_node(std::string type, std::string value);

	/**
	 * @brief Add a child to the tree
	 * 
	 * @param node The node to add.
	 * @param pos The position to erase
	 * 
	 * @remarks pos = -1 is the end. pos = 0 is the beginning.
	 * @return tree_node& The tree node added.
	 */
	tree_node& add_child(tree_node node, size_t pos = -1);
	/// Find a child node based on a predicate.
	std::optional<tree_node> find_child(std::function<bool(const tree_node& node)> pred) const;

	/// Remove the n-th child.
	void remove_child(size_t n);

	/// Get the n-th tree node
	tree_node& operator[](size_t n);
	const tree_node& operator[](size_t n) const;
	const tree_node& at(size_t n) const;
	/// Get the amount of children contained in this node.
	size_t size() const;

	/// Check recursively for equality.
	bool operator==(const tree_node& other) const;

	std::string type() const;
	std::string value() const;

	/// For iterating over all children.
	const std::vector<tree_node>& children() const;
	/// For retrieving a slice of all children.
	const std::vector<tree_node> child_slice(size_t begin, size_t end) const;

	/// Get the next tree node from the parent.
	tree_node* next() const;

	/// Convert to a printable string.
	std::string str(std::string prefix = "") const;

private:
	std::string m_type;
	std::string m_value;

	tree_node* m_parent;
	size_t m_index;

	std::vector<tree_node> m_children;
};

tree_node parse(std::vector<lexer::token> tokens);

}