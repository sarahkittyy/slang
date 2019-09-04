#pragma once

#include <iostream>

/**
 * @brief Manages what does and doesn't get sent to stdout based on the verbosity parameter.
 * 
 */
class output
{
public:
	/// Initialize with the global accepted verbosity level.
	output(int verbosity);

	/**
	 * @brief Output to stdout. Will not output anything with a higher verbosity than the intended one.
	 * 
	 * @tparam Printable Any type that can be passed to std::ostream::operator<<
	 * @param intended_verbosity The indended verbosity of the message to be printed.
	 * @param p The string / variable to send to stdout.
	 * @return output& *this
	 */
	template <typename Printable>
	output& operator()(int intended_verbosity, Printable p)
	{
		if (intended_verbosity <= m_verbosity)
		{
			std::cout << p;
		}
		return *this;
	}

private:
	/// The output verbosity.
	int m_verbosity;
};