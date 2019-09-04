#include <regex>
#include "preprocessor.hpp"

namespace preprocessor
{

std::string preprocess(const std::string& in)
{
	// Move the code into an iss.
	std::istringstream iss(in);
	// Final return string.
	std::string ret;

	// Remove all trailing content after a # indicator.
	bool in_string = false;   // To ignore comments inside strings.
	for (std::string line; std::getline(iss, line);)
	{
		bool comment_found = false;
		// Iterate over all line chars.
		for (size_t i = 0; i < line.size(); ++i)
		{
			char ch = line[i];

			if (ch == '"' && i > 0 && line[i - 1] != '\\')
			{
				in_string = !in_string;
			}
			else if (!in_string && ch == '#')
			{
				ret += line.substr(0, i);
				comment_found = true;
				break;
			}
		}

		if (!comment_found)
		{
			ret += line;
		}
	}

	return ret;
}

}