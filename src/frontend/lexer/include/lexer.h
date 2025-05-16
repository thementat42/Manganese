#ifndef LEXER_H
#define LEXER_H

#include <memory>
#include <optional>
#include <queue>

#include "../../../global_macros.h"
#include "../../../io/include/filereader.h"
#include "../../../io/include/reader.h"
#include "../../../io/include/stringreader.h"
#include "keywords.h"
#include "operators.h"
#include "token.h"

MANG_BEGIN
namespace lexer {
using str = std::string;
using std::optional;
using std::unique_ptr;
constexpr auto NONE = std::nullopt;

// ~ Constants for UTF-8 encoding
constexpr uint32_t UTF8_1B_MAX = 0x7F;
constexpr uint32_t UTF8_2B_MAX = 0x7FF;
constexpr uint32_t UTF8_3B_MAX = 0xFFFF;
constexpr uint32_t UTF8_4B_MAX = 0x10FFFF;

constexpr uint8_t UTF8_2B_PRE = 0xC0;
constexpr uint8_t UTF8_3B_PRE = 0xE0;
constexpr uint8_t UTF8_4B_PRE = 0xF0;
constexpr uint8_t UTF8_CONT_PRE = 0x80;

constexpr uint8_t UTF8_1B_MASK = 0x7F;
constexpr uint8_t UTF8_2B_MASK = 0x1F;
constexpr uint8_t UTF8_3B_MASK = 0x0F;
constexpr uint8_t UTF8_4B_MASK = 0x07;
constexpr uint8_t UTF8_CONT_MASK = 0x3F;

constexpr uint8_t UTF8_CONT_SHIFT = 6;
constexpr uint8_t UTF8_2B_SHIFT = 12;
constexpr uint8_t UTF8_3B_SHIFT = 18;

enum class Mode {
    String,  // Source code passed in as a string
    File     // Filename passed in
};

class Lexer {
   private:  // private variables
    std::unique_ptr<io::Reader> reader;

   public:  // public variables
    std::queue<Token> tokenStream;
    bool isTokenizingDone;

   private:  // private methods

   //~ Main tokenization functions
    void tokenizeCharLiteral();
    void tokenizeStringLiteral();
    void tokenizeNumber();
    void tokenizeKeywordOrIdentifier();
    void tokenizeSymbol();
    void makeTokens(size_t numTokens = 1);

    //~ Helper tokenization functions
    void processCharEscapeSequence(const str& charLiteral);
    void skipMultilineComment();

    //~ Reader wrapper functions
    inline char peekChar(size_t offset = 0) { return reader->peekChar(offset); }
    [[nodiscard]] inline char consumeChar() { return reader->consumeChar(); }
    inline size_t getLine() { return reader->getLine(); }
    inline size_t getCol() { return reader->getColumn(); }
    inline size_t getPosition() { return reader->getPosition(); }
    inline void advance(size_t n = 1) {
        reader->setPosition(reader->getPosition() + n);
    }
    inline bool done() { return reader->done(); }

   public:   // public methods
    Lexer(const str& source, Mode mode = Mode::File);
    ~Lexer() = default;

    Token peekToken(int offset = 0);
    Token consumeToken();
};
}  // namespace lexer
MANG_END

#endif  // LEXER_H