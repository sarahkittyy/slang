#pragma once

#include <string>

namespace preprocessor
{

/**
 * @brief For any extra necessary code preprocessing.
 * 
 * @param in The raw code file's contents
 * @return std::string The pre-processed code.
 */
std::string preprocess(const std::string& in);

}