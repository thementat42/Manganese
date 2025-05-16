/**
 * @file lexer.h
 * @brief This file contains the definition of the lexer functionality for the Manganese compiler.
 */
#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "../../../global_macros.h"
#include "token.h"

MANG_BEGIN
namespace lexer {
using TokenList = std::vector<Token>;

enum class Mode {
    String,  // Source code passed in as a string
    File     // Filename passed in
};

// Only expose the actual tokenization function to other code
// Other functions are internal, to aid with tokenization

/**
 * @brief Splits the source code into tokens
 * @param source Where to get the source code (string -- the raw source code, or a file -- the filename)
 * @param mode The mode to use (string or file). Default is file.
 * @return A vector of tokens (see token.h)
 */
TokenList tokenize(const std::string& source, Mode mode = Mode::File);
}  // namespace lexer
MANG_END

#endif // LEXER_H