#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP

#include <concepts>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser/operators.hpp>
#include <io/logging.hpp>
#include <memory>
#include <mnstl/chunk_allocator.hxx>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Manganese::parser {
using lexer::TokenType, lexer::Token;

struct ParsedFile {
    ast::ModuleDeclarationStatement* fileModule;
    std::vector<ast::ImportStatement*> imports;
    ast::Block program;
};

class Parser {
   private:
    std::unique_ptr<lexer::Lexer> lexer;
    constexpr static inline ast::Visibility defaultVisibility = ast::Visibility::Private;
    std::optional<Token> previousToken;
    mnstl::chunk_allocator& arena;

    // Some flags
    struct {
        bool hasParsedFileHeader : 1 = false;  // Processing module and import
        bool hasError : 1 = false;
        bool hasWarning : 1 = false;
        bool parsingAliasStatement : 1 = false;
        bool hasModuleDeclaration : 1 = false;
    } flags;

   public:
    Parser(const std::string& source, lexer::Mode mode, mnstl::chunk_allocator& allocatorReference);

    ~Parser() noexcept = default;

    ParsedFile parse();

   private:  // private methods
    friend consteval void initializeLookups(struct LookupTable&) noexcept;
    friend consteval void initializeTypeLookups(struct LookupTable&) noexcept;

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
    ast::Expression* parseGenericInstantiationExpression(ast::Expression* left, Precedence precedence);
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
    ast::Statement* parseNamespace();
    ast::Statement* parseRedundantSemicolon();
    ast::Statement* parseReturnStatement();
    ast::Statement* parseSwitchStatement();
    ast::Statement* parseVariableDeclarationStatement();
    ast::Statement* parseVisibilityAffectedStatement();
    ast::Statement* parseWhileLoopStatement();

    ast::EnumValue parseEnumMember();
    std::vector<std::string> parseGenericsList(std::string_view context_name);
    std::optional<ast::AggregateField> parseAggregateField(std::string_view aggregateName,
                                                           std::span<ast::AggregateField> existingFields);

    std::optional<ast::FunctionParameter> parseFunctionParameter(std::string_view functionName,
                                                                 std::span<ast::FunctionParameter> existingParams,
                                                                 bool& hasDefaultParameter, bool& hasVariadicParameter);

    ast::CaseClause parseCaseClause();
    ast::Block parseDefaultClause();

    std::vector<std::string> parseImportPath();
    std::optional<std::string> parseImportAlias();

    // Type Parsing

    ast::Type* parseType(Precedence precedence);
    ast::Type* parseArrayType(ast::Type* left, Precedence precedence);
    ast::Type* parseAggregateType();
    ast::Type* parseFunctionType();
    ast::Type* parseGenericInstantiationType(ast::Type* left, Precedence precedence);
    ast::Type* parseIdentifierType();
    ast::Type* parsePointerType();
    ast::Type* parseParenthesizedType();
    ast::Type* parseScopedType(ast::Type* left, Precedence precedence);
    ast::Type* parseTypeofType();

    ast::Type* parseAggregateTypeField();
    ast::FunctionParameterType parseFunctionTypeParameter(bool& seenVariadic);
    std::string parseGenericTypeParameter(std::vector<std::string>& existingGenerics, std::string_view contextName);

    // ~ Helpers
    ast::Block parseBlock(std::string_view blockName);

    template <class T, class ParseFunction>
    std::vector<T> parseCommaSeparatedList(TokenType closeToken, const char* missingCommaMessage,
                                           ParseFunction&& parseFunction) {
        std::vector<T> items;
        while (!done() && peekTokenType() != closeToken) {
            auto item = parseFunction();
            if constexpr (std::is_same_v<T, std::string>) {
                if (!item.empty()) { items.push_back(std::move(item)); }
            } else {
                items.push_back(std::move(item));
            }
            if (peekTokenType() != closeToken) { expectToken(TokenType::Comma, missingCommaMessage); }
        }
        expectToken(closeToken);
        return items;
    }

    template <class T, class... Args>
        requires(std::is_convertible_v<T*, ast::ASTNode*> && std::is_constructible_v<T, Args...>)
    T* makeNode(const Token& startToken, Args&&... args) {
        if constexpr (std::is_same_v<T, ast::PoisonedExpression> || std::is_same_v<T, ast::PoisonedStatement>
                      || std::is_same_v<T, ast::PoisonedType>) {
            // any poisoned node should be treated as having an error
            flags.hasError = true;
        }
        T* const node = arena.emplace<T>(std::forward<Args>(args)...);
        node->line = startToken.getLine();
        node->column = startToken.getColumn();
        return node;
    }

    bool isUnaryContext() const noexcept;

    [[nodiscard]] inline Token& peekToken() const { return lexer->peekToken(); }
    [[nodiscard]] inline TokenType peekTokenType() { return lexer->peekToken().getType(); }

    [[nodiscard]] inline Token consumeToken() {
        previousToken = peekToken();
        return lexer->consumeToken();
    }

    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, std::string_view errorMessage);

    template <class... Args>
    inline void logError(const Token& token, std::format_string<Args...> message, Args&&... args) {
        logging::logError(token.getLine(), token.getColumn(), message, std::forward<Args>(args)...);
        flags.hasError = true;
    }

    template <class T, class... Args>
        requires(requires(const T& t) {
            { t.line } -> std::convertible_to<std::size_t>;
            { t.column } -> std::convertible_to<std::size_t>;
        })
    inline void logError(const T& t, std::format_string<Args...> message, Args&&... args) {
        logging::logError(t.line, t.column, message, std::forward<Args>(args)...);
        flags.hasError = true;
    }

    template <class... Args>
    inline void logWarning(const Token& token, std::format_string<Args...> message, Args&&... args) {
        logging::logWarning(token.getLine(), token.getColumn(), message, std::forward<Args>(args)...);
        flags.hasWarning = true;
    }

    inline bool done() noexcept { return peekTokenType() == TokenType::EndOfFile; }
};

}  // namespace Manganese::parser

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
