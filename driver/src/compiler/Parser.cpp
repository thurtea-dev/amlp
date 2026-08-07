#include "lpcdriver/compiler/Parser.hpp"
#include "lpcdriver/core/Errors.hpp"

namespace lpcdriver {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

bool Parser::atEnd() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::End;
}

const Token& Parser::peek() const {
    return tokens_[pos_ < tokens_.size() ? pos_ : tokens_.size() - 1];
}

const Token& Parser::peekAt(size_t offset) const {
    size_t idx = pos_ + offset;
    return tokens_[idx < tokens_.size() ? idx : tokens_.size() - 1];
}

const Token& Parser::advance() {
    const Token& t = peek();
    if (!atEnd()) ++pos_;
    return t;
}

bool Parser::check(TokenType type) const {
    return !atEnd() && peek().type == type;
}

bool Parser::checkText(const std::string& text) const {
    if (atEnd()) return false;
    const Token& t = peek();
    // Every checkText() call site in this file passes a symbol or keyword
    // literal ("(", "if", "++", ...), never an arbitrary identifier/
    // string/number value -- there is no call site anywhere that wants to
    // match a literal's *content*. Without this type guard, a string
    // literal whose contents happen to equal an operator's text (e.g.
    // "!", "-", "for") would spuriously match here too, since Token
    // equality was previously text-only. Restricting to Symbol/Keyword
    // tokens closes that off without changing behavior for any real call
    // site.
    if (t.type != TokenType::Symbol && t.type != TokenType::Keyword) return false;
    return t.text == text;
}

const Token& Parser::expect(TokenType type, const std::string& context) {
    if (!check(type)) {
        throw LpcRuntimeError("parse error: expected token type in " + context +
                               " at line " + std::to_string(peek().line) +
                               " (got \"" + peek().text + "\")");
    }
    return advance();
}

const Token& Parser::expectText(const std::string& text, const std::string& context) {
    if (!checkText(text)) {
        throw LpcRuntimeError("parse error: expected \"" + text + "\" in " + context +
                               " at line " + std::to_string(peek().line) +
                               " (got \"" + peek().text + "\")");
    }
    return advance();
}

bool Parser::isTypeKeyword(const Token& tok) const {
    if (tok.type != TokenType::Keyword) return false;
    static const std::vector<std::string> nonTypeKeywords = {
        "return", "if", "else", "while", "for", "inherit", "break", "continue",
        "foreach", "in", "switch", "case", "default",
        "static", "private", "public", "protected", "nomask", "varargs"
    };
    for (const auto& kw : nonTypeKeywords) {
        if (tok.text == kw) return false;
    }
    return true;
}

bool Parser::isModifierKeyword(const Token& tok) const {
    if (tok.type != TokenType::Keyword) return false;
    static const std::vector<std::string> modifierKeywords = {
        "static", "private", "public", "protected", "nomask", "varargs"
    };
    for (const auto& kw : modifierKeywords) {
        if (tok.text == kw) return true;
    }
    return false;
}

std::vector<AstPtr> Parser::parseArgList() {
    std::vector<AstPtr> args;
    if (checkText(")")) return args;
    for (;;) {
        args.push_back(parseExpr());
        if (checkText(",")) { advance(); continue; }
        break;
    }
    return args;
}

AstPtr Parser::parsePrimary() {
    if (check(TokenType::String)) {
        // Adjacent string literals concatenate with no operator, same as
        // C (grammar.y's "string_con2: L_STRING | string_con2 L_STRING",
        // used directly as an expr0 primary). Hit live in
        // secure/daemon/master.c: a message split across two lines purely
        // for source readability --
        //   shout("...Saving all players; "
        //         "please reconnect shortly.\n");
        std::string value = advance().text;
        while (check(TokenType::String)) {
            value += advance().text;
        }
        auto lit = std::make_unique<StringLiteral>();
        lit->value = std::move(value);
        return lit;
    }

    if (check(TokenType::Number)) {
        // The Lexer folds a float's '.' straight into this same Number
        // token's text (see Lexer::lexNumber()'s own comment) rather than
        // using a distinct token type, so int vs float is told apart
        // here, the one place that text is actually consumed.
        const std::string& text = peek().text;
        if (text.find('.') != std::string::npos) {
            auto lit = std::make_unique<FloatLiteral>();
            lit->value = std::stod(advance().text);
            return lit;
        }
        auto lit = std::make_unique<IntLiteral>();
        lit->value = std::stoll(advance().text);
        return lit;
    }

    if (checkText("(") && peekAt(1).text == "[") {
        advance(); // (
        advance(); // [
        auto lit = std::make_unique<MappingLiteralExpr>();
        if (!checkText("]")) {
            for (;;) {
                AstPtr key = parseExpr();
                expectText(":", "mapping literal entry");
                AstPtr value = parseExpr();
                lit->entries.emplace_back(std::move(key), std::move(value));
                if (checkText(",")) {
                    advance();
                    // Trailing comma before the closing "]" -- same real
                    // LPC allowance as the array literal case just above.
                    if (checkText("]")) break;
                    continue;
                }
                break;
            }
        }
        expectText("]", "mapping literal");
        expectText(")", "mapping literal");
        return lit;
    }

    // "(: name, bound_args... :)" -- closure/function-pointer literal
    // (see Ast.hpp's ClosureLiteralExpr and Value.hpp's Closure for the
    // real-source citation and scope). Recognized the same way "({"/"(["
    // just above/below already are: by the literal two-character open
    // sequence, since the Lexer tokenizes "(" and ":" separately (no
    // dedicated "(:" token, unlike "::"/"->" -- see Lexer::lexSymbol()).
    //
    // Real LPC's grammar.y has two separate productions here, not one:
    // this bare-identifier form (efun/local/simul_efun/global-var
    // reference plus optional bound args, resolved by name at call
    // time), and a general fallback, "L_FUNCTION_OPEN comma_expr ':'
    // ')'" -- any comma-separated expression list at all, compiled to
    // its own anonymous function (see Ast.hpp's InlineLambdaExpr for the
    // full citation and the two real call sites in this mudlib that need
    // it). They are told apart the same way real LPC's LALR parser
    // resolves the ambiguity: only a bare identifier immediately
    // followed by "," or ":" can be this first production (a plain name
    // and nothing else); anything else -- a call expression, a string
    // literal, a parenthesized expression, etc -- falls through to the
    // general comma_expr form below.
    if (checkText("(") && peekAt(1).text == ":") {
        bool isBareName = peekAt(2).type == TokenType::Ident &&
            peekAt(3).type == TokenType::Symbol &&
            (peekAt(3).text == "," || peekAt(3).text == ":");

        if (isBareName) {
            advance(); // (
            advance(); // :
            Token nameTok = expect(TokenType::Ident, "closure literal function name");
            auto closure = std::make_unique<ClosureLiteralExpr>();
            closure->functionName = nameTok.text;
            if (checkText(",")) {
                advance();
                for (;;) {
                    closure->boundArgs.push_back(parseExpr());
                    if (checkText(",")) {
                        advance();
                        continue;
                    }
                    break;
                }
            }
            expectText(":", "closure literal");
            expectText(")", "closure literal");
            return closure;
        }

        advance(); // (
        advance(); // :
        auto lambda = std::make_unique<InlineLambdaExpr>();
        for (;;) {
            lambda->bodyExprs.push_back(parseExpr());
            if (checkText(",")) {
                advance();
                continue;
            }
            break;
        }
        expectText(":", "closure literal");
        expectText(")", "closure literal");
        return lambda;
    }

    // "(*fp)(args...)" -- call-through-a-function-pointer-value syntax
    // (grammar.y: "'(' '*' comma_expr ')' '(' expr_list ')'", confirmed
    // by direct reading, not guessed). Real LPC desugars this straight
    // to a call to the core "evaluate" efun (predefs[evaluate_efun]),
    // unconditionally -- not the usual tiered local/inherited/simul_efun
    // lookup a plain "name(...)" call goes through -- with the pointer
    // expression as the first argument ahead of the real call args. This
    // driver's own "evaluate"/"funcall" efuns already do exactly that
    // (EfunTable.cpp's evaluateImpl(), which calls VM::callClosure()),
    // so this is a pure syntax-level desugaring into a forced-efun
    // CallExpr, matching how "efun::name(...)" already reaches the core
    // efun table directly -- no new opcode or VM change needed. Only the
    // single-expression form of the pointer clause is handled (real
    // grammar allows a full comma_expr there too), since every real call
    // site in this mudlib (std/user/editor.c's own "(*__Callback)(...)"/
    // "(*__Abort)()") uses a single bare local variable.
    if (checkText("(") && peekAt(1).text == "*") {
        advance(); // (
        advance(); // *
        AstPtr fpExpr = parseExpr();
        expectText(")", "function pointer call");
        expectText("(", "function pointer call");
        auto call = std::make_unique<CallExpr>();
        call->callee = "evaluate";
        call->forceEfun = true;
        call->args.push_back(std::move(fpExpr));
        for (auto& arg : parseArgList()) {
            call->args.push_back(std::move(arg));
        }
        expectText(")", "function pointer call");
        return call;
    }

    if (checkText("(") && peekAt(1).text == "{") {
        advance(); // (
        advance(); // {
        auto lit = std::make_unique<ArrayLiteralExpr>();
        if (!checkText("}")) {
            for (;;) {
                lit->elements.push_back(parseExpr());
                if (checkText(",")) {
                    advance();
                    // Trailing comma before the closing "}" (real LPC
                    // allows one, and this mudlib's own code uses it --
                    // secure/SimulEfun/misc.c's own "({ ... "hamster",
                    // })"): without this check the loop would try to
                    // parse another element starting at "}" itself and
                    // fail with "expected expression".
                    if (checkText("}")) break;
                    continue;
                }
                break;
            }
        }
        expectText("}", "array literal");
        expectText(")", "array literal");
        return lit;
    }

    if (checkText("(")) {
        advance();
        auto expr = parseExpr();
        expectText(")", "parenthesized expression");
        return expr;
    }

    // Bare "::name(...)" -- explicit call to an inherited definition of
    // `name`, no identifier before the "::" (see Ast.hpp's CallExpr::
    // parentCall comment; grammar.y's "function_name: L_COLON_COLON
    // identifier"). Checked before the check(TokenType::Ident) branch
    // below since there is no leading identifier token here at all --
    // "::" is Lexer::lexSymbol()'s own single "::" token, so this cannot
    // be confused with a ternary's plain ":".
    if (checkText("::")) {
        advance(); // "::"
        Token fnNameTok = expect(TokenType::Ident, "::name function name");
        expectText("(", "::name call arguments");
        auto parentArgs = parseArgList();
        expectText(")", "::name call arguments");

        auto call = std::make_unique<CallExpr>();
        call->callee = fnNameTok.text;
        call->args = std::move(parentArgs);
        call->parentCall = true;
        return call;
    }

    if (check(TokenType::Ident)) {
        std::string name = advance().text;

        // "efun::name(...)" -- real LPC's explicit escape hatch straight
        // to the core efun table (see CallExpr::forceEfun's own
        // comment). "efun" is not reserved as a keyword (nothing in this
        // mudlib uses it as a plain identifier either way, so there is
        // no ambiguity to worry about), so this is recognized here by
        // its literal text plus the following "::", the same way "->"'s
        // call_other is recognized by its own literal syntax rather than
        // a reserved word.
        if (name == "efun" && checkText("::")) {
            advance(); // "::"
            Token fnNameTok = expect(TokenType::Ident, "efun:: function name");
            expectText("(", "efun:: call arguments");
            auto forcedArgs = parseArgList();
            expectText(")", "efun:: call arguments");

            auto call = std::make_unique<CallExpr>();
            call->callee = fnNameTok.text;
            call->args = std::move(forcedArgs);
            call->forceEfun = true;
            return call;
        }

        // "qualifier::name(...)" -- e.g. "daemon::create()" -- the named
        // form of the same explicit inherited-call syntax as bare
        // "::name(...)" above (see Ast.hpp's CallExpr::parentCall
        // comment; grammar.y's "identifier L_COLON_COLON identifier").
        // "efun" is excluded here since that specific identifier was
        // already handled by its own dedicated branch just above.
        if (name != "efun" && checkText("::")) {
            advance(); // "::"
            Token fnNameTok = expect(TokenType::Ident, "qualifier:: function name");
            expectText("(", "qualifier:: call arguments");
            auto qualifiedArgs = parseArgList();
            expectText(")", "qualifier:: call arguments");

            auto call = std::make_unique<CallExpr>();
            call->callee = fnNameTok.text;
            call->args = std::move(qualifiedArgs);
            call->parentCall = true;
            call->parentQualifier = name;
            return call;
        }

        // "catch(expr)" -- real LPC's own control-flow construct, not a
        // function call (grammar.y: "catch: L_CATCH expr_or_block", a
        // dedicated grammar production -- see Ast.hpp's CatchExpr
        // comment). Recognized the same way "efun::" just above is: by
        // its literal identifier text plus the syntax that follows, not
        // a reserved keyword (matching this parser's existing pattern
        // for sscanf/call_other, both handled the same way a few lines
        // down). Only the parenthesized-expression form is implemented,
        // not real LPC's "catch { block }" alternative -- nothing in
        // this mudlib uses that form.
        if (name == "catch" && checkText("(")) {
            advance(); // "("
            auto guarded = parseExpr();
            expectText(")", "catch expression");

            auto catchExpr = std::make_unique<CatchExpr>();
            catchExpr->guarded = std::move(guarded);
            return catchExpr;
        }

        if (!checkText("(")) {
            auto ref = std::make_unique<VarRefExpr>();
            ref->name = name;
            return ref;
        }

        advance();
        auto args = parseArgList();
        expectText(")", "call arguments");

        if (name == "sscanf") {
            if (args.size() < 2) {
                throw LpcRuntimeError("sscanf requires at least (string, format) arguments");
            }
            auto sscanfExpr = std::make_unique<SscanfExpr>();
            sscanfExpr->target = std::move(args[0]);
            sscanfExpr->format = std::move(args[1]);
            for (size_t i = 2; i < args.size(); ++i) {
                auto* ref = dynamic_cast<VarRefExpr*>(args[i].get());
                if (!ref) {
                    throw LpcRuntimeError(
                        "sscanf: output argument " + std::to_string(i - 1) +
                        " must be a plain variable name (sscanf's output arguments are "
                        "implicit lvalues in real LPC, there is no \"&var\" reference syntax)");
                }
                sscanfExpr->varNames.push_back(ref->name);
            }
            return sscanfExpr;
        }

        if (name == "call_other") {
            if (args.size() < 2) {
                throw LpcRuntimeError("call_other requires at least (target, function) arguments");
            }
            auto callOther = std::make_unique<CallOtherExpr>();
            callOther->target = std::move(args[0]);
            callOther->function = std::move(args[1]);
            for (size_t i = 2; i < args.size(); ++i) {
                callOther->args.push_back(std::move(args[i]));
            }
            return callOther;
        }

        auto call = std::make_unique<CallExpr>();
        call->callee = name;
        call->args = std::move(args);
        return call;
    }

    throw LpcRuntimeError("parse error: expected expression at line " + std::to_string(peek().line) +
                           " (got \"" + peek().text + "\")");
}

AstPtr Parser::parsePostfix() {
    AstPtr expr = parsePrimary();

    // "->" and "[" both bind at this same postfix level and can chain in
    // either order and repeatedly -- e.g. secure/SimulEfun/misc.c's own
    // "inv[i]->query_property(...)" (index then call_other) or
    // "a->b()->c()" (call_other then call_other again). A single-shot
    // "if" for one followed by a separate "while" for the other (the
    // previous shape here) only handles each starting a chain, not
    // "[" appearing first and being followed by "->": once the index
    // while-loop finished, nothing looped back to check for "->" again,
    // so a leftover "->" was returned unconsumed to the caller. One loop
    // that keeps matching either token, in whatever order they actually
    // appear, handles every ordering and repeat count uniformly.
    for (;;) {
        if (checkText("->")) {
            advance();
            Token nameTok = expect(TokenType::Ident, "call_other operator function name");
            expectText("(", "call_other operator arguments");
            auto args = parseArgList();
            expectText(")", "call_other operator arguments");

            // "->" always names its function literally in the syntax
            // (there is no "target->(expr)(...)" dynamic-dispatch form),
            // so wrap it in a StringLiteral to match CallOtherExpr::
            // function's general expression shape.
            auto funcNameLit = std::make_unique<StringLiteral>();
            funcNameLit->value = nameTok.text;

            auto callOther = std::make_unique<CallOtherExpr>();
            callOther->target = std::move(expr);
            callOther->function = std::move(funcNameLit);
            callOther->args = std::move(args);
            expr = std::move(callOther);
            continue;
        }

        if (checkText("[")) {
            advance();

            // "<N" means "N from the end" (real LPC: grammar.y's
            // "expr4 '[' '<' comma_expr ...", see Ast.hpp's IndexExpr
            // comment) -- a bare "<" can never itself start a valid
            // expression, so seeing it as the very first token here is
            // unambiguous, no lookahead needed beyond this one check.
            bool startFromEnd = false;
            if (checkText("<")) {
                advance();
                startFromEnd = true;
            }
            AstPtr startExpr = parseExpr();
            AstPtr endExpr = nullptr;
            bool endFromEnd = false;
            if (checkText("..")) {
                advance();
                if (checkText("]")) {
                    // Open-ended range ("arr[start..]", real LPC for
                    // "start to the end" -- grammar.y: "expr4 '['
                    // comma_expr L_RANGE ']'"). Synthesizing a large
                    // sentinel end value works for free with
                    // RangeIndex's existing "clampedEnd = min(end, len -
                    // 1)" clamp at the VM level, no opcode change
                    // needed.
                    auto sentinel = std::make_unique<IntLiteral>();
                    sentinel->value = 2000000000; // clamped down by RangeIndex, see above
                    endExpr = std::move(sentinel);
                } else {
                    if (checkText("<")) {
                        advance();
                        endFromEnd = true;
                    }
                    endExpr = parseExpr();
                }
            }
            expectText("]", "index expression");

            auto idx = std::make_unique<IndexExpr>();
            idx->target = std::move(expr);
            idx->index = std::move(startExpr);
            idx->rangeEnd = std::move(endExpr);
            idx->indexFromEnd = startFromEnd;
            idx->rangeEndFromEnd = endFromEnd;
            expr = std::move(idx);
            continue;
        }

        break;
    }

    if (checkText("++") || checkText("--")) {
        auto* ref = dynamic_cast<VarRefExpr*>(expr.get());
        bool isInc = checkText("++");
        if (ref) {
            std::string name = ref->name;
            advance();
            auto incDec = std::make_unique<IncDecExpr>();
            incDec->op = isInc ? IncDecOp::Inc : IncDecOp::Dec;
            incDec->prefix = false;
            incDec->name = name;
            return incDec;
        }

        // Indexed target, e.g. std/living.c's own "healing[\"intox\"]--"
        // -- see Ast.hpp's IncDecExpr comment.
        auto* idx = dynamic_cast<IndexExpr*>(expr.get());
        if (idx && !idx->rangeEnd) {
            advance();
            auto incDec = std::make_unique<IncDecExpr>();
            incDec->op = isInc ? IncDecOp::Inc : IncDecOp::Dec;
            incDec->prefix = false;
            incDec->indexTarget = std::move(idx->target);
            incDec->indexKey = std::move(idx->index);
            return incDec;
        }

        throw LpcRuntimeError(
            "parse error: postfix " + peek().text +
            " target must be a simple variable name or indexed target at line " +
            std::to_string(peek().line));
    }

    return expr;
}

AstPtr Parser::parseComparison() {
    AstPtr left = parseAdditive();

    while (checkText("<") || checkText("<=") || checkText(">") || checkText(">=")) {
        std::string opText = advance().text;
        BinOp op = (opText == "<") ? BinOp::Lt
                 : (opText == "<=") ? BinOp::Lte
                 : (opText == ">") ? BinOp::Gt
                 : BinOp::Gte;

        auto right = parseAdditive();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = op;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

AstPtr Parser::parseAdditive() {
    AstPtr left = parseMultiplicative();

    while (checkText("+") || checkText("-")) {
        std::string opText = advance().text;
        BinOp op = (opText == "+") ? BinOp::Add : BinOp::Sub;

        auto right = parseMultiplicative();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = op;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

AstPtr Parser::parseMultiplicative() {
    AstPtr left = parseUnary();

    while (checkText("*") || checkText("/") || checkText("%")) {
        std::string opText = advance().text;
        BinOp op = (opText == "*") ? BinOp::Mul
                 : (opText == "/") ? BinOp::Div
                 : BinOp::Mod;

        auto right = parseUnary();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = op;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

AstPtr Parser::parseUnary() {
    // C-style type cast, e.g. "(string)call_other(...)". grammar.y gives
    // this the exact same precedence as unary "!"/"~" ("cast expr0
    // %prec L_NOT"), and its grammar action is purely a compile-time type
    // annotation -- it just overwrites the inner expression node's
    // declared type for the type-checker ("$$ = $2; $$->type = $1;"),
    // with no runtime conversion efun or opcode involved. This driver has
    // no static type checker to feed that annotation to, so a cast is a
    // genuine no-op here: parse past "(type)" or "(type*)" and return
    // whatever the cast operand itself parses to, unchanged. Only
    // recognized when '(' is immediately followed by a type keyword,
    // which is otherwise never a valid start of a parenthesized
    // expression, so this cannot misfire on a plain "(x)".
    if (checkText("(") && peekAt(1).type == TokenType::Keyword && isTypeKeyword(peekAt(1))) {
        size_t save = pos_;
        advance(); // '('
        advance(); // type keyword
        if (checkText("*")) advance();
        if (checkText(")")) {
            advance(); // ')'
            return parseUnary();
        }
        pos_ = save;
    }
    if (checkText("!")) {
        advance();
        auto operand = parseUnary(); // right-associative: "!!x" chains
        auto notExpr = std::make_unique<UnaryExpr>();
        notExpr->op = UnaryOp::Not;
        notExpr->operand = std::move(operand);
        return notExpr;
    }
    if (checkText("-")) {
        advance();
        auto operand = parseUnary(); // right-associative
        auto negExpr = std::make_unique<UnaryExpr>();
        negExpr->op = UnaryOp::Neg;
        negExpr->operand = std::move(operand);
        return negExpr;
    }
    if (checkText("++") || checkText("--")) {
        bool isInc = checkText("++");
        advance();
        auto operand = parseUnary();
        auto* ref = dynamic_cast<VarRefExpr*>(operand.get());
        if (ref) {
            auto incDec = std::make_unique<IncDecExpr>();
            incDec->op = isInc ? IncDecOp::Inc : IncDecOp::Dec;
            incDec->prefix = true;
            incDec->name = ref->name;
            return incDec;
        }

        // Indexed target -- see Ast.hpp's IncDecExpr comment.
        auto* idx = dynamic_cast<IndexExpr*>(operand.get());
        if (idx && !idx->rangeEnd) {
            auto incDec = std::make_unique<IncDecExpr>();
            incDec->op = isInc ? IncDecOp::Inc : IncDecOp::Dec;
            incDec->prefix = true;
            incDec->indexTarget = std::move(idx->target);
            incDec->indexKey = std::move(idx->index);
            return incDec;
        }

        throw LpcRuntimeError(
            std::string("parse error: prefix ") + (isInc ? "++" : "--") +
            " target must be a simple variable name or indexed target at line " +
            std::to_string(peek().line));
    }

    AstPtr operand = parsePostfix();

    // Assignment ("=", "+=", "-=", "*=", "/=", "%=") is recognized right
    // here, immediately after parsing what could be an lvalue, rather
    // than only at the very top of the expression grammar: real LPC's
    // grammar rule is "lvalue L_ASSIGN expr0" (a restricted "lvalue"
    // nonterminal on the left, not any expr0), which yacc's LALR parser
    // resolves by committing to an assignment as soon as it sees "IDENT
    // ... =" regardless of surrounding operators. A naive
    // precedence-climbing parser that only checks for "=" after the
    // *entire* tighter-precedence chain has already returned gets this
    // wrong for exactly the shape real code uses:
    // secure/SimulEfun/domains.c's "stringp(val) && val=load_object(val)
    // && domain_exists(...)" -- it would see "stringp(val) && val"
    // reduce to a single (non-lvalue) expression before ever noticing
    // the "=", and fail to parse at all. Checking here, right where a
    // bare identifier operand has just been parsed, matches where the
    // real grammar's "lvalue" actually gets recognized.
    //
    // The right-hand side below is still a full parseExpr(), not some
    // tighter level: real LPC's assignment is its single loosest-binding
    // operator (grammar.y declares "%right L_ASSIGN" before "%left
    // L_LAND", and bison's own generated table for fluffos-2.9-ds2.08's
    // grammar.y shifts on "&&" rather than reducing the assignment right
    // there -- confirmed directly, not just inferred from the precedence
    // declarations). So domains.c's line actually parses as
    // "stringp(val) && (val = (load_object(val) && domain_exists(...)))"
    // -- the assignment's value swallows the trailing "&& domain_exists"
    // too, not just "load_object(val)". That is not a bug to route
    // around: domain_exists(tmp=(string)val->query_domain()) still runs
    // against val's *old* (string) value at that point (relying on real
    // LPC's implicit string-call_other coercion), and the branch returns
    // "tmp" immediately afterward without ever reading the reassigned
    // (now-boolean) val again.
    bool isCompound = false;
    BinOp compoundOp = BinOp::Add;
    bool sawAssignOp = checkText("=");
    if (!sawAssignOp) {
        static const std::pair<const char*, BinOp> kCompoundOps[] = {
            {"+=", BinOp::Add}, {"-=", BinOp::Sub}, {"*=", BinOp::Mul},
            {"/=", BinOp::Div}, {"%=", BinOp::Mod},
        };
        for (const auto& [opText, binOp] : kCompoundOps) {
            if (checkText(opText)) {
                sawAssignOp = true;
                isCompound = true;
                compoundOp = binOp;
                break;
            }
        }
    }

    if (sawAssignOp) {
        auto* ref = dynamic_cast<VarRefExpr*>(operand.get());
        if (ref) {
            std::string name = ref->name;
            advance();
            // Right-associative ("a = b = c" means "a = (b = c)"): the
            // right-hand side is a full expression, which may itself
            // embed another assignment resolved the same way.
            AstPtr value = parseExpr();

            auto assign = std::make_unique<AssignExpr>();
            assign->name = std::move(name);
            assign->value = std::move(value);
            assign->isCompound = isCompound;
            assign->compoundOp = compoundOp;
            return assign;
        }

        // An indexed target used as a sub-expression rather than a bare
        // statement, e.g. std/user/more.c's own
        // "if(!(__More[\"class\"] = cl)) ...": see Ast.hpp's
        // IndexAssignExpr comment for the confirmed real call site and
        // why this needs its own AST node/codegen rather than reusing
        // the statement-level IndexAssignStmt path.
        auto* idx = dynamic_cast<IndexExpr*>(operand.get());
        if (idx && !idx->rangeEnd) {
            advance();
            AstPtr value = parseExpr();

            auto assign = std::make_unique<IndexAssignExpr>();
            assign->target = std::move(idx->target);
            assign->index = std::move(idx->index);
            assign->value = std::move(value);
            assign->isCompound = isCompound;
            assign->compoundOp = compoundOp;
            return assign;
        }

        // Anything else (a range-index target, a call result, etc) is
        // not a real lvalue in LPC either -- matches real grammar.y's
        // own restricted "lvalue" nonterminal.
        throw LpcRuntimeError(
            "parse error: assignment target must be a simple variable name or "
            "indexed target at line " + std::to_string(peek().line));
    }

    return operand;
}

AstPtr Parser::parseEquality() {
    AstPtr left = parseComparison();

    while (checkText("==") || checkText("!=")) {
        std::string opText = advance().text;
        BinOp op = (opText == "==") ? BinOp::Eq : BinOp::Neq;

        auto right = parseComparison();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = op;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

AstPtr Parser::parseLogicalOr() {
    AstPtr left = parseLogicalAnd();

    while (checkText("||")) {
        advance();
        auto right = parseLogicalAnd();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = BinOp::Or;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

AstPtr Parser::parseLogicalAnd() {
    AstPtr left = parseBitOr();

    while (checkText("&&")) {
        advance();
        auto right = parseBitOr();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = BinOp::And;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

// Plain "|" (bitwise/flags OR, int-only this slice -- see Ast.hpp's
// BinOp::BitOr comment). grammar.y places it between "&&" and "^" in
// precedence ("%left L_LAND" declared before "%left '|'", declared
// before "%left '^'"; earlier declarations bind looser). Hit live in
// secure/std/login.c: "input_to(\"get_password\", 1 | 2)".
AstPtr Parser::parseBitOr() {
    AstPtr left = parseBitXor();

    // checkText("|") alone already excludes "||": the Lexer tokenizes
    // that as one distinct two-character token (text "||"), never as two
    // adjacent single-"|" tokens, so there is no ambiguity to resolve
    // here the way BitAnd's "&" vs "&&" needed none either.
    while (checkText("|")) {
        advance();
        auto right = parseBitXor();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = BinOp::BitOr;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

// Plain "^" (bitwise XOR, int-only this slice -- see Ast.hpp's
// BinOp::BitXor comment). grammar.y places it between "|" and "&".
AstPtr Parser::parseBitXor() {
    AstPtr left = parseBitAnd();

    while (checkText("^")) {
        advance();
        auto right = parseBitAnd();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = BinOp::BitXor;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

// Plain "&" (bitwise AND on numbers, set intersection on arrays -- see
// Ast.hpp's BinOp comment). grammar.y places it between "^" and "==" in
// precedence ("%left '^'" declared before "%left '&'", which is
// declared before "%left L_EQ L_NE"; earlier declarations bind looser).
AstPtr Parser::parseBitAnd() {
    AstPtr left = parseEquality();

    while (checkText("&")) {
        advance();
        auto right = parseEquality();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = BinOp::BitAnd;
        bin->left = std::move(left);
        bin->right = std::move(right);
        left = std::move(bin);
    }

    return left;
}

AstPtr Parser::parseTernary() {
    AstPtr condition = parseLogicalOr();

    if (checkText("?")) {
        advance();
        AstPtr thenBranch = parseTernary();
        expectText(":", "ternary expression");
        AstPtr elseBranch = parseTernary();

        auto tern = std::make_unique<TernaryExpr>();
        tern->condition = std::move(condition);
        tern->thenBranch = std::move(thenBranch);
        tern->elseBranch = std::move(elseBranch);
        return tern;
    }

    return condition;
}

AstPtr Parser::parseExpr() {
    return parseTernary();
}

// Real LPC's for-loop init/update clauses are a "comma_expr" (grammar.y:
// "for_expr: /* EMPTY */ | comma_expr", "comma_expr: expr0 | comma_expr
// ',' expr0"), i.e. one or more comma-separated expressions each
// evaluated in order purely for side effect (confirmed live in
// secure/SimulEfun/misc.c: "for(i = 0, s = sizeof(stack1); i < s; i++)").
// A single expression is returned as-is; more than one is wrapped in a
// Block of ExprStmts, reusing the same "Block standing in for a sequence
// of statements in a single-AstPtr slot" shape the comma-separated local
// var decl already uses -- CodeGen::emitForStmt() dispatches on that
// shape the same way emitStatement() already does for local decls.
AstPtr Parser::parseCommaExprChain() {
    AstPtr first = parseExpr();
    if (!checkText(",")) return first;

    auto wrap = [](AstPtr e) {
        auto s = std::make_unique<ExprStmt>();
        s->expr = std::move(e);
        return s;
    };

    auto block = std::make_unique<Block>();
    block->statements.push_back(wrap(std::move(first)));
    while (checkText(",")) {
        advance();
        block->statements.push_back(wrap(parseExpr()));
    }
    return block;
}

AstPtr Parser::parseReturnStatement() {
    expectText("return", "return statement");
    auto stmt = std::make_unique<ReturnStmt>();
    if (!checkText(";")) {
        stmt->expr = parseExpr();
    }
    expectText(";", "return statement");
    return stmt;
}

// Parses one "type name [= expr]" declaration, without consuming a
// trailing ';' or handling comma-separated follow-on names -- the two
// callers below (a plain statement, and a for-loop init clause) each want
// different tail handling, so that part is left to them.
std::unique_ptr<VarDeclStmt> Parser::parseSingleVarDecl(const Token& typeTok) {
    // An optional '*' marks an array-typed local, e.g. "mixed *items;",
    // same pattern as the parameter/return type array marker.
    bool isArray = false;
    if (checkText("*")) {
        advance();
        isArray = true;
    }

    Token nameTok = expect(TokenType::Ident, "variable declaration name");

    auto decl = std::make_unique<VarDeclStmt>();
    decl->type = typeTok.text;
    decl->isArray = isArray;
    decl->name = nameTok.text;

    if (checkText("=")) {
        advance();
        decl->initializer = parseExpr();
    }

    return decl;
}

AstPtr Parser::parseVarDeclStatement() {
    Token typeTok = expect(TokenType::Keyword, "variable declaration type");
    auto first = parseSingleVarDecl(typeTok);

    // Real LPC allows comma-separated local declarations sharing one type,
    // e.g. "string file, fl, ac;" (confirmed live in the mudlib's
    // secure/daemon/master.c). Each name gets independently wrapped in its
    // own VarDeclStmt and appended as its own statement, mirroring how
    // parseObjectVarDeclRest already handles the same shape for object
    // variables -- there is no multi-name AST node, just several
    // single-name ones emitted back to back.
    if (!checkText(",")) {
        expectText(";", "variable declaration");
        return first;
    }

    auto block = std::make_unique<Block>();
    block->statements.push_back(std::move(first));
    while (checkText(",")) {
        advance();
        block->statements.push_back(parseSingleVarDecl(typeTok));
    }
    expectText(";", "variable declaration");
    return block;
}

AstPtr Parser::parseAssignStatement() {
    Token nameTok = expect(TokenType::Ident, "assignment target");
    expectText("=", "assignment");
    auto stmt = std::make_unique<AssignStmt>();
    stmt->name = nameTok.text;
    stmt->value = parseExpr();
    expectText(";", "assignment statement");
    return stmt;
}

std::unique_ptr<Block> Parser::parseBranch() {
    if (checkText("{")) {
        return parseBlock();
    }
    auto block = std::make_unique<Block>();
    block->statements.push_back(parseStatement());
    return block;
}

AstPtr Parser::parseIfStatement() {
    expectText("if", "if statement");
    expectText("(", "if condition");
    auto stmt = std::make_unique<IfStmt>();
    stmt->condition = parseExpr();
    expectText(")", "if condition");
    stmt->thenBranch = parseBranch();

    if (checkText("else")) {
        advance();
        stmt->elseBranch = parseBranch();
    }

    return stmt;
}

AstPtr Parser::parseWhileStatement() {
    expectText("while", "while statement");
    expectText("(", "while condition");
    auto stmt = std::make_unique<WhileStmt>();
    stmt->condition = parseExpr();
    expectText(")", "while condition");
    stmt->body = parseBranch();
    return stmt;
}

// "for (first_for_expr ; for_expr ; for_expr) statement" (grammar.y).
// first_for_expr is either empty, a plain expression, or a single
// declaration with an optional initializer; the other two clauses are
// either empty or a plain expression. None of the three consume their own
// trailing separator -- this function consumes the ';' / ')' delimiters
// itself so the shared parseExpr()/parseSingleVarDecl() helpers don't need
// a "no trailing punctuation" variant beyond the one already factored out
// for var decls.
AstPtr Parser::parseForStatement() {
    expectText("for", "for statement");
    expectText("(", "for statement");

    auto stmt = std::make_unique<ForStmt>();

    if (!checkText(";")) {
        if (check(TokenType::Keyword) && isTypeKeyword(peek())) {
            Token typeTok = advance();
            stmt->init = parseSingleVarDecl(typeTok);
        } else {
            stmt->init = parseCommaExprChain();
        }
    }
    expectText(";", "for statement init clause");

    if (!checkText(";")) {
        stmt->condition = parseExpr();
    }
    expectText(";", "for statement condition clause");

    if (!checkText(")")) {
        stmt->update = parseCommaExprChain();
    }
    expectText(")", "for statement update clause");

    stmt->body = parseBranch();
    return stmt;
}

AstPtr Parser::parseBreakStatement() {
    expectText("break", "break statement");
    expectText(";", "break statement");
    return std::make_unique<BreakStmt>();
}

AstPtr Parser::parseContinueStatement() {
    expectText("continue", "continue statement");
    expectText(";", "continue statement");
    return std::make_unique<ContinueStmt>();
}

// A foreach loop variable is either a pre-existing name (a plain
// identifier, resolved like any other variable reference at codegen
// time) or a new inline declaration ("type name", the same shape a
// parameter or local var decl uses). The optional array-star marker
// ("mixed *item", seen live in secure/SimulEfun/misc.c) is consumed and
// ignored, same as everywhere else in this parser.
Parser::ForeachVarSpec Parser::parseForeachVar() {
    if (check(TokenType::Keyword) && isTypeKeyword(peek())) {
        advance(); // type
        if (checkText("*")) advance();
        Token nameTok = expect(TokenType::Ident, "foreach variable name");
        return ForeachVarSpec{nameTok.text, true};
    }
    Token nameTok = expect(TokenType::Ident, "foreach variable name");
    return ForeachVarSpec{nameTok.text, false};
}

AstPtr Parser::parseForeachStatement() {
    expectText("foreach", "foreach statement");
    expectText("(", "foreach statement");

    ForeachVarSpec first = parseForeachVar();
    ForeachVarSpec second;
    bool hasSecond = false;
    if (checkText(",")) {
        advance();
        second = parseForeachVar();
        hasSecond = true;
    }
    expectText("in", "foreach statement");
    AstPtr collection = parseExpr();
    expectText(")", "foreach statement");

    auto stmt = std::make_unique<ForeachStmt>();
    stmt->varName = first.name;
    stmt->declareVar = first.isNewDecl;
    if (hasSecond) {
        stmt->hasValueVar = true;
        stmt->valueVarName = second.name;
        stmt->declareValueVar = second.isNewDecl;
    }
    stmt->collection = std::move(collection);
    stmt->body = parseBranch();
    return stmt;
}

// "switch (subject) { case_body }". case_body is a sequence of
// "case constExpr:" / "default:" labels interleaved with ordinary
// statements, matching real C/LPC fallthrough switch (there is no
// implicit break between cases -- see SwitchStmt's comment). Range case
// labels ("case A..B:") are not implemented; nothing in this mudlib uses
// them (confirmed by grep across the whole tree).
AstPtr Parser::parseSwitchStatement() {
    expectText("switch", "switch statement");
    expectText("(", "switch statement subject");
    AstPtr subject = parseExpr();
    expectText(")", "switch statement subject");
    expectText("{", "switch statement body");

    auto stmt = std::make_unique<SwitchStmt>();
    stmt->subject = std::move(subject);

    while (!checkText("}")) {
        if (atEnd()) {
            throw LpcRuntimeError("parse error: unterminated switch statement");
        }
        if (checkText("case")) {
            advance();
            AstPtr value = parseExpr();
            if (checkText("..")) {
                throw NotImplementedError("switch \"case A..B:\" range labels");
            }
            expectText(":", "case label");
            auto label = std::make_unique<CaseLabel>();
            label->value = std::move(value);
            stmt->body.push_back(std::move(label));
            continue;
        }
        if (checkText("default")) {
            advance();
            expectText(":", "default label");
            stmt->body.push_back(std::make_unique<CaseLabel>());
            continue;
        }
        stmt->body.push_back(parseStatement());
    }
    expectText("}", "switch statement body");
    return stmt;
}

AstPtr Parser::parseStatement() {
    // Null statement: a bare ";", most commonly a loop whose entire body
    // is the condition's own side effects (e.g. secure/SimulEfun/misc.c's
    // "while( i-- && ( str[i..i] != ":" ) );"). An empty Block is already
    // this codebase's stand-in for "zero or more statements in a single
    // AstPtr slot" (see parseCommaExprChain()/parseVarDeclStatement()),
    // and CodeGen::emitBlock() over zero statements naturally emits
    // nothing, so it doubles as a no-op here with no CodeGen changes
    // needed.
    if (checkText(";")) {
        advance();
        return std::make_unique<Block>();
    }

    // A bare "{ ... }" block used as a standalone statement (not
    // attached to any if/while/for), purely for local-variable scoping
    // -- real, standard LPC/C syntax. Found live compiling std/user.c's
    // own quit()-adjacent cleanup code: a "{ int total_credits,
    // chip_amount; object chip; ... }" block sitting directly in a
    // function body between two other statements. No CodeGen changes
    // needed: CodeGen::emitStatement() already flattens any Block node
    // it encounters as a statement (originally for comma-separated
    // local var decls, "string a, b;"), which is exactly right here
    // too -- this driver has no lexical block-scoping to enforce beyond
    // what already exists for locals declared inside an if/while body
    // via this same parseBlock().
    if (checkText("{")) {
        return parseBlock();
    }

    if (checkText("if")) {
        return parseIfStatement();
    }
    if (checkText("while")) {
        return parseWhileStatement();
    }
    if (checkText("for")) {
        return parseForStatement();
    }
    if (checkText("foreach")) {
        return parseForeachStatement();
    }
    if (checkText("switch")) {
        return parseSwitchStatement();
    }
    if (checkText("return")) {
        return parseReturnStatement();
    }
    if (checkText("break")) {
        return parseBreakStatement();
    }
    if (checkText("continue")) {
        return parseContinueStatement();
    }

    if (check(TokenType::Keyword) && isTypeKeyword(peek())) {
        return parseVarDeclStatement();
    }

    if (check(TokenType::Ident) && peekAt(1).type == TokenType::Symbol && peekAt(1).text == "=") {
        return parseAssignStatement();
    }

    if (check(TokenType::Ident) && peekAt(1).type == TokenType::Symbol && peekAt(1).text == "[") {
        // Might be an indexed assignment ("ref[fl] = ...;" or "ref[fl]
        // += ...;", real compound-assignment forms too -- confirmed
        // live compiling std/user.c's own "player_data[\"general\"]
        // [\"quest points\"] += (int)call_other(...)", see Ast.hpp's
        // IndexAssignStmt comment) or just an indexed read used as a
        // bare expression statement ("ref[fl];"). Reuse parsePostfix()
        // to parse the target -- it already knows how to build up
        // chained IndexExpr nodes -- then decide which case this is. If
        // it is not an indexed assignment, rewind and let the plain
        // expression-statement path below parse it from scratch.
        size_t save = pos_;
        AstPtr target = parsePostfix();
        if (auto* idx = dynamic_cast<IndexExpr*>(target.get())) {
            bool isCompound = false;
            BinOp compoundOp = BinOp::Add;
            bool sawAssignOp = checkText("=");
            if (!sawAssignOp) {
                static const std::pair<const char*, BinOp> kCompoundOps[] = {
                    {"+=", BinOp::Add}, {"-=", BinOp::Sub}, {"*=", BinOp::Mul},
                    {"/=", BinOp::Div}, {"%=", BinOp::Mod},
                };
                for (const auto& [opText, binOp] : kCompoundOps) {
                    if (checkText(opText)) {
                        sawAssignOp = true;
                        isCompound = true;
                        compoundOp = binOp;
                        break;
                    }
                }
            }
            if (sawAssignOp) {
                advance();
                auto stmt = std::make_unique<IndexAssignStmt>();
                stmt->target = std::move(idx->target);
                stmt->index = std::move(idx->index);
                stmt->value = parseExpr();
                stmt->isCompound = isCompound;
                stmt->compoundOp = compoundOp;
                expectText(";", "indexed assignment statement");
                return stmt;
            }
        }
        pos_ = save;
    }

    auto expr = parseExpr();
    expectText(";", "expression statement");
    auto stmt = std::make_unique<ExprStmt>();
    stmt->expr = std::move(expr);
    return stmt;
}

std::unique_ptr<Block> Parser::parseBlock() {
    expectText("{", "function body");
    auto block = std::make_unique<Block>();
    while (!checkText("}")) {
        if (atEnd()) {
            throw LpcRuntimeError("parse error: unterminated block");
        }
        block->statements.push_back(parseStatement());
    }
    expectText("}", "function body");
    return block;
}

std::vector<Param> Parser::parseParamList() {
    std::vector<Param> params;
    if (checkText(")")) return params;
    for (;;) {
        Token typeTok = expect(TokenType::Keyword, "function parameter type");

        // An optional '*' marks an array/pointer type, e.g. "mixed *info".
        // No array semantics are implemented, this only records the flag
        // so the syntax parses instead of erroring or losing the '*'.
        bool isArray = false;
        if (checkText("*")) {
            advance();
            isArray = true;
        }

        Token nameTok = expect(TokenType::Ident, "function parameter name");
        params.push_back(Param{typeTok.text, nameTok.text, isArray});
        if (checkText(",")) { advance(); continue; }
        break;
    }

    // Trailing "..." marks the function as accepting more actual
    // arguments than declared here (real LPC varargs, e.g.
    // secure/SimulEfun/misc.c's "int true(mixed args...)"). This driver's
    // calling convention already tolerates a call with fewer or more
    // arguments than a function declares -- extra ones are simply
    // ignored, missing ones read as 0 -- so, like the "varargs" modifier
    // keyword before the return type, this is parsed and discarded
    // rather than actually collecting the extra arguments into "args".
    if (checkText("...")) {
        advance();
    }

    return params;
}

Parser::DeclPrefix Parser::parseDeclPrefix(const std::string& context) {
    // Consume any leading modifier keywords (static, private, public,
    // protected, nomask, varargs), in any order, any number of times.
    // This driver has no visibility or staticness semantics to enforce
    // for most of them, so they are otherwise discarded before reading
    // the real type -- "private" is the one exception, recorded via
    // DeclPrefix::isPrivate (see its own comment) because it actually
    // changes object-variable slot-naming semantics, not just access
    // control this driver doesn't enforce anyway.
    bool isPrivate = false;
    while (check(TokenType::Keyword) && isModifierKeyword(peek())) {
        if (peek().text == "private") isPrivate = true;
        advance();
    }

    std::string type;
    bool isArray = false;

    if (check(TokenType::Keyword)) {
        type = advance().text;

        // An optional '*' marks an array/pointer type, e.g. "string
        // *epilog(int x)" or "mixed *items;". This only records the
        // flag, no array semantics are implemented.
        if (checkText("*")) {
            advance();
            isArray = true;
        }
    } else {
        // Real LPC allows the return/declaration type to be omitted
        // entirely when only modifiers precede the name -- grammar.y's
        // own type production is genuinely optional there, historically
        // defaulting to "mixed". Confirmed live compiling std/user.c:
        // "private static register_channels();" (a prototype) and
        // "static private register_channels() { ... }" (its
        // definition), neither naming a return type at all. This driver
        // has no static type checking for the omitted type to feed
        // into anyway, so "mixed" here is just a placeholder label; the
        // next token is the declaration's name, not a type, since a
        // type keyword and a name are different token types
        // (TokenType::Keyword vs. TokenType::Ident) and can never be
        // confused for each other.
        type = "mixed";
    }

    Token nameTok = expect(TokenType::Ident, context + " name");

    return DeclPrefix{type, isArray, nameTok.text, isPrivate};
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionRest(DeclPrefix prefix) {
    auto fn = std::make_unique<FunctionDecl>();
    fn->returnType = std::move(prefix.type);
    fn->returnTypeIsArray = prefix.isArray;
    fn->name = std::move(prefix.name);

    expectText("(", "function declaration parameters");
    fn->params = parseParamList();
    expectText(")", "function declaration parameters");

    if (checkText(";")) {
        // Prototype-only declaration, e.g. "void create();". No body to
        // parse; fn->body stays nullptr, matching IfStmt::elseBranch's
        // precedent for an absent optional block.
        advance();
        return fn;
    }

    fn->body = parseBlock();
    return fn;
}

std::vector<std::unique_ptr<ObjectVarDecl>> Parser::parseObjectVarDeclRest(DeclPrefix prefix) {
    std::vector<std::unique_ptr<ObjectVarDecl>> decls;

    // "private" (like every other modifier here) applies to the whole
    // comma-separated declaration list, not per-name -- e.g.
    // std/living.c's own "static private int __Locked, __LastAged;"
    // makes both names private, not just the first.
    bool isPrivate = prefix.isPrivate;
    auto makeDecl = [isPrivate](const std::string& type, bool isArray, const std::string& name,
                                 AstPtr initializer) {
        auto decl = std::make_unique<ObjectVarDecl>();
        decl->type = type;
        decl->isArray = isArray;
        decl->name = name;
        decl->isPrivate = isPrivate;
        decl->initializer = std::move(initializer);
        return decl;
    };

    // Declaration-time initializers ("type name = expr;") -- real,
    // standard LPC, confirmed live needed by secure/daemon/wiztools.c's
    // own "string *REISSUED_TOOLS = ({ ... });" (surfaced walking the
    // admin-bootstrap path, see Ast.hpp's ObjectVarDecl comment). Only a
    // single expression per name is parsed here; CodeGen::generate()
    // decides how it actually gets evaluated (once per instance, before
    // create() runs).
    AstPtr firstInit;
    if (checkText("=")) {
        advance();
        firstInit = parseExpr();
    }
    decls.push_back(makeDecl(prefix.type, prefix.isArray, prefix.name, std::move(firstInit)));

    while (checkText(",")) {
        advance();

        // Each comma-separated name gets its own independent optional
        // '*', matching real LPC (e.g. "mixed a, *b;"), not just
        // inheriting the first name's array flag.
        bool isArray = false;
        if (checkText("*")) {
            advance();
            isArray = true;
        }

        Token nameTok = expect(TokenType::Ident, "object variable declaration name");

        AstPtr init;
        if (checkText("=")) {
            advance();
            init = parseExpr();
        }

        decls.push_back(makeDecl(prefix.type, isArray, nameTok.text, std::move(init)));
    }

    expectText(";", "object variable declaration");
    return decls;
}

// Real LPC lets an inherit target be any compile-time string constant,
// most commonly a macro ("inherit DAEMON;") or a macro concatenation
// ("inherit REFS_D;", where REFS_D expands to "DIR_DAEMONS+\"/refs\"").
// cpp resolves the macros themselves but has no idea "+" between two now-
// literal strings is foldable, so by the time this driver's Lexer sees
// it, the token stream looks like `"..." + "..."`. This mirrors grammar.y's
// own "string_con1: string_con2 | string_con1 '+' string_con1" rule by
// folding a left-associative chain of string literals here, at parse
// time, since Program::inherits needs a final resolved path string, not
// an expression tree -- there is no VM yet to evaluate one against.
std::string Parser::parseInheritPathString() {
    std::string result = expect(TokenType::String, "inherit statement path").text;
    while (checkText("+")) {
        advance();
        result += expect(TokenType::String, "inherit statement path (string concatenation)").text;
    }
    return result;
}

void Parser::parseInheritStatement(Program& program) {
    expectText("inherit", "inherit statement");
    program.inherits.push_back(parseInheritPathString());
    expectText(";", "inherit statement");
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();
    while (!atEnd()) {
        if (checkText("inherit")) {
            parseInheritStatement(*program);
            continue;
        }

        DeclPrefix prefix = parseDeclPrefix("top-level declaration");

        if (checkText("(")) {
            program->functions.push_back(parseFunctionRest(std::move(prefix)));
        } else {
            auto decls = parseObjectVarDeclRest(std::move(prefix));
            for (auto& decl : decls) {
                program->objectVars.push_back(std::move(decl));
            }
        }
    }
    return program;
}

} // namespace lpcdriver
