#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP

#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser/operators.hpp>
#include <io/logging.hpp>
#include <memory>
#include <mnstl/chunk_allocator.hxx>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Manganese::parser {
using lexer::TokenType, lexer::Token;

struct Import {
    std::vector<std::string> path;
    std::string alias;
};

struct ParsedFile {
    std::string moduleName;
    std::vector<Import> imports;
    ast::Block program;
};

//~ Helper functions that don't depend on the parser class's methods/variables
std::string importToString(const Import& import);

class Parser {
   private:
    std::unique_ptr<lexer::Lexer> lexer;
    constexpr static inline ast::Visibility defaultVisibility = ast::Visibility::Private;
    std::optional<Token> previousToken;

    std::string moduleName;
    std::vector<Import> imports;
    mnstl::chunk_allocator& arena;

    // Some flags
    struct {
        bool hasParsedFileHeader : 1 = false;  // Processing module and import
        bool hasError : 1 = false;
        bool parsingBlockPrecursor : 1 = false;  // if/for/while, etc.
        bool parsingAliasStatement : 1 = false;
    } flags;

   public:
    Parser(const std::string& source, lexer::Mode mode, mnstl::chunk_allocator& allocatorReference) :
        lexer(std::make_unique<lexer::Lexer>(source, mode)), arena(allocatorReference) {
        initializeLookups();
        initializeTypeLookups();
    }

    ~Parser() noexcept = default;

    ParsedFile parse();

   private:  // private methods
    using statementHandler_t = ast::Statement* (Parser::*)();
    using nudHandler_t = ast::Expression* (Parser::*)();
    using nudHandler_types_t = ast::Type* (Parser::*)();
    using ledHandler_t = ast::Expression* (Parser::*)(ast::Expression*, Precedence);
    using ledHandler_types_t = ast::Type* (Parser::*)(ast::Type*, Precedence);

    //~ Lookups
    constexpr static inline auto _lookupSize = static_cast<std::size_t>(TokenType::_tokenCount);
    static inline std::array<statementHandler_t, _lookupSize> statementLookup{};
    static inline std::array<nudHandler_t, _lookupSize> nudLookup{};
    static inline std::array<ledHandler_t, _lookupSize> ledLookup{};
    static inline std::array<Operator, _lookupSize> operatorPrecedenceMap{};

    static inline std::array<nudHandler_types_t, _lookupSize> nudLookup_types{};
    static inline std::array<ledHandler_types_t, _lookupSize> ledLookup_types{};
    static inline std::array<Operator, _lookupSize> operatorPrecedenceMap_type{};

    //~ Parsing functions

    // Expression Parsing
    ast::Expression* parseExpression(Precedence precedence);
    ast::Expression* parseAggregateInstantiationExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseAggregateLiteralExpression();
    ast::Expression* parseAlignofExpression();
    ast::Expression* parseArrayInstantiationExpression();
    ast::Expression* parseAssignmentExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseBinaryExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseFunctionCallExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseGenericExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseIndexingExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseMemberAccessExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseParenthesizedExpression();
    ast::Expression* parsePostfixExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parsePrefixExpression();
    ast::Expression* parsePrimaryExpression();
    ast::Expression* parseScopeResolutionExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseSizeofExpression();
    ast::Expression* parseTypeCastExpression(ast::Expression* left, Precedence precedence);

    // Statement Parsing
    ast::Statement* parseStatement();
    ast::Statement* parseAggregateDeclarationStatement();
    ast::Statement* parseAliasStatement();
    ast::Statement* parseBreakStatement();
    ast::Statement* parseContinueStatement();
    ast::Statement* parseDoWhileLoopStatement();
    ast::Statement* parseEnumDeclarationStatement();
    ast::Statement* parseForLoopStatement();
    ast::Statement* parseFunctionDeclarationStatement();
    ast::Statement* parseIfStatement();
    ast::Statement* parseImportStatement();
    ast::Statement* parseModuleDeclarationStatement();
    ast::Statement* parseRedundantSemicolon();
    ast::Statement* parseReturnStatement();
    ast::Statement* parseSwitchStatement();
    ast::Statement* parseVariableDeclarationStatement();
    ast::Statement* parseVisibilityAffectedStatement();
    ast::Statement* parseWhileLoopStatement();

    // Type Parsing

    ast::Type* parseType(Precedence precedence);
    ast::Type* parseArrayType(ast::Type* left, Precedence precedence);
    ast::Type* parseAggregateType();
    ast::Type* parseFunctionType();
    ast::Type* parseGenericType(ast::Type* left, Precedence precedence);
    ast::Type* parsePointerType();
    ast::Type* parseParenthesizedType();
    ast::Type* parseScopedType(ast::Type* left, Precedence precedence);
    ast::Type* parseSymbolType();
    ast::Type* parseTypeofType();

    // ~ Helpers
    ast::Block parseBlock(const std::string& blockName);

    bool isUnaryContext() const noexcept;

    [[nodiscard]] inline Token peekToken() const { return lexer->peekToken(); }
    [[nodiscard]] inline TokenType peekTokenType() { return lexer->peekToken().getType(); }

    [[nodiscard]] inline Token consumeToken() {
        previousToken = peekToken();
        return lexer->consumeToken();
    }

    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, const std::string& errorMessage);

    template <class... Args>
    inline void logError(std::size_t line, std::size_t col, std::format_string<Args...> message,
                         Args&&... args) noexcept {
        logging::logError(line, col, message, std::forward<Args>(args)...);
        flags.hasError = true;
    }

    inline bool done() noexcept { return peekTokenType() == TokenType::EndOfFile; }

    // ~ Helpers for lookups
    constexpr static std::size_t tokenToIndex(TokenType t) noexcept { return static_cast<std::size_t>(t); }

    static void registerLedHandler_binary(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    static void registerLedHandler_postfix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    static void registerLedHandler_prefix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    static void registerLedHandler_type(TokenType type, Precedence precedence, ledHandler_types_t handler) noexcept;

    static void registerNudHandler_binary(TokenType type, nudHandler_t handler) noexcept;
    static void registerNudHandler_prefix(TokenType type, nudHandler_t handler) noexcept;
    static void registerNudHandler_type(TokenType type, nudHandler_types_t handler) noexcept;

    static void registerStmtHandler(TokenType type, statementHandler_t handler) noexcept;

    static void initializeLookups() noexcept;
    static void initializeTypeLookups() noexcept;
};

}  // namespace Manganese::parser

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
