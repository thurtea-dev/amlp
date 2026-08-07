#pragma once
#include <memory>
#include <vector>
#include "lpcdriver/compiler/Lexer.hpp"
#include "lpcdriver/compiler/Ast.hpp"

namespace lpcdriver {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parseProgram();

private:
    const Token& peek() const;
    const Token& peekAt(size_t offset) const;
    const Token& advance();
    bool check(TokenType type) const;
    bool checkText(const std::string& text) const;
    const Token& expect(TokenType type, const std::string& context);
    const Token& expectText(const std::string& text, const std::string& context);
    bool atEnd() const;
    bool isTypeKeyword(const Token& tok) const;
    bool isModifierKeyword(const Token& tok) const;

    // Shared prefix of every top-level declaration (modifiers, type, an
    // optional array '*', and a name), factored out so parseProgram()
    // can consume it exactly once and then branch on whether '(' follows
    // (a function) or not (an object variable declaration), rather than
    // duplicating the modifier/type/star/name consumption to peek ahead.
    struct DeclPrefix {
        std::string type;
        bool isArray;
        std::string name;
    };
    DeclPrefix parseDeclPrefix(const std::string& context);
    std::unique_ptr<FunctionDecl> parseFunctionRest(DeclPrefix prefix);
    std::vector<std::unique_ptr<ObjectVarDecl>> parseObjectVarDeclRest(DeclPrefix prefix);
    std::vector<Param> parseParamList();
    std::unique_ptr<Block> parseBlock();
    std::unique_ptr<Block> parseBranch();
    AstPtr parseStatement();
    AstPtr parseIfStatement();
    AstPtr parseWhileStatement();
    AstPtr parseForStatement();
    AstPtr parseReturnStatement();
    AstPtr parseBreakStatement();
    AstPtr parseContinueStatement();
    AstPtr parseForeachStatement();
    AstPtr parseSwitchStatement();
    struct ForeachVarSpec { std::string name; bool isNewDecl; };
    ForeachVarSpec parseForeachVar();
    std::unique_ptr<VarDeclStmt> parseSingleVarDecl(const Token& typeTok);
    AstPtr parseVarDeclStatement();
    AstPtr parseAssignStatement();
    void parseInheritStatement(Program& program);
    std::string parseInheritPathString();

    AstPtr parseExpr();
    AstPtr parseCommaExprChain();
    AstPtr parseTernary();
    AstPtr parseLogicalOr();
    AstPtr parseLogicalAnd();
    AstPtr parseBitOr();
    AstPtr parseBitXor();
    AstPtr parseBitAnd();
    AstPtr parseEquality();
    AstPtr parseComparison();
    AstPtr parseAdditive();
    AstPtr parseMultiplicative();
    AstPtr parseUnary();
    AstPtr parsePostfix();
    AstPtr parsePrimary();
    std::vector<AstPtr> parseArgList();

    std::vector<Token> tokens_;
    size_t pos_ = 0;
};

} // namespace lpcdriver
