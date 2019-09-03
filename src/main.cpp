#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include "lexer.hpp"

int main(int argc, char** argv)
{
	// Create the options
	cxxopts::Options options("slang", "A simple programming language.");

	// clang-format off
	options.add_options()
		("h,help", "Print this help dialog")
		("i,input", "The input file to interpret", cxxopts::value<std::string>());
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

	// Lex the code.
	auto tokens = lexer::lex(code);
	// Check for token errors.
	if (tokens.back().type == "ERROR")
	{
		std::cerr << "Lexer failed.\nError: " << tokens.back().value << std::endl;
		return -1;
	}

	for (auto& tok : tokens)
	{
		std::cout << tok.type << ": " << tok.value << std::endl;
	}

	return 0;
}