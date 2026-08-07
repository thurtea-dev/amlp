#pragma once
#include <string>
#include <vector>

namespace lpcdriver {

enum class TokenType { Ident, Number, String, Symbol, Keyword, End };

struct Token {
    TokenType type;
    std::string text;
    int line = 1;
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool atEnd() const;
    void skipWhitespaceAndComments();
    Token lexIdentOrKeyword();
    Token lexNumber();
    Token lexString();
    Token lexChar();
    Token lexSymbol();
    Token lexHeredoc();

    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;
};

} // namespace lpcdriver
