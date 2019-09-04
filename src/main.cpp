#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include "interpreter.hpp"
#include "lexer.hpp"
#include "output.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"

int main(int argc, char** argv)
{
	// Create the options
	cxxopts::Options options("slang", "A simple programming language.");

	// clang-format off
	options.add_options()
		("h,help", "Print this help dialog")
		("i,input", "The input file to interpret", cxxopts::value<std::string>())
		("v,verbose", "Increases the verbosity.");
	// clang-format on

	options.parse_positional({ "input" });

	// Parse options
	auto result = options.parse(argc, argv);

	// Print help if needed
	if (result["help"].count() != 0 || result.arguments().size() == 0)
	{
		std::cout << options.help();
		return 0;
	}

	// Check for the input file
	if (result["input"].count() == 0)
	{
		std::cerr << "No input file specified!";
		return -1;
	}

	// Get verbosity.
	int verbosity = result["verbose"].count();

	// Initialize stdout.
	output out(verbosity);
	out(1, "Reading input file...\n");

	// Get the input file
	std::string input = result["input"].as<std::string>();
	// Read the input file
	std::ifstream file(input);
	if (!file)
	{
		std::cerr << "Could not open source file for reading!";
		return -1;
	}
	std::string code = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	file.close();
	out(1, "File read successfully.\n");

	out(3, "Preprocessing...\n");
	code = preprocessor::preprocess(code);
	out(3, "Preprocessed!\n");

	out(3, "Lexing code for tokens.\n");
	// Lex the code.
	auto tokens = lexer::lex(code);
	if (tokens.size() == 0)
	{
		std::cerr << "Input file is empty.\n";
		return -1;
	}

	// Check for token errors.
	if (tokens.back().type == "ERROR")
	{
		std::cerr << "Lexer failed.\nError: " << tokens.back().value << std::endl;
		return -1;
	}

	// Print all tokens.
	out(3, "\nTokens retrieved. Tokens:\n--\n");
	for (auto& tok : tokens)
	{
		out(3, tok.type + ": " + tok.value + "\n");
	}

	// parse tokens
	out(3, "\nParsing tokens...");
	auto parsed = parser::parse(tokens);

	out(3, "\nParsing complete. Parse tree:\n");
	out(3, parsed.str());

	// Begin interpreting the code.
	out(0, "-- slang interpreter begin --\n");

	auto end_state = interpreter::interpret(parsed);

	out(0, "\n-- slang interpreter end --");

	std::ostringstream ss;
	for (auto& var : end_state.vars)
	{
		ss << var.first << ": " << var.second.type << " = " << var.second.val << std::endl;
	}
	out(3, "\nEnding variable trace:\n" + ss.str());

	return 0;
}