#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP

#include <concepts>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser/operators.hpp>
#include <io/logging.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <utils/chunk_allocator.hpp>
#include <vector>

namespace Manganese::parser {
using lexer::TokenType, lexer::Token;

std::string flattenPath(const std::vector<std::string>& path);

struct ParsedFile {
    ast::ModuleDeclarationStatement* fileModule;
    utils::Slice<ast::ImportStatement*> imports;
    ast::Block program;
};

class Parser {
   private:
    lexer::Lexer lexer;
    utils::chunk_allocator& arena;
    utils::StringInterner& interner;
    std::optional<Token> previousToken;

    constexpr static inline ast::Visibility defaultVisibility = ast::Visibility::Private;

    struct {
        bool hasParsedFileHeader : 1 = false;  // Processing module and import
        bool hasError : 1 = false;
        bool hasWarning : 1 = false;
        bool hasModuleDeclaration : 1 = false;
        bool hasImports : 1 = false;
    } flags;

   public:
    Parser(const std::string& source, lexer::Mode mode, utils::chunk_allocator& allocatorReference,
           utils::StringInterner& internerReference);

    ~Parser() noexcept = default;

    ParsedFile parse();

   private:
    friend consteval void initializeLookups(struct LookupTable&) noexcept;
    friend consteval void initializeTypeLookups(struct LookupTable&) noexcept;

    // Expression Parsing
    ast::Expression* parseExpression(Precedence precedence);
    ast::Expression* parseAggregateInstantiationExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseAggregateLiteralExpression();
    ast::Expression* parseAlignofExpression();
    ast::Expression* parseArrayInstantiationExpression();
    ast::Expression* parseAssignmentExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseBinaryExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseChainedComparison(ast::Expression* left, const Token& op, Precedence precedence);
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
    ast::Expression* parseTernaryExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseTypeCastExpression(ast::Expression* left, Precedence precedence);

    // Statement Parsing
    ast::Statement* parseStatement();
    ast::Statement* parseAggregateDeclarationStatement();
    ast::Statement* parseAliasStatement();
    ast::Statement* parseBreakStatement();
    ast::Statement* parseContinueStatement();
    ast::Statement* parseDoWhileLoopStatement();
    ast::Statement* parseEnumDeclarationStatement();
    ast::Statement* parseForEachLoopStatement();
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
    ast::FunctionParameterType parseFunctionTypeParameter();
    utils::StringID parseGenericTypeParameter(std::vector<utils::StringID>& existingGenerics,
                                              utils::StringID contextName);

        // Parsing Helpers
    ast::EnumValue parseEnumMember();
    std::vector<utils::StringID> parseGenericsList(utils::StringID contextName);
    std::optional<ast::AggregateField> parseAggregateField(utils::StringID aggregateName,
                                                           std::span<ast::AggregateField> existingFields);
    std::optional<ast::FunctionParameter> parseFunctionParameter(utils::StringID functionName,
                                                                 std::span<ast::FunctionParameter> existingParams,
                                                                 bool& hasDefaultParameter);
    ast::CaseClause parseCaseClause();
    std::optional<ast::Block> parseDefaultClause();
    std::vector<utils::StringID> parseImportPath();
    utils::OptionalStringID parseImportAlias();

    // Other helpers
    ast::Block parseBlock(std::string_view blockName);

    template <class T, class ParseFunction>
    std::vector<T> parseCommaSeparatedList(TokenType closeToken, const char* missingCommaMessage,
                                           ParseFunction&& parseFunction) {
        std::vector<T> items;
        while (!done() && peekTokenType() != closeToken) {
            items.push_back(parseFunction());
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
            // Any poisoned node should be treated as having an error
            flags.hasError = true;
        }
        T* const node = arena.emplace<T>(std::forward<Args>(args)...);
        node->line = startToken.getLine();
        node->column = startToken.getColumn();
        return node;
    }

    [[nodiscard]] bool isUnaryContext() const noexcept;
    
    [[nodiscard]] constexpr static bool isComparisonOperator(TokenType op) noexcept {
        using enum TokenType;
        return utils::enum_matches(op, GreaterThan, GreaterThanOrEqual, LessThan, LessThanOrEqual, Equal, NotEqual);
    }
    
    [[nodiscard]] inline bool done() noexcept { return peekTokenType() == TokenType::EndOfFile; }

    [[nodiscard]] inline Token& peekToken() { return lexer.peekToken(); }
    [[nodiscard]] inline TokenType peekTokenType() { return lexer.peekToken().getType(); }

    [[nodiscard]] inline Token consumeToken() {
        previousToken = peekToken();
        return lexer.consumeToken();
    }

    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, std::string_view errorMessage);

    // Logging
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
};

}  // namespace Manganese::parser

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP