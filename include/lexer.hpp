#pragma once

#include <string>
#include <vector>

namespace lexer
{

/**
 * @brief A single token.
 * 
 */
struct token
{
	std::string type;
	std::string value;
};

/**
 * @brief Tokenizes the input into a list of valid tokens.
 * 
 * @param code The input code.
 * @return std::vector<token> A list of valid tokens.
 */
std::vector<token> lex(const std::string& code);

}