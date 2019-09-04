#include <regex>
#include <stdexcept>
#include <tuple>
#include "lexer.hpp"

namespace lexer
{

/**
 * @brief Associates a token type with a regex matcher.
 * 
 */
struct matcher
{
	matcher(std::string type, std::string pattern)
		: type(type), regex(pattern)
	{
	}

	/**
	 * @brief Tries to match the next few characters in the code with this matcher's pattern.
	 * 
	 * @param code The code
	 * @return std::tuple<bool>
	 * 	1: bool -> If it successfully matched.
	 * 	2: int -> Length of match
	 * 	3: string -> matched token value
	 */
	std::tuple<bool, int, std::string> try_match(const std::string& code) const
	{
		std::cmatch m;
		if (std::regex_search(code.c_str(), m, regex) && m.position() == 0)
		{
			return {
				true, m.length(), m.str()
			};
		}
		else
		{
			return {
				false, -1, ""
			};
		}
	}

	std::string type;
	std::regex regex;
};

/// All valid tokens in the language.
std::vector<matcher> valid_tokens = {
	matcher("identifier", "[A-Za-z_$][A-Za-z0-9]*"),
	matcher("number", "[0-9]+"),
	matcher("string", "\".*\""),
	matcher("operator", "\\+|-|\\*|/|="),
	matcher("separator", ";|\n"),
	matcher("parens", "\\(|\\)"),
	matcher("colon", ":"),
	matcher("comma", ",")
};

/// Strips leading and trailing whitespace off input string.
std::string strip(const std::string& in)
{
	return std::regex_replace(in, std::regex("[^\\S\n]*(.*)[^\\S\n]*"), "$1");
}

/**
 * @brief Consumes the next valid token from the code string.
 * 
 * @param code RW access to the input code.
 * @return token The next valid token in the code.
 */
token next_token(std::string& code)
{
	for (auto& matcher : valid_tokens)
	{
		auto [success, length, value] = matcher.try_match(code);
		if (success)
		{
			code = strip(code.substr(length));
			return {
				matcher.type, value
			};
		}
	}
	// This code here is only reached if no token matched, indicating a syntax error.
	token err = { "ERROR", "Syntax Error at: " + code.substr(0, 10) + "..." };
	code	  = "";   // this is to terminate the lexer early.
	return err;
}

std::vector<token> lex(const std::string& code)
{
	// Copy code for writing.
	std::string code_rw = code;

	// vector of tokens
	std::vector<token> tokens;

	// While there is still code left
	while (code_rw.size() != 0)
	{
		// gobble up the next token, and append it to the vector.
		tokens.push_back(next_token(code_rw));
	}

	// return the tokens.
	return tokens;
}

}