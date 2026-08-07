#include "lpcdriver/compiler/Lexer.hpp"
#include "lpcdriver/core/Errors.hpp"
#include <cctype>
#include <unordered_set>

namespace lpcdriver {

namespace {
// "array" is deliberately not reserved here even though real LPC/FluffOS
// does reserve it (lex.c: {"array", L_ARRAY, 0}): this whole mudlib never
// once uses it as an actual type (every array-typed declaration uses
// "mixed *", "string *", etc. instead), but does use it as a plain
// identifier -- secure/SimulEfun/SimulEfun.h/exclude_array.c's own
// "exclude_array(mixed *array, int from, int to)" names a parameter
// "array". Reserving a word that is never needed as a type but is needed
// as an identifier would only break real code for no benefit.
const std::unordered_set<std::string> kKeywords = {
    "void", "int", "string", "object", "float", "mapping",
    // "status": real lex.c's own "{\"status\", L_BASIC_TYPE,
    // TYPE_NUMBER}" -- a legacy basic-type keyword that is just a plain
    // synonym for "int" (same TYPE_NUMBER), needing no separate CodeGen/
    // VM handling here since this driver's own Value model is already
    // dynamically typed regardless of the declared type -- confirmed
    // live: std/user.c's own "static status snoop, earmuffs;". Safe to
    // reserve (unlike "array", see this file's own comment on that one):
    // every "status" in this mudlib outside that declaration is inside
    // a string literal (a "status" command word), never a bare
    // identifier.
    "status",
    "mixed", "function", "return", "if", "else", "while", "for",
    "static", "private", "public", "protected", "nomask", "varargs",
    "inherit", "break", "continue", "foreach", "in",
    "switch", "case", "default"
};
}

Lexer::Lexer(std::string source) : src_(std::move(source)) {}

bool Lexer::atEnd() const { return pos_ >= src_.size(); }

char Lexer::peek() const { return atEnd() ? '\0' : src_[pos_]; }

char Lexer::peekNext() const {
    return (pos_ + 1 < src_.size()) ? src_[pos_ + 1] : '\0';
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') ++line_;
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        if (atEnd()) return;
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
            continue;
        }
        if (c == '/' && peekNext() == '/') {
            while (!atEnd() && peek() != '\n') advance();
            continue;
        }
        if (c == '/' && peekNext() == '*') {
            advance(); advance();
            while (!atEnd() && !(peek() == '*' && peekNext() == '/')) {
                advance();
            }
            if (!atEnd()) { advance(); advance(); }
            continue;
        }
        return;
    }
}

Token Lexer::lexIdentOrKeyword() {
    int startLine = line_;
    std::string text;
    while (!atEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        text += advance();
    }
    TokenType type = kKeywords.count(text) ? TokenType::Keyword : TokenType::Ident;
    return Token{type, text, startLine};
}

Token Lexer::lexNumber() {
    int startLine = line_;
    std::string text;
    while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text += advance();
    }

    // Float literal ("1.5"): a '.' immediately after the integer part,
    // followed by a digit, distinct from the ".." range operator
    // ("arr[a..b]") and "..." varargs marker, which both start with a
    // second '.' rather than a digit. The '.' is folded straight into
    // this same Number token's text (no separate token type at the lexer
    // level) -- the Parser tells int and float apart downstream just by
    // checking for a '.' in the text, same as how it already relies on
    // this token's raw text for int parsing. The leading-dot form
    // (".5", no digit before the '.') is handled by tokenize() routing
    // straight here instead of through lexIdentOrKeyword/lexSymbol --
    // see its own comment -- so this loop simply does nothing on its
    // first pass for that case (text starts empty, peek() is already
    // '.') and falls through to the same handling below.
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        text += advance(); // '.'
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            text += advance();
        }
    }

    return Token{TokenType::Number, text, startLine};
}

Token Lexer::lexString() {
    int startLine = line_;
    advance();
    std::string value;
    while (!atEnd() && peek() != '"') {
        char c = advance();
        if (c == '\\' && !atEnd()) {
            char esc = advance();
            switch (esc) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default: value += esc; break;
            }
        } else {
            value += c;
        }
    }
    if (atEnd()) {
        throw LpcRuntimeError("unterminated string literal at line " + std::to_string(startLine));
    }
    advance();
    return Token{TokenType::String, value, startLine};
}

Token Lexer::lexChar() {
    int startLine = line_;
    advance(); // consume opening '

    if (atEnd()) {
        throw LpcRuntimeError("unterminated character literal at line " +
                               std::to_string(startLine));
    }

    char c = advance();
    int64_t code;
    if (c == '\\' && !atEnd()) {
        char esc = advance();
        switch (esc) {
            case 'n': code = '\n'; break;
            case 't': code = '\t'; break;
            case '\'': code = '\''; break;
            case '"': code = '"'; break;
            case '\\': code = '\\'; break;
            default: code = static_cast<unsigned char>(esc); break;
        }
    } else {
        code = static_cast<unsigned char>(c);
    }

    if (atEnd() || peek() != '\'') {
        throw LpcRuntimeError(
            "character literal must contain exactly one character, at line " +
            std::to_string(startLine));
    }
    advance(); // consume closing '

    return Token{TokenType::Number, std::to_string(code), startLine};
}

// Real LPC's "@TERM ... TERM" heredoc string literal (confirmed against
// the FluffOS reference driver's lex.c: "case '@'": get_terminator() then
// get_text_block()). Hit live in secure/SimulEfun/misc.c:
//   ret = @END
//   Fd    State      Mode       Local Address          Remote Address
//   --  ---------  --------  ---------------------  ---------------------
//   END;
// The terminator is read up to the end of the "@TERM" line; the text
// block runs verbatim (no escape processing) until a line that *starts*
// with the terminator, at which point the terminator is consumed and
// whatever follows on that same line (here, ";") resumes as normal LPC
// source. The "@@TERM ... TERM" array-block variant (each line becomes
// a separate array element) is not implemented -- not hit anywhere in
// this mudlib's boot-path files -- and throws rather than silently
// misparsing.
Token Lexer::lexHeredoc() {
    int startLine = line_;
    advance(); // consume '@'

    if (peek() == '@') {
        throw NotImplementedError("\"@@\" array-block heredoc syntax");
    }

    std::string terminator;
    while (!atEnd() && peek() != '\n') {
        terminator += advance();
    }
    if (terminator.empty()) {
        throw LpcRuntimeError("heredoc: missing terminator at line " + std::to_string(startLine));
    }
    if (atEnd()) {
        throw LpcRuntimeError("unterminated heredoc: missing closing \"" + terminator +
                               "\" at line " + std::to_string(startLine));
    }
    advance(); // consume the newline ending the "@TERM" line

    std::string value;
    bool atLineStart = true;
    for (;;) {
        if (atLineStart) {
            bool matches = pos_ + terminator.size() <= src_.size() &&
                           src_.compare(pos_, terminator.size(), terminator) == 0;
            if (matches) {
                for (size_t i = 0; i < terminator.size(); ++i) advance();
                break;
            }
        }
        if (atEnd()) {
            throw LpcRuntimeError("unterminated heredoc: missing closing \"" + terminator +
                                   "\" at line " + std::to_string(startLine));
        }
        char c = advance();
        value += c;
        atLineStart = (c == '\n');
    }

    return Token{TokenType::String, value, startLine};
}

Token Lexer::lexSymbol() {
    int startLine = line_;
    char c = advance();

    if (c == '-' && peek() == '>') {
        advance();
        return Token{TokenType::Symbol, "->", startLine};
    }
    if (c == ':' && peek() == ':') {
        advance();
        // "efun::name(...)", real LPC's explicit escape hatch to the
        // core efun table (grammar.y: "L_EFUN L_COLON_COLON identifier"
        // -- confirmed live: secure/SimulEfun/misc.c's own
        // "efun::destruct(ob)"). A single ':' is otherwise only ever a
        // ternary's own, so this cannot misfire there.
        return Token{TokenType::Symbol, "::", startLine};
    }
    if (c == '+' && peek() == '+') {
        advance();
        return Token{TokenType::Symbol, "++", startLine};
    }
    if (c == '-' && peek() == '-') {
        advance();
        return Token{TokenType::Symbol, "--", startLine};
    }
    if ((c == '+' || c == '-' || c == '*' || c == '/' || c == '%') && peek() == '=') {
        advance();
        return Token{TokenType::Symbol, std::string(1, c) + "=", startLine};
    }
    if (c == '=' && peek() == '=') {
        advance();
        return Token{TokenType::Symbol, "==", startLine};
    }
    if (c == '!' && peek() == '=') {
        advance();
        return Token{TokenType::Symbol, "!=", startLine};
    }
    if (c == '<' && peek() == '=') {
        advance();
        return Token{TokenType::Symbol, "<=", startLine};
    }
    if (c == '>' && peek() == '=') {
        advance();
        return Token{TokenType::Symbol, ">=", startLine};
    }
    if (c == '|' && peek() == '|') {
        advance();
        return Token{TokenType::Symbol, "||", startLine};
    }
    if (c == '&' && peek() == '&') {
        advance();
        return Token{TokenType::Symbol, "&&", startLine};
    }
    if (c == '.' && peek() == '.') {
        advance();
        if (peek() == '.') {
            advance();
            // Trailing varargs marker after a function's parameter list,
            // e.g. "int true(mixed args...)" (confirmed against
            // grammar.y: "argument_list L_DOT_DOT_DOT"), distinct from
            // the two-dot range operator ("arr[a..b]").
            return Token{TokenType::Symbol, "...", startLine};
        }
        return Token{TokenType::Symbol, "..", startLine};
    }

    return Token{TokenType::Symbol, std::string(1, c), startLine};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    for (;;) {
        skipWhitespaceAndComments();
        if (atEnd()) break;

        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(lexIdentOrKeyword());
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(lexNumber());
        } else if (c == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
            // Leading-dot float literal (".5", real LPC/C for "0.5"),
            // confirmed used live across the mudlib. A bare '.' otherwise
            // never starts a token (LPC has no member-access operator --
            // "->" fills that role -- so outside of a float literal '.'
            // only ever appears as ".." range or "..." varargs, both
            // handled below by lexSymbol(), neither of which is a digit).
            tokens.push_back(lexNumber());
        } else if (c == '"') {
            tokens.push_back(lexString());
        } else if (c == '\'') {
            tokens.push_back(lexChar());
        } else if (c == '@') {
            tokens.push_back(lexHeredoc());
        } else if (c == '(' || c == ')' || c == '{' || c == '}' ||
                   c == '[' || c == ']' || c == ':' ||
                   c == ';' || c == ',' || c == '-' || c == '=' ||
                   c == '!' || c == '<' || c == '>' || c == '*' || c == '+' ||
                   c == '|' || c == '&' || c == '/' || c == '%' || c == '.' ||
                   c == '?' || c == '^') {
            tokens.push_back(lexSymbol());
        } else {
            int errLine = line_;
            char unrecognized = advance();
            throw LpcRuntimeError(
                "lexer: unrecognized character '" + std::string(1, unrecognized) +
                "' at line " + std::to_string(errLine));
        }
    }

    tokens.push_back(Token{TokenType::End, "", line_});
    return tokens;
}

} // namespace lpcdriver
