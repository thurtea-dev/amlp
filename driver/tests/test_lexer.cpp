#include "lpcdriver/compiler/Lexer.hpp"
#include "lpcdriver/compiler/Parser.hpp"
#include "lpcdriver/compiler/CodeGen.hpp"
#include "lpcdriver/compiler/Ast.hpp"
#include "lpcdriver/vm/Bytecode.hpp"
#include "lpcdriver/vm/Value.hpp"
#include "lpcdriver/vm/VM.hpp"
#include "lpcdriver/object/LpcObject.hpp"
#include "lpcdriver/object/ObjectManager.hpp"
#include "lpcdriver/config/Config.hpp"
#include "lpcdriver/efun/EfunTable.hpp"
#include "lpcdriver/core/Errors.hpp"
#include "lpcdriver/net/Connection.hpp"
#include "lpcdriver/net/OutputContext.hpp"
#include "lpcdriver/net/Server.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sstream>

static void testBasicTokenize() {
    std::string src =
        "void create() {\n"
        "    write(\"Hello from simple_login.c create()!\");\n"
        "}\n";

    lpcdriver::Lexer lexer(src);
    auto tokens = lexer.tokenize();

    assert(tokens.size() == 12);
    assert(tokens[0].type == lpcdriver::TokenType::Keyword && tokens[0].text == "void");
    assert(tokens[1].type == lpcdriver::TokenType::Ident && tokens[1].text == "create");
    assert(tokens[7].type == lpcdriver::TokenType::String);
    assert(tokens[7].text == "Hello from simple_login.c create()!");

    std::cout << "testBasicTokenize OK\n";
}

static void testArrowTokenizes() {
    lpcdriver::Lexer lexer("ob->greet();");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 7);
    assert(tokens[1].type == lpcdriver::TokenType::Symbol && tokens[1].text == "->");
    std::cout << "testArrowTokenizes OK\n";
}

static void testComparisonOperatorsTokenize() {
    lpcdriver::Lexer lexer("a == b != c <= d >= e < f > g");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"==", "!=", "<=", ">=", "<", ">"};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == lpcdriver::TokenType::Symbol) {
            assert(t.text == expectedOps[opIdx]);
            ++opIdx;
        }
    }
    assert(opIdx == expectedOps.size());
    std::cout << "testComparisonOperatorsTokenize OK\n";
}

static void testCallOtherParsesToCallOtherExpr() {
    std::string src =
        "void create() {\n"
        "    call_other(clone_object(\"/obj/x\"), \"greet\");\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* callOther = dynamic_cast<lpcdriver::CallOtherExpr*>(exprStmt->expr.get());
    assert(callOther != nullptr);
    // Literal call_other(target, "name", ...) still parses its function
    // name down to a StringLiteral, same as before this became a general
    // expression (see Ast.hpp's CallOtherExpr comment).
    auto* funcLit = dynamic_cast<lpcdriver::StringLiteral*>(callOther->function.get());
    assert(funcLit != nullptr);
    assert(funcLit->value == "greet");

    std::cout << "testCallOtherParsesToCallOtherExpr OK\n";
}

static void testArrowOperatorParsesToCallOtherExpr() {
    std::string src =
        "void create() {\n"
        "    clone_object(\"/obj/x\")->greet_again();\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    auto* callOther = dynamic_cast<lpcdriver::CallOtherExpr*>(exprStmt->expr.get());
    assert(callOther != nullptr);
    auto* funcLit = dynamic_cast<lpcdriver::StringLiteral*>(callOther->function.get());
    assert(funcLit != nullptr);
    assert(funcLit->value == "greet_again");

    std::cout << "testArrowOperatorParsesToCallOtherExpr OK\n";
}

static void testCodegenEmitsCallEfunForCallOther() {
    std::string src =
        "void create() {\n"
        "    call_other(clone_object(\"/obj/x\"), \"greet\");\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    // clone_object(...) is a plain bare call, so it compiles to OpCode::Call
    // now (resolved local-function-first, efun-fallback at run time -- see
    // the same-object-calls slice); call_other() is always forced to a
    // real CallEfun regardless of any local function named "call_other",
    // since it is a compiler-level construct, not an ordinary efun lookup.
    bool sawCloneObject = false;
    bool sawCallOther = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::Call) {
            const std::string& name = compiled.stringPool[instr.operand];
            if (name == "clone_object") sawCloneObject = true;
        }
        if (instr.op == lpcdriver::OpCode::CallEfun) {
            const std::string& name = compiled.stringPool[instr.operand];
            if (name == "call_other") sawCallOther = true;
        }
    }
    assert(sawCloneObject);
    assert(sawCallOther);

    std::cout << "testCodegenEmitsCallEfunForCallOther OK\n";
}

static void testMultiFunctionProgramParses() {
    std::string src =
        "void create() {\n"
        "    write(\"hi\");\n"
        "}\n"
        "void receive_message() {\n"
        "    write(\"ack\");\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    assert(program->functions.size() == 2);
    assert(program->functions[0]->name == "create");
    assert(program->functions[1]->name == "receive_message");
    std::cout << "testMultiFunctionProgramParses OK\n";
}

static void testFunctionWithParameterParses() {
    std::string src =
        "void receive_message(string msg) {\n"
        "    write(msg);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    assert(program->functions.size() == 1);
    assert(program->functions[0]->params.size() == 1);
    assert(program->functions[0]->params[0].type == "string");
    assert(program->functions[0]->params[0].name == "msg");

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* call = dynamic_cast<lpcdriver::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr);
    auto* ref = dynamic_cast<lpcdriver::VarRefExpr*>(call->args[0].get());
    assert(ref != nullptr);
    assert(ref->name == "msg");

    std::cout << "testFunctionWithParameterParses OK\n";
}

static void testVarDeclAndAssignParse() {
    std::string src =
        "void receive_message(string msg) {\n"
        "    string s;\n"
        "    s = msg;\n"
        "    write(s);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 3);

    auto* decl = dynamic_cast<lpcdriver::VarDeclStmt*>(body[0].get());
    assert(decl != nullptr);
    assert(decl->type == "string");
    assert(decl->name == "s");
    assert(decl->initializer == nullptr);

    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[1].get());
    assert(assign != nullptr);
    assert(assign->name == "s");
    auto* rhsRef = dynamic_cast<lpcdriver::VarRefExpr*>(assign->value.get());
    assert(rhsRef != nullptr);
    assert(rhsRef->name == "msg");

    std::cout << "testVarDeclAndAssignParse OK\n";
}

static void testCodegenBindsParamAndEchoesRuntimeValue() {
    std::string src =
        "void receive_message(string msg) {\n"
        "    string s;\n"
        "    s = msg;\n"
        "    write(s);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    assert(compiled.functions.size() == 1);
    assert(compiled.functions[0].numArgs == 1);
    assert(compiled.functions[0].numLocals == 2);

    bool sawPushLocal = false;
    bool sawStoreLocal = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::PushLocal) sawPushLocal = true;
        if (instr.op == lpcdriver::OpCode::StoreLocal) sawStoreLocal = true;
    }
    assert(sawPushLocal);
    assert(sawStoreLocal);

    std::cout << "testCodegenBindsParamAndEchoesRuntimeValue OK\n";
}

static void testIfElseParsesToIfStmt() {
    std::string src =
        "void receive_message(string msg) {\n"
        "    if (msg == \"quit\") {\n"
        "        write(\"Goodbye.\");\n"
        "    } else {\n"
        "        write(msg);\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 1);
    auto* ifStmt = dynamic_cast<lpcdriver::IfStmt*>(body[0].get());
    assert(ifStmt != nullptr);
    assert(ifStmt->thenBranch != nullptr);
    assert(ifStmt->elseBranch != nullptr);

    auto* cond = dynamic_cast<lpcdriver::BinaryExpr*>(ifStmt->condition.get());
    assert(cond != nullptr);
    assert(cond->op == lpcdriver::BinOp::Eq);

    std::cout << "testIfElseParsesToIfStmt OK\n";
}

static void testWhileParsesToWhileStmt() {
    std::string src =
        "void create() {\n"
        "    int i;\n"
        "    i = 0;\n"
        "    while (i < 3) {\n"
        "        i = i;\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 3);
    auto* whileStmt = dynamic_cast<lpcdriver::WhileStmt*>(body[2].get());
    assert(whileStmt != nullptr);

    auto* cond = dynamic_cast<lpcdriver::BinaryExpr*>(whileStmt->condition.get());
    assert(cond != nullptr);
    assert(cond->op == lpcdriver::BinOp::Lt);

    std::cout << "testWhileParsesToWhileStmt OK\n";
}

static void testCodegenEmitsJumpOpcodesForIf() {
    std::string src =
        "void receive_message(string msg) {\n"
        "    if (msg == \"quit\") {\n"
        "        write(\"Goodbye.\");\n"
        "    } else {\n"
        "        write(msg);\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawEq = false, sawJumpIfFalse = false, sawJump = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::Eq) sawEq = true;
        if (instr.op == lpcdriver::OpCode::JumpIfFalse) sawJumpIfFalse = true;
        if (instr.op == lpcdriver::OpCode::Jump) sawJump = true;
    }
    assert(sawEq);
    assert(sawJumpIfFalse);
    assert(sawJump);

    // Every jump target must be a valid in-range instruction index.
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::Jump || instr.op == lpcdriver::OpCode::JumpIfFalse) {
            assert(instr.operand >= 0);
            assert(static_cast<size_t>(instr.operand) <= compiled.code.size());
        }
    }

    std::cout << "testCodegenEmitsJumpOpcodesForIf OK\n";
}

static void testPrototypeThenDefinitionParsesAndCodegenEmitsOnlyOne() {
    std::string src =
        "void create();\n"
        "void create() {\n"
        "    write(\"hi\");\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 2);
    assert(program->functions[0]->name == "create");
    assert(program->functions[0]->body == nullptr);
    assert(program->functions[1]->name == "create");
    assert(program->functions[1]->body != nullptr);

    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    assert(compiled.functions.size() == 1);
    assert(compiled.functions[0].name == "create");

    // write(...) is a plain bare call -> OpCode::Call (see
    // testCodegenEmitsCallEfunForCallOther's comment on the same-object
    // calls slice).
    bool sawWrite = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::Call) {
            const std::string& name = compiled.stringPool[instr.operand];
            if (name == "write") sawWrite = true;
        }
    }
    assert(sawWrite);

    std::cout << "testPrototypeThenDefinitionParsesAndCodegenEmitsOnlyOne OK\n";
}

static void testTwoModifiersBeforeReturnTypeParseAsPrototype() {
    std::string src = "static private void load_access(string cfg, mapping ref);\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    auto* fn = program->functions[0].get();
    assert(fn->name == "load_access");
    assert(fn->body == nullptr);
    assert(fn->params.size() == 2);
    assert(fn->params[0].type == "string");
    assert(fn->params[0].name == "cfg");
    assert(fn->params[1].type == "mapping");
    assert(fn->params[1].name == "ref");

    std::cout << "testTwoModifiersBeforeReturnTypeParseAsPrototype OK\n";
}

static void testSingleModifierBeforeReturnTypeParsesAsPrototype() {
    std::string src = "private void flag(string str);\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    auto* fn = program->functions[0].get();
    assert(fn->name == "flag");
    assert(fn->body == nullptr);
    assert(fn->params.size() == 1);
    assert(fn->params[0].type == "string");
    assert(fn->params[0].name == "str");

    std::cout << "testSingleModifierBeforeReturnTypeParsesAsPrototype OK\n";
}

static void testUnrecognizedCharacterThrows() {
    // "@" used to be this probe's unrecognized character, but it is now
    // real syntax (heredoc string literals, see the heredoc slice) --
    // "`" is still nowhere in the recognized-symbol list.
    std::string src = "void create() { int x; x = 1 ` 2; }\n";
    lpcdriver::Lexer lexer(src);
    bool threw = false;
    try {
        lexer.tokenize();
    } catch (const lpcdriver::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("unrecognized character") != std::string::npos);
    }
    assert(threw);
    std::cout << "testUnrecognizedCharacterThrows OK\n";
}

static void testFunctionTypeParameterParsesAsPrototype() {
    std::string src = "mixed apply_unguarded(function f);\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    auto* fn = program->functions[0].get();
    assert(fn->name == "apply_unguarded");
    assert(fn->body == nullptr);
    assert(fn->params.size() == 1);
    assert(fn->params[0].type == "function");
    assert(fn->params[0].name == "f");
    assert(fn->params[0].isArray == false);

    std::cout << "testFunctionTypeParameterParsesAsPrototype OK\n";
}

static void testAsteriskParameterTypeParsesWithIsArrayTrue() {
    std::string src = "int valid_socket(mixed *info);\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    auto* fn = program->functions[0].get();
    assert(fn->params.size() == 1);
    assert(fn->params[0].type == "mixed");
    assert(fn->params[0].name == "info");
    assert(fn->params[0].isArray == true);

    std::cout << "testAsteriskParameterTypeParsesWithIsArrayTrue OK\n";
}

static void testPlainParameterTypeDefaultsIsArrayFalse() {
    std::string src = "int valid_socket(mixed info);\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    auto* fn = program->functions[0].get();
    assert(fn->params.size() == 1);
    assert(fn->params[0].type == "mixed");
    assert(fn->params[0].name == "info");
    assert(fn->params[0].isArray == false);

    std::cout << "testPlainParameterTypeDefaultsIsArrayFalse OK\n";
}

static void testArrayCheckObjectParsesAndCodegens() {
    std::string src =
        "void create() {\n"
        "    mixed *items;\n"
        "    mapping scores;\n"
        "    int total;\n"
        "\n"
        "    items = ({ \"sword\", \"shield\", \"potion\" });\n"
        "    scores = ([]);\n"
        "    scores[\"sword\"] = 10;\n"
        "\n"
        "    total = sizeof(items);\n"
        "\n"
        "    if (total == 3) {\n"
        "        write(\"items: 3\\n\");\n"
        "    } else {\n"
        "        write(\"items: wrong\\n\");\n"
        "    }\n"
        "\n"
        "    write(\"first: \" + items[0] + \"\\n\");\n"
        "\n"
        "    if (scores[\"sword\"] == 10) {\n"
        "        write(\"sword score: 10\\n\");\n"
        "    } else {\n"
        "        write(\"sword score: wrong\\n\");\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 10);

    auto* itemsDecl = dynamic_cast<lpcdriver::VarDeclStmt*>(body[0].get());
    assert(itemsDecl != nullptr);
    assert(itemsDecl->isArray == true);
    assert(itemsDecl->type == "mixed");
    assert(itemsDecl->name == "items");

    auto* itemsAssign = dynamic_cast<lpcdriver::AssignStmt*>(body[3].get());
    assert(itemsAssign != nullptr);
    auto* arrLit = dynamic_cast<lpcdriver::ArrayLiteralExpr*>(itemsAssign->value.get());
    assert(arrLit != nullptr);
    assert(arrLit->elements.size() == 3);

    auto* scoresAssign = dynamic_cast<lpcdriver::AssignStmt*>(body[4].get());
    assert(scoresAssign != nullptr);
    auto* mapLit = dynamic_cast<lpcdriver::MappingLiteralExpr*>(scoresAssign->value.get());
    assert(mapLit != nullptr);
    assert(mapLit->entries.empty());

    auto* indexAssign = dynamic_cast<lpcdriver::IndexAssignStmt*>(body[5].get());
    assert(indexAssign != nullptr);

    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawMakeArray = false, sawMakeMapping = false, sawIndex = false, sawIndexAssign = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::MakeArray) sawMakeArray = true;
        if (instr.op == lpcdriver::OpCode::MakeMapping) sawMakeMapping = true;
        if (instr.op == lpcdriver::OpCode::Index) sawIndex = true;
        if (instr.op == lpcdriver::OpCode::IndexAssign) sawIndexAssign = true;
    }
    assert(sawMakeArray);
    assert(sawMakeMapping);
    assert(sawIndex);
    assert(sawIndexAssign);

    std::cout << "testArrayCheckObjectParsesAndCodegens OK\n";
}

static void testLogicalOperatorsTokenize() {
    lpcdriver::Lexer lexer("a || b && c");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"||", "&&"};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == lpcdriver::TokenType::Symbol) {
            assert(t.text == expectedOps[opIdx]);
            ++opIdx;
        }
    }
    assert(opIdx == expectedOps.size());

    // A lone '|' or '&' (not doubled) still lexes fine as its own
    // one-character Symbol token, same as a lone '-' already does today.
    lpcdriver::Lexer loneLexer("a | b & c");
    auto loneTokens = loneLexer.tokenize();
    std::vector<std::string> expectedLoneOps = {"|", "&"};
    size_t loneOpIdx = 0;
    for (const auto& t : loneTokens) {
        if (t.type == lpcdriver::TokenType::Symbol) {
            assert(t.text == expectedLoneOps[loneOpIdx]);
            ++loneOpIdx;
        }
    }
    assert(loneOpIdx == expectedLoneOps.size());

    std::cout << "testLogicalOperatorsTokenize OK\n";
}

static void testGuardConditionShapeParsesWithCorrectPrecedence() {
    // Same shape as the real blocking line in secure/daemon/master.c:
    // if(!lines[i] || lines[i] == "" || lines[i][0] == '#') continue;
    std::string src =
        "void create() {\n"
        "    mixed *lines;\n"
        "    int i;\n"
        "    if (!lines[i] || lines[i] == \"\" || lines[i] == \"#\") {\n"
        "        write(\"skip\");\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ifStmt = dynamic_cast<lpcdriver::IfStmt*>(body[2].get());
    assert(ifStmt != nullptr);

    // Outermost operator must be Or: "(!lines[i] || lines[i] == "") || (lines[i] == "#")".
    auto* outer = dynamic_cast<lpcdriver::BinaryExpr*>(ifStmt->condition.get());
    assert(outer != nullptr);
    assert(outer->op == lpcdriver::BinOp::Or);

    // Its left side is itself an Or, whose left side is a UnaryExpr(Not)
    // over an IndexExpr, and whose right side is an Eq comparison. This
    // confirms == binds tighter than ||, and ! binds only to its
    // immediate operand.
    auto* innerOr = dynamic_cast<lpcdriver::BinaryExpr*>(outer->left.get());
    assert(innerOr != nullptr);
    assert(innerOr->op == lpcdriver::BinOp::Or);

    auto* notExpr = dynamic_cast<lpcdriver::UnaryExpr*>(innerOr->left.get());
    assert(notExpr != nullptr);
    assert(notExpr->op == lpcdriver::UnaryOp::Not);
    auto* notOperand = dynamic_cast<lpcdriver::IndexExpr*>(notExpr->operand.get());
    assert(notOperand != nullptr);

    auto* firstEq = dynamic_cast<lpcdriver::BinaryExpr*>(innerOr->right.get());
    assert(firstEq != nullptr);
    assert(firstEq->op == lpcdriver::BinOp::Eq);

    auto* secondEq = dynamic_cast<lpcdriver::BinaryExpr*>(outer->right.get());
    assert(secondEq != nullptr);
    assert(secondEq->op == lpcdriver::BinOp::Eq);

    std::cout << "testGuardConditionShapeParsesWithCorrectPrecedence OK\n";
}

static void testCodegenEmitsDupAndNotForLogicalAndUnary() {
    std::string src =
        "void create() {\n"
        "    int x;\n"
        "    int y;\n"
        "    if (!x || x == 0 || y == 0) {\n"
        "        x = 1;\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawDup = false, sawNot = false, sawJumpIfFalse = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::Dup) sawDup = true;
        if (instr.op == lpcdriver::OpCode::Not) sawNot = true;
        if (instr.op == lpcdriver::OpCode::JumpIfFalse) sawJumpIfFalse = true;
    }
    assert(sawDup);
    assert(sawNot);
    assert(sawJumpIfFalse);

    // Every jump target must be in range, and at least one JumpIfFalse
    // (the short-circuit test) must patch forward past at least one
    // instruction it is skipping over, confirming the short-circuit
    // shape rather than just opcode presence.
    bool sawForwardSkip = false;
    for (size_t idx = 0; idx < compiled.code.size(); ++idx) {
        const auto& instr = compiled.code[idx];
        if (instr.op == lpcdriver::OpCode::Jump || instr.op == lpcdriver::OpCode::JumpIfFalse) {
            assert(instr.operand >= 0);
            assert(static_cast<size_t>(instr.operand) <= compiled.code.size());
            if (instr.op == lpcdriver::OpCode::JumpIfFalse &&
                static_cast<size_t>(instr.operand) > idx + 1) {
                sawForwardSkip = true;
            }
        }
    }
    assert(sawForwardSkip);

    std::cout << "testCodegenEmitsDupAndNotForLogicalAndUnary OK\n";
}

// Helper for the VM-level short-circuit tests below: compiles a single
// "int probe() { ... }" function and runs it, returning the Value it
// produces. Uses a default-constructed Config and an ObjectManager that
// is never actually asked to load or clone anything, since the probe
// functions below only touch locals, literals, and (for the intentional
// crash cases) an efun name that is never registered in this test
// binary.
static lpcdriver::Value runProbe(const std::string& probeBody) {
    std::string src = "int probe() {\n" + probeBody + "\n}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = std::make_shared<lpcdriver::CompiledProgram>(codegen.generate(*program));

    auto obj = std::make_shared<lpcdriver::LpcObject>("probe_object", compiled);
    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    return vm.callFunction(obj, "probe", {});
}

static void testLogicalOrShortCircuitsWhenLeftTruthy() {
    // If || evaluated its right operand unconditionally, calling this
    // nonexistent efun would throw "undefined efun" and fail the test.
    // It must never run because "x" (1) already decides the result.
    lpcdriver::Value result = runProbe(
        "int x;\n"
        "x = 1;\n"
        "return x || nonexistent_marker_efun();\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testLogicalOrShortCircuitsWhenLeftTruthy OK\n";
}

static void testLogicalOrEvaluatesRightWhenLeftFalsy() {
    lpcdriver::Value result = runProbe(
        "int x;\n"
        "x = 0;\n"
        "return x || 5;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 5);

    std::cout << "testLogicalOrEvaluatesRightWhenLeftFalsy OK\n";
}

static void testLogicalAndShortCircuitsWhenLeftFalsy() {
    // Same reasoning as the Or case above, mirrored: the right operand
    // must never be evaluated because "x" (0) already decides the
    // result.
    lpcdriver::Value result = runProbe(
        "int x;\n"
        "x = 0;\n"
        "return x && nonexistent_marker_efun();\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testLogicalAndShortCircuitsWhenLeftFalsy OK\n";
}

static void testLogicalAndEvaluatesRightWhenLeftTruthy() {
    lpcdriver::Value result = runProbe(
        "int x;\n"
        "x = 1;\n"
        "return x && 5;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 5);

    std::cout << "testLogicalAndEvaluatesRightWhenLeftTruthy OK\n";
}

static void testUnaryNotNegatesTruthiness() {
    lpcdriver::Value resultOfFalsy = runProbe(
        "int x;\n"
        "x = 0;\n"
        "return !x;\n");
    assert(std::holds_alternative<int64_t>(resultOfFalsy.data));
    assert(std::get<int64_t>(resultOfFalsy.data) == 1);

    lpcdriver::Value resultOfTruthy = runProbe(
        "int x;\n"
        "x = 1;\n"
        "return !x;\n");
    assert(std::holds_alternative<int64_t>(resultOfTruthy.data));
    assert(std::get<int64_t>(resultOfTruthy.data) == 0);

    std::cout << "testUnaryNotNegatesTruthiness OK\n";
}

static void testGuardConditionDoesNotCrashOnEmptyArray() {
    // Reproduces the exact risk the plan called out, using constructs
    // already fully implemented (character literals like '#' are a
    // separate, still-unimplemented gap, out of scope for this slice,
    // so this uses array indexing instead of string character indexing
    // to exercise the same "must not evaluate an unsafe index after an
    // earlier condition already decided the result" shape). "items" is
    // empty, so "items[0]" is out of bounds and the VM's existing Index
    // opcode already throws for that. If short-circuiting did not stop
    // evaluation once "sizeof(items) == 0" already decided the result,
    // this would throw instead of returning 1.
    lpcdriver::Value result = runProbe(
        "mixed *items;\n"
        "items = ({});\n"
        "if (sizeof(items) == 0 || items[0] == 1) {\n"
        "    return 1;\n"
        "}\n"
        "return 0;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testGuardConditionDoesNotCrashOnEmptyArray OK\n";
}

static void testCharLiteralsTokenizeToCorrectAsciiValue() {
    struct Case { std::string src; std::string expected; };
    std::vector<Case> cases = {
        {"'#'", "35"},
        {"'/'", "47"},
        {"'a'", "97"},
        {"'z'", "122"},
    };
    for (const auto& c : cases) {
        lpcdriver::Lexer lexer(c.src);
        auto tokens = lexer.tokenize();
        assert(tokens.size() == 2); // the literal, then End
        assert(tokens[0].type == lpcdriver::TokenType::Number);
        assert(tokens[0].text == c.expected);
    }

    std::cout << "testCharLiteralsTokenizeToCorrectAsciiValue OK\n";
}

static void testCharLiteralEscapeSequenceTokenizes() {
    // Not present anywhere in master.c today, but the lexChar() design
    // mirrors lexString()'s escape table, so this confirms it actually
    // works, not just that the plain-character case does.
    lpcdriver::Lexer lexer("'\\n'");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 2);
    assert(tokens[0].type == lpcdriver::TokenType::Number);
    assert(tokens[0].text == "10");

    std::cout << "testCharLiteralEscapeSequenceTokenizes OK\n";
}

static void testMalformedCharLiteralThrows() {
    bool threwForEmpty = false;
    try {
        lpcdriver::Lexer lexer("''");
        lexer.tokenize();
    } catch (const lpcdriver::LpcRuntimeError&) {
        threwForEmpty = true;
    }
    assert(threwForEmpty);

    bool threwForTwoChars = false;
    try {
        lpcdriver::Lexer lexer("'ab'");
        lexer.tokenize();
    } catch (const lpcdriver::LpcRuntimeError&) {
        threwForTwoChars = true;
    }
    assert(threwForTwoChars);

    std::cout << "testMalformedCharLiteralThrows OK\n";
}

static void testCharLiteralParsesAsIntLiteral() {
    std::string src =
        "void create() {\n"
        "    int x;\n"
        "    if (x == '#') {\n"
        "        x = 1;\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ifStmt = dynamic_cast<lpcdriver::IfStmt*>(body[1].get());
    assert(ifStmt != nullptr);

    auto* cond = dynamic_cast<lpcdriver::BinaryExpr*>(ifStmt->condition.get());
    assert(cond != nullptr);
    assert(cond->op == lpcdriver::BinOp::Eq);

    // No new AST node for character literals: the right-hand side must
    // be a plain IntLiteral, same as any other integer literal, per the
    // plan's decision to reuse the existing pipeline instead of adding
    // a dedicated CharLiteral node.
    auto* rightLit = dynamic_cast<lpcdriver::IntLiteral*>(cond->right.get());
    assert(rightLit != nullptr);
    assert(rightLit->value == 35);

    std::cout << "testCharLiteralParsesAsIntLiteral OK\n";
}

static void testStringIndexingReturnsByteValue() {
    lpcdriver::Value matchResult = runProbe(
        "string s;\n"
        "s = \"#comment\";\n"
        "return s[0] == '#';\n");
    assert(std::holds_alternative<int64_t>(matchResult.data));
    assert(std::get<int64_t>(matchResult.data) == 1);

    lpcdriver::Value noMatchResult = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[0] == '#';\n");
    assert(std::holds_alternative<int64_t>(noMatchResult.data));
    assert(std::get<int64_t>(noMatchResult.data) == 0);

    std::cout << "testStringIndexingReturnsByteValue OK\n";
}

static void testStringIndexingOutOfBoundsThrows() {
    bool threw = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"\";\n"
            "return s[0] == '#';\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testStringIndexingOutOfBoundsThrows OK\n";
}

static void testGuardConditionShapeWithRealStringIndexing() {
    // Same shape as the real blocking line in secure/daemon/master.c:
    //   if(!lines[i] || lines[i] == "" || lines[i][0] == '#') continue;
    // Now using an actual string and real character-literal comparison,
    // no array-of-fields stand-in needed (that stand-in existed only
    // because this slice's constructs did not exist yet).
    lpcdriver::Value skipOnComment = runProbe(
        "string s;\n"
        "s = \"#comment\";\n"
        "if (!s || s == \"\" || s[0] == '#') {\n"
        "    return 1;\n"
        "}\n"
        "return 0;\n");
    assert(std::holds_alternative<int64_t>(skipOnComment.data));
    assert(std::get<int64_t>(skipOnComment.data) == 1);

    lpcdriver::Value skipOnEmpty = runProbe(
        "string s;\n"
        "s = \"\";\n"
        "if (!s || s == \"\" || s[0] == '#') {\n"
        "    return 1;\n"
        "}\n"
        "return 0;\n");
    assert(std::holds_alternative<int64_t>(skipOnEmpty.data));
    assert(std::get<int64_t>(skipOnEmpty.data) == 1);

    lpcdriver::Value keepNormalLine = runProbe(
        "string s;\n"
        "s = \"sword\";\n"
        "if (!s || s == \"\" || s[0] == '#') {\n"
        "    return 1;\n"
        "}\n"
        "return 0;\n");
    assert(std::holds_alternative<int64_t>(keepNormalLine.data));
    assert(std::get<int64_t>(keepNormalLine.data) == 0);

    std::cout << "testGuardConditionShapeWithRealStringIndexing OK\n";
}

static void testDivisionAndModuloTokenizeAsSymbols() {
    lpcdriver::Lexer lexer("t/60 t%60");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"/", "%"};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == lpcdriver::TokenType::Symbol) {
            assert(t.text == expectedOps[opIdx]);
            ++opIdx;
        }
    }
    assert(opIdx == expectedOps.size());

    std::cout << "testDivisionAndModuloTokenizeAsSymbols OK\n";
}

static void testCommentsStillWorkAfterSlashWhitelisting() {
    // Regression check: adding '/' to the whitelist must not break the
    // existing '//' and '/* ... */' comment handling, since
    // skipWhitespaceAndComments() must still consume both fully before
    // the main dispatch ever sees a bare '/'.
    std::string src =
        "void create() {\n"
        "    // a line comment with a / in it\n"
        "    int x;\n"
        "    /* a block comment\n"
        "       with a / and a % in it */\n"
        "    x = 1;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 2);

    std::cout << "testCommentsStillWorkAfterSlashWhitelisting OK\n";
}

static void testDivisionAndModuloParseToCorrectBinOp() {
    std::string src =
        "void create() {\n"
        "    int t;\n"
        "    int a;\n"
        "    int b;\n"
        "    a = t / 60;\n"
        "    b = t % 60;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* divAssign = dynamic_cast<lpcdriver::AssignStmt*>(body[3].get());
    assert(divAssign != nullptr);
    auto* divExpr = dynamic_cast<lpcdriver::BinaryExpr*>(divAssign->value.get());
    assert(divExpr != nullptr);
    assert(divExpr->op == lpcdriver::BinOp::Div);

    auto* modAssign = dynamic_cast<lpcdriver::AssignStmt*>(body[4].get());
    assert(modAssign != nullptr);
    auto* modExpr = dynamic_cast<lpcdriver::BinaryExpr*>(modAssign->value.get());
    assert(modExpr != nullptr);
    assert(modExpr->op == lpcdriver::BinOp::Mod);

    std::cout << "testDivisionAndModuloParseToCorrectBinOp OK\n";
}

static void testUnaryMinusParsesAsNegExpr() {
    std::string src =
        "void create() {\n"
        "    int x;\n"
        "    x = -1;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[1].get());
    assert(assign != nullptr);

    // No literal-only special case: -1 must parse through the general
    // unary path as a UnaryExpr(Neg) wrapping a plain IntLiteral(1),
    // not as some dedicated negative-literal token.
    auto* negExpr = dynamic_cast<lpcdriver::UnaryExpr*>(assign->value.get());
    assert(negExpr != nullptr);
    assert(negExpr->op == lpcdriver::UnaryOp::Neg);

    auto* innerLit = dynamic_cast<lpcdriver::IntLiteral*>(negExpr->operand.get());
    assert(innerLit != nullptr);
    assert(innerLit->value == 1);

    std::cout << "testUnaryMinusParsesAsNegExpr OK\n";
}

static void testMultiplicativeBindsTighterThanAdditive() {
    std::string src =
        "void create() {\n"
        "    int a;\n"
        "    int b;\n"
        "    int c;\n"
        "    int r;\n"
        "    r = a - b * c;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[4].get());
    assert(assign != nullptr);

    auto* outer = dynamic_cast<lpcdriver::BinaryExpr*>(assign->value.get());
    assert(outer != nullptr);
    assert(outer->op == lpcdriver::BinOp::Sub);

    auto* right = dynamic_cast<lpcdriver::BinaryExpr*>(outer->right.get());
    assert(right != nullptr);
    assert(right->op == lpcdriver::BinOp::Mul);

    std::cout << "testMultiplicativeBindsTighterThanAdditive OK\n";
}

static void testCodegenEmitsDivAndModOpcodes() {
    std::string src =
        "void create() {\n"
        "    int t;\n"
        "    int a;\n"
        "    int b;\n"
        "    a = t / 60;\n"
        "    b = t % 60;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawDiv = false, sawMod = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::Div) sawDiv = true;
        if (instr.op == lpcdriver::OpCode::Mod) sawMod = true;
    }
    assert(sawDiv);
    assert(sawMod);

    std::cout << "testCodegenEmitsDivAndModOpcodes OK\n";
}

static void testArithmeticVmExecution() {
    lpcdriver::Value subResult = runProbe("return 10 - 3;\n");
    assert(std::holds_alternative<int64_t>(subResult.data));
    assert(std::get<int64_t>(subResult.data) == 7);

    lpcdriver::Value mulResult = runProbe("return 4 * 5;\n");
    assert(std::holds_alternative<int64_t>(mulResult.data));
    assert(std::get<int64_t>(mulResult.data) == 20);

    lpcdriver::Value divResult = runProbe("return 17 / 5;\n");
    assert(std::holds_alternative<int64_t>(divResult.data));
    assert(std::get<int64_t>(divResult.data) == 3);

    lpcdriver::Value modResult = runProbe("return 17 % 5;\n");
    assert(std::holds_alternative<int64_t>(modResult.data));
    assert(std::get<int64_t>(modResult.data) == 2);

    lpcdriver::Value negResult = runProbe("return -1;\n");
    assert(std::holds_alternative<int64_t>(negResult.data));
    assert(std::get<int64_t>(negResult.data) == -1);

    std::cout << "testArithmeticVmExecution OK\n";
}

// Found compiling the real secure/SimulEfun/SimulEfun.c (misc.c's own
// "return 0.0;" and "percent()"'s trailing-digit/leading-dot mix): float
// literals were not lexed at all -- lexNumber() only ever consumed the
// integer part, leaving a bare '.' for the caller to trip over.
static void testFloatLiteralsTokenizeAndVmExecute() {
    lpcdriver::Value trailing = runProbe("return 1.5;\n");
    assert(std::holds_alternative<double>(trailing.data));
    assert(std::get<double>(trailing.data) == 1.5);

    lpcdriver::Value leading = runProbe("return .5;\n");
    assert(std::holds_alternative<double>(leading.data));
    assert(std::get<double>(leading.data) == 0.5);

    lpcdriver::Value zero = runProbe("return 0.0;\n");
    assert(std::holds_alternative<double>(zero.data));
    assert(std::get<double>(zero.data) == 0.0);

    // Mixed int/float arithmetic promotes to float, and a float local
    // variable round-trips through PushObjectVar/StoreObjectVar-style
    // local storage the same as any other Value.
    lpcdriver::Value mixed = runProbe(
        "float f;\n"
        "f = 1.5 + 2;\n"
        "return f;\n");
    assert(std::holds_alternative<double>(mixed.data));
    assert(std::get<double>(mixed.data) == 3.5);

    // The real code's own range-terminated shape ("arr[a..b]") must still
    // tokenize as the ".." range operator, not misfire as a float, now
    // that '.' can also start a number.
    lpcdriver::Value rangeStillWorks = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[1..3];\n");
    assert(std::holds_alternative<std::string>(rangeStillWorks.data));
    assert(std::get<std::string>(rangeStillWorks.data) == "ell");

    std::cout << "testFloatLiteralsTokenizeAndVmExecute OK\n";
}

// Found compiling the real secure/SimulEfun/SimulEfun.c (misc.c's own
// "({ "womble", ..., "hamster", })"): a trailing comma right before the
// closing "})"/"])" is real LPC's normal array/mapping literal style, but
// the comma-loop here always tried to parse one more element after any
// comma, and choked on the "}"/"]" itself with "expected expression".
static void testTrailingCommaInArrayAndMappingLiteralsParses() {
    lpcdriver::Value arr = runProbe(
        "mixed *items;\n"
        "items = ({ 1, 2, 3, });\n"
        "return sizeof(items);\n");
    assert(std::holds_alternative<int64_t>(arr.data));
    assert(std::get<int64_t>(arr.data) == 3);

    lpcdriver::Value map = runProbe(
        "mapping m;\n"
        "m = ([ \"a\": 1, \"b\": 2, ]);\n"
        "return m[\"b\"];\n");
    assert(std::holds_alternative<int64_t>(map.data));
    assert(std::get<int64_t>(map.data) == 2);

    std::cout << "testTrailingCommaInArrayAndMappingLiteralsParses OK\n";
}

static void testArithmeticOnNonNumericOperandThrows() {
    bool subThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return s - 1;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        subThrew = true;
    }
    assert(subThrew);

    bool mulThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return s * 1;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        mulThrew = true;
    }
    assert(mulThrew);

    bool divThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return s / 1;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        divThrew = true;
    }
    assert(divThrew);

    bool modThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return s % 1;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        modThrew = true;
    }
    assert(modThrew);

    bool negThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return -s;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        negThrew = true;
    }
    assert(negThrew);

    std::cout << "testArithmeticOnNonNumericOperandThrows OK\n";
}

static void testDivisionAndModuloByZeroThrow() {
    bool divThrew = false;
    try {
        runProbe("return 5 / 0;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        divThrew = true;
    }
    assert(divThrew);

    bool modThrew = false;
    try {
        runProbe("return 5 % 0;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        modThrew = true;
    }
    assert(modThrew);

    std::cout << "testDivisionAndModuloByZeroThrow OK\n";
}

static void testRealBlockingLineArithmetic() {
    // Reproduces the exact arithmetic from the real blocking line:
    //   write("("+(t/60)+"."+(t%60)+")\n");
    // with t = 125, matching 2 minutes and 5 seconds.
    lpcdriver::Value divResult = runProbe(
        "int t;\n"
        "t = 125;\n"
        "return t / 60;\n");
    assert(std::holds_alternative<int64_t>(divResult.data));
    assert(std::get<int64_t>(divResult.data) == 2);

    lpcdriver::Value modResult = runProbe(
        "int t;\n"
        "t = 125;\n"
        "return t % 60;\n");
    assert(std::holds_alternative<int64_t>(modResult.data));
    assert(std::get<int64_t>(modResult.data) == 5);

    std::cout << "testRealBlockingLineArithmetic OK\n";
}

static void testRangeDotDotTokenizes() {
    lpcdriver::Lexer lexer("..");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 2); // the ".." symbol, then End
    assert(tokens[0].type == lpcdriver::TokenType::Symbol);
    assert(tokens[0].text == "..");

    // The exact real shape, name[0..0], must tokenize as a single ".."
    // symbol between the two Number tokens, not two separate "." tokens
    // (which is not even a valid symbol on its own in this lexer).
    lpcdriver::Lexer shapeLexer("name[0..0]");
    auto shapeTokens = shapeLexer.tokenize();
    std::vector<std::pair<lpcdriver::TokenType, std::string>> expected = {
        {lpcdriver::TokenType::Ident, "name"},
        {lpcdriver::TokenType::Symbol, "["},
        {lpcdriver::TokenType::Number, "0"},
        {lpcdriver::TokenType::Symbol, ".."},
        {lpcdriver::TokenType::Number, "0"},
        {lpcdriver::TokenType::Symbol, "]"},
        {lpcdriver::TokenType::End, ""},
    };
    assert(shapeTokens.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(shapeTokens[i].type == expected[i].first);
        assert(shapeTokens[i].text == expected[i].second);
    }

    // Regression check: an ordinary single index, name[0], must be
    // completely unaffected, no ".." token appears anywhere.
    lpcdriver::Lexer singleLexer("name[0]");
    auto singleTokens = singleLexer.tokenize();
    for (const auto& t : singleTokens) {
        assert(t.text != "..");
    }

    std::cout << "testRangeDotDotTokenizes OK\n";
}

static void testRangeIndexParsesWithRangeEndSet() {
    std::string src =
        "void create() {\n"
        "    string name;\n"
        "    string shard;\n"
        "    shard = name[0..0];\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[2].get());
    assert(assign != nullptr);

    auto* idx = dynamic_cast<lpcdriver::IndexExpr*>(assign->value.get());
    assert(idx != nullptr);
    assert(idx->rangeEnd != nullptr);

    auto* startLit = dynamic_cast<lpcdriver::IntLiteral*>(idx->index.get());
    assert(startLit != nullptr);
    assert(startLit->value == 0);

    auto* endLit = dynamic_cast<lpcdriver::IntLiteral*>(idx->rangeEnd.get());
    assert(endLit != nullptr);
    assert(endLit->value == 0);

    std::cout << "testRangeIndexParsesWithRangeEndSet OK\n";
}

static void testSingleIndexStillParsesWithNullRangeEnd() {
    std::string src =
        "void create() {\n"
        "    mixed *items;\n"
        "    mixed one;\n"
        "    one = items[0];\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[2].get());
    assert(assign != nullptr);

    auto* idx = dynamic_cast<lpcdriver::IndexExpr*>(assign->value.get());
    assert(idx != nullptr);
    assert(idx->rangeEnd == nullptr);

    std::cout << "testSingleIndexStillParsesWithNullRangeEnd OK\n";
}

static void testRealShapeRangeIndexBindsToNameNotConcatenation() {
    // Same shape as the real sites in secure/daemon/master.c:
    //   DIR_USERS+"/"+name[0..0]
    // The range index must bind only to "name" (parsePostfix runs at
    // higher precedence than "+"), so the outer expression is still a
    // two-level Add chain, not something the range index disrupts.
    std::string src =
        "void create() {\n"
        "    string name;\n"
        "    string path;\n"
        "    path = \"/secure/save/users\"+\"/\"+name[0..0];\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[2].get());
    assert(assign != nullptr);

    auto* outerAdd = dynamic_cast<lpcdriver::BinaryExpr*>(assign->value.get());
    assert(outerAdd != nullptr);
    assert(outerAdd->op == lpcdriver::BinOp::Add);

    auto* rangeIdx = dynamic_cast<lpcdriver::IndexExpr*>(outerAdd->right.get());
    assert(rangeIdx != nullptr);
    assert(rangeIdx->rangeEnd != nullptr);

    auto* nameRef = dynamic_cast<lpcdriver::VarRefExpr*>(rangeIdx->target.get());
    assert(nameRef != nullptr);
    assert(nameRef->name == "name");

    std::cout << "testRealShapeRangeIndexBindsToNameNotConcatenation OK\n";
}

static void testCodegenEmitsRangeIndexOnlyWhenRangeEndPresent() {
    std::string src =
        "void create() {\n"
        "    string name;\n"
        "    string shard;\n"
        "    mixed *items;\n"
        "    mixed one;\n"
        "    shard = name[0..0];\n"
        "    one = items[0];\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawRangeIndex = false, sawPlainIndex = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::RangeIndex) sawRangeIndex = true;
        if (instr.op == lpcdriver::OpCode::Index) sawPlainIndex = true;
    }
    assert(sawRangeIndex);
    assert(sawPlainIndex);

    std::cout << "testCodegenEmitsRangeIndexOnlyWhenRangeEndPresent OK\n";
}

static void testStringRangeIndexExecutesWithCorrectBounds() {
    lpcdriver::Value firstChar = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[0..0];\n");
    assert(std::holds_alternative<std::string>(firstChar.data));
    assert(std::get<std::string>(firstChar.data) == "h");

    lpcdriver::Value middle = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[1..3];\n");
    assert(std::holds_alternative<std::string>(middle.data));
    assert(std::get<std::string>(middle.data) == "ell");

    lpcdriver::Value whole = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[0..4];\n");
    assert(std::holds_alternative<std::string>(whole.data));
    assert(std::get<std::string>(whole.data) == "hello");

    // End beyond the string's length is clamped, not an error.
    lpcdriver::Value clamped = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[0..99];\n");
    assert(std::holds_alternative<std::string>(clamped.data));
    assert(std::get<std::string>(clamped.data) == "hello");

    // An inverted range names no characters, and returns "" rather
    // than throwing.
    lpcdriver::Value inverted = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[3..1];\n");
    assert(std::holds_alternative<std::string>(inverted.data));
    assert(std::get<std::string>(inverted.data).empty());

    // An empty target at [0..0] also names no characters.
    lpcdriver::Value emptyTarget = runProbe(
        "string s;\n"
        "s = \"\";\n"
        "return s[0..0];\n");
    assert(std::holds_alternative<std::string>(emptyTarget.data));
    assert(std::get<std::string>(emptyTarget.data).empty());

    std::cout << "testStringRangeIndexExecutesWithCorrectBounds OK\n";
}

static void testStringRangeIndexNegativeStartThrows() {
    bool threw = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"hello\";\n"
            "return s[-1..2];\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testStringRangeIndexNegativeStartThrows OK\n";
}

static void testArrayRangeIndexExecutesWithCorrectBounds() {
    lpcdriver::Value middleSlice = runProbe(
        "mixed *items;\n"
        "mixed *sliced;\n"
        "items = ({ 1, 2, 3, 4 });\n"
        "sliced = items[1..2];\n"
        "return sizeof(sliced) == 2 && sliced[0] == 2 && sliced[1] == 3;\n");
    assert(std::holds_alternative<int64_t>(middleSlice.data));
    assert(std::get<int64_t>(middleSlice.data) == 1);

    // End beyond the array's length is clamped, not an error.
    lpcdriver::Value clampedSlice = runProbe(
        "mixed *items;\n"
        "mixed *sliced;\n"
        "items = ({ 1, 2, 3 });\n"
        "sliced = items[0..99];\n"
        "return sizeof(sliced) == 3 && sliced[0] == 1 && sliced[2] == 3;\n");
    assert(std::holds_alternative<int64_t>(clampedSlice.data));
    assert(std::get<int64_t>(clampedSlice.data) == 1);

    // An inverted range yields an empty array, not an error.
    lpcdriver::Value invertedSlice = runProbe(
        "mixed *items;\n"
        "mixed *sliced;\n"
        "items = ({ 1, 2, 3 });\n"
        "sliced = items[2..0];\n"
        "return sizeof(sliced) == 0;\n");
    assert(std::holds_alternative<int64_t>(invertedSlice.data));
    assert(std::get<int64_t>(invertedSlice.data) == 1);

    std::cout << "testArrayRangeIndexExecutesWithCorrectBounds OK\n";
}

static void testRealBlockingLineRangeIndex() {
    // Reproduces the exact real shape from all seven confirmed sites in
    // secure/daemon/master.c:
    //   DIR_USERS+"/"+name[0..0]
    // with a fixed name, matching the single-character-prefix sharding
    // idiom directly.
    lpcdriver::Value shard = runProbe(
        "string name;\n"
        "name = \"thurtea\";\n"
        "return name[0..0];\n");
    assert(std::holds_alternative<std::string>(shard.data));
    assert(std::get<std::string>(shard.data) == "t");

    std::cout << "testRealBlockingLineRangeIndex OK\n";
}

static void testTernaryQuestionMarkTokenizes() {
    lpcdriver::Lexer lexer("?");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 2); // the "?" symbol, then End
    assert(tokens[0].type == lpcdriver::TokenType::Symbol);
    assert(tokens[0].text == "?");

    std::cout << "testTernaryQuestionMarkTokenizes OK\n";
}

static void testTernaryRealShapeTokenizes() {
    // Real shape from secure/daemon/master.c raw line 433, minus its
    // surrounding parens: caught ? "catch" : "runtime". Confirms '?'
    // and the already-whitelisted ':' both come through correctly in
    // sequence.
    lpcdriver::Lexer lexer("caught ? \"catch\" : \"runtime\"");
    auto tokens = lexer.tokenize();
    std::vector<std::pair<lpcdriver::TokenType, std::string>> expected = {
        {lpcdriver::TokenType::Ident, "caught"},
        {lpcdriver::TokenType::Symbol, "?"},
        {lpcdriver::TokenType::String, "catch"},
        {lpcdriver::TokenType::Symbol, ":"},
        {lpcdriver::TokenType::String, "runtime"},
        {lpcdriver::TokenType::End, ""},
    };
    assert(tokens.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(tokens[i].type == expected[i].first);
        assert(tokens[i].text == expected[i].second);
    }

    std::cout << "testTernaryRealShapeTokenizes OK\n";
}

static void testTernaryParsesToTernaryExpr() {
    std::string src =
        "void create() {\n"
        "    mixed a;\n"
        "    mixed b;\n"
        "    mixed c;\n"
        "    mixed result;\n"
        "    result = a ? b : c;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[4].get());
    assert(assign != nullptr);

    auto* tern = dynamic_cast<lpcdriver::TernaryExpr*>(assign->value.get());
    assert(tern != nullptr);

    auto* cond = dynamic_cast<lpcdriver::VarRefExpr*>(tern->condition.get());
    assert(cond != nullptr);
    assert(cond->name == "a");

    auto* thenRef = dynamic_cast<lpcdriver::VarRefExpr*>(tern->thenBranch.get());
    assert(thenRef != nullptr);
    assert(thenRef->name == "b");

    auto* elseRef = dynamic_cast<lpcdriver::VarRefExpr*>(tern->elseBranch.get());
    assert(elseRef != nullptr);
    assert(elseRef->name == "c");

    std::cout << "testTernaryParsesToTernaryExpr OK\n";
}

static void testParenthesizedTernaryParsesUnchanged() {
    // Real shape from raw line 519:
    //   return (__PlayerName ? __PlayerName : "Mudlib");
    // Confirms the existing "(" grouping path in parsePrimary() returns
    // the inner TernaryExpr unchanged, not wrapped in anything extra.
    std::string src =
        "string create() {\n"
        "    mixed name;\n"
        "    return (name ? name : \"Mudlib\");\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ret = dynamic_cast<lpcdriver::ReturnStmt*>(body[1].get());
    assert(ret != nullptr);

    auto* tern = dynamic_cast<lpcdriver::TernaryExpr*>(ret->expr.get());
    assert(tern != nullptr);

    std::cout << "testParenthesizedTernaryParsesUnchanged OK\n";
}

static void testTernaryRightAssociativity() {
    std::string src =
        "void create() {\n"
        "    mixed a;\n"
        "    mixed b;\n"
        "    mixed c;\n"
        "    mixed d;\n"
        "    mixed e;\n"
        "    mixed result;\n"
        "    result = a ? b : c ? d : e;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[6].get());
    assert(assign != nullptr);

    auto* outer = dynamic_cast<lpcdriver::TernaryExpr*>(assign->value.get());
    assert(outer != nullptr);

    auto* outerCond = dynamic_cast<lpcdriver::VarRefExpr*>(outer->condition.get());
    assert(outerCond != nullptr);
    assert(outerCond->name == "a");

    auto* outerThen = dynamic_cast<lpcdriver::VarRefExpr*>(outer->thenBranch.get());
    assert(outerThen != nullptr);
    assert(outerThen->name == "b");

    // a ? b : (c ? d : e), not a left-associative (incorrect) grouping.
    auto* inner = dynamic_cast<lpcdriver::TernaryExpr*>(outer->elseBranch.get());
    assert(inner != nullptr);

    auto* innerCond = dynamic_cast<lpcdriver::VarRefExpr*>(inner->condition.get());
    assert(innerCond != nullptr);
    assert(innerCond->name == "c");

    std::cout << "testTernaryRightAssociativity OK\n";
}

static void testTernaryThenBranchNesting() {
    std::string src =
        "void create() {\n"
        "    mixed a;\n"
        "    mixed b;\n"
        "    mixed c;\n"
        "    mixed d;\n"
        "    mixed e;\n"
        "    mixed result;\n"
        "    result = a ? (b ? c : d) : e;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<lpcdriver::AssignStmt*>(body[6].get());
    assert(assign != nullptr);

    auto* outer = dynamic_cast<lpcdriver::TernaryExpr*>(assign->value.get());
    assert(outer != nullptr);

    auto* inner = dynamic_cast<lpcdriver::TernaryExpr*>(outer->thenBranch.get());
    assert(inner != nullptr);

    auto* innerCond = dynamic_cast<lpcdriver::VarRefExpr*>(inner->condition.get());
    assert(innerCond != nullptr);
    assert(innerCond->name == "b");

    std::cout << "testTernaryThenBranchNesting OK\n";
}

static void testCodegenEmitsJumpOpcodesForTernary() {
    std::string src =
        "void create() {\n"
        "    int a;\n"
        "    int b;\n"
        "    int c;\n"
        "    int result;\n"
        "    result = a ? b : c;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    int jumpIfFalseIdx = -1, jumpIdx = -1;
    for (size_t i = 0; i < compiled.code.size(); ++i) {
        if (compiled.code[i].op == lpcdriver::OpCode::JumpIfFalse) {
            assert(jumpIfFalseIdx == -1); // exactly one
            jumpIfFalseIdx = static_cast<int>(i);
        }
        if (compiled.code[i].op == lpcdriver::OpCode::Jump) {
            assert(jumpIdx == -1); // exactly one
            jumpIdx = static_cast<int>(i);
        }
    }
    assert(jumpIfFalseIdx != -1);
    assert(jumpIdx != -1);

    // JumpIfFalse's target must land right after the unconditional Jump
    // that ends the then-branch, i.e. at the start of the else-branch's
    // code, matching emitIfStmt()'s own jump-target convention.
    assert(compiled.code[jumpIfFalseIdx].operand == jumpIdx + 1);

    std::cout << "testCodegenEmitsJumpOpcodesForTernary OK\n";
}

static void testTernaryVmExecutesCorrectBranch() {
    lpcdriver::Value truthy = runProbe("return 1 ? 10 : 20;\n");
    assert(std::holds_alternative<int64_t>(truthy.data));
    assert(std::get<int64_t>(truthy.data) == 10);

    lpcdriver::Value falsy = runProbe("return 0 ? 10 : 20;\n");
    assert(std::holds_alternative<int64_t>(falsy.data));
    assert(std::get<int64_t>(falsy.data) == 20);

    lpcdriver::Value fromComparison = runProbe("return (5 == 5) ? \"yes\" : \"no\";\n");
    assert(std::holds_alternative<std::string>(fromComparison.data));
    assert(std::get<std::string>(fromComparison.data) == "yes");

    std::cout << "testTernaryVmExecutesCorrectBranch OK\n";
}

static void testTernaryOnlyEvaluatesTakenBranch() {
    // If the ternary evaluated both branches unconditionally, calling
    // this nonexistent efun would throw "undefined efun" and fail the
    // test. It must never run, because the condition (1) already
    // selects the then-branch. Same reasoning as the logical-operators
    // slice's own short-circuit tests above.
    lpcdriver::Value result = runProbe("return 1 ? 1 : nonexistent_marker_efun();\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testTernaryOnlyEvaluatesTakenBranch OK\n";
}

static void testRealBlockingLineTernary() {
    // Reproduces the real blocking shape from secure/daemon/master.c raw
    // line 469:
    //   string objfn = obj ? file_name(obj) : "<none>";
    // file_name() is not an implemented efun in this driver (a separate,
    // unrelated gap; confirmed absent from EfunTable.cpp), so this probe
    // substitutes a plain string literal for the then-branch and instead
    // exercises what this slice actually adds: a ternary whose condition
    // is an object-typed local. An unset/null object is represented the
    // same way this VM already represents "no object" for an "object"-
    // typed slot -- int 0 stored directly, no distinct null sentinel --
    // and isTruthy() must treat that as false, taking the else-branch.
    lpcdriver::Value nullCase = runProbe(
        "object obj;\n"
        "string objfn;\n"
        "obj = 0;\n"
        "objfn = obj ? \"has object\" : \"<none>\";\n"
        "return objfn;\n");
    assert(std::holds_alternative<std::string>(nullCase.data));
    assert(std::get<std::string>(nullCase.data) == "<none>");

    std::cout << "testRealBlockingLineTernary OK\n";
}

// ---------------------------------------------------------------------
// Object variables (top-level per-object variable declarations).
// ---------------------------------------------------------------------

static void testObjectVarDeclParsesSingleDeclaration() {
    std::string src = "object __Unguarded;\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.empty());
    assert(program->objectVars.size() == 1);
    assert(program->objectVars[0]->type == "object");
    assert(program->objectVars[0]->name == "__Unguarded");
    assert(program->objectVars[0]->isArray == false);

    std::cout << "testObjectVarDeclParsesSingleDeclaration OK\n";
}

static void testObjectVarDeclParsesCommaSeparatedNames() {
    // Real shape from secure/daemon/master.c raw line 21.
    std::string src = "mapping __Groups, __ReadAccess, __WriteAccess;\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.size() == 3);
    assert(program->objectVars[0]->type == "mapping");
    assert(program->objectVars[0]->name == "__Groups");
    assert(program->objectVars[1]->type == "mapping");
    assert(program->objectVars[1]->name == "__ReadAccess");
    assert(program->objectVars[2]->type == "mapping");
    assert(program->objectVars[2]->name == "__WriteAccess");

    std::cout << "testObjectVarDeclParsesCommaSeparatedNames OK\n";
}

static void testObjectVarDeclThenFunctionBothParse() {
    std::string src =
        "object foo;\n"
        "void create() {\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.size() == 1);
    assert(program->objectVars[0]->name == "foo");
    assert(program->functions.size() == 1);
    assert(program->functions[0]->name == "create");

    std::cout << "testObjectVarDeclThenFunctionBothParse OK\n";
}

static void testFunctionDeclarationStillParsesAfterPrefixRefactor() {
    // Regression check for the parseDeclPrefix()/parseFunctionRest()
    // split: a real prototype shape from master.h must parse exactly as
    // it did before the refactor.
    std::string src = "static private void load_access(string cfg, mapping ref);\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.empty());
    assert(program->functions.size() == 1);
    auto& fn = *program->functions[0];
    assert(fn.name == "load_access");
    assert(fn.returnType == "void");
    assert(fn.body == nullptr); // prototype only
    assert(fn.params.size() == 2);
    assert(fn.params[0].type == "string");
    assert(fn.params[0].name == "cfg");
    assert(fn.params[1].type == "mapping");
    assert(fn.params[1].name == "ref");

    std::cout << "testFunctionDeclarationStillParsesAfterPrefixRefactor OK\n";
}

static void testObjectVarDeclArrayStarParses() {
    std::string src = "mixed *items;\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.size() == 1);
    assert(program->objectVars[0]->isArray == true);

    std::cout << "testObjectVarDeclArrayStarParses OK\n";
}

static void testObjectVarDeclParsesInitializerExpression() {
    // Declaration-time initializers are real, standard LPC (confirmed
    // against the FluffOS reference driver's grammar) -- surfaced live
    // needed by secure/daemon/wiztools.c's own "string *REISSUED_TOOLS =
    // ({ ... });" (see Ast.hpp's ObjectVarDecl comment and
    // CodeGen::generate()'s own "$objvarinit" synthesis).
    std::string src = "int x = 5;\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.size() == 1);
    assert(program->objectVars[0]->name == "x");
    auto* init = dynamic_cast<lpcdriver::IntLiteral*>(program->objectVars[0]->initializer.get());
    assert(init != nullptr && init->value == 5);

    std::cout << "testObjectVarDeclParsesInitializerExpression OK\n";
}

static void testObjectVarDeclParsesInitializerInCommaList() {
    // Same shape, but for the second-or-later name in a comma-separated
    // list, not just the first: "a" has no initializer, "b" does.
    std::string src = "int a, b = 5;\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.size() == 2);
    assert(program->objectVars[0]->name == "a");
    assert(program->objectVars[0]->initializer == nullptr);
    assert(program->objectVars[1]->name == "b");
    auto* init = dynamic_cast<lpcdriver::IntLiteral*>(program->objectVars[1]->initializer.get());
    assert(init != nullptr && init->value == 5);

    std::cout << "testObjectVarDeclParsesInitializerInCommaList OK\n";
}

static void testRealBlockingLinesObjectVarDecl() {
    // Reproduces raw lines 18-21 from secure/daemon/master.c verbatim:
    // the exact blocking shape from the eleventh boot attempt.
    std::string src =
        "static private object __Unguarded;\n"
        "static private string __PlayerName;\n"
        "static private object __NewPlayer;\n"
        "static private mapping __Groups, __ReadAccess, __WriteAccess;\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.empty());
    assert(program->objectVars.size() == 6);
    std::vector<std::string> expectedNames = {
        "__Unguarded", "__PlayerName", "__NewPlayer",
        "__Groups", "__ReadAccess", "__WriteAccess"
    };
    for (size_t i = 0; i < expectedNames.size(); ++i) {
        assert(program->objectVars[i]->name == expectedNames[i]);
    }

    std::cout << "testRealBlockingLinesObjectVarDecl OK\n";
}

static void testCodegenEmitsPushObjectVarForRead() {
    std::string src =
        "object ob;\n"
        "mixed get() {\n"
        "    return ob;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    assert(compiled.objectVarNames.size() == 1);
    assert(compiled.objectVarNames[0] == "ob");

    bool sawPushObjectVar = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::PushObjectVar) {
            assert(instr.operand == 0);
            sawPushObjectVar = true;
        }
    }
    assert(sawPushObjectVar);

    std::cout << "testCodegenEmitsPushObjectVarForRead OK\n";
}

static void testCodegenEmitsStoreObjectVarForWrite() {
    std::string src =
        "int x;\n"
        "void set(int v) {\n"
        "    x = v;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawStoreObjectVar = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::StoreObjectVar) {
            assert(instr.operand == 0);
            sawStoreObjectVar = true;
        }
    }
    assert(sawStoreObjectVar);

    std::cout << "testCodegenEmitsStoreObjectVarForWrite OK\n";
}

static void testCodegenLocalShadowsObjectVariableOfSameName() {
    std::string src =
        "int x;\n"
        "void probe() {\n"
        "    int x;\n"
        "    x = 1;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    // The inner "x" is a local declared inside probe(), which must
    // shadow the object variable "x" of the same name: assigning to it
    // must emit StoreLocal, never StoreObjectVar, matching real LPC's
    // local-wins-over-global precedence (confirmed against the FluffOS
    // reference driver's compiler.c/grammar.y).
    bool sawStoreLocal = false, sawStoreObjectVar = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == lpcdriver::OpCode::StoreLocal) sawStoreLocal = true;
        if (instr.op == lpcdriver::OpCode::StoreObjectVar) sawStoreObjectVar = true;
    }
    assert(sawStoreLocal);
    assert(!sawStoreObjectVar);

    std::cout << "testCodegenLocalShadowsObjectVariableOfSameName OK\n";
}

static void testCodegenDuplicateObjectVariableThrows() {
    std::string src =
        "int x;\n"
        "int x;\n"
        "void create() {\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;

    bool threw = false;
    try {
        codegen.generate(*program);
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testCodegenDuplicateObjectVariableThrows OK\n";
}

static void testCodegenUndeclaredVariableStillThrows() {
    // Neither a local/parameter nor an object variable: resolveVariable()
    // must still fall through to a hard error, the same as before this
    // slice added the second (object-variable) lookup tier.
    std::string src =
        "void probe() {\n"
        "    write(nowhere);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;

    bool threw = false;
    try {
        codegen.generate(*program);
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testCodegenUndeclaredVariableStillThrows OK\n";
}

// Helper for the object-variable VM-level tests below that need more
// than a single wrapped probe() function: compiles a whole program (its
// own top-level object variable declarations plus one or more named
// functions) directly in memory, the same way runProbe() compiles its
// single wrapped function, bypassing ObjectManager and disk entirely.
// Returns the constructed LpcObject so a test can call multiple named
// functions against it in sequence and confirm object-variable state
// persists across separate VM::callFunction() calls, not just within
// one, and so a test can construct more than one LpcObject from the
// same compiled program to confirm storage is per-instance.
static std::shared_ptr<lpcdriver::LpcObject> compileProgramObject(const std::string& src) {
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = std::make_shared<lpcdriver::CompiledProgram>(codegen.generate(*program));
    return std::make_shared<lpcdriver::LpcObject>("program_object", compiled);
}

static void testObjectVariablePersistsAcrossSeparateCalls() {
    std::string src =
        "int counter;\n"
        "\n"
        "void write_counter(int v) {\n"
        "    counter = v;\n"
        "}\n"
        "\n"
        "int read_counter() {\n"
        "    return counter;\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    // Before any write, the slot holds real LPC's own default: the
    // integer 0 (see LpcObject.cpp's own comment -- not this driver's
    // separate monostate "no value" sentinel, which real LPC has no
    // equivalent of at the ordinary-declared-variable level).
    lpcdriver::Value before = vm.callFunction(obj, "read_counter", {});
    assert(std::holds_alternative<int64_t>(before.data));
    assert(std::get<int64_t>(before.data) == 0);

    vm.callFunction(obj, "write_counter",
                     std::vector<lpcdriver::Value>{lpcdriver::Value(static_cast<int64_t>(7))});

    // A separate, later call must observe the earlier call's write:
    // object-variable state lives on the LpcObject instance, not in any
    // one call's locals.
    lpcdriver::Value after = vm.callFunction(obj, "read_counter", {});
    assert(std::holds_alternative<int64_t>(after.data));
    assert(std::get<int64_t>(after.data) == 7);

    std::cout << "testObjectVariablePersistsAcrossSeparateCalls OK\n";
}

static void testObjectVariableIsPerInstanceNotSharedAcrossObjects() {
    std::string src =
        "int counter;\n"
        "\n"
        "void write_counter(int v) {\n"
        "    counter = v;\n"
        "}\n"
        "\n"
        "int read_counter() {\n"
        "    return counter;\n"
        "}\n";
    auto objA = compileProgramObject(src);
    auto objB = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    vm.callFunction(objA, "write_counter",
                     std::vector<lpcdriver::Value>{lpcdriver::Value(static_cast<int64_t>(99))});

    lpcdriver::Value aResult = vm.callFunction(objA, "read_counter", {});
    assert(std::holds_alternative<int64_t>(aResult.data));
    assert(std::get<int64_t>(aResult.data) == 99);

    // objB is a separate LpcObject instance (even though compiled from
    // the same source), so its own storage must be untouched -- still at
    // real LPC's own default (0), not objA's write.
    lpcdriver::Value bResult = vm.callFunction(objB, "read_counter", {});
    assert(std::holds_alternative<int64_t>(bResult.data));
    assert(std::get<int64_t>(bResult.data) == 0);

    std::cout << "testObjectVariableIsPerInstanceNotSharedAcrossObjects OK\n";
}

static void testObjectVariableShadowedByLocalAtRuntime() {
    std::string src =
        "int x;\n"
        "\n"
        "void set_object_x(int v) {\n"
        "    x = v;\n"
        "}\n"
        "\n"
        "int shadow_probe() {\n"
        "    int x;\n"
        "    x = 999;\n"
        "    return x;\n"
        "}\n"
        "\n"
        "int read_object_x() {\n"
        "    return x;\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    vm.callFunction(obj, "set_object_x",
                     std::vector<lpcdriver::Value>{lpcdriver::Value(static_cast<int64_t>(1))});

    // shadow_probe()'s own local "x" must shadow the object variable: it
    // returns the local's value, not the object variable's.
    lpcdriver::Value shadowed = vm.callFunction(obj, "shadow_probe", {});
    assert(std::holds_alternative<int64_t>(shadowed.data));
    assert(std::get<int64_t>(shadowed.data) == 999);

    // The object variable itself must be untouched by shadow_probe()'s
    // local, confirmed by a separate function with no local named "x".
    lpcdriver::Value objectX = vm.callFunction(obj, "read_object_x", {});
    assert(std::holds_alternative<int64_t>(objectX.data));
    assert(std::get<int64_t>(objectX.data) == 1);

    std::cout << "testObjectVariableShadowedByLocalAtRuntime OK\n";
}

static void testRealShapeMappingObjectVariableReadWrite() {
    // Reproduces raw line 21's real shape (a mapping object variable,
    // e.g. __Groups) at the VM level: confirms MakeMapping/Index compose
    // correctly with PushObjectVar/StoreObjectVar, with no special-casing
    // needed since both already operate purely on stack values regardless
    // of where those values came from.
    std::string src =
        "mapping groups;\n"
        "\n"
        "void set_groups() {\n"
        "    groups = ([ \"a\": 1 ]);\n"
        "}\n"
        "\n"
        "int get_a() {\n"
        "    return groups[\"a\"];\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    vm.callFunction(obj, "set_groups", {});
    lpcdriver::Value result = vm.callFunction(obj, "get_a", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testRealShapeMappingObjectVariableReadWrite OK\n";
}

// Helper for the reentrancy test below. Unlike compileProgramObject()
// above (which compiles a single program entirely in memory, bypassing
// ObjectManager and disk entirely), this test needs a real ObjectManager
// wired to a real VM so that clone_object() actually works: it has to
// preprocess and compile a second real file from disk and invoke that
// file's create() through the same VM, producing a genuinely nested
// VM::run() call while the outer run() call is still active on the C++
// call stack. Fixture files are written into a fresh temp directory per
// call so this test does not depend on or interfere with the checked-in
// mudlib_stub fixtures.
struct ObjectVarHarness {
    std::string tempDir;
    lpcdriver::Config config;
    lpcdriver::ObjectManager objects;
    lpcdriver::VM vm;

    ObjectVarHarness() : objects(config), vm(objects, config) {
        objects.setVM(&vm);

        char dirTemplate[] = "/tmp/lpcdriver_objvar_test_XXXXXX";
        char* created = mkdtemp(dirTemplate);
        assert(created != nullptr);
        tempDir = created;

        std::string cfgPath = tempDir + "/driver.cfg";
        std::ofstream cfg(cfgPath);
        cfg << "mudlib_root: " << tempDir << "\n";
        cfg << "master_file: /unused\n";
        cfg << "include_dir: " << tempDir << "\n";
        cfg << "port: 0\n";
        // Harmless when unused: only takes effect if a test explicitly
        // writes "/simul_efun.c" and calls objects.loadSimulEfunObject().
        cfg << "simul_efun_file: /simul_efun\n";
        cfg.close();

        bool loaded = config.loadFromFile(cfgPath);
        assert(loaded);
    }

    void writeFile(const std::string& relPath, const std::string& contents) {
        std::ofstream f(tempDir + relPath);
        f << contents;
    }
};

static void testObjectVariableReentrancySafeAcrossNestedCloneObject() {
    ObjectVarHarness harness;

    // The outer object: writes its own object variable, then calls
    // clone_object() on a second, unrelated object mid-function, which
    // recursively runs that object's own create() (its own object
    // variable write) through the same VM while the outer call is still
    // on the C++ call stack.
    harness.writeFile("/outer.c",
        "int counter;\n"
        "\n"
        "void create() {\n"
        "    counter = 0;\n"
        "}\n"
        "\n"
        "int probe() {\n"
        "    object inner;\n"
        "    int innerVal;\n"
        "    counter = 41;\n"
        "    inner = clone_object(\"/inner\");\n"
        "    innerVal = call_other(inner, \"get_marker\");\n"
        "    return counter == 41 && innerVal == 99;\n"
        "}\n");

    harness.writeFile("/inner.c",
        "int inner_marker;\n"
        "\n"
        "void create() {\n"
        "    inner_marker = 99;\n"
        "}\n"
        "\n"
        "int get_marker() {\n"
        "    return inner_marker;\n"
        "}\n");

    auto outer = harness.objects.cloneObject("/outer");
    assert(outer != nullptr);

    // probe() writes counter, then triggers the nested run() call via
    // clone_object("/inner"), then reads counter back, all within the
    // same function activation, still on the C++ call stack throughout.
    // If the nested call corrupted or lost track of which object's
    // variables the outer frame was operating on (e.g. a stale reference,
    // or a single VM member overwritten instead of a properly
    // stack-scoped parameter), either counter would no longer read 41,
    // or inner_marker would not have been correctly written to 99 against
    // the inner object's own separate storage, or both.
    lpcdriver::Value result = harness.vm.callFunction(outer, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testObjectVariableReentrancySafeAcrossNestedCloneObject OK\n";
}

// --- Prerequisite: local var decl comma lists ---------------------------
// Discovered as the actual first parse failure when re-attempting the real
// master.c boot for this slice (secure/daemon/master.c line 58:
// "string file, fl, ac;", inside a function body) -- object variables
// already supported this shape, plain locals did not.

static void testLocalVarDeclCommaSeparatedNamesParse() {
    std::string src =
        "void probe() {\n"
        "    string file, fl, ac;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 1);
    auto* block = dynamic_cast<lpcdriver::Block*>(body[0].get());
    assert(block != nullptr);
    assert(block->statements.size() == 3);

    std::vector<std::string> names;
    for (auto& stmt : block->statements) {
        auto* decl = dynamic_cast<lpcdriver::VarDeclStmt*>(stmt.get());
        assert(decl != nullptr);
        names.push_back(decl->name);
    }
    assert(names[0] == "file");
    assert(names[1] == "fl");
    assert(names[2] == "ac");

    std::cout << "testLocalVarDeclCommaSeparatedNamesParse OK\n";
}

static void testLocalVarDeclCommaListVmExecution() {
    lpcdriver::Value result = runProbe(
        "string a, b, c;\n"
        "a = \"x\"; b = \"y\"; c = \"z\";\n"
        "return a + b + c;\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "xyz");

    std::cout << "testLocalVarDeclCommaListVmExecution OK\n";
}

// Real block scoping: two sibling "{ ... }" blocks (neither nested in the
// other) in the same function may each declare a same-named local without
// colliding. Confirmed live needed compiling domains/Praxis/setter.c's own
// "Store PPE"/"Store ISP" blocks, each with their own "int ppe, ppe_lvl,
// me;"/"int isp, isp_lvl, me;" -- CodeGen previously used one flat
// per-function locals_ map with no block-scope tracking at all, so the
// second block's "me" threw "codegen: variable \"me\" already declared in
// this scope" against the first block's own "me", which should already be
// out of scope by then. Also confirms the *other* direction still throws
// correctly: a name declared in one sibling block is genuinely gone once
// that block ends, not visible to code after it (real LPC/C block scoping,
// not the old flat/leaky behavior).
static void testSiblingBlocksMayReuseALocalNameNeitherNestedInTheOther() {
    lpcdriver::Value result = runProbe(
        "int total;\n"
        "{\n"
        "    int me;\n"
        "    me = 10;\n"
        "    total = me;\n"
        "}\n"
        "{\n"
        "    int me;\n"
        "    me = 32;\n"
        "    total = total + me;\n"
        "}\n"
        "return total;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testSiblingBlocksMayReuseALocalNameNeitherNestedInTheOther OK\n";
}

static void testNameDeclaredInABlockIsUndeclaredOnceThatBlockEnds() {
    bool threw = false;
    try {
        runProbe(
            "{\n"
            "    int onlyHere;\n"
            "    onlyHere = 1;\n"
            "}\n"
            "return onlyHere;\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testNameDeclaredInABlockIsUndeclaredOnceThatBlockEnds OK\n";
}

// --- for loops ------------------------------------------------------------

static void testForLoopParsesToForStmt() {
    std::string src =
        "void probe() {\n"
        "    for (int i = 0; i < 5; i = i + 1) {\n"
        "        write(\"x\");\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* forStmt = dynamic_cast<lpcdriver::ForStmt*>(body[0].get());
    assert(forStmt != nullptr);
    assert(dynamic_cast<lpcdriver::VarDeclStmt*>(forStmt->init.get()) != nullptr);
    assert(forStmt->condition != nullptr);
    assert(dynamic_cast<lpcdriver::AssignExpr*>(forStmt->update.get()) != nullptr);
    assert(forStmt->body->statements.size() == 1);

    std::cout << "testForLoopParsesToForStmt OK\n";
}

static void testForLoopEmptyClausesParse() {
    std::string src =
        "void probe() {\n"
        "    for (;;) {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* forStmt = dynamic_cast<lpcdriver::ForStmt*>(body[0].get());
    assert(forStmt != nullptr);
    assert(forStmt->init == nullptr);
    assert(forStmt->condition == nullptr);
    assert(forStmt->update == nullptr);

    std::cout << "testForLoopEmptyClausesParse OK\n";
}

static void testForLoopWithAssignInitVmSumsExpectedTotal() {
    lpcdriver::Value result = runProbe(
        "int sum;\n"
        "int i;\n"
        "sum = 0;\n"
        "for (i = 0; i < 5; i = i + 1) {\n"
        "    sum = sum + i;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 10);

    std::cout << "testForLoopWithAssignInitVmSumsExpectedTotal OK\n";
}

static void testForLoopWithDeclInitAndIncDecUpdateVmExecution() {
    // Exercises the two shapes real for-loops in this mudlib actually use:
    // a declaration-with-initializer init clause, and "i++" (rather than
    // "i = i + 1") in the update clause.
    lpcdriver::Value result = runProbe(
        "int sum;\n"
        "sum = 0;\n"
        "for (int i = 0; i < 5; i++) {\n"
        "    sum = sum + i;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 10);

    std::cout << "testForLoopWithDeclInitAndIncDecUpdateVmExecution OK\n";
}

// --- ++/-- ------------------------------------------------------------

static void testIncDecOperatorsTokenize() {
    lpcdriver::Lexer lexer("i++ ++i i-- --i");
    auto tokens = lexer.tokenize();
    int plusPlusCount = 0, minusMinusCount = 0;
    for (auto& t : tokens) {
        if (t.type == lpcdriver::TokenType::Symbol && t.text == "++") ++plusPlusCount;
        if (t.type == lpcdriver::TokenType::Symbol && t.text == "--") ++minusMinusCount;
    }
    assert(plusPlusCount == 2);
    assert(minusMinusCount == 2);

    std::cout << "testIncDecOperatorsTokenize OK\n";
}

static void testPrefixAndPostfixIncDecParseToIncDecExpr() {
    std::string src =
        "void probe() {\n"
        "    ++a;\n"
        "    b--;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* prefixStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    auto* prefixExpr = dynamic_cast<lpcdriver::IncDecExpr*>(prefixStmt->expr.get());
    assert(prefixExpr != nullptr);
    assert(prefixExpr->prefix == true);
    assert(prefixExpr->op == lpcdriver::IncDecOp::Inc);
    assert(prefixExpr->name == "a");

    auto* postfixStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[1].get());
    auto* postfixExpr = dynamic_cast<lpcdriver::IncDecExpr*>(postfixStmt->expr.get());
    assert(postfixExpr != nullptr);
    assert(postfixExpr->prefix == false);
    assert(postfixExpr->op == lpcdriver::IncDecOp::Dec);
    assert(postfixExpr->name == "b");

    std::cout << "testPrefixAndPostfixIncDecParseToIncDecExpr OK\n";
}

// "arr[0]++" is a real, supported indexed target (see Ast.hpp's
// IncDecExpr comment; confirmed live against std/living.c's own
// "healing[\"intox\"]--"), so this no longer belongs in a "throws" test
// -- see testPostfixIncDecOnIndexedTargetParsesToIndexedIncDecExpr and
// testIndexedPostfixIncDecVmExecutionReturnsOldValueAndMutates below for
// its own coverage. A range-index target ("arr[0..1]++") still is not a
// real lvalue in LPC (matches real grammar.y's own restricted "lvalue"
// nonterminal), so that is what still throws here.
static void testPostfixIncDecOnRangeIndexTargetThrows() {
    std::string src =
        "void probe() {\n"
        "    arr[0..1]++;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());

    bool threw = false;
    try {
        parser.parseProgram();
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testPostfixIncDecOnRangeIndexTargetThrows OK\n";
}

static void testPostfixIncDecOnIndexedTargetParsesToIndexedIncDecExpr() {
    std::string src =
        "void probe() {\n"
        "    healing[\"intox\"]--;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* stmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(stmt != nullptr);
    auto* incDec = dynamic_cast<lpcdriver::IncDecExpr*>(stmt->expr.get());
    assert(incDec != nullptr);
    assert(incDec->prefix == false);
    assert(incDec->op == lpcdriver::IncDecOp::Dec);
    assert(incDec->name.empty());
    assert(incDec->indexTarget != nullptr);
    auto* target = dynamic_cast<lpcdriver::VarRefExpr*>(incDec->indexTarget.get());
    assert(target != nullptr && target->name == "healing");
    auto* key = dynamic_cast<lpcdriver::StringLiteral*>(incDec->indexKey.get());
    assert(key != nullptr && key->value == "intox");

    std::cout << "testPostfixIncDecOnIndexedTargetParsesToIndexedIncDecExpr OK\n";
}

static void testIndexedPostfixIncDecVmExecutionReturnsOldValueAndMutates() {
    // Mirrors testPostfixIncrementVmExecutionReturnsOldValueAndMutates
    // below, but on a mapping-indexed target -- std/living.c's own real
    // shape ("healing[\"intox\"]--"), confirmed against the reference
    // driver's grammar.y restricted "lvalue" nonterminal covering both
    // a bare variable and an indexed target.
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "m = ([]);\n"
        "m[\"x\"] = 5;\n"
        "int y;\n"
        "y = m[\"x\"]--;\n"
        "return y * 100 + m[\"x\"];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 504); // y == 5 (old value), m["x"] == 4

    std::cout << "testIndexedPostfixIncDecVmExecutionReturnsOldValueAndMutates OK\n";
}

static void testIndexedPrefixIncDecVmExecutionReturnsNewValueAndMutates() {
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "m = ([]);\n"
        "m[\"x\"] = 5;\n"
        "int y;\n"
        "y = ++m[\"x\"];\n"
        "return y * 100 + m[\"x\"];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 606); // y == 6 (new value), m["x"] == 6

    std::cout << "testIndexedPrefixIncDecVmExecutionReturnsNewValueAndMutates OK\n";
}

static void testPrefixIncrementVmExecutionReturnsNewValueAndMutates() {
    lpcdriver::Value result = runProbe(
        "int x;\n"
        "x = 5;\n"
        "int y;\n"
        "y = ++x;\n"
        "return y * 100 + x;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 606); // y == 6, x == 6

    std::cout << "testPrefixIncrementVmExecutionReturnsNewValueAndMutates OK\n";
}

static void testPostfixIncrementVmExecutionReturnsOldValueAndMutates() {
    lpcdriver::Value result = runProbe(
        "int x;\n"
        "x = 5;\n"
        "int y;\n"
        "y = x++;\n"
        "return y * 100 + x;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 506); // y == 5, x == 6

    std::cout << "testPostfixIncrementVmExecutionReturnsOldValueAndMutates OK\n";
}

// --- same-object bare calls ---------------------------------------------

static void testBareCallToLocalFunctionEmitsCallOpcode() {
    std::string src =
        "int helper() { return 1; }\n"
        "int probe() { return helper(); }\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawCall = false;
    for (auto& instr : compiled.code) {
        // A plain bare call must never fall back to the old
        // always-CallEfun routing -- resolution now happens at run time.
        assert(instr.op != lpcdriver::OpCode::CallEfun);
        if (instr.op == lpcdriver::OpCode::Call) {
            const std::string& name = compiled.stringPool[instr.operand];
            if (name == "helper") sawCall = true;
        }
    }
    assert(sawCall);

    std::cout << "testBareCallToLocalFunctionEmitsCallOpcode OK\n";
}

static void testSameObjectBareCallInvokesLocalFunctionAtRuntime() {
    std::string src =
        "int helper() {\n"
        "    return 42;\n"
        "}\n"
        "int caller() {\n"
        "    return helper();\n"
        "}\n";
    auto obj = compileProgramObject(src);
    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "caller", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testSameObjectBareCallInvokesLocalFunctionAtRuntime OK\n";
}

static void testBareCallFallsBackToEfunWhenNoLocalFunctionMatches() {
    lpcdriver::Value result = runProbe(
        "mixed *arr;\n"
        "arr = ({ 1, 2, 3 });\n"
        "return sizeof(arr);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 3);

    std::cout << "testBareCallFallsBackToEfunWhenNoLocalFunctionMatches OK\n";
}

static void testUndefinedBareCallThrowsClearError() {
    bool threw = false;
    try {
        runProbe("return totally_undefined_name();\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testUndefinedBareCallThrowsClearError OK\n";
}

// --- sscanf ---------------------------------------------------------------

static void testSscanfParsesToSscanfExprWithVarNames() {
    std::string src =
        "void probe() {\n"
        "    string a, b;\n"
        "    sscanf(\"foo bar\", \"%s %s\", a, b);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    // body[0] is the "string a, b;" comma-decl Block; the sscanf() call
    // itself is body[1].
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[1].get());
    assert(exprStmt != nullptr);
    auto* sscanfExpr = dynamic_cast<lpcdriver::SscanfExpr*>(exprStmt->expr.get());
    assert(sscanfExpr != nullptr);
    assert(sscanfExpr->varNames.size() == 2);
    assert(sscanfExpr->varNames[0] == "a");
    assert(sscanfExpr->varNames[1] == "b");

    std::cout << "testSscanfParsesToSscanfExprWithVarNames OK\n";
}

static void testSscanfNonIdentifierOutputArgThrows() {
    std::string src =
        "void probe() {\n"
        "    sscanf(\"foo\", \"%s\", 5);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());

    bool threw = false;
    try {
        parser.parseProgram();
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testSscanfNonIdentifierOutputArgThrows OK\n";
}

static void testSscanfVmMatchesLiteralDelimitedTokens() {
    // Mirrors secure/daemon/master.c's own
    // "sscanf(lines[i], \"(%s) %s\", fl, ac)" shape.
    lpcdriver::Value result = runProbe(
        "string fl, ac;\n"
        "int n;\n"
        "n = sscanf(\"(foo) bar\", \"(%s) %s\", fl, ac);\n"
        "return n == 2 && fl == \"foo\" && ac == \"bar\";\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testSscanfVmMatchesLiteralDelimitedTokens OK\n";
}

static void testSscanfVmMatchesIntegerSpecifier() {
    // Mirrors master.c's "sscanf(str, \"for %d\", x)" shape.
    lpcdriver::Value result = runProbe(
        "int x;\n"
        "int n;\n"
        "n = sscanf(\"for 5\", \"for %d\", x);\n"
        "return n == 1 && x == 5;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testSscanfVmMatchesIntegerSpecifier OK\n";
}

static void testSscanfVmSkipModifierDoesNotConsumeOutputSlot() {
    // Mirrors master.c's "sscanf(file, REALMS_DIRS+\"/%s/%*s\", nom)"
    // shape: "%*s" matches and counts toward the match total but does not
    // consume (or require) an output variable.
    lpcdriver::Value result = runProbe(
        "string nom;\n"
        "int n;\n"
        "n = sscanf(\"/realms/foo/bar\", \"/realms/%s/%*s\", nom);\n"
        "return n == 2 && nom == \"foo\";\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testSscanfVmSkipModifierDoesNotConsumeOutputSlot OK\n";
}

static void testSscanfVmPartialMatchLeavesLaterVarsUntouchedAndReturnsPartialCount() {
    // "%s" matches "abc" up to the literal "#", but "%d" then fails to
    // find any digits in what follows ("xyz") -- sscanf stops there,
    // returning only the matches made so far and leaving "b" (whose slot
    // was never reached) at whatever it already held.
    lpcdriver::Value result = runProbe(
        "string a;\n"
        "int b;\n"
        "b = 999;\n"
        "int n;\n"
        "n = sscanf(\"abc#xyz\", \"%s#%d\", a, b);\n"
        "return n == 1 && a == \"abc\" && b == 999;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testSscanfVmPartialMatchLeavesLaterVarsUntouchedAndReturnsPartialCount OK\n";
}

// --- inherit ----------------------------------------------------------

static void testInheritStatementParsesPathAndConcatenation() {
    std::string src =
        "inherit \"/secure/std/daemon\";\n"
        "inherit \"/secure/daemon\" + \"/refs\";\n"
        "void create() {}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->inherits.size() == 2);
    assert(program->inherits[0] == "/secure/std/daemon");
    assert(program->inherits[1] == "/secure/daemon/refs");

    std::cout << "testInheritStatementParsesPathAndConcatenation OK\n";
}

static void testInheritedFunctionFallbackInvokedAtRuntime() {
    ObjectVarHarness harness;

    harness.writeFile("/base.c",
        "int greet_count() {\n"
        "    return 7;\n"
        "}\n");

    harness.writeFile("/child.c",
        "inherit \"/base\";\n"
        "\n"
        "int probe() {\n"
        "    return greet_count();\n"
        "}\n");

    auto child = harness.objects.cloneObject("/child");
    assert(child != nullptr);

    // greet_count() is never defined in child.c: probe()'s bare call to
    // it must fall through to the inherited program.
    lpcdriver::Value result = harness.vm.callFunction(child, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 7);

    std::cout << "testInheritedFunctionFallbackInvokedAtRuntime OK\n";
}

static void testInheritedObjectVariableSlotsShareStorageWithParent() {
    ObjectVarHarness harness;

    harness.writeFile("/base.c",
        "int shared;\n"
        "\n"
        "void set_shared(int v) {\n"
        "    shared = v;\n"
        "}\n");

    harness.writeFile("/child.c",
        "inherit \"/base\";\n"
        "\n"
        "int read_shared() {\n"
        "    return shared;\n"
        "}\n");

    auto child = harness.objects.cloneObject("/child");
    assert(child != nullptr);

    // set_shared() runs entirely via the PARENT's own bytecode (child
    // never redefines it) against this same LpcObject's variables();
    // read_shared() runs the CHILD's own bytecode, reading the same
    // object variable by name. Both must agree on which slot "shared"
    // lives in -- this is CodeGen's inheritedObjectVarNames flattening.
    harness.vm.callFunction(child, "set_shared",
        std::vector<lpcdriver::Value>{lpcdriver::Value(static_cast<int64_t>(55))});
    lpcdriver::Value result = harness.vm.callFunction(child, "read_shared", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 55);

    std::cout << "testInheritedObjectVariableSlotsShareStorageWithParent OK\n";
}

static void testInheritCycleDetectedAsCompileFailure() {
    ObjectVarHarness harness;

    harness.writeFile("/cyclea.c", "inherit \"/cycleb\";\n");
    harness.writeFile("/cycleb.c", "inherit \"/cyclea\";\n");

    // Must fail cleanly (nullptr) rather than recurse forever.
    auto obj = harness.objects.cloneObject("/cyclea");
    assert(obj == nullptr);

    std::cout << "testInheritCycleDetectedAsCompileFailure OK\n";
}

// --- type casts -------------------------------------------------------
// Hit immediately after the inherit slice while re-attempting the real
// master.c boot (line 98: "write(\"Got \"+(string)call_other(file, arg)+
// \" back.\\n\");"). Real LPC casts are a compile-time-only type
// annotation (see grammar.y's "cast" rule), so this driver treats them as
// a pure no-op: parse past "(type)"/"(type*)" and keep the operand.

static void testTypeCastParsesAsNoOpWrappingInnerExpr() {
    std::string src =
        "void probe() {\n"
        "    (string)x;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    // The cast contributes nothing to the AST: the expression is exactly
    // the bare VarRefExpr it wrapped, not some CastExpr wrapper.
    auto* ref = dynamic_cast<lpcdriver::VarRefExpr*>(exprStmt->expr.get());
    assert(ref != nullptr);
    assert(ref->name == "x");

    std::cout << "testTypeCastParsesAsNoOpWrappingInnerExpr OK\n";
}

static void testTypeCastVmExecutionIsNoOp() {
    lpcdriver::Value result = runProbe(
        "string s;\n"
        "s = \"hi\";\n"
        "return (string)s + (string)\"!\";\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "hi!");

    std::cout << "testTypeCastVmExecutionIsNoOp OK\n";
}

// Found while writing the cast tests above: a string literal whose
// contents happen to equal an operator's token text (e.g. "!") must never
// be mistaken for that operator by the parser's lookahead.
static void testStringLiteralMatchingOperatorTextParsesAsLiteralNotOperator() {
    lpcdriver::Value result = runProbe("return (string)\"!\" + \"x\";\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "!x");

    std::cout << "testStringLiteralMatchingOperatorTextParsesAsLiteralNotOperator OK\n";
}

// call_other's function-name argument is a full expression, not
// necessarily a literal (hit re-attempting the boot right after casts --
// master.c line 98: "call_other(file, arg)", where "arg" is a plain
// variable holding the function name to call at run time).
static void testCallOtherWithVariableFunctionNameVmExecution() {
    ObjectVarHarness harness;

    harness.writeFile("/target.c",
        "int greet() {\n"
        "    return 5;\n"
        "}\n"
        "int farewell() {\n"
        "    return 9;\n"
        "}\n");

    harness.writeFile("/caller.c",
        "int probe() {\n"
        "    object t;\n"
        "    string which;\n"
        "    t = clone_object(\"/target\");\n"
        "    which = \"farewell\";\n"
        "    return call_other(t, which);\n"
        "}\n");

    auto caller = harness.objects.cloneObject("/caller");
    assert(caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 9);

    std::cout << "testCallOtherWithVariableFunctionNameVmExecution OK\n";
}

// Found compiling the real secure/SimulEfun/SimulEfun.c (misc.c's own
// "inv[i]->query_property(...)"): "->" is checked once, before indexing,
// then indexing loops separately -- so "target[i]->fn()" (index *then*
// arrow) left the "->" unconsumed once the index loop finished, and the
// caller saw a stray "->" where it expected ";". "->" and "[" need to
// interleave in one loop so either order, and repeats, both work.
static void testIndexThenCallOtherOnResultVmExecution() {
    ObjectVarHarness harness;

    harness.writeFile("/target.c",
        "int query_property(string which) {\n"
        "    return which == \"light\" ? 7 : 0;\n"
        "}\n");

    harness.writeFile("/caller.c",
        "int probe() {\n"
        "    object *inv;\n"
        "    inv = ({ clone_object(\"/target\") });\n"
        "    return inv[0]->query_property(\"light\");\n"
        "}\n");

    auto caller = harness.objects.cloneObject("/caller");
    assert(caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 7);

    std::cout << "testIndexThenCallOtherOnResultVmExecution OK\n";
}

// Found compiling the real secure/SimulEfun/SimulEfun.c (misc.c's own
// "efun::destruct(ob)", needed there because that same file defines its
// own simul_efun wrapper named "destruct"): "efun::name(...)" must reach
// the real core efun even when a same-named local function would
// otherwise shadow it via the normal tiered Call resolution -- unlike a
// plain bare call, which correctly prefers the local one (see
// testBareCallToLocalFunctionEmitsCallOpcode and friends).
static void testEfunOverrideBypassesLocalFunctionOfSameName() {
    ObjectVarHarness harness;

    harness.writeFile("/caller.c",
        "int sizeof(mixed x) {\n"
        "    return -1;\n" // local shadow: always wrong, so any non
        "}\n"              // -1 result below proves efun:: bypassed it
        "int probe() {\n"
        "    mixed *arr;\n"
        "    arr = ({ 1, 2, 3 });\n"
        "    return efun::sizeof(arr);\n"
        "}\n");

    auto caller = harness.objects.cloneObject("/caller");
    assert(caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 3); // real sizeof(), not the local shadow

    std::cout << "testEfunOverrideBypassesLocalFunctionOfSameName OK\n";
}

// --- compound assignment -----------------------------------------------
// Hit immediately after the dynamic call_other fix while re-attempting the
// real master.c boot (line 115: "files += ({ lines[i] });").

static void testCompoundAssignOperatorsTokenize() {
    lpcdriver::Lexer lexer("a += b -= c *= d /= e %= f");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"+=", "-=", "*=", "/=", "%="};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == lpcdriver::TokenType::Symbol) {
            assert(t.text == expectedOps[opIdx]);
            ++opIdx;
        }
    }
    assert(opIdx == expectedOps.size());

    std::cout << "testCompoundAssignOperatorsTokenize OK\n";
}

static void testCompoundAssignParsesToCompoundAssignExpr() {
    std::string src =
        "void probe() {\n"
        "    x += 1;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* assign = dynamic_cast<lpcdriver::AssignExpr*>(exprStmt->expr.get());
    assert(assign != nullptr);
    assert(assign->isCompound);
    assert(assign->compoundOp == lpcdriver::BinOp::Add);
    assert(assign->name == "x");

    std::cout << "testCompoundAssignParsesToCompoundAssignExpr OK\n";
}

static void testCompoundAssignVmExecutionOnIntAndArray() {
    lpcdriver::Value intResult = runProbe(
        "int x;\n"
        "x = 5;\n"
        "x += 3;\n"
        "return x;\n");
    assert(std::holds_alternative<int64_t>(intResult.data));
    assert(std::get<int64_t>(intResult.data) == 8);

    // Mirrors master.c's own "files += ({ lines[i] });" shape: "+="
    // reuses the Add opcode's existing array-concatenation behavior.
    lpcdriver::Value arrResult = runProbe(
        "mixed *files;\n"
        "files = ({ 1 });\n"
        "files += ({ 2, 3 });\n"
        "return sizeof(files);\n");
    assert(std::holds_alternative<int64_t>(arrResult.data));
    assert(std::get<int64_t>(arrResult.data) == 3);

    std::cout << "testCompoundAssignVmExecutionOnIntAndArray OK\n";
}

// --- bitwise / array-intersection "&" ------------------------------------
// Hit immediately after compound assignment while re-attempting the real
// master.c boot (line 241: "sizeof(privs & ok)").

static void testBitAndParsesToBinaryExprWithBitAndOp() {
    std::string src =
        "void probe() {\n"
        "    a & b;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* bin = dynamic_cast<lpcdriver::BinaryExpr*>(exprStmt->expr.get());
    assert(bin != nullptr);
    assert(bin->op == lpcdriver::BinOp::BitAnd);

    std::cout << "testBitAndParsesToBinaryExprWithBitAndOp OK\n";
}

static void testBitAndVmExecutionOnInts() {
    lpcdriver::Value result = runProbe("return 6 & 3;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 2); // 110 & 011 == 010

    std::cout << "testBitAndVmExecutionOnInts OK\n";
}

static void testBitAndVmExecutionOnArraysIsIntersection() {
    // Mirrors master.c's own "sizeof(privs & ok)" shape.
    lpcdriver::Value result = runProbe(
        "mixed *privs, *ok;\n"
        "privs = ({ \"a\", \"b\", \"c\" });\n"
        "ok = ({ \"b\", \"c\", \"d\" });\n"
        "return sizeof(privs & ok);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 2); // "b" and "c" are shared

    lpcdriver::Value empty = runProbe(
        "mixed *privs, *ok;\n"
        "privs = ({ \"a\" });\n"
        "ok = ({ \"z\" });\n"
        "return sizeof(privs & ok);\n");
    assert(std::holds_alternative<int64_t>(empty.data));
    assert(std::get<int64_t>(empty.data) == 0);

    std::cout << "testBitAndVmExecutionOnArraysIsIntersection OK\n";
}

// Found compiling the real secure/std/login.c ("input_to(\"get_password\",
// 1 | 2)"): plain "|" and "^" were not lexed/parsed/codegen'd at all --
// "^" was not even in the Lexer's dispatch table, and "|" fell all the
// way through to being (correctly) tokenized but never consumed by any
// parse level, since parseLogicalAnd went straight to parseBitAnd with
// nothing in between.
static void testBitOrAndBitXorVmExecutionOnInts() {
    lpcdriver::Value orResult = runProbe("return 1 | 2;\n");
    assert(std::holds_alternative<int64_t>(orResult.data));
    assert(std::get<int64_t>(orResult.data) == 3);

    lpcdriver::Value xorResult = runProbe("return 6 ^ 3;\n");
    assert(std::holds_alternative<int64_t>(xorResult.data));
    assert(std::get<int64_t>(xorResult.data) == 5);

    // "||" must still tokenize and parse as logical-or, not two adjacent
    // "|" bitwise-or tokens.
    lpcdriver::Value orElse = runProbe("return 0 || 5;\n");
    assert(std::holds_alternative<int64_t>(orElse.data));
    assert(std::get<int64_t>(orElse.data) == 5);

    std::cout << "testBitOrAndBitXorVmExecutionOnInts OK\n";
}

// --- catch(expr) ---------------------------------------------------------
// Real LPC's own control-flow construct for trapping a runtime error, not
// a function call (see Ast.hpp's CatchExpr comment; confirmed directly
// against the FluffOS reference driver's grammar.y/icode.c/interpret.c,
// not inferred). Found needed live: secure/daemon/master.c's own
// connect() is "if(err=catch(ob = clone_object(OB_LOGIN))) { ... }".

static void testCatchEvaluatesToErrorMessageStringWhenGuardedExprThrows() {
    lpcdriver::Value result = runProbe(
        "mixed err;\n"
        "err = catch(totally_undefined_thing_xyz());\n"
        "return err;\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(!std::get<std::string>(result.data).empty());

    std::cout << "testCatchEvaluatesToErrorMessageStringWhenGuardedExprThrows OK\n";
}

// Confirmed against interpret.c's F_END_CATCH ("catch_value = const0;
// ... push_number(0)") and trees.c's insert_pop_value() (applied to the
// catch argument in grammar.y's own "'(' comma_expr ')' { $$ =
// insert_pop_value($2); }" production): catch(expr) evaluates to plain
// int 0 on success, discarding expr's own result entirely -- not expr's
// value, and not an empty string.
static void testCatchEvaluatesToZeroAndDiscardsGuardedExprValueOnSuccess() {
    lpcdriver::Value result = runProbe(
        "mixed err;\n"
        "err = catch(42);\n"
        "return err;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testCatchEvaluatesToZeroAndDiscardsGuardedExprValueOnSuccess OK\n";
}

// Execution resumes normally right after the whole catch() expression,
// as if nothing happened -- not just "catch() itself produces a value",
// the rest of the enclosing statement/function keeps running.
static void testExecutionContinuesNormallyAfterCatchTrapsAnError() {
    lpcdriver::Value result = runProbe(
        "mixed err;\n"
        "int after;\n"
        "err = catch(totally_undefined_thing_xyz());\n"
        "after = 99;\n"
        "return after;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 99);

    std::cout << "testExecutionContinuesNormallyAfterCatchTrapsAnError OK\n";
}

// Nested catch(): the inner catch() traps its own error first (LIFO,
// matching real FluffOS's own nested do_catch() call stack), so the
// outer catch() never sees an error at all and evaluates to 0, not to
// the inner failure's message.
static void testNestedCatchInnerFailureDoesNotTriggerOuterCatch() {
    lpcdriver::Value result = runProbe(
        "mixed innerErr, outerErr;\n"
        "outerErr = catch(innerErr = catch(totally_undefined_thing_xyz()));\n"
        "return outerErr;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0); // outer: no error reached it

    // The inner catch, checked via a second probe, still caught its own
    // error and produced the message.
    lpcdriver::Value innerResult = runProbe(
        "mixed innerErr, outerErr;\n"
        "outerErr = catch(innerErr = catch(totally_undefined_thing_xyz()));\n"
        "return innerErr;\n");
    assert(std::holds_alternative<std::string>(innerResult.data));
    assert(!std::get<std::string>(innerResult.data).empty());

    std::cout << "testNestedCatchInnerFailureDoesNotTriggerOuterCatch OK\n";
}

// A caught error must still be visible without adding temporary
// instrumentation -- confirmed against real FluffOS's own default build
// (simulate.c's error_handler(), gated on LOG_CATCHES, which is defined
// by default in every shipped local_options.*, including this exact
// mudlib's own local_options.nm3). VM::run()'s catch-frame branch logs
// one "[catch] <object>::<function>(): <message>" line to stderr,
// unconditionally, before resuming -- this test redirects std::cerr's
// buffer to capture it rather than just checking the catch() return
// value (already covered by testCatchEvaluatesToErrorMessageStringWhenGuardedExprThrows).
static void testCatchLogsTrappedErrorToStderrByDefault() {
    std::ostringstream captured;
    std::streambuf* originalCerr = std::cerr.rdbuf(captured.rdbuf());
    lpcdriver::Value result = runProbe(
        "mixed err;\n"
        "err = catch(totally_undefined_thing_xyz());\n"
        "return err;\n");
    std::cerr.rdbuf(originalCerr);

    assert(std::holds_alternative<std::string>(result.data));
    std::string logged = captured.str();
    assert(logged.find("[catch] probe_object::probe(): ") != std::string::npos);
    // The logged line must carry the same error message catch() itself
    // returned, not a generic placeholder.
    assert(logged.find(std::get<std::string>(result.data)) != std::string::npos);

    std::cout << "testCatchLogsTrappedErrorToStderrByDefault OK\n";
}

// A runtime error thrown *inside a called function*, with no catch() of
// its own there, must still be trapped by a catch() at the *caller*'s
// level -- confirmed against interpret.c's do_catch() itself recursing
// into eval_instruction() for the guarded code, so a callee's own
// longjmp unwinds straight through to the caller's SETJMP. Here that
// means a caller's active catchFrames entry must survive the callee's
// own (empty) catchFrames rethrowing back out of its nested run() call.
static void testCatchTrapsErrorThrownInsideCalledFunctionWithNoCatchOfItsOwn() {
    ObjectVarHarness harness;

    harness.writeFile("/callee.c",
        "int explode_now() {\n"
        "    return totally_undefined_thing_xyz();\n"
        "}\n");

    harness.writeFile("/caller.c",
        "mixed probe() {\n"
        "    object callee;\n"
        "    mixed err;\n"
        "    callee = clone_object(\"/callee\");\n"
        "    err = catch(callee->explode_now());\n"
        "    return err;\n"
        "}\n");

    auto caller = harness.objects.cloneObject("/caller");
    assert(caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(!std::get<std::string>(result.data).empty());

    std::cout << "testCatchTrapsErrorThrownInsideCalledFunctionWithNoCatchOfItsOwnOK OK\n";
}

// The real master.c shape end to end: an assignment inside catch()'s
// guarded expression, itself the condition of an if statement --
// "if(err=catch(ob = clone_object(OB_LOGIN))) { ... }". Exercised both
// ways: target compiles/loads fine (no error, "ob" ends up set, "err"
// stays falsy, the if-branch is skipped) and target fails to load (an
// error, "err" is truthy, the if-branch runs) -- mirroring
// master.c::connect()'s own real two-outcome shape.
static void testCatchInlineInsideIfConditionMatchesMasterConnectShape() {
    ObjectVarHarness harness;

    harness.writeFile("/good_login.c",
        "int marker() { return 7; }\n");
    // No "/bad_login.c" written at all: clone_object() on a nonexistent
    // path is real LPC's own everyday "the target failed to load" case,
    // the same failure master.c's own OB_LOGIN clone_object() guards
    // against.

    harness.writeFile("/caller.c",
        "int probe(int wantFailure) {\n"
        "    object ob;\n"
        "    string err;\n"
        "    if (wantFailure) {\n"
        "        if(err=catch(ob = clone_object(\"/bad_login\"))) {\n"
        "            return 1;\n" // matches master.c's own error branch shape
        "        }\n"
        "        return 0;\n"
        "    }\n"
        "    if(err=catch(ob = clone_object(\"/good_login\"))) {\n"
        "        return -1;\n" // should never be reached on success
        "    }\n"
        "    return ob->marker();\n"
        "}\n");

    auto caller = harness.objects.cloneObject("/caller");
    assert(caller != nullptr);

    lpcdriver::Value failureResult =
        harness.vm.callFunction(caller, "probe", {lpcdriver::Value(static_cast<int64_t>(1))});
    assert(std::holds_alternative<int64_t>(failureResult.data));
    assert(std::get<int64_t>(failureResult.data) == 1); // error branch taken

    lpcdriver::Value successResult =
        harness.vm.callFunction(caller, "probe", {lpcdriver::Value(static_cast<int64_t>(0))});
    assert(std::holds_alternative<int64_t>(successResult.data));
    assert(std::get<int64_t>(successResult.data) == 7); // ob correctly assigned, marker() reachable

    std::cout << "testCatchInlineInsideIfConditionMatchesMasterConnectShape OK\n";
}

// --- adjacent string literal concatenation --------------------------
// Hit immediately after the & operator while re-attempting the real
// master.c boot (a shout() message split across two source lines purely
// for readability, with no "+" between the two string literals).

static void testAdjacentStringLiteralsParseAsSingleConcatenatedLiteral() {
    std::string src =
        "void probe() {\n"
        "    \"foo\" \"bar\";\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* lit = dynamic_cast<lpcdriver::StringLiteral*>(exprStmt->expr.get());
    assert(lit != nullptr);
    assert(lit->value == "foobar");

    std::cout << "testAdjacentStringLiteralsParseAsSingleConcatenatedLiteral OK\n";
}

static void testAdjacentStringLiteralsVmExecution() {
    lpcdriver::Value result = runProbe(
        "return \"Saving all players; \"\n"
        "       \"please reconnect shortly.\";\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "Saving all players; please reconnect shortly.");

    std::cout << "testAdjacentStringLiteralsVmExecution OK\n";
}

// --- break / continue -------------------------------------------------
// Hit immediately after adjacent-string-literal concatenation while
// re-attempting the real master.c boot: check_access()'s guard clauses
// are built almost entirely out of "if (...) continue;" inside a while
// loop over the caller stack.

static void testBreakAndContinueParseToDedicatedStmtNodes() {
    std::string src =
        "void probe() {\n"
        "    while (1) {\n"
        "        break;\n"
        "        continue;\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* whileStmt = dynamic_cast<lpcdriver::WhileStmt*>(body[0].get());
    assert(whileStmt != nullptr);
    assert(dynamic_cast<lpcdriver::BreakStmt*>(whileStmt->body->statements[0].get()) != nullptr);
    assert(dynamic_cast<lpcdriver::ContinueStmt*>(whileStmt->body->statements[1].get()) != nullptr);

    std::cout << "testBreakAndContinueParseToDedicatedStmtNodes OK\n";
}

static void testBreakOutsideLoopThrowsAtCodegen() {
    std::string src = "void probe() { break; }\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    lpcdriver::CodeGen codegen;

    bool threw = false;
    try {
        codegen.generate(*program);
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testBreakOutsideLoopThrowsAtCodegen OK\n";
}

static void testBreakStopsForLoopEarlyVmExecution() {
    lpcdriver::Value result = runProbe(
        "int sum;\n"
        "sum = 0;\n"
        "for (int i = 0; i < 10; i++) {\n"
        "    if (i == 3) break;\n"
        "    sum = sum + i;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 3); // 0 + 1 + 2, stops before adding 3

    std::cout << "testBreakStopsForLoopEarlyVmExecution OK\n";
}

static void testContinueSkipsRestOfForLoopBodyVmExecution() {
    lpcdriver::Value result = runProbe(
        "int sum;\n"
        "sum = 0;\n"
        "for (int i = 0; i < 5; i++) {\n"
        "    if (i == 2) continue;\n"
        "    sum = sum + i;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 8); // 0+1+3+4, skipping 2

    std::cout << "testContinueSkipsRestOfForLoopBodyVmExecution OK\n";
}

static void testContinueInWhileLoopSkipsToConditionRecheckVmExecution() {
    // Mirrors check_access()'s real shape: a while loop whose body is
    // mostly "if (...) continue;" guard clauses.
    lpcdriver::Value result = runProbe(
        "int i;\n"
        "int sum;\n"
        "i = 0;\n"
        "sum = 0;\n"
        "while (i < 5) {\n"
        "    i = i + 1;\n"
        "    if (i == 3) continue;\n"
        "    sum = sum + i;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 12); // 1+2+4+5, skipping 3

    std::cout << "testContinueInWhileLoopSkipsToConditionRecheckVmExecution OK\n";
}

static void testBreakInInnerLoopDoesNotAffectOuterLoopVmExecution() {
    // Nested loops: break/continue must resolve against the innermost
    // enclosing loop only (CodeGen's loopStack_ push/pop per loop).
    lpcdriver::Value result = runProbe(
        "int outerSum;\n"
        "outerSum = 0;\n"
        "for (int i = 0; i < 3; i++) {\n"
        "    int innerCount;\n"
        "    innerCount = 0;\n"
        "    for (int j = 0; j < 10; j++) {\n"
        "        if (j == 2) break;\n"
        "        innerCount = innerCount + 1;\n"
        "    }\n"
        "    outerSum = outerSum + innerCount;\n"
        "}\n"
        "return outerSum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 6); // inner loop stops at count 2, three times

    std::cout << "testBreakInInnerLoopDoesNotAffectOuterLoopVmExecution OK\n";
}

// A bare ";" (null statement) as a loop body -- real code's way of
// putting all of a loop's work into the condition itself and leaving the
// body empty. Found compiling the real secure/SimulEfun/SimulEfun.c
// (misc.c's own "while( i-- && ( str[i..i] != ":" ) );"): previously fell
// through parseStatement()'s expression-statement path, and parseExpr()
// seeing ";" as its very first token threw "expected expression".
static void testNullStatementAsLoopBodyParsesAndExecutesAsNoOp() {
    lpcdriver::Value result = runProbe(
        "string str;\n"
        "int i;\n"
        "str = \"ab:cd\";\n"
        "i = 5;\n"
        "while (i-- && (str[i..i] != \":\"));\n"
        "return i;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 2); // stops with i at the ':' index

    std::cout << "testNullStatementAsLoopBodyParsesAndExecutesAsNoOp OK\n";
}

// --- read_file / write_file / graceful create() failure -----------------
// Hit continuing the real boot past the language-level fixes above:
// master.c's create() -> new_read()/new_write()/new_groups() ->
// load_access() chain calls the real read_file() efun, which did not
// exist yet, and an uncaught runtime error from it was crashing the whole
// driver process instead of just failing that one object's load.

static void testReadFileReturnsFileContentAndFalsyForMissingFile() {
    ObjectVarHarness harness;
    harness.writeFile("/data.txt", "line one\nline two\n");

    harness.writeFile("/reader.c",
        "string read_it() {\n"
        "    return read_file(\"/data.txt\");\n"
        "}\n"
        "mixed read_missing() {\n"
        "    return read_file(\"/does_not_exist.txt\");\n"
        "}\n");

    auto obj = harness.objects.cloneObject("/reader");
    assert(obj != nullptr);

    lpcdriver::Value content = harness.vm.callFunction(obj, "read_it", {});
    assert(std::holds_alternative<std::string>(content.data));
    assert(std::get<std::string>(content.data) == "line one\nline two\n");

    lpcdriver::Value missing = harness.vm.callFunction(obj, "read_missing", {});
    assert(std::holds_alternative<int64_t>(missing.data));
    assert(std::get<int64_t>(missing.data) == 0);

    std::cout << "testReadFileReturnsFileContentAndFalsyForMissingFile OK\n";
}

static void testWriteFileThenReadFileRoundTrips() {
    ObjectVarHarness harness;

    harness.writeFile("/writer.c",
        "int write_it() {\n"
        "    return write_file(\"/output.txt\", \"hello world\\n\", 1);\n"
        "}\n"
        "string read_it() {\n"
        "    return read_file(\"/output.txt\");\n"
        "}\n");

    auto obj = harness.objects.cloneObject("/writer");
    assert(obj != nullptr);

    lpcdriver::Value writeResult = harness.vm.callFunction(obj, "write_it", {});
    assert(std::holds_alternative<int64_t>(writeResult.data));
    assert(std::get<int64_t>(writeResult.data) == 1);

    lpcdriver::Value content = harness.vm.callFunction(obj, "read_it", {});
    assert(std::holds_alternative<std::string>(content.data));
    assert(std::get<std::string>(content.data) == "hello world\n");

    std::cout << "testWriteFileThenReadFileRoundTrips OK\n";
}

static void testCreateRuntimeErrorFailsLoadInsteadOfCrashing() {
    ObjectVarHarness harness;

    harness.writeFile("/broken.c",
        "void create() {\n"
        "    totally_undefined_efun_name();\n"
        "}\n");

    // Must fail this one load cleanly (nullptr), not crash the process.
    auto obj = harness.objects.cloneObject("/broken");
    assert(obj == nullptr);

    std::cout << "testCreateRuntimeErrorFailsLoadInsteadOfCrashing OK\n";
}

// --- absolute #include paths / cpp warnings not failing preprocessing ---
// Both hit loading the real secure/SimulEfun/SimulEfun.c, which #includes
// ~50 other files by absolute mudlib path and pulls in a header
// (debug.h) that trips a real (harmless) cpp warning.

static void testAbsoluteIncludePathResolvesAgainstMudlibRoot() {
    ObjectVarHarness harness;

    harness.writeFile("/helper_defs.c",
        "int helper() {\n"
        "    return 42;\n"
        "}\n");

    harness.writeFile("/main_with_include.c",
        "#include \"/helper_defs.c\"\n"
        "int probe() {\n"
        "    return helper();\n"
        "}\n");

    auto obj = harness.objects.cloneObject("/main_with_include");
    assert(obj != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testAbsoluteIncludePathResolvesAgainstMudlibRoot OK\n";
}

static void testCppWarningsDoNotFailPreprocessing() {
    ObjectVarHarness harness;

    // "#endif LABEL" (a trailing token after #endif) is valid, older-style
    // C that GCC's cpp warns about but does not fail on -- the same shape
    // as the real mudlib's secure/include/debug.h.
    harness.writeFile("/warns.c",
        "#ifndef _WARNS_H\n"
        "#define _WARNS_H\n"
        "int probe() {\n"
        "    return 1;\n"
        "}\n"
        "#endif _WARNS_H\n");

    auto obj = harness.objects.cloneObject("/warns");
    assert(obj != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testCppWarningsDoNotFailPreprocessing OK\n";
}

// --- simul_efun resolution tier ------------------------------------------
// Fourth fallback for a bare call: local -> inherited -> simul_efun object
// -> core efun table, matching real FluffOS's own compile-time resolution
// order (lex.c's F_SIMUL_EFUN / function.c's call_simul_efun()), just
// resolved at run time here like the other three tiers already are.

static void testSimulEfunResolvesUnknownBareCallToSimulEfunObject() {
    ObjectVarHarness harness;

    harness.writeFile("/simul_efun.c",
        "int simul_marker;\n"
        "void create() {\n"
        "    simul_marker = 7;\n"
        "}\n"
        "int double_it(int x) {\n"
        "    return x * 2;\n"
        "}\n"
        "int read_marker() {\n"
        "    return simul_marker;\n"
        "}\n");
    assert(harness.objects.loadSimulEfunObject());

    harness.writeFile("/caller.c",
        "int probe() {\n"
        "    return double_it(21);\n"
        "}\n");

    auto caller = harness.objects.cloneObject("/caller");
    assert(caller != nullptr);

    // double_it() is defined only on the simul_efun object, never locally
    // or via inherit -- this must fall through all the way to tier 3.
    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    // A simul_efun function's own object variables belong to the
    // simul_efun object itself, not whichever object called it.
    lpcdriver::Value marker = harness.vm.callFunction(harness.objects.simulEfunObject(),
                                                        "read_marker", {});
    assert(std::holds_alternative<int64_t>(marker.data));
    assert(std::get<int64_t>(marker.data) == 7);

    std::cout << "testSimulEfunResolvesUnknownBareCallToSimulEfunObject OK\n";
}

static void testLocalFunctionShadowsSimulEfunOfSameName() {
    ObjectVarHarness harness;

    harness.writeFile("/simul_efun.c",
        "int labeled() {\n"
        "    return 100;\n"
        "}\n");
    assert(harness.objects.loadSimulEfunObject());

    harness.writeFile("/shadower.c",
        "int labeled() {\n"
        "    return 1;\n"
        "}\n"
        "int probe() {\n"
        "    return labeled();\n"
        "}\n");

    auto obj = harness.objects.cloneObject("/shadower");
    assert(obj != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1); // local wins, not the simul_efun's 100

    std::cout << "testLocalFunctionShadowsSimulEfunOfSameName OK\n";
}

// --- heredoc string literals ("@TERM ... TERM") ---------------------
// Hit loading the real secure/SimulEfun/SimulEfun.c (misc.c's
// dump_socket_status()).

static void testHeredocTokenizesToStringWithLiteralContent() {
    std::string src = "\"before\" @END\nline one\nline two\nEND;\n\"after\"";
    lpcdriver::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    // ["before"](String) ["\nline one\nline two\n"](String) [;](Symbol) ["after"](String) [End]
    assert(tokens.size() == 5);
    assert(tokens[0].type == lpcdriver::TokenType::String && tokens[0].text == "before");
    assert(tokens[1].type == lpcdriver::TokenType::String);
    assert(tokens[1].text == "line one\nline two\n");
    assert(tokens[2].type == lpcdriver::TokenType::Symbol && tokens[2].text == ";");
    assert(tokens[3].type == lpcdriver::TokenType::String && tokens[3].text == "after");

    std::cout << "testHeredocTokenizesToStringWithLiteralContent OK\n";
}

static void testHeredocVmExecutionMatchesRealShape() {
    // Mirrors secure/SimulEfun/misc.c's own "ret = @END ... END;" shape.
    lpcdriver::Value result = runProbe(
        "string ret;\n"
        "ret = @END\n"
        "Fd    State\n"
        "--  ---------\n"
        "END;\n"
        "return ret;\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "Fd    State\n--  ---------\n");

    std::cout << "testHeredocVmExecutionMatchesRealShape OK\n";
}

static void testUnterminatedHeredocThrows() {
    std::string src = "\"x\" @END\nno terminator here\n";
    lpcdriver::Lexer lexer(src);
    bool threw = false;
    try {
        lexer.tokenize();
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testUnterminatedHeredocThrows OK\n";
}

// --- foreach --------------------------------------------------------------

static void testForeachSingleVarParsesToForeachStmt() {
    std::string src =
        "void probe() {\n"
        "    foreach (mixed *item in arr) {\n"
        "        write(\"x\");\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* fe = dynamic_cast<lpcdriver::ForeachStmt*>(body[0].get());
    assert(fe != nullptr);
    assert(fe->varName == "item");
    assert(fe->declareVar == true);
    assert(fe->hasValueVar == false);

    std::cout << "testForeachSingleVarParsesToForeachStmt OK\n";
}

static void testForeachTwoVarParsesWithValueVar() {
    std::string src =
        "void probe() {\n"
        "    foreach (key, val in m) {\n"
        "        write(\"x\");\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* fe = dynamic_cast<lpcdriver::ForeachStmt*>(body[0].get());
    assert(fe != nullptr);
    assert(fe->varName == "key");
    assert(fe->declareVar == false);
    assert(fe->hasValueVar == true);
    assert(fe->valueVarName == "val");
    assert(fe->declareValueVar == false);

    std::cout << "testForeachTwoVarParsesWithValueVar OK\n";
}

static void testForeachOverArraySumsElementsVmExecution() {
    lpcdriver::Value result = runProbe(
        "mixed *arr;\n"
        "int sum;\n"
        "arr = ({ 1, 2, 3, 4 });\n"
        "sum = 0;\n"
        "foreach (int x in arr) {\n"
        "    sum = sum + x;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 10);

    std::cout << "testForeachOverArraySumsElementsVmExecution OK\n";
}

static void testForeachOverMappingSingleVarIteratesKeysVmExecution() {
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "int sum;\n"
        "m = ([ 1: \"a\", 2: \"b\", 3: \"c\" ]);\n"
        "sum = 0;\n"
        "foreach (int k in m) {\n"
        "    sum = sum + k;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 6);

    std::cout << "testForeachOverMappingSingleVarIteratesKeysVmExecution OK\n";
}

static void testForeachOverMappingTwoVarGivesKeyValuePairsVmExecution() {
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "int keySum;\n"
        "string cat;\n"
        "m = ([ 1: \"a\", 2: \"b\" ]);\n"
        "keySum = 0;\n"
        "cat = \"\";\n"
        "foreach (int k, string v in m) {\n"
        "    keySum = keySum + k;\n"
        "    cat = cat + v;\n"
        "}\n"
        "return keySum == 3 && (cat == \"ab\" || cat == \"ba\");\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testForeachOverMappingTwoVarGivesKeyValuePairsVmExecution OK\n";
}

static void testBreakAndContinueInsideForeachVmExecution() {
    lpcdriver::Value result = runProbe(
        "mixed *arr;\n"
        "int sum;\n"
        "arr = ({ 1, 2, 3, 4, 5 });\n"
        "sum = 0;\n"
        "foreach (int x in arr) {\n"
        "    if (x == 2) continue;\n"
        "    if (x == 4) break;\n"
        "    sum = sum + x;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 4); // 1 + 3, skip 2, stop before 4

    std::cout << "testBreakAndContinueInsideForeachVmExecution OK\n";
}

static void testNestedForeachLoopsDoNotCollideVmExecution() {
    // Exercises foreachCounter_'s per-statement unique hidden-slot naming.
    lpcdriver::Value result = runProbe(
        "mixed *outer, *inner;\n"
        "int total;\n"
        "outer = ({ 1, 2 });\n"
        "inner = ({ 10, 20 });\n"
        "total = 0;\n"
        "foreach (int a in outer) {\n"
        "    foreach (int b in inner) {\n"
        "        total = total + a * b;\n"
        "    }\n"
        "}\n"
        "return total;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // (1*10 + 1*20) + (2*10 + 2*20) = 30 + 60 = 90
    assert(std::get<int64_t>(result.data) == 90);

    std::cout << "testNestedForeachLoopsDoNotCollideVmExecution OK\n";
}

// "array" is not reserved (see Lexer.cpp's kKeywords comment): real code
// uses it as a plain identifier (secure/SimulEfun/exclude_array.c's own
// "mixed *array" parameter).
static void testArrayUsableAsParameterNameNotReservedAsType() {
    std::string src =
        "mixed *exclude_array(mixed *array, int from, int to) {\n"
        "    return array;\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    assert(program->functions[0]->params.size() == 3);
    assert(program->functions[0]->params[0].name == "array");

    std::cout << "testArrayUsableAsParameterNameNotReservedAsType OK\n";
}

static void testTrailingVarargsEllipsisParsesAndIsDiscarded() {
    // Mirrors secure/SimulEfun/misc.c's own "int true(mixed args...)".
    std::string src = "int true(mixed args...) { return 1; }\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    assert(program->functions[0]->params.size() == 1);
    assert(program->functions[0]->params[0].name == "args");

    std::cout << "testTrailingVarargsEllipsisParsesAndIsDiscarded OK\n";
}

static void testOpenEndedRangeIndexVmExecution() {
    // Mirrors secure/SimulEfun/misc.c's own
    // "str[strsrch(str, \"\\n\")+1..]" shape.
    lpcdriver::Value result = runProbe(
        "string s;\n"
        "s = \"hello world\";\n"
        "return s[6..];\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "world");

    std::cout << "testOpenEndedRangeIndexVmExecution OK\n";
}

static void testForLoopCommaExprChainInInitAndUpdateVmExecution() {
    // Mirrors secure/SimulEfun/misc.c's own
    // "for(i = 0, s = sizeof(stack1); i < s; i++)" shape (two
    // comma-separated expressions in the init clause here; the update
    // clause tests the same chain shape independently).
    lpcdriver::Value result = runProbe(
        "int i, j, sum;\n"
        "sum = 0;\n"
        "for (i = 0, j = 10; i < 3; i = i + 1, j = j - 1) {\n"
        "    sum = sum + j;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 27); // j = 10, 9, 8 while i = 0, 1, 2

    std::cout << "testForLoopCommaExprChainInInitAndUpdateVmExecution OK\n";
}

// --- switch -----------------------------------------------------------
// Hit immediately after for-loop comma-expression chains while
// re-attempting the real boot: secure/SimulEfun/alignment.c's own
// "switch(str_class) { case \"monk\": return ...; default: return 1; }".

static void testSwitchParsesToSwitchStmtWithInterleavedLabels() {
    std::string src =
        "void probe() {\n"
        "    switch (x) {\n"
        "        case 1:\n"
        "            write(\"one\");\n"
        "        case 2:\n"
        "            write(\"two\");\n"
        "            break;\n"
        "        default:\n"
        "            write(\"other\");\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* sw = dynamic_cast<lpcdriver::SwitchStmt*>(body[0].get());
    assert(sw != nullptr);
    // [case 1, write("one"), case 2, write("two"), break, default, write("other")]
    assert(sw->body.size() == 7);
    auto* case1 = dynamic_cast<lpcdriver::CaseLabel*>(sw->body[0].get());
    assert(case1 != nullptr && case1->value != nullptr);
    auto* defaultLabel = dynamic_cast<lpcdriver::CaseLabel*>(sw->body[5].get());
    assert(defaultLabel != nullptr && defaultLabel->value == nullptr);

    std::cout << "testSwitchParsesToSwitchStmtWithInterleavedLabels OK\n";
}

static void testSwitchRangeCaseLabelThrowsNotImplemented() {
    std::string src =
        "void probe() {\n"
        "    switch (x) {\n"
        "        case 1..5:\n"
        "            break;\n"
        "    }\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());

    bool threw = false;
    try {
        parser.parseProgram();
    } catch (const lpcdriver::NotImplementedError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testSwitchRangeCaseLabelThrowsNotImplemented OK\n";
}

static void testSwitchMatchingCaseVmExecution() {
    // Mirrors secure/SimulEfun/alignment.c's own shape, including a
    // string subject and each case returning immediately.
    lpcdriver::Value result = runProbe(
        "string cls;\n"
        "cls = \"cleric\";\n"
        "switch (cls) {\n"
        "    case \"monk\": return 1;\n"
        "    case \"cleric\": return 2;\n"
        "    default: return 3;\n"
        "}\n"
        "return -1;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 2);

    std::cout << "testSwitchMatchingCaseVmExecution OK\n";
}

static void testSwitchDefaultCaseVmExecution() {
    lpcdriver::Value result = runProbe(
        "string cls;\n"
        "cls = \"barbarian\";\n"
        "switch (cls) {\n"
        "    case \"monk\": return 1;\n"
        "    case \"cleric\": return 2;\n"
        "    default: return 3;\n"
        "}\n"
        "return -1;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 3);

    std::cout << "testSwitchDefaultCaseVmExecution OK\n";
}

static void testSwitchFallthroughWithoutBreakVmExecution() {
    lpcdriver::Value result = runProbe(
        "int x, sum;\n"
        "x = 1;\n"
        "sum = 0;\n"
        "switch (x) {\n"
        "    case 1:\n"
        "        sum = sum + 1;\n"
        "    case 2:\n"
        "        sum = sum + 10;\n"
        "        break;\n"
        "    case 3:\n"
        "        sum = sum + 100;\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 11); // falls through case 1 into case 2, stops at break

    std::cout << "testSwitchFallthroughWithoutBreakVmExecution OK\n";
}

static void testContinueInsideSwitchInsideLoopTargetsLoopVmExecution() {
    // "continue" inside a switch must skip the switch and continue the
    // enclosing loop, not be treated as (invalid) switch-continue.
    lpcdriver::Value result = runProbe(
        "int i, sum;\n"
        "sum = 0;\n"
        "for (i = 0; i < 5; i++) {\n"
        "    switch (i) {\n"
        "        case 2:\n"
        "            continue;\n"
        "        default:\n"
        "            sum = sum + i;\n"
        "    }\n"
        "}\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 8); // 0+1+3+4, skipping 2

    std::cout << "testContinueInsideSwitchInsideLoopTargetsLoopVmExecution OK\n";
}

// Found re-attempting the boot right after switch: an embedded assignment
// inside a "&&" chain, with no parens, must still parse at all -- a naive
// precedence-climbing parser that only checks for "=" after an entire
// tighter-precedence chain has already returned would see "1 && val"
// reduce to a BinaryExpr before ever noticing the "=", then fail with
// "invalid assignment target" (secure/SimulEfun/domains.c's own
// "stringp(val) && val=load_object(val) && ..." is exactly this shape).
// Checking for "=" immediately after a bare identifier operand -- before
// returning up through the "&&"/"||"/etc layers -- fixes that, matching
// where real LPC's grammar (grammar.y) actually restricts its "lvalue"
// nonterminal to (a narrow, primary-level "expr4", not any "expr0").
//
// Verified against fluffos-2.9-ds2.08's real grammar.y directly (bison -v
// on it, no conflicts reported): "%right L_ASSIGN" is declared before
// "%left L_LAND", i.e. assignment is LPC's single loosest-binding
// operator, looser even than "&&". So once the "=" is found, its
// right-hand side still greedily extends through any following "&&"
// (state 214 in the generated table shifts on L_LAND rather than
// reducing the assignment) -- assignment does not stop at the next "&&"
// the way the first (fully parenthesized, intentionally unambiguous)
// case below does. For domains.c's own line this is why it works at
// all: "val" only gets reassigned to whatever the *whole* right-hand
// "load_object(val) && domain_exists(tmp=(string)val->query_domain())"
// evaluates to, and "val->query_domain()" inside that runs against
// val's *old* (string) value, relying on real LPC's implicit
// string-call_other coercion, not a fresh (val = load_object(val)).
static void testEmbeddedAssignmentInsideLogicalAndBindsToImmediateVariable() {
    lpcdriver::Value result = runProbe(
        "int val, sideEffect;\n"
        "val = 0;\n"
        "sideEffect = 5 && (val = 9) && val == 9;\n"
        "return sideEffect * 100 + val;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 109); // sideEffect == 1, val == 9

    // The unparenthesized shape from the real mudlib file itself, minus
    // the string/object-coercion part (not needed to demonstrate the
    // precedence): "val=5 && val==5" parses as "val = (5 && (val==5))",
    // and at the moment "val==5" evaluates, val is still its *old* value
    // (0), so the assignment's own right-hand side is 0, not 1.
    lpcdriver::Value real = runProbe(
        "int val;\n"
        "int ok;\n"
        "val = 0;\n"
        "ok = 1 && val=5 && val == 5;\n"
        "return ok && val == 5;\n");
    assert(std::holds_alternative<int64_t>(real.data));
    assert(std::get<int64_t>(real.data) == 0); // ok == 0, val == 0 (see above)

    std::cout << "testEmbeddedAssignmentInsideLogicalAndBindsToImmediateVariable OK\n";
}

// ---------------------------------------------------------------------
// connect/input protocol: logon(), input_to(), and Server::dispatchLine()
// ---------------------------------------------------------------------
//
// These use ObjectVarHarness (a real ObjectManager + VM backed by a temp
// mudlib directory, defined above) rather than the in-memory-only
// compileProgramObject()/runProbe() helpers: input_to() reads
// OutputContext::current() and vm.currentObject(), both of which are
// only populated correctly by a genuine VM::run() call reached through
// ObjectManager, not by hand-assembled bytecode. Each test opens its own
// AF_UNIX socketpair to stand in for a real client connection --
// Connection only needs a valid fd (for send()/close()), never actually
// parses telnet bytes off it in these tests.

static void testInputToRegistersPendingHandlerWithExtraArgsAndTargetObject() {
    ObjectVarHarness harness;
    harness.writeFile("/login1.c",
        "void start() {\n"
        "    input_to(\"get_name\", \"extra1\", \"extra2\");\n"
        "}\n"
        "void get_name(string str) {}\n");
    auto loginObj = harness.objects.cloneObject("/login1");
    assert(loginObj != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    lpcdriver::Connection conn(fds[0]);

    lpcdriver::OutputContext::set(&conn);
    lpcdriver::Value regResult = harness.vm.callFunction(loginObj, "start", {});
    (void)regResult;
    lpcdriver::OutputContext::set(nullptr);

    assert(conn.hasPendingInputTo());
    auto pending = conn.takePendingInputTo();
    assert(pending.has_value());
    assert(pending->function == "get_name");
    assert(pending->extraArgs.size() == 2);
    assert(std::get<std::string>(pending->extraArgs[0].data) == "extra1");
    assert(std::get<std::string>(pending->extraArgs[1].data) == "extra2");
    auto target = pending->object.lock();
    assert(target == loginObj);

    // Consumed: take is destructive, matching real FluffOS clearing
    // interactive_t::input_to before invoking the callback.
    assert(!conn.hasPendingInputTo());

    ::close(fds[1]);
    std::cout << "testInputToRegistersPendingHandlerWithExtraArgsAndTargetObject OK\n";
}

static void testInputToNumericFlagArgumentIsSkippedNotTreatedAsExtraArg() {
    ObjectVarHarness harness;
    harness.writeFile("/login2.c",
        "void start() {\n"
        "    input_to(\"get_password\", 3, \"tail\");\n"
        "}\n"
        "void get_password(string str) {}\n");
    auto loginObj = harness.objects.cloneObject("/login2");
    assert(loginObj != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    lpcdriver::Connection conn(fds[0]);

    lpcdriver::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "start", {});
    lpcdriver::OutputContext::set(nullptr);

    auto pending = conn.takePendingInputTo();
    assert(pending.has_value());
    assert(pending->function == "get_password");
    // The int 3 is the echo-flag slot (real f_input_to()'s "arg[1].type
    // == T_NUMBER" check), not an extra arg -- only "tail" is carried
    // over.
    assert(pending->extraArgs.size() == 1);
    assert(std::get<std::string>(pending->extraArgs[0].data) == "tail");

    ::close(fds[1]);
    std::cout << "testInputToNumericFlagArgumentIsSkippedNotTreatedAsExtraArg OK\n";
}

static void testInputToReturnsZeroWithNoActiveConnection() {
    ObjectVarHarness harness;
    harness.writeFile("/login_noconn.c",
        "int start() { return input_to(\"get_name\"); }\n"
        "void get_name(string str) {}\n");
    auto loginObj = harness.objects.cloneObject("/login_noconn");
    assert(loginObj != nullptr);

    // No OutputContext set at all -- matches simulate.c's input_to():
    // "if (!command_giver || ...) return 0".
    lpcdriver::OutputContext::set(nullptr);
    lpcdriver::Value result = harness.vm.callFunction(loginObj, "start", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testInputToReturnsZeroWithNoActiveConnection OK\n";
}

static void testDispatchLinePrefersPendingInputToHandlerOverProcessInput() {
    ObjectVarHarness harness;
    harness.writeFile("/login3.c",
        "string lastCalled;\n"
        "string lastLine;\n"
        "void start() { input_to(\"get_name\"); }\n"
        "void get_name(string str) { lastCalled = \"get_name\"; lastLine = str; }\n"
        "void process_input(string str) { lastCalled = \"process_input\"; lastLine = str; }\n"
        "string query_last_called() { return lastCalled; }\n"
        "string query_last_line() { return lastLine; }\n");
    auto loginObj = harness.objects.cloneObject("/login3");
    assert(loginObj != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    lpcdriver::Connection conn(fds[0]);
    conn.attach(loginObj);

    lpcdriver::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "start", {}); // registers input_to("get_name")
    assert(conn.hasPendingInputTo());

    // comm.c's process_user_command(): call_function_interactive() (the
    // pending input_to handler) is checked and consumed first; only when
    // it was NOT pending does process_input() run for that line.
    lpcdriver::Server::dispatchLine(harness.vm, conn, "Bob");
    assert(!conn.hasPendingInputTo());

    lpcdriver::Value called = harness.vm.callFunction(loginObj, "query_last_called", {});
    assert(std::get<std::string>(called.data) == "get_name");
    lpcdriver::Value lineVal = harness.vm.callFunction(loginObj, "query_last_line", {});
    assert(std::get<std::string>(lineVal.data) == "Bob");

    // Second line: nothing pending this time, falls back to
    // process_input().
    lpcdriver::Server::dispatchLine(harness.vm, conn, "look");
    lpcdriver::OutputContext::set(nullptr);

    called = harness.vm.callFunction(loginObj, "query_last_called", {});
    assert(std::get<std::string>(called.data) == "process_input");
    lineVal = harness.vm.callFunction(loginObj, "query_last_line", {});
    assert(std::get<std::string>(lineVal.data) == "look");

    ::close(fds[1]);
    std::cout << "testDispatchLinePrefersPendingInputToHandlerOverProcessInput OK\n";
}

static void testInputToCanReRegisterFromWithinDispatchedHandler() {
    ObjectVarHarness harness;
    harness.writeFile("/login4.c",
        "string step;\n"
        "void start() { input_to(\"get_name\"); }\n"
        "void get_name(string str) { step = \"name:\" + str; input_to(\"get_password\"); }\n"
        "void get_password(string str) { step = \"password:\" + str; }\n"
        "string query_step() { return step; }\n");
    auto loginObj = harness.objects.cloneObject("/login4");
    assert(loginObj != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    lpcdriver::Connection conn(fds[0]);
    conn.attach(loginObj);

    lpcdriver::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "start", {});
    lpcdriver::Server::dispatchLine(harness.vm, conn, "bob");
    lpcdriver::OutputContext::set(nullptr);

    // get_name() called input_to("get_password") from inside its own
    // dispatched invocation -- comm.c's own comment on why input_to's
    // fields are cleared *before* the apply(): "someone might want to
    // set up a new input_to()". Must be the new handler, not left
    // clear or stuck on the old registration.
    assert(conn.hasPendingInputTo());
    auto pending = conn.takePendingInputTo();
    assert(pending->function == "get_password");

    lpcdriver::Value step = harness.vm.callFunction(loginObj, "query_step", {});
    assert(std::get<std::string>(step.data) == "name:bob");

    ::close(fds[1]);
    std::cout << "testInputToCanReRegisterFromWithinDispatchedHandler OK\n";
}

static void testLogonSendsBannerAndRegistersInputToPrompt() {
    ObjectVarHarness harness;
    harness.writeFile("/login5.c",
        "void logon() {\n"
        "    receive(\"Welcome!\\n\");\n"
        "    receive(\"Name? \");\n"
        "    input_to(\"get_name\");\n"
        "}\n"
        "void get_name(string str) {}\n");
    auto loginObj = harness.objects.cloneObject("/login5");
    assert(loginObj != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    lpcdriver::Connection conn(fds[0]);
    conn.attach(loginObj);

    // Mirrors Server::onNewConnection()'s own logon() call: zero
    // arguments, OutputContext set to the connection for the duration.
    lpcdriver::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "logon", {});
    lpcdriver::OutputContext::set(nullptr);

    assert(conn.hasPendingInputTo());
    auto pending = conn.takePendingInputTo();
    assert(pending->function == "get_name");

    char buf[256];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    std::string received(buf, static_cast<size_t>(n));
    assert(received == "Welcome!\nName? ");

    ::close(fds[1]);
    std::cout << "testLogonSendsBannerAndRegistersInputToPrompt OK\n";
}

static void testCallOutAcceptsRealArgumentShapeAndReturnsHandle() {
    ObjectVarHarness harness;
    harness.writeFile("/callout_probe.c",
        "int probe() { return call_out(\"idle\", 180); }\n"
        "void idle() {}\n");
    auto obj = harness.objects.cloneObject("/callout_probe");
    assert(obj != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testCallOutAcceptsRealArgumentShapeAndReturnsHandle OK\n";
}

// remove_call_out() -- always -1 (nothing found), matching real
// semantics honestly given call_out() above never actually schedules
// anything in this driver yet. Found live needing this: domains/Praxis/
// obj/mon/rift_survivor.c's own init().
static void testRemoveCallOutAlwaysReturnsMinusOneSinceNothingIsEverScheduled() {
    ObjectVarHarness harness;
    harness.writeFile("/rco_probe.c",
        "int probe() { return remove_call_out(\"idle\"); }\n"
        "void idle() {}\n");
    auto obj = harness.objects.cloneObject("/rco_probe");
    assert(obj != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == -1);
    std::cout << "testRemoveCallOutAlwaysReturnsMinusOneSinceNothingIsEverScheduled OK\n";
}

static void testCallOtherWithStringTargetResolvesAlreadyLoadedObject() {
    ObjectVarHarness harness;
    harness.writeFile("/daemon_a.c", "int ping() { return 42; }\n");
    auto daemon = harness.objects.loadObject("/daemon_a");
    assert(daemon != nullptr);

    harness.writeFile("/caller.c",
        "int probe() { return call_other(\"/daemon_a\", \"ping\"); }\n");
    auto caller = harness.objects.cloneObject("/caller");
    assert(caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);
    std::cout << "testCallOtherWithStringTargetResolvesAlreadyLoadedObject OK\n";
}

// real find_object() (simulate.c) compiles and loads a cache miss
// rather than only ever looking one up -- confirmed by reading its own
// body, not just the call_other() wrapper around it (see VM.hpp's
// findObject() comment for the full citation). A daemon file that was
// never explicitly preloaded or clone_object()'d still has to work the
// first time some other object reaches it via "SOME_D->func()", which
// is exactly how real master.c's own preload() forces a load with
// nothing more than "call_other(str, \"???\")".
static void testCallOtherWithStringTargetAutoCompilesAndLoadsOnFirstUse() {
    ObjectVarHarness harness;
    harness.writeFile("/daemon_b.c", "int ping() { return 7; }\n");

    harness.writeFile("/caller3.c",
        "int probe() { return call_other(\"/daemon_b\", \"ping\"); }\n");
    auto caller = harness.objects.cloneObject("/caller3");
    assert(caller != nullptr);

    // Nothing has loaded /daemon_b yet -- probe()'s own call_other() is
    // what should compile and load it, on demand.
    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 7);
    std::cout << "testCallOtherWithStringTargetAutoCompilesAndLoadsOnFirstUse OK\n";
}

static void testCallOtherWithStringTargetToNonexistentFileThrows() {
    ObjectVarHarness harness;
    harness.writeFile("/caller2.c",
        "int probe() { return call_other(\"/never_exists\", \"ping\"); }\n");
    auto caller = harness.objects.cloneObject("/caller2");
    assert(caller != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(caller, "probe", {});
    } catch (const lpcdriver::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("couldn't find object") != std::string::npos);
    }
    assert(threw);
    std::cout << "testCallOtherWithStringTargetToNonexistentFileThrows OK\n";
}

static void testConvertNameMudlibFunctionWorksWithNewLowerCaseAndReplaceStringEfuns() {
    // The real secure/SimulEfun/convert_name.c body, verbatim, run
    // against this driver's own lower_case()/replace_string() efuns
    // rather than the real simul_efun resolution chain (which is
    // already covered by testSimulEfunResolvesUnknownBareCallToSimulEfunObject
    // elsewhere in this file) -- this test is specifically about the two
    // new string efuns convert_name() depends on.
    ObjectVarHarness harness;
    harness.writeFile("/convert_name_probe.c",
        "string convert_name(string str) {\n"
        "    str = replace_string(str, \" \", \"\");\n"
        "    str = replace_string(str, \"'\", \"\");\n"
        "    return lower_case(replace_string(str, \"-\", \"\"));\n"
        "}\n"
        "string probe() { return convert_name(\"O'Brien Smith-Jones\"); }\n");
    auto obj = harness.objects.cloneObject("/convert_name_probe");
    assert(obj != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "obriensmithjones");
    std::cout << "testConvertNameMudlibFunctionWorksWithNewLowerCaseAndReplaceStringEfuns OK\n";
}

// ---------------------------------------------------------------------
// "::name(...)" / "qualifier::name(...)" -- explicit inherited-function
// calls (grammar.y's function_name production; found live compiling
// secure/daemon/account_d.c's own "::create();", the parser gap the
// live login-sequence test hit right after the connect/input protocol
// work above).
// ---------------------------------------------------------------------

static void testBareParentCallInvokesInheritedFunctionNotLocalOverride() {
    ObjectVarHarness harness;
    harness.writeFile("/parent1.c",
        "string tag;\n"
        "void create() { tag = \"parent\"; }\n");
    harness.writeFile("/child1.c",
        "inherit \"/parent1\";\n"
        "void create() {\n"
        "    tag = \"child\";\n"
        "    ::create();\n" // must run parent1's create(), not recurse into this one
        "}\n"
        "string query_tag() { return tag; }\n");
    auto obj = harness.objects.cloneObject("/child1");
    assert(obj != nullptr);

    // child1's own create() ran (tag started as "child"), then ::create()
    // overwrote it with parent1's own "parent" -- proves the inherited
    // definition ran, not an infinite/self recursion into child1's own
    // create() (which real LPC's "::" syntax exists specifically to
    // avoid).
    lpcdriver::Value result = harness.vm.callFunction(obj, "query_tag", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "parent");
    std::cout << "testBareParentCallInvokesInheritedFunctionNotLocalOverride OK\n";
}

static void testQualifiedParentCallMatchesInheritPathBasename() {
    ObjectVarHarness harness;
    harness.writeFile("/daemon.c",
        "string source() { return \"daemon\"; }\n");
    harness.writeFile("/other_parent.c",
        "string source() { return \"other_parent\"; }\n");
    harness.writeFile("/child2.c",
        "inherit \"/daemon\";\n"
        "inherit \"/other_parent\";\n"
        "string probe() { return daemon::source(); }\n");
    auto obj = harness.objects.cloneObject("/child2");
    assert(obj != nullptr);

    // Two inherited parents both define source() -- the qualifier picks
    // the one whose own inherit path's basename is "daemon", not
    // whichever happens to be searched first (real
    // secure/daemon/banish.c's own "daemon::create();" shape).
    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "daemon");
    std::cout << "testQualifiedParentCallMatchesInheritPathBasename OK\n";
}

static void testParentCallOnFileWithNoInheritThrows() {
    ObjectVarHarness harness;
    harness.writeFile("/lonely.c",
        "void create() { ::create(); }\n");
    auto obj = harness.objects.cloneObject("/lonely");
    // ObjectManager::cloneObject() catches create()'s own runtime error
    // and returns null, the same "one object's runtime error is not a
    // crash" guarantee used everywhere else in this driver.
    assert(obj == nullptr);
    std::cout << "testParentCallOnFileWithNoInheritThrows OK\n";
}

// ---------------------------------------------------------------------
// "(: name, bound_args... :)" closure literals -- see Value.hpp's
// Closure comment and STATUS.md's closure recon notes for the real-
// source citations and why only the bare-identifier form (confirmed as
// the only shape this mudlib's boot/login/account-creation path
// actually uses) is implemented.
// ---------------------------------------------------------------------

static void testClosureLiteralParsesToClosureLiteralExprBareForm() {
    std::string src =
        "void probe() {\n"
        "    unguarded((: file_size :));\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* call = dynamic_cast<lpcdriver::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr);
    assert(call->args.size() == 1);
    auto* closure = dynamic_cast<lpcdriver::ClosureLiteralExpr*>(call->args[0].get());
    assert(closure != nullptr);
    assert(closure->functionName == "file_size");
    assert(closure->boundArgs.empty());

    std::cout << "testClosureLiteralParsesToClosureLiteralExprBareForm OK\n";
}

static void testClosureLiteralParsesToClosureLiteralExprWithBoundArgs() {
    std::string src =
        "void probe(string p) {\n"
        "    unguarded((: file_size, p, \"extra\" :));\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    auto* call = dynamic_cast<lpcdriver::CallExpr*>(exprStmt->expr.get());
    auto* closure = dynamic_cast<lpcdriver::ClosureLiteralExpr*>(call->args[0].get());
    assert(closure != nullptr);
    assert(closure->functionName == "file_size");
    assert(closure->boundArgs.size() == 2);
    auto* firstArg = dynamic_cast<lpcdriver::VarRefExpr*>(closure->boundArgs[0].get());
    assert(firstArg != nullptr && firstArg->name == "p");
    auto* secondArg = dynamic_cast<lpcdriver::StringLiteral*>(closure->boundArgs[1].get());
    assert(secondArg != nullptr && secondArg->value == "extra");

    std::cout << "testClosureLiteralParsesToClosureLiteralExprWithBoundArgs OK\n";
}

static void testClosureLiteralVmExecutionProducesClosureValueWithOwnerAndBoundArgs() {
    std::string src =
        "mixed make_closure(string p) {\n"
        "    return (: file_size, p :);\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "make_closure", {lpcdriver::Value(std::string("/some/path"))});
    auto* closurePtr = std::get_if<std::shared_ptr<lpcdriver::Closure>>(&result.data);
    assert(closurePtr != nullptr && *closurePtr != nullptr);
    assert((*closurePtr)->functionName == "file_size");
    assert((*closurePtr)->boundArgs.size() == 1);
    assert(std::get<std::string>((*closurePtr)->boundArgs[0].data) == "/some/path");
    // Owner is whatever object was executing when the literal ran.
    assert((*closurePtr)->owner.lock() == obj);

    std::cout << "testClosureLiteralVmExecutionProducesClosureValueWithOwnerAndBoundArgs OK\n";
}

static void testEvaluateInvokesEfunBoundClosureWithBoundArgsBeforeExtraArgs() {
    // evaluate()'s own extra argument is appended *after* the closure's
    // own bound args (real merge_arg_lists() order, see Value.hpp's
    // Closure comment) -- built here with replace_string, whose first
    // two args (str, pattern) are bound at construction time and whose
    // third (replacement) is only supplied at evaluate()-call time, so
    // this only produces the expected string if bound-then-extra
    // ordering is correct.
    std::string src =
        "string probe() {\n"
        "    mixed f;\n"
        "    f = (: replace_string, \"hello world\", \"world\" :);\n"
        "    return evaluate(f, \"there\");\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "hello there");

    std::cout << "testEvaluateInvokesEfunBoundClosureWithBoundArgsBeforeExtraArgs OK\n";
}

static void testFuncallIsAnAliasOfEvaluate() {
    std::string src =
        "string probe() {\n"
        "    return funcall((: lower_case, \"ABC\" :));\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "abc");

    std::cout << "testFuncallIsAnAliasOfEvaluate OK\n";
}

static void testEvaluateOnNonFunctionValueIsSilentNoOp() {
    // Real f__evaluate(): a non-T_FUNCTION first argument pops its args
    // and returns with no error, not a type-mismatch throw.
    std::string src =
        "mixed probe() {\n"
        "    return evaluate(\"not a closure\");\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "probe", {});
    assert(result.isVoid());

    std::cout << "testEvaluateOnNonFunctionValueIsSilentNoOp OK\n";
}

static void testEvaluateInvokesLocalFunctionBoundClosure() {
    std::string src =
        "int target(int x) { return x * 2; }\n"
        "int probe() {\n"
        "    mixed f;\n"
        "    f = (: target, 21 :);\n"
        "    return evaluate(f);\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testEvaluateInvokesLocalFunctionBoundClosure OK\n";
}

static void testEvaluateThrowsWhenClosureOwnerDestructed() {
    ObjectVarHarness harness;
    harness.writeFile("/owner_dies.c",
        "mixed make_closure() { return (: file_size, \"/x\" :); }\n");
    auto owner = harness.objects.cloneObject("/owner_dies");
    assert(owner != nullptr);

    lpcdriver::Value closureVal = harness.vm.callFunction(owner, "make_closure", {});
    auto closurePtr = std::get<std::shared_ptr<lpcdriver::Closure>>(closureVal.data);
    assert(closurePtr != nullptr);

    owner.reset(); // only the weak_ptr in the closure should be left

    bool threw = false;
    try {
        harness.vm.callClosure(closurePtr, {});
    } catch (const lpcdriver::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("destructed") != std::string::npos);
    }
    assert(threw);

    std::cout << "testEvaluateThrowsWhenClosureOwnerDestructed OK\n";
}

static void testCallOutAcceptsClosureAsFirstArgument() {
    ObjectVarHarness harness;
    harness.writeFile("/callout_closure_probe.c",
        "void idle() {}\n"
        "int probe() { return call_out((: idle :), 5); }\n");
    auto obj = harness.objects.cloneObject("/callout_closure_probe");
    assert(obj != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testCallOutAcceptsClosureAsFirstArgument OK\n";
}

static void testPreviousObjectReturnsCallerAcrossCallOther() {
    ObjectVarHarness harness;
    harness.writeFile("/callee.c",
        "object probe() { return previous_object(0); }\n");
    harness.writeFile("/caller.c",
        "object run() { return call_other(\"/callee\", \"probe\"); }\n");
    auto callerObj = harness.objects.cloneObject("/caller");
    assert(callerObj != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(callerObj, "run", {});
    auto* obPtr = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&result.data);
    assert(obPtr != nullptr && *obPtr != nullptr);
    assert(*obPtr == callerObj);

    std::cout << "testPreviousObjectReturnsCallerAcrossCallOther OK\n";
}

static void testPreviousObjectDoesNotChangeAcrossSameObjectLocalCall() {
    // A bare same-object call (no call_other/simul_efun crossing) must
    // NOT push a new previous_object() frame -- real FluffOS only does
    // so for an actual FRAME_OB_CHANGE. inner() here is called directly
    // (not via call_other) from within outer(), which was itself
    // reached via call_other from caller2.c -- previous_object(0) from
    // inside inner() must still be caller2.c's own object, not this
    // object itself.
    ObjectVarHarness harness;
    harness.writeFile("/callee2.c",
        "object inner() { return previous_object(0); }\n"
        "object outer() { return inner(); }\n");
    harness.writeFile("/caller2.c",
        "object run() { return call_other(\"/callee2\", \"outer\"); }\n");
    auto callerObj = harness.objects.cloneObject("/caller2");
    assert(callerObj != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(callerObj, "run", {});
    auto* obPtr = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&result.data);
    assert(obPtr != nullptr && *obPtr != nullptr);
    assert(*obPtr == callerObj);

    std::cout << "testPreviousObjectDoesNotChangeAcrossSameObjectLocalCall OK\n";
}

static void testPreviousObjectMinusOneReturnsFullChain() {
    ObjectVarHarness harness;
    harness.writeFile("/level_c.c",
        "mixed *probe() { return previous_object(-1); }\n");
    harness.writeFile("/level_b.c",
        "mixed *run() { return call_other(\"/level_c\", \"probe\"); }\n");
    harness.writeFile("/level_a.c",
        "mixed *run() { return call_other(\"/level_b\", \"run\"); }\n");
    auto a = harness.objects.cloneObject("/level_a");
    assert(a != nullptr);
    auto b = harness.objects.loadObject("/level_b");
    assert(b != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(a, "run", {});
    auto* arrPtr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&result.data);
    assert(arrPtr != nullptr && *arrPtr != nullptr);
    // Nearest first: whoever called level_c's probe() (level_b's own
    // loaded singleton), then whoever called level_b's run() (level_a).
    assert((*arrPtr)->items.size() == 2);
    assert(std::get<std::shared_ptr<lpcdriver::LpcObject>>((*arrPtr)->items[0].data) == b);
    assert(std::get<std::shared_ptr<lpcdriver::LpcObject>>((*arrPtr)->items[1].data) == a);

    std::cout << "testPreviousObjectMinusOneReturnsFullChain OK\n";
}

static void testUnguardedClosureRoundTripsThroughSecurityAndMasterShape() {
    // Mirrors the real secure/SimulEfun/security.c unguarded()/
    // master.c apply_unguarded() shape (trimmed to just what those two
    // files actually need for this), proving the closure mechanism,
    // evaluate(), and previous_object() all compose correctly through a
    // real call_other hop -- the same chain that blocked
    // secure/daemon/account_d.c live (see STATUS.md's closure recon and
    // "next real blocker" notes).
    ObjectVarHarness harness;
    harness.writeFile("/master_probe.c",
        "mixed apply_unguarded(mixed f) {\n"
        "    object caller;\n"
        "    caller = previous_object(0);\n"
        "    if(!caller) return \"no caller\";\n"
        "    return evaluate(f);\n"
        "}\n");
    harness.writeFile("/security_probe.c",
        "mixed unguarded(mixed f) { return call_other(\"/master_probe\", \"apply_unguarded\", f); }\n");
    harness.writeFile("/account_probe.c",
        "int probe(string p) {\n"
        "    return (int)call_other(\"/security_probe\", \"unguarded\", (: file_size, p :));\n"
        "}\n");

    auto accountObj = harness.objects.cloneObject("/account_probe");
    assert(accountObj != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(accountObj, "probe", {lpcdriver::Value(std::string("/does_not_exist"))});
    assert(std::holds_alternative<int64_t>(result.data));
    // file_size() on a nonexistent path: -1, proving the closure really
    // did get invoked (not just "wrong caller" or a thrown error).
    assert(std::get<int64_t>(result.data) == -1);

    std::cout << "testUnguardedClosureRoundTripsThroughSecurityAndMasterShape OK\n";
}

static void testSaveObjectRestoreObjectRoundTripsNestedMappingsAndArrays() {
    // save_object()/restore_object()'s recursive serializer (see
    // EfunTable.cpp's serializeValue()/deserializeValue()), found live
    // needing to grow past a flat int/float/string/string-array set
    // when daemon/banish.c's own create() saved a mapping object
    // variable (__TmpBanish) and this driver's earlier, non-recursive
    // save format threw "unsupported value kind" for it.
    ObjectVarHarness harness;
    harness.writeFile("/save_probe.c",
        "int n;\n"
        "string s;\n"
        "mapping m;\n"
        "mixed *nested;\n"
        "void create() {\n"
        "    n = 42;\n"
        "    s = \"hello\";\n"
        "    m = ([ \"a\": 1, \"b\": ({ \"x\", \"y\" }) ]);\n"
        "    nested = ({ 1, \"two\", ({ 3, 4 }) });\n"
        "}\n"
        "int save() { return save_object(\"/probe.o\"); }\n"
        "void clear() { n = 0; s = 0; m = 0; nested = 0; }\n"
        "int load() { return restore_object(\"/probe.o\"); }\n"
        "int query_n() { return n; }\n"
        "string query_s() { return s; }\n"
        "int query_m_a() { return m[\"a\"]; }\n"
        "string query_m_b_1() { return m[\"b\"][1]; }\n"
        "mixed *query_nested() { return nested; }\n");
    auto obj = harness.objects.cloneObject("/save_probe");
    assert(obj != nullptr);

    lpcdriver::Value saveResult = harness.vm.callFunction(obj, "save", {});
    assert(std::holds_alternative<int64_t>(saveResult.data));
    assert(std::get<int64_t>(saveResult.data) == 1);

    harness.vm.callFunction(obj, "clear", {});
    lpcdriver::Value nAfterClear = harness.vm.callFunction(obj, "query_n", {});
    assert(std::holds_alternative<int64_t>(nAfterClear.data));
    assert(std::get<int64_t>(nAfterClear.data) == 0);

    lpcdriver::Value loadResult = harness.vm.callFunction(obj, "load", {});
    assert(std::holds_alternative<int64_t>(loadResult.data));
    assert(std::get<int64_t>(loadResult.data) == 1);

    lpcdriver::Value n = harness.vm.callFunction(obj, "query_n", {});
    assert(std::get<int64_t>(n.data) == 42);
    lpcdriver::Value s = harness.vm.callFunction(obj, "query_s", {});
    assert(std::get<std::string>(s.data) == "hello");
    lpcdriver::Value mA = harness.vm.callFunction(obj, "query_m_a", {});
    assert(std::get<int64_t>(mA.data) == 1);
    lpcdriver::Value mB1 = harness.vm.callFunction(obj, "query_m_b_1", {});
    assert(std::get<std::string>(mB1.data) == "y");
    lpcdriver::Value nested = harness.vm.callFunction(obj, "query_nested", {});
    auto* nestedArr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&nested.data);
    assert(nestedArr != nullptr && *nestedArr != nullptr);
    assert((*nestedArr)->items.size() == 3);
    assert(std::get<int64_t>((*nestedArr)->items[0].data) == 1);
    assert(std::get<std::string>((*nestedArr)->items[1].data) == "two");
    auto* innerArr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&(*nestedArr)->items[2].data);
    assert(innerArr != nullptr && *innerArr != nullptr);
    assert((*innerArr)->items.size() == 2);
    assert(std::get<int64_t>((*innerArr)->items[0].data) == 3);
    assert(std::get<int64_t>((*innerArr)->items[1].data) == 4);

    std::cout << "testSaveObjectRestoreObjectRoundTripsNestedMappingsAndArrays OK\n";
}

static void testEvaluateOfEfunBoundClosureSetsCurrentObjectToClosureOwnerNotCaller() {
    // Regression test for a real bug found live: secure/daemon/
    // account_d.c's own "unguarded((: save_object, path :))" chain
    // (account_d.c -> security.c's unguarded() -> master.c's
    // apply_unguarded() -> evaluate(f)) saved *master.c's own*
    // variables instead of account_d.c's, because VM::callClosure()'s
    // core-efun branch called straight into EfunTable without updating
    // vm.currentObject() -- save_object() (like real FluffOS's own
    // f_save_object(), confirmed against efuns_main.c: "save_object
    // (current_object, ...)") saves whatever object is "current" at
    // the moment it actually runs, which must be the closure's own
    // owner (real setup_fake_frame(): "current_object = fun->hdr.
    // owner"), not whichever object's call frame happens to be
    // innermost when evaluate() itself is invoked. Mirrors the real
    // three-hop shape closely enough to have caught this: middleman.c
    // stands in for master.c's apply_unguarded(), calling evaluate() on
    // a closure it did not construct.
    ObjectVarHarness harness;
    harness.writeFile("/owner_probe.c",
        "int marker;\n"
        "void create() { marker = 99; }\n"
        "mixed make_closure() { return (: save_object, \"/owner_probe.o\" :); }\n"
        "int query_marker() { return marker; }\n");
    harness.writeFile("/middleman_probe.c",
        "int marker;\n"
        "void create() { marker = -1; }\n" // must NOT be what gets saved
        "mixed run(mixed f) { return evaluate(f); }\n");
    auto owner = harness.objects.cloneObject("/owner_probe");
    assert(owner != nullptr);
    auto middleman = harness.objects.cloneObject("/middleman_probe");
    assert(middleman != nullptr);

    lpcdriver::Value closureVal = harness.vm.callFunction(owner, "make_closure", {});
    auto closurePtr = std::get<std::shared_ptr<lpcdriver::Closure>>(closureVal.data);
    assert(closurePtr != nullptr);

    // middleman calls evaluate() on a closure it did not build --
    // vm.currentObject() during save_object()'s own execution must
    // still be "owner", not "middleman".
    lpcdriver::Value saveResult = harness.vm.callFunction(middleman, "run", {closureVal});
    assert(std::holds_alternative<int64_t>(saveResult.data));
    assert(std::get<int64_t>(saveResult.data) == 1);

    // Read back what actually landed on disk through a third, unrelated
    // object with the same single "marker" variable layout: confirms
    // the *content* saved was owner's own 99, not middleman's -1 (a
    // check via owner's own variables() wouldn't prove this -- it never
    // changed in memory either way).
    harness.writeFile("/restore_probe.c",
        "int marker;\n"
        "int load() { return restore_object(\"/owner_probe.o\"); }\n"
        "int query_marker() { return marker; }\n");
    auto restoreObj = harness.objects.cloneObject("/restore_probe");
    assert(restoreObj != nullptr);
    lpcdriver::Value loaded = harness.vm.callFunction(restoreObj, "load", {});
    assert(std::holds_alternative<int64_t>(loaded.data));
    assert(std::get<int64_t>(loaded.data) == 1);
    lpcdriver::Value marker = harness.vm.callFunction(restoreObj, "query_marker", {});
    assert(std::holds_alternative<int64_t>(marker.data));
    assert(std::get<int64_t>(marker.data) == 99);

    std::cout << "testEvaluateOfEfunBoundClosureSetsCurrentObjectToClosureOwnerNotCaller OK\n";
}

// ---------------------------------------------------------------------
// master()->compile_object() -- virtual objects (real int_load_object()'s
// own fallback when a load_object()/find_object() path has no matching
// ".c" file on disk, see simulate.c's load_virtual_object() and
// ObjectManager::loadVirtualObject()'s own citation).
// ---------------------------------------------------------------------

static void testLoadObjectFallsBackToCompileObjectOnMissingSourceFile() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "int calls;\n"
        "void create() { calls = 0; }\n"
        "object compile_object(string str) {\n"
        "    calls = calls + 1;\n"
        "    return clone_object(\"/player_class\");\n"
        "}\n"
        "int query_calls() { return calls; }\n");
    harness.writeFile("/player_class.c",
        "int marker;\n"
        "void create() { marker = 7; }\n"
        "int query_marker() { return marker; }\n");
    bool masterLoaded = harness.objects.loadMasterObject();
    assert(masterLoaded);

    // No "/secure/save/users/t/testchar.c" exists anywhere -- this must
    // fall back to compile_object() rather than just failing.
    auto ob = harness.objects.loadObject("/secure/save/users/t/testchar");
    assert(ob != nullptr);

    lpcdriver::Value marker = harness.vm.callFunction(ob, "query_marker", {});
    assert(std::holds_alternative<int64_t>(marker.data));
    assert(std::get<int64_t>(marker.data) == 7);

    std::cout << "testLoadObjectFallsBackToCompileObjectOnMissingSourceFile OK\n";
}

static void testLoadVirtualObjectRebindsFilenameToVirtualPath() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "object compile_object(string str) { return clone_object(\"/player_class2\"); }\n");
    harness.writeFile("/player_class2.c", "void create() {}\n");
    assert(harness.objects.loadMasterObject());

    auto ob = harness.objects.loadObject("/secure/save/users/z/zed");
    assert(ob != nullptr);
    // Real load_virtual_object(): the returned clone is renamed to the
    // requested virtual path, not left as "/player_class2".
    assert(ob->filename() == "/secure/save/users/z/zed");

    std::cout << "testLoadVirtualObjectRebindsFilenameToVirtualPath OK\n";
}

static void testLoadObjectCachesVirtualObjectAcrossRepeatedCalls() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "int calls;\n"
        "void create() { calls = 0; }\n"
        "object compile_object(string str) {\n"
        "    calls = calls + 1;\n"
        "    return clone_object(\"/player_class3\");\n"
        "}\n"
        "int query_calls() { return calls; }\n");
    harness.writeFile("/player_class3.c", "void create() {}\n");
    assert(harness.objects.loadMasterObject());
    auto master = harness.objects.masterObject();

    auto first = harness.objects.loadObject("/secure/save/users/q/quux");
    assert(first != nullptr);
    auto second = harness.objects.loadObject("/secure/save/users/q/quux");
    assert(second != nullptr);
    assert(first == second); // same cached object, not a fresh compile_object() call

    lpcdriver::Value calls = harness.vm.callFunction(master, "query_calls", {});
    assert(std::holds_alternative<int64_t>(calls.data));
    assert(std::get<int64_t>(calls.data) == 1);

    std::cout << "testLoadObjectCachesVirtualObjectAcrossRepeatedCalls OK\n";
}

static void testLoadObjectReturnsNullWhenCompileObjectDeclines() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int compile_object(string str) { return 0; }\n"); // declines, like real master.c's own fallthrough
    assert(harness.objects.loadMasterObject());

    auto ob = harness.objects.loadObject("/does/not/exist/at/all");
    assert(ob == nullptr);

    std::cout << "testLoadObjectReturnsNullWhenCompileObjectDeclines OK\n";
}

// Real simulate.c's own init_privs_for_object(): every freshly compiled
// or cloned object gets query_privs() populated from
// master()->privs_file(filename) automatically, with no set_privs()
// call needed from the object's own code. Found live needing this:
// secure/SimulEfun/log_file.c's own "explode(query_privs(
// previous_object()), \":\")" throws when an object's privs were never
// assigned at all (this driver had no equivalent of init_privs_for_object
// before this fix -- see ObjectManager.hpp's own comment).
static void testLoadObjectAndCloneObjectAutoPopulatePrivsFromMasterPrivsFile() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "string privs_file(string file) {\n"
        "    if(file == \"/priv_item\") return \"TestPriv\";\n"
        "    return 0;\n"
        "}\n");
    harness.writeFile("/priv_item.c",
        "string probe() { return query_privs(this_object()); }\n");
    assert(harness.objects.loadMasterObject());

    auto loaded = harness.objects.loadObject("/priv_item");
    assert(loaded != nullptr);
    lpcdriver::Value loadedPrivs = harness.vm.callFunction(loaded, "probe", {});
    auto* loadedStr = std::get_if<std::string>(&loadedPrivs.data);
    assert(loadedStr != nullptr && *loadedStr == "TestPriv");

    auto cloned = harness.objects.cloneObject("/priv_item");
    assert(cloned != nullptr);
    lpcdriver::Value clonedPrivs = harness.vm.callFunction(cloned, "probe", {});
    auto* clonedStr = std::get_if<std::string>(&clonedPrivs.data);
    assert(clonedStr != nullptr && *clonedStr == "TestPriv");

    std::cout << "testLoadObjectAndCloneObjectAutoPopulatePrivsFromMasterPrivsFile OK\n";
}

// explode(string, string) -- confirmed against fluffos-2.9-ds2.08's own
// array.c explode_string() and this exact vendored reference's own
// options.h (neither REVERSIBLE_EXPLODE_STRING nor SANE_EXPLODE_STRING
// defined): all leading separator occurrences are stripped before
// splitting, and a trailing separator never produces a trailing "".
// Found live root-causing why secure/SimulEfun/security.c's own
// file_privs() never matched any switch(path[0]) case for a real
// "/domains/..." object path.
static void testExplodeStripsAllLeadingSeparatorsAndNeverEmitsTrailingEmpty() {
    ObjectVarHarness harness;
    harness.writeFile("/ex_probe.c",
        "mixed *leading(string s) { return explode(s, \"/\"); }\n"
        "mixed *trailing(string s) { return explode(s, \"\\n\"); }\n"
        "mixed *middle(string s) { return explode(s, \"/\"); }\n"
        "mixed *all_seps(string s) { return explode(s, \"/\"); }\n");
    auto ob = harness.objects.cloneObject("/ex_probe");
    assert(ob != nullptr);

    // Leading "/" stripped entirely -- path[0] must be "domains", not "".
    lpcdriver::Value leading = harness.vm.callFunction(ob, "leading",
        {lpcdriver::Value(std::string("/domains/Praxis/rift_survivor"))});
    auto* leadingArr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&leading.data);
    assert(leadingArr != nullptr && (*leadingArr)->items.size() == 3);
    assert(std::get<std::string>((*leadingArr)->items[0].data) == "domains");
    assert(std::get<std::string>((*leadingArr)->items[1].data) == "Praxis");
    assert(std::get<std::string>((*leadingArr)->items[2].data) == "rift_survivor");

    // Trailing separator: no spurious trailing "" element.
    lpcdriver::Value trailing = harness.vm.callFunction(ob, "trailing",
        {lpcdriver::Value(std::string("line one\nline two\n"))});
    auto* trailingArr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&trailing.data);
    assert(trailingArr != nullptr && (*trailingArr)->items.size() == 2);
    assert(std::get<std::string>((*trailingArr)->items[1].data) == "line two");

    // A separator in the middle still produces an empty element there --
    // only LEADING runs are collapsed, nothing in the middle is special.
    lpcdriver::Value middle = harness.vm.callFunction(ob, "middle",
        {lpcdriver::Value(std::string("a//b"))});
    auto* middleArr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&middle.data);
    assert(middleArr != nullptr && (*middleArr)->items.size() == 3);
    assert(std::get<std::string>((*middleArr)->items[1].data) == "");

    // A string made entirely of the separator explodes to an empty array.
    lpcdriver::Value allSeps = harness.vm.callFunction(ob, "all_seps",
        {lpcdriver::Value(std::string("///"))});
    auto* allSepsArr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&allSeps.data);
    assert(allSepsArr != nullptr && (*allSepsArr)->items.empty());

    std::cout << "testExplodeStripsAllLeadingSeparatorsAndNeverEmitsTrailingEmpty OK\n";
}

static void testLoadObjectWithNoMasterLoadedFailsCleanlyOnMissingFile() {
    ObjectVarHarness harness;
    // loadMasterObject() deliberately not called -- master_ stays null,
    // matching real load_virtual_object()'s own "if (!master_ob) ...
    // return 0" guard (e.g. this call is itself the attempt to load the
    // master file).
    auto ob = harness.objects.loadObject("/nothing/here");
    assert(ob == nullptr);

    std::cout << "testLoadObjectWithNoMasterLoadedFailsCleanlyOnMissingFile OK\n";
}

static void testNewEfunIsAnAliasOfCloneObject() {
    ObjectVarHarness harness;
    harness.writeFile("/aliased_target.c",
        "int marker;\n"
        "void create() { marker = 55; }\n"
        "int query_marker() { return marker; }\n");
    harness.writeFile("/new_probe.c",
        "object probe() { return new(\"/aliased_target\"); }\n");
    auto caller = harness.objects.cloneObject("/new_probe");
    assert(caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {});
    auto* obPtr = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&result.data);
    assert(obPtr != nullptr && *obPtr != nullptr);

    lpcdriver::Value marker = harness.vm.callFunction(*obPtr, "query_marker", {});
    assert(std::holds_alternative<int64_t>(marker.data));
    assert(std::get<int64_t>(marker.data) == 55);

    std::cout << "testNewEfunIsAnAliasOfCloneObject OK\n";
}

static void testStatusTypeKeywordParsesAsPlainIntSynonym() {
    // Real lex.c: "{\"status\", L_BASIC_TYPE, TYPE_NUMBER}" -- found
    // live compiling std/user.c's own "static status snoop, earmuffs;"
    // while resolving master()->compile_object()'s own "new(OB_USER)".
    // No CodeGen/VM distinction from "int" is needed (see Lexer.cpp's
    // own comment); this just confirms the parser accepts it as an
    // object-var type and a local-var type, and that the resulting
    // variable behaves as a plain int.
    std::string src =
        "status flag;\n"
        "void set_it() { flag = 1; }\n"
        "int get_it() {\n"
        "    status local;\n"
        "    local = flag + 1;\n"
        "    return local;\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    vm.callFunction(obj, "set_it", {});
    lpcdriver::Value result = vm.callFunction(obj, "get_it", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 2);

    std::cout << "testStatusTypeKeywordParsesAsPlainIntSynonym OK\n";
}

static void testFunctionDeclWithOnlyModifiersAndNoTypeParses() {
    // Real LPC allows the return type to be omitted entirely when only
    // modifiers precede a function's name (historically implicit
    // "mixed") -- found live compiling std/user.c's own "private static
    // register_channels();" (prototype) and "static private
    // register_channels() { ... }" (definition), neither naming a
    // return type. Confirms both the prototype-only form and the real
    // definition parse and execute, and that modifier order does not
    // matter (matching the existing bare-type modifier-order tests).
    std::string src =
        "private static probe();\n"
        "static private probe() { return 5; }\n"
        "int call_it() { return probe(); }\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "call_it", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 5);

    std::cout << "testFunctionDeclWithOnlyModifiersAndNoTypeParses OK\n";
}

static void testBareBlockStatementScopesLocalsAndExecutesInline() {
    // A standalone "{ ... }" block statement, not attached to any if/
    // while/for -- real, standard LPC/C syntax used purely for local-
    // variable scoping. Found live compiling std/user.c's own quit()-
    // adjacent cleanup code. Confirms it parses as a statement (not
    // "expected expression"), that locals declared inside it are
    // usable within it, and that execution continues normally with
    // statements before and after it in the same function body.
    std::string src =
        "int probe() {\n"
        "    int before;\n"
        "    before = 1;\n"
        "    {\n"
        "        int inner;\n"
        "        inner = 41;\n"
        "        before = before + inner;\n"
        "    }\n"
        "    return before;\n"
        "}\n";
    auto obj = compileProgramObject(src);

    lpcdriver::Config config;
    lpcdriver::ObjectManager objects(config);
    lpcdriver::VM vm(objects, config);

    lpcdriver::Value result = vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testBareBlockStatementScopesLocalsAndExecutesInline OK\n";
}

// ---------------------------------------------------------------------
// "<N" from-the-end indexing (real LPC: grammar.y's "expr4 '[' '<'
// comma_expr ...", confirmed against eoperators.c's f_range()/
// f_extract_range() -- see Ast.hpp's IndexExpr comment). Found live
// compiling std/user.c's own "files[j][<2..]".
// ---------------------------------------------------------------------

static void testFromEndSingleIndexOnStringAndArray() {
    lpcdriver::Value strResult = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[<1];\n"); // last char: 'o' (111)
    assert(std::holds_alternative<int64_t>(strResult.data));
    assert(std::get<int64_t>(strResult.data) == 'o');

    lpcdriver::Value arrResult = runProbe(
        "mixed *items;\n"
        "items = ({ 10, 20, 30 });\n"
        "return items[<2];\n"); // second-to-last: 20
    assert(std::holds_alternative<int64_t>(arrResult.data));
    assert(std::get<int64_t>(arrResult.data) == 20);

    std::cout << "testFromEndSingleIndexOnStringAndArray OK\n";
}

static void testFromEndOpenRangeMatchesRealUserCShape() {
    // Mirrors std/user.c's own "files[j][<2..] != \".o\"" shape exactly:
    // the last 2 characters of a string.
    lpcdriver::Value result = runProbe(
        "string name;\n"
        "name = \"testchar.o\";\n"
        "return name[<2..];\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == ".o");

    std::cout << "testFromEndOpenRangeMatchesRealUserCShape OK\n";
}

static void testFromEndBothBoundsOnRangeIndex() {
    // "arr[<a..<b]" -- both bounds counted from the end.
    lpcdriver::Value result = runProbe(
        "mixed *items;\n"
        "items = ({ 1, 2, 3, 4, 5 });\n"
        "return items[<4..<2];\n"); // indices 1..3 -> ({2,3,4})
    auto* arr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&result.data);
    assert(arr != nullptr && *arr != nullptr);
    assert((*arr)->items.size() == 3);
    assert(std::get<int64_t>((*arr)->items[0].data) == 2);
    assert(std::get<int64_t>((*arr)->items[1].data) == 3);
    assert(std::get<int64_t>((*arr)->items[2].data) == 4);

    std::cout << "testFromEndBothBoundsOnRangeIndex OK\n";
}

static void testFromEndStartBeyondLengthClampsToZeroInsteadOfThrowing() {
    // real eoperators.c: "if (from < 0) from = 0;" once "<N" has
    // already been resolved against the target's own length -- a
    // legitimate outcome when N is at least the target's length, not a
    // caller mistake the way a bare negative literal (no "<") is
    // (see testStringRangeIndexNegativeStartThrows, unaffected by this).
    lpcdriver::Value result = runProbe(
        "string s;\n"
        "s = \"hi\";\n"
        "return s[<10..];\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "hi");

    std::cout << "testFromEndStartBeyondLengthClampsToZeroInsteadOfThrowing OK\n";
}

// ---------------------------------------------------------------------
// Compound indexed assignment ("target[index] += value" etc). Found
// live compiling std/user.c's own "player_data[\"general\"]
// [\"quest points\"] += (int)call_other(...)" -- confirms both a
// single-level and a chained/nested mapping target, matching that real
// shape.
// ---------------------------------------------------------------------

static void testCompoundIndexAssignOnSingleLevelMapping() {
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "m = ([ \"a\": 10 ]);\n"
        "m[\"a\"] += 5;\n"
        "return m[\"a\"];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 15);

    std::cout << "testCompoundIndexAssignOnSingleLevelMapping OK\n";
}

static void testCompoundIndexAssignOnChainedNestedMappingMatchesRealUserCShape() {
    // Mirrors std/user.c's own "player_data[\"general\"][\"quest
    // points\"] += ..." exactly: a nested mapping-of-mappings target.
    lpcdriver::Value result = runProbe(
        "mapping player_data;\n"
        "player_data = ([ \"general\": ([ \"quest points\": 3 ]) ]);\n"
        "player_data[\"general\"][\"quest points\"] += 7;\n"
        "return player_data[\"general\"][\"quest points\"];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 10);

    std::cout << "testCompoundIndexAssignOnChainedNestedMappingMatchesRealUserCShape OK\n";
}

static void testCompoundIndexAssignOnArrayElement() {
    lpcdriver::Value result = runProbe(
        "mixed *items;\n"
        "items = ({ 1, 2, 3 });\n"
        "items[1] *= 10;\n"
        "return items[1];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 20);

    std::cout << "testCompoundIndexAssignOnArrayElement OK\n";
}

// ---------------------------------------------------------------------
// Indexed assignment used as a sub-expression rather than a standalone
// statement ("if(!(m[\"class\"] = cl)) ..."). Found live compiling
// std/user/more.c's own line 20 -- see Ast.hpp's IndexAssignExpr
// comment for why this needs its own AST node/codegen, distinct from
// the statement-only IndexAssignStmt the tests above already cover.
// ---------------------------------------------------------------------

static void testIndexAssignAsSubExpressionParsesToIndexAssignExpr() {
    std::string src =
        "void probe(string cl) {\n"
        "    mapping m;\n"
        "    if(!(m[\"class\"] = cl)) m[\"class\"] = \"info\";\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ifStmt = dynamic_cast<lpcdriver::IfStmt*>(body[1].get());
    assert(ifStmt != nullptr);
    auto* notExpr = dynamic_cast<lpcdriver::UnaryExpr*>(ifStmt->condition.get());
    assert(notExpr != nullptr && notExpr->op == lpcdriver::UnaryOp::Not);
    auto* idxAssign = dynamic_cast<lpcdriver::IndexAssignExpr*>(notExpr->operand.get());
    assert(idxAssign != nullptr);
    assert(idxAssign->isCompound == false);
    auto* target = dynamic_cast<lpcdriver::VarRefExpr*>(idxAssign->target.get());
    assert(target != nullptr && target->name == "m");
    auto* key = dynamic_cast<lpcdriver::StringLiteral*>(idxAssign->index.get());
    assert(key != nullptr && key->value == "class");

    std::cout << "testIndexAssignAsSubExpressionParsesToIndexAssignExpr OK\n";
}

static void testIndexAssignAsSubExpressionVmExecutionMatchesMoreCShape() {
    // Mirrors std/user/more.c's own
    // "if(!(__More[\"class\"] = cl)) __More[\"class\"] = \"info\";"
    // exactly: a non-empty cl leaves the assigned value in place, and
    // the assignment's own value (not 0/1) is what the "!" tests.
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "m = ([]);\n"
        "string cl;\n"
        "cl = \"combat\";\n"
        "if(!(m[\"class\"] = cl)) m[\"class\"] = \"info\";\n"
        "return m[\"class\"];\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "combat");

    std::cout << "testIndexAssignAsSubExpressionVmExecutionMatchesMoreCShape OK\n";
}

static void testIndexAssignAsSubExpressionFallsThroughOnEmptyString() {
    // An empty string is falsy in LPC, so "!(m[\"class\"] = cl)" is true
    // and the fallback assignment runs, exactly like more.c's own
    // "__More[\"class\"] = \"info\"" default when no class was passed.
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "m = ([]);\n"
        "string cl;\n"
        "cl = \"\";\n"
        "if(!(m[\"class\"] = cl)) m[\"class\"] = \"info\";\n"
        "return m[\"class\"];\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "info");

    std::cout << "testIndexAssignAsSubExpressionFallsThroughOnEmptyString OK\n";
}

// ---------------------------------------------------------------------
// "(: comma_expr :)" inline lambdas -- the general fallback grammar.y
// keeps distinct from the bare-identifier closure literal form covered
// above (see Ast.hpp's InlineLambdaExpr comment). Found live compiling
// std/user/editor.c's own "(: previous_object(), \"abort\" :)" (line
// 31) and "(: \"return_to_edit\" :)" (line 64).
// ---------------------------------------------------------------------

static void testInlineLambdaWithCallExpressionFirstOperandParsesAsInlineLambdaExpr() {
    std::string src =
        "void probe() {\n"
        "    mixed f;\n"
        "    f = (: previous_object(), \"abort\" :);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assignStmt = dynamic_cast<lpcdriver::AssignStmt*>(body[1].get());
    assert(assignStmt != nullptr);
    auto* lambda = dynamic_cast<lpcdriver::InlineLambdaExpr*>(assignStmt->value.get());
    assert(lambda != nullptr);
    assert(lambda->bodyExprs.size() == 2);
    auto* firstCall = dynamic_cast<lpcdriver::CallExpr*>(lambda->bodyExprs[0].get());
    assert(firstCall != nullptr && firstCall->callee == "previous_object");
    auto* secondStr = dynamic_cast<lpcdriver::StringLiteral*>(lambda->bodyExprs[1].get());
    assert(secondStr != nullptr && secondStr->value == "abort");

    std::cout << "testInlineLambdaWithCallExpressionFirstOperandParsesAsInlineLambdaExpr OK\n";
}

static void testInlineLambdaBareStringConstantParsesAsInlineLambdaExpr() {
    std::string src =
        "void probe() {\n"
        "    mixed f;\n"
        "    f = (: \"return_to_edit\" :);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assignStmt = dynamic_cast<lpcdriver::AssignStmt*>(body[1].get());
    assert(assignStmt != nullptr);
    auto* lambda = dynamic_cast<lpcdriver::InlineLambdaExpr*>(assignStmt->value.get());
    assert(lambda != nullptr);
    assert(lambda->bodyExprs.size() == 1);
    auto* str = dynamic_cast<lpcdriver::StringLiteral*>(lambda->bodyExprs[0].get());
    assert(str != nullptr && str->value == "return_to_edit");

    std::cout << "testInlineLambdaBareStringConstantParsesAsInlineLambdaExpr OK\n";
}

static void testInlineLambdaVmExecutionEvaluatesBodyAtCallTimeNotConstructionTime() {
    // Confirms the grammar.y citation in Ast.hpp's InlineLambdaExpr
    // comment: a body ending in a string constant really does just
    // return that string when funcall()'d, it does not call a same-
    // named method -- exactly std/user/editor.c's own real (if
    // arguably surprising) behavior for "(: previous_object(),
    // \"abort\" :)". Also confirms the body runs at call time, not
    // construction time: side_effect is still 0 right after building
    // the closure, and only becomes 1 once evaluate() actually calls it.
    //
    // Uses an object variable, not a runProbe() local: real LPC forbids
    // a functional literal's body from touching the *enclosing
    // function's* own locals (grammar.y's "Illegal to use local
    // variable in functional", confirmed live -- this driver's own
    // synthesized lambda function has its own separate locals_ scope
    // from the function that constructed it, same restriction). An
    // object variable has no such problem: it lives on the object, not
    // a call frame, and CodeGen's objectVars_ is shared across every
    // function in one generate() call, including synthesized lambdas.
    // create() explicitly zeroes side_effect/before rather than relying
    // on any assumed default-initialized value: this driver leaves a
    // freshly-declared object variable as void until first assigned
    // (unlike real LPC's own auto-zeroed int), so reading one before an
    // explicit assignment is not a meaningful thing for this test to
    // depend on either way.
    ObjectVarHarness harness;
    harness.writeFile("/lambda_probe.c",
        "int side_effect;\n"
        "int before;\n"
        "mixed called;\n"
        "void create() { side_effect = 0; before = 0; called = 0; }\n"
        "int probe() {\n"
        "    mixed f;\n"
        "    f = (: side_effect = 1, \"abort\" :);\n"
        "    before = side_effect;\n"
        "    called = evaluate(f);\n"
        "    return before * 1000 + side_effect * 100 + (called == \"abort\");\n"
        "}\n");
    auto obj = harness.objects.cloneObject("/lambda_probe");
    assert(obj != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    // before == 0 (not yet run), side_effect == 1 (ran once evaluate()
    // called it), called == "abort" (the string, not a method call).
    assert(std::get<int64_t>(result.data) == 101);

    std::cout << "testInlineLambdaVmExecutionEvaluatesBodyAtCallTimeNotConstructionTime OK\n";
}

// ---------------------------------------------------------------------
// "(*fp)(args...)" call-through-a-function-pointer-value syntax --
// desugars to the "evaluate" efun (see grammar.y's own "'(' '*'
// comma_expr ')' '(' expr_list ')'" production and Parser.cpp's own
// comment at the recognition site). Found live compiling
// std/user/editor.c's own "(*__Callback)(__Arguments)" and
// std/user/more.c's own "(*__More[\"endfun\"])(__More[\"args\"])".
// ---------------------------------------------------------------------

static void testFunctionPointerCallThroughParsesToForcedEvaluateCall() {
    std::string src =
        "void probe(function cb, mixed args) {\n"
        "    (*cb)(args);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* call = dynamic_cast<lpcdriver::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr);
    assert(call->callee == "evaluate");
    assert(call->forceEfun == true);
    assert(call->args.size() == 2);
    auto* fpArg = dynamic_cast<lpcdriver::VarRefExpr*>(call->args[0].get());
    assert(fpArg != nullptr && fpArg->name == "cb");
    auto* argsArg = dynamic_cast<lpcdriver::VarRefExpr*>(call->args[1].get());
    assert(argsArg != nullptr && argsArg->name == "args");

    std::cout << "testFunctionPointerCallThroughParsesToForcedEvaluateCall OK\n";
}

static void testFunctionPointerCallThroughOnIndexedTargetParses() {
    // std/user/more.c's own real shape: the pointer clause is itself an
    // indexed expression, not a bare identifier.
    std::string src =
        "void probe(mapping m) {\n"
        "    (*m[\"endfun\"])(m[\"args\"]);\n"
        "}\n";
    lpcdriver::Lexer lexer(src);
    lpcdriver::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<lpcdriver::ExprStmt*>(body[0].get());
    auto* call = dynamic_cast<lpcdriver::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr && call->forceEfun && call->callee == "evaluate");
    assert(call->args.size() == 2);
    auto* fpArg = dynamic_cast<lpcdriver::IndexExpr*>(call->args[0].get());
    assert(fpArg != nullptr);

    std::cout << "testFunctionPointerCallThroughOnIndexedTargetParses OK\n";
}

static void testFunctionPointerCallThroughVmExecutionCallsClosure() {
    // "sizeof" is a real efun registerCoreEfuns() actually registers
    // (unlike the short-circuit tests' deliberately-undefined marker
    // names), so a correct array-length result here confirms the
    // "(*f)(...)" desugaring genuinely reached
    // evaluate()->VM::callClosure(), not just that it parsed.
    lpcdriver::Value result = runProbe(
        "mixed f;\n"
        "f = (: sizeof :);\n"
        "return (*f)(({ 1, 2, 3, 4 }));\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 4);
    std::cout << "testFunctionPointerCallThroughVmExecutionCallsClosure OK\n";
}

// ---------------------------------------------------------------------
// int to_int(string | float | int) -- efuns_main.c's f__to_int().
// Surfaced live compiling std/user/more.c, std/living.c, and
// std/user.c itself.
// ---------------------------------------------------------------------

static void testToIntPassesThroughAnInt() {
    lpcdriver::Value result = runProbe("return to_int(42);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);
    std::cout << "testToIntPassesThroughAnInt OK\n";
}

static void testToIntTruncatesAFloatTowardZero() {
    lpcdriver::Value result = runProbe("return to_int(5.9) * 100 + to_int(-5.9);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // f__to_int()'s real "(long) sp->u.real" cast truncates toward
    // zero, not round-to-nearest or floor: 5.9 -> 5, -5.9 -> -5.
    assert(std::get<int64_t>(result.data) == 495);
    std::cout << "testToIntTruncatesAFloatTowardZero OK\n";
}

static void testToIntParsesALeadingIntegerFromAStringIgnoringTrailingGarbage() {
    lpcdriver::Value result = runProbe("return to_int(\"10x\");\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // Real f__to_int()'s own documented behavior: "to_int(\"10x\") == 10".
    assert(std::get<int64_t>(result.data) == 10);
    std::cout << "testToIntParsesALeadingIntegerFromAStringIgnoringTrailingGarbage OK\n";
}

static void testToIntReturnsZeroForAStringWithNoLeadingNumber() {
    lpcdriver::Value result = runProbe("return to_int(\"nothing here\");\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);
    std::cout << "testToIntReturnsZeroForAStringWithNoLeadingNumber OK\n";
}

// ---------------------------------------------------------------------
// "private" object variable scoping -- a private object variable is
// invisible to (and non-collidable with) an inheriting child's own
// variable of the same name, though it still occupies a real slot in
// the flattened per-object layout. Found live compiling std/user.c:
// std/living.c's own "static private int __Locked, __LastAged;" and
// std/user.c's separate, unrelated "static int __LastAged;" previously
// collided as a codegen error ("object variable already declared").
// ---------------------------------------------------------------------

static void testPrivateObjectVariableDoesNotCollideWithChildsOwnSameNamedVariable() {
    ObjectVarHarness harness;
    harness.writeFile("/priv_parent.c",
        "static private int tag;\n"
        "void create() { tag = 1; }\n"
        "int query_parent_tag() { return tag; }\n");
    harness.writeFile("/priv_child.c",
        "inherit \"/priv_parent\";\n"
        "static int tag;\n"
        "void create() {\n"
        "    ::create();\n"
        "    tag = 2;\n"
        "}\n"
        "int query_child_tag() { return tag; }\n");

    // Compiling priv_child must not throw "object variable \"tag\"
    // already declared" -- that is the exact bug this test guards.
    auto obj = harness.objects.cloneObject("/priv_child");
    assert(obj != nullptr);

    // Both variables are real and independent: the parent's own private
    // "tag" is still 1 (set by its own create(), reachable only through
    // the parent's own inherited function), and the child's separate
    // "tag" is 2 (set by the child's own create(), reachable only
    // through the child's own function).
    lpcdriver::Value parentResult = harness.vm.callFunction(obj, "query_parent_tag", {});
    assert(std::holds_alternative<int64_t>(parentResult.data));
    assert(std::get<int64_t>(parentResult.data) == 1);

    lpcdriver::Value childResult = harness.vm.callFunction(obj, "query_child_tag", {});
    assert(std::holds_alternative<int64_t>(childResult.data));
    assert(std::get<int64_t>(childResult.data) == 2);

    std::cout << "testPrivateObjectVariableDoesNotCollideWithChildsOwnSameNamedVariable OK\n";
}

// ---------------------------------------------------------------------
// Object-variable slot offsets across sibling and multi-level inherit
// chains. Regression coverage for the real std/user.c bug root-caused
// live: a file inherited alongside sibling files (each with no inherits
// of its own, e.g. std/user/autosave.c, editor.c, nmsh.c) had its own
// bytecode's PushObjectVar/StoreObjectVar slot numbers computed relative
// only to its own standalone compile (always starting at local slot 0),
// with no per-program base offset applied when that cached bytecode
// later ran as part of a larger flattened object -- so two unrelated
// sibling files' variables silently aliased the same actual storage
// slot whenever their local slot numbers happened to match. See
// STATUS.md's "do_alias() root-caused" section and
// Bytecode.hpp's CompiledProgram::ancestorBaseOffsets comment for the
// full citation and fix design.
// ---------------------------------------------------------------------

static void testSiblingLeafObjectVariablesDoNotAliasEachOther() {
    ObjectVarHarness harness;

    // Two independent leaf mixins (no inherits of their own), each with
    // several private variables in its own declaration -- mirrors
    // std/user/nmsh.c's "private mapping __Nicknames, __Aliases,
    // __Xverbs;" sitting alongside std/user/autosave.c's own "private
    // mixed *__AutoLoad; private int __ItemsValue; static private int
    // __LastSave;" once both are inherited by the same child. Before the
    // fix, leaf_one's third variable (a3, local slot 2) and leaf_two's
    // third variable (b3, also local slot 2) would silently alias the
    // same absolute storage slot once flattened together.
    harness.writeFile("/leaf_one.c",
        "private int a1, a2, a3;\n"
        "void init_leaf_one() { a1 = 10; a2 = 11; a3 = 12; }\n"
        "int query_a1() { return a1; }\n"
        "int query_a2() { return a2; }\n"
        "int query_a3() { return a3; }\n");
    harness.writeFile("/leaf_two.c",
        "private int b1, b2, b3, b4;\n"
        "void init_leaf_two() { b1 = 20; b2 = 21; b3 = 22; b4 = 23; }\n"
        "int query_b1() { return b1; }\n"
        "int query_b2() { return b2; }\n"
        "int query_b3() { return b3; }\n"
        "int query_b4() { return b4; }\n");
    harness.writeFile("/multi_sibling_child.c",
        "inherit \"/leaf_one\";\n"
        "inherit \"/leaf_two\";\n"
        "void create() {\n"
        "    init_leaf_one();\n"
        "    init_leaf_two();\n"
        "}\n");

    auto obj = harness.objects.cloneObject("/multi_sibling_child");
    assert(obj != nullptr);

    lpcdriver::Value a1 = harness.vm.callFunction(obj, "query_a1", {});
    lpcdriver::Value a2 = harness.vm.callFunction(obj, "query_a2", {});
    lpcdriver::Value a3 = harness.vm.callFunction(obj, "query_a3", {});
    lpcdriver::Value b1 = harness.vm.callFunction(obj, "query_b1", {});
    lpcdriver::Value b2 = harness.vm.callFunction(obj, "query_b2", {});
    lpcdriver::Value b3 = harness.vm.callFunction(obj, "query_b3", {});
    lpcdriver::Value b4 = harness.vm.callFunction(obj, "query_b4", {});

    assert(std::get<int64_t>(a1.data) == 10);
    assert(std::get<int64_t>(a2.data) == 11);
    // The exact collision this bug produced: without the base-offset
    // fix, leaf_two's init (b3 = 22, same raw slot as a3) would leave
    // this reading 22, not 12.
    assert(std::get<int64_t>(a3.data) == 12);
    assert(std::get<int64_t>(b1.data) == 20);
    assert(std::get<int64_t>(b2.data) == 21);
    assert(std::get<int64_t>(b3.data) == 22);
    assert(std::get<int64_t>(b4.data) == 23);

    std::cout << "testSiblingLeafObjectVariablesDoNotAliasEachOther OK\n";
}

static void testObjectVariableOffsetsComposeAcrossMultiLevelInheritChain() {
    ObjectVarHarness harness;

    // Mirrors std/living.c's own real shape: a grandparent with its own
    // variable (like CONTAINER), inherited by a middle file that adds
    // its own variable (like BODY, which inherits CONTAINER), which is
    // in turn inherited by a top file alongside an unrelated sibling
    // leaf (like std/living/combat.c's "inherit BODY; inherit SKILLS;").
    // Confirms the base offset composes correctly across three levels of
    // nesting, not just one level of direct siblings.
    harness.writeFile("/deep_root.c",
        "private int root_val;\n"
        "void init_deep_root() { root_val = 100; }\n"
        "int query_root_val() { return root_val; }\n");
    harness.writeFile("/deep_mid.c",
        "inherit \"/deep_root\";\n"
        "private int mid_val;\n"
        "void init_deep_mid() { init_deep_root(); mid_val = 200; }\n"
        "int query_mid_val() { return mid_val; }\n");
    harness.writeFile("/deep_sibling.c",
        "private int sib1, sib2;\n"
        "void init_deep_sibling() { sib1 = 40; sib2 = 41; }\n"
        "int query_sib1() { return sib1; }\n"
        "int query_sib2() { return sib2; }\n");
    harness.writeFile("/deep_leaf.c",
        "inherit \"/deep_mid\";\n"
        "inherit \"/deep_sibling\";\n"
        "private int leaf_val;\n"
        "void create() {\n"
        "    init_deep_mid();\n"
        "    init_deep_sibling();\n"
        "    leaf_val = 300;\n"
        "}\n"
        "int query_leaf_val() { return leaf_val; }\n");

    auto obj = harness.objects.cloneObject("/deep_leaf");
    assert(obj != nullptr);

    lpcdriver::Value rootVal = harness.vm.callFunction(obj, "query_root_val", {});
    lpcdriver::Value midVal = harness.vm.callFunction(obj, "query_mid_val", {});
    lpcdriver::Value sib1 = harness.vm.callFunction(obj, "query_sib1", {});
    lpcdriver::Value sib2 = harness.vm.callFunction(obj, "query_sib2", {});
    lpcdriver::Value leafVal = harness.vm.callFunction(obj, "query_leaf_val", {});

    assert(std::get<int64_t>(rootVal.data) == 100);
    assert(std::get<int64_t>(midVal.data) == 200);
    assert(std::get<int64_t>(sib1.data) == 40);
    assert(std::get<int64_t>(sib2.data) == 41);
    assert(std::get<int64_t>(leafVal.data) == 300);

    std::cout << "testObjectVariableOffsetsComposeAcrossMultiLevelInheritChain OK\n";
}

// ---------------------------------------------------------------------
// add_action()/enable_commands() command dispatch subsystem. Grounded in
// fluffos-2.9-ds2.08/add_action.c directly (not guessed) and this
// mudlib's own std/living.c, std/room/exits.c, std/room/senses.c,
// std/Object.c, std/user/nmsh.c real usage -- see STATUS.md's own
// recon/design writeup and citations.
// ---------------------------------------------------------------------

static void testEnvironmentDefaultsToNullBeforeAnyMove() {
    ObjectVarHarness harness;
    harness.writeFile("/env_lone.c", "object probe() { return environment(this_object()); }\n");
    auto ob = harness.objects.cloneObject("/env_lone");
    assert(ob != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::monostate>(result.data));
    std::cout << "testEnvironmentDefaultsToNullBeforeAnyMove OK\n";
}

static void testMoveObjectUpdatesEnvironmentAndInventory() {
    ObjectVarHarness harness;
    harness.writeFile("/mo_room.c", "void init() {}\n");
    harness.writeFile("/mo_item.c",
        "object probe_env() { return environment(this_object()); }\n"
        "void go(object dest) { move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/mo_room");
    auto item = harness.objects.cloneObject("/mo_item");
    assert(room != nullptr && item != nullptr);

    harness.vm.callFunction(item, "go", {lpcdriver::Value(room)});

    lpcdriver::Value env = harness.vm.callFunction(item, "probe_env", {});
    auto* envPtr = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&env.data);
    assert(envPtr != nullptr && *envPtr == room);
    assert(room->inventory().size() == 1);
    assert(room->inventory()[0] == item);

    std::cout << "testMoveObjectUpdatesEnvironmentAndInventory OK\n";
}

static void testEnableCommandsGatesWhetherMoveObjectRegistersDestinationActions() {
    // Mirrors std/room.c's real init() -> std/room/exits.c/senses.c's
    // own add_action() calls, and the O_ENABLE_COMMANDS gate
    // setup_new_commands() checks before ever calling init() on behalf
    // of a mover (add_action.c: "if (item->flags & O_ENABLE_COMMANDS) {
    // ... apply(APPLY_INIT, dest, ...) ... }").
    ObjectVarHarness harness;
    harness.writeFile("/ea_room.c",
        "void init() { add_action(\"cmd_look\", \"look\"); }\n"
        "int cmd_look(string arg) { return 1; }\n");
    harness.writeFile("/ea_silent_mover.c", "void go(object dest) { move_object(dest); }\n");
    harness.writeFile("/ea_enabled_mover.c",
        "void go(object dest) { enable_commands(); move_object(dest); }\n");

    auto room1 = harness.objects.cloneObject("/ea_room");
    auto silent = harness.objects.cloneObject("/ea_silent_mover");
    assert(room1 != nullptr && silent != nullptr);
    harness.vm.callFunction(silent, "go", {lpcdriver::Value(room1)});
    assert(harness.vm.dispatchCommand(silent, "look") == false);

    auto room2 = harness.objects.cloneObject("/ea_room");
    auto enabled = harness.objects.cloneObject("/ea_enabled_mover");
    assert(room2 != nullptr && enabled != nullptr);
    harness.vm.callFunction(enabled, "go", {lpcdriver::Value(room2)});
    assert(harness.vm.dispatchCommand(enabled, "look") == true);

    std::cout << "testEnableCommandsGatesWhetherMoveObjectRegistersDestinationActions OK\n";
}

// all_inventory()/deep_inventory() -- func_spec.c: "object *all_inventory
// (object default: F__THIS_OBJECT);" / "object *deep_inventory(object
// default: F__THIS_OBJECT);". Confirmed against fluffos-2.9-ds2.08's own
// array.c: all_inventory() is direct children only, no recursion;
// deep_inventory() is depth-first, target itself never included. Found
// live needing all_inventory(): std/clean_up.c's own remove(), reached
// from secure/std/login.c's new_user() decline path.
static void testAllInventoryReturnsDirectChildrenOnlyNotGrandchildren() {
    ObjectVarHarness harness;
    harness.writeFile("/ai_room.c",
        "void init() {}\n"
        "object *probe() { return all_inventory(this_object()); }\n");
    harness.writeFile("/ai_item.c",
        "object *probe() { return all_inventory(this_object()); }\n"
        "void go(object dest) { move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/ai_room");
    auto outer = harness.objects.cloneObject("/ai_item");
    auto inner = harness.objects.cloneObject("/ai_item");
    assert(room != nullptr && outer != nullptr && inner != nullptr);

    harness.vm.callFunction(outer, "go", {lpcdriver::Value(room)});
    harness.vm.callFunction(inner, "go", {lpcdriver::Value(outer)});

    lpcdriver::Value result = harness.vm.callFunction(room, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 1);
    auto* childOb = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&(*arr)->items[0].data);
    assert(childOb != nullptr && *childOb == outer);

    // Default-argument form (this_object() when no argument given).
    harness.writeFile("/ai_default_probe.c",
        "object *probe() { return all_inventory(); }\n");
    auto defaultProbe = harness.objects.cloneObject("/ai_default_probe");
    harness.vm.callFunction(outer, "go", {lpcdriver::Value(defaultProbe)});
    lpcdriver::Value defResult = harness.vm.callFunction(defaultProbe, "probe", {});
    auto* defArr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&defResult.data);
    assert(defArr != nullptr && (*defArr)->items.size() == 1);

    std::cout << "testAllInventoryReturnsDirectChildrenOnlyNotGrandchildren OK\n";
}

static void testDeepInventoryRecursesThroughNestedChildren() {
    ObjectVarHarness harness;
    harness.writeFile("/di_room.c",
        "void init() {}\n"
        "object *probe() { return deep_inventory(this_object()); }\n");
    harness.writeFile("/di_item.c",
        "object *probe() { return deep_inventory(this_object()); }\n"
        "void go(object dest) { move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/di_room");
    auto outer = harness.objects.cloneObject("/di_item");
    auto inner = harness.objects.cloneObject("/di_item");
    assert(room != nullptr && outer != nullptr && inner != nullptr);

    harness.vm.callFunction(outer, "go", {lpcdriver::Value(room)});
    harness.vm.callFunction(inner, "go", {lpcdriver::Value(outer)});

    lpcdriver::Value result = harness.vm.callFunction(room, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 2);
    auto* first = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&(*arr)->items[0].data);
    auto* second = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&(*arr)->items[1].data);
    assert(first != nullptr && *first == outer);
    assert(second != nullptr && *second == inner);

    std::cout << "testDeepInventoryRecursesThroughNestedChildren OK\n";
}

// strcmp(string, string) -- func_spec.c: "int strcmp(string, string);",
// backed by real efuns_main.c's own f_strcmp(): a plain C strcmp() call.
// Found live needing this: /secure/daemon/player.c's own sort_list(),
// silently swallowed by login.c's catch(__Player->setup()) with no
// console trace until instrumented -- a fresh player's setup() was
// quietly failing to register itself with player.c's online list.
static void testStrcmpMatchesRealCComparisonSemantics() {
    ObjectVarHarness harness;
    harness.writeFile("/sc_probe.c",
        "int probe(string a, string b) { return strcmp(a, b); }\n");
    auto ob = harness.objects.cloneObject("/sc_probe");
    assert(ob != nullptr);

    lpcdriver::Value eq = harness.vm.callFunction(ob, "probe",
        {lpcdriver::Value(std::string("abc")), lpcdriver::Value(std::string("abc"))});
    assert(std::get<int64_t>(eq.data) == 0);

    lpcdriver::Value lt = harness.vm.callFunction(ob, "probe",
        {lpcdriver::Value(std::string("abc")), lpcdriver::Value(std::string("abd"))});
    assert(std::get<int64_t>(lt.data) < 0);

    lpcdriver::Value gt = harness.vm.callFunction(ob, "probe",
        {lpcdriver::Value(std::string("abd")), lpcdriver::Value(std::string("abc"))});
    assert(std::get<int64_t>(gt.data) > 0);

    std::cout << "testStrcmpMatchesRealCComparisonSemantics OK\n";
}

// map_delete(mapping, mixed key) -- func_spec.c's primary declared form
// ("void map_delete(mapping, mixed);"), mutates the mapping in place per
// real efuns_main.c's f_map_delete()/mapping_delete(). Found live
// needing this: std/living/env.c's own remove_env() ("if(env_var &&
// env_var[env]) { map_delete(env_var, env); ... }"), reached
// unguarded (no catch()) from domains/Praxis/setter.c's own
// alignment_cmd() -- this one was fatal to the connection, not silently
// swallowed, and is what actually blocked STEP 4 (alignment) from ever
// reaching STEP 5 (OCC) live.
static void testMapDeleteRemovesKeyInPlaceAndLeavesOthersIntact() {
    ObjectVarHarness harness;
    harness.writeFile("/md_probe.c",
        "mapping m;\n"
        "int setup() { m = ([ \"a\": 1, \"b\": 2, \"c\": 3 ]); return sizeof(m); }\n"
        "int after_delete() { map_delete(m, \"b\"); return sizeof(m); }\n"
        "int still_has(string k) { return m[k]; }\n"
        "int has_key(string k) { return member_array(k, keys(m)) != -1; }\n");
    auto ob = harness.objects.cloneObject("/md_probe");
    assert(ob != nullptr);

    lpcdriver::Value before = harness.vm.callFunction(ob, "setup", {});
    assert(std::get<int64_t>(before.data) == 3);

    lpcdriver::Value after = harness.vm.callFunction(ob, "after_delete", {});
    assert(std::get<int64_t>(after.data) == 2);

    lpcdriver::Value hasB = harness.vm.callFunction(ob, "has_key",
        {lpcdriver::Value(std::string("b"))});
    assert(std::get<int64_t>(hasB.data) == 0);

    lpcdriver::Value stillA = harness.vm.callFunction(ob, "still_has",
        {lpcdriver::Value(std::string("a"))});
    assert(std::get<int64_t>(stillA.data) == 1);
    lpcdriver::Value stillC = harness.vm.callFunction(ob, "still_has",
        {lpcdriver::Value(std::string("c"))});
    assert(std::get<int64_t>(stillC.data) == 3);

    std::cout << "testMapDeleteRemovesKeyInPlaceAndLeavesOthersIntact OK\n";
}

// clone_object()/find_object()/load_object() must accept a path that
// already carries a trailing ".c" (real LPC object paths never include
// the extension internally -- this driver appends it itself to find the
// file on disk -- but plenty of real mudlib call sites pass one anyway,
// e.g. daemon/rifts_start_d.c's own give_item(player, "id_card.c")).
// Found live: ObjectManager::compile() appended ".c" unconditionally, so
// a caller-supplied ".c" produced a literal "id_card.c.c" lookup that
// never existed on disk, aborting finish_creation() partway through
// granting starting equipment -- this was the actual blocker stopping a
// fresh character from ever reaching a real room. Confirmed against
// ObjectManager::normalizeFilename()'s own comment for the fix.
static void testCloneObjectAcceptsPathWithTrailingDotCWithoutDoublingExtension() {
    ObjectVarHarness harness;
    harness.writeFile("/dotc_item.c",
        "string probe() { return file_name(this_object()); }\n");

    auto withExt = harness.objects.cloneObject("/dotc_item.c");
    assert(withExt != nullptr);
    lpcdriver::Value name = harness.vm.callFunction(withExt, "probe", {});
    auto* namePtr = std::get_if<std::string>(&name.data);
    assert(namePtr != nullptr);
    assert(namePtr->find(".c.c") == std::string::npos);

    auto withoutExt = harness.objects.cloneObject("/dotc_item");
    assert(withoutExt != nullptr);

    std::cout << "testCloneObjectAcceptsPathWithTrailingDotCWithoutDoublingExtension OK\n";
}

// intp(mixed) -- func_spec.c: "int intp(mixed);". Found live needing
// this the same pass as the id_card.c.c fix above: /domains/Praxis/
// equipment/id_card.c's own set_value() calls it directly.
static void testIntpTrueOnlyForIntNotStringObjectOrUnsetVariable() {
    ObjectVarHarness harness;
    harness.writeFile("/intp_probe.c",
        "int n;\n"
        "int probe_int() { return intp(5); }\n"
        "int probe_string() { return intp(\"5\"); }\n"
        "int probe_unset_var() { return intp(n); }\n");
    auto ob = harness.objects.cloneObject("/intp_probe");
    assert(ob != nullptr);

    lpcdriver::Value isInt = harness.vm.callFunction(ob, "probe_int", {});
    assert(std::get<int64_t>(isInt.data) == 1);

    lpcdriver::Value isStr = harness.vm.callFunction(ob, "probe_string", {});
    assert(std::get<int64_t>(isStr.data) == 0);

    // An object variable with no explicit initializer defaults to a
    // real int64_t 0 (see STATUS.md's "Root-causing the __HistorySize
    // report"), which IS a real int -- intp() on it must be true.
    lpcdriver::Value unsetVar = harness.vm.callFunction(ob, "probe_unset_var", {});
    assert(std::get<int64_t>(unsetVar.data) == 1);

    std::cout << "testIntpTrueOnlyForIntNotStringObjectOrUnsetVariable OK\n";
}

// string repeat_string(string, int) -- real fluffos-2.9-ds2.08's own
// f_repeat_string() (packages/contrib.c): concatenate the string with
// itself "repeat" times, "" for repeat <= 0. Found live needing this:
// cmds/mortal/_score.c's own panel_border(), called while
// finish_creation() auto-displays a fresh character's score sheet.
static void testRepeatStringConcatenatesNTimesAndEmptyForZeroOrNegative() {
    ObjectVarHarness harness;
    harness.writeFile("/rs_probe.c",
        "string probe(string s, int n) { return repeat_string(s, n); }\n");
    auto ob = harness.objects.cloneObject("/rs_probe");
    assert(ob != nullptr);

    lpcdriver::Value three = harness.vm.callFunction(ob, "probe",
        {lpcdriver::Value(std::string("ab")), lpcdriver::Value(int64_t{3})});
    assert(std::get<std::string>(three.data) == "ababab");

    lpcdriver::Value zero = harness.vm.callFunction(ob, "probe",
        {lpcdriver::Value(std::string("ab")), lpcdriver::Value(int64_t{0})});
    assert(std::get<std::string>(zero.data) == "");

    lpcdriver::Value negative = harness.vm.callFunction(ob, "probe",
        {lpcdriver::Value(std::string("ab")), lpcdriver::Value(int64_t{-2})});
    assert(std::get<std::string>(negative.data) == "");

    std::cout << "testRepeatStringConcatenatesNTimesAndEmptyForZeroOrNegative OK\n";
}

// present(object | string, void | object) -- func_spec.c's real
// signature. Found live needing this: domains/ChiTown/areas/
// chitown_start.c's own reset() (the first starting room a fresh
// character reaches), via the present("id", this_object()) idiom
// CLAUDE.md's rule 11 documents as the standard anti-duplication check.
static void testPresentFindsInventoryItemByIdApplyNotByOtherFunctions() {
    ObjectVarHarness harness;
    harness.writeFile("/pr_room.c",
        "int check(string id) { return present(id, this_object()) ? 1 : 0; }\n"
        "int check_default(string id) { return present(id) ? 1 : 0; }\n");
    harness.writeFile("/pr_sword.c",
        "int id(string str) { return str == \"sword\"; }\n"
        "void go(object dest) { move_object(dest); }\n");
    harness.writeFile("/pr_no_id.c",
        "void go(object dest) { move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/pr_room");
    auto sword = harness.objects.cloneObject("/pr_sword");
    auto plain = harness.objects.cloneObject("/pr_no_id");
    assert(room != nullptr && sword != nullptr && plain != nullptr);

    harness.vm.callFunction(sword, "go", {lpcdriver::Value(room)});
    harness.vm.callFunction(plain, "go", {lpcdriver::Value(room)});

    lpcdriver::Value foundExplicit = harness.vm.callFunction(room, "check",
        {lpcdriver::Value(std::string("sword"))});
    assert(std::get<int64_t>(foundExplicit.data) == 1);

    lpcdriver::Value foundDefault = harness.vm.callFunction(room, "check_default",
        {lpcdriver::Value(std::string("sword"))});
    assert(std::get<int64_t>(foundDefault.data) == 1);

    // An object with no id() at all never matches (VM::callFunction()'s
    // missing-function return is a falsy monostate, not a match).
    lpcdriver::Value notFound = harness.vm.callFunction(room, "check",
        {lpcdriver::Value(std::string("shield"))});
    assert(std::get<int64_t>(notFound.data) == 0);

    std::cout << "testPresentFindsInventoryItemByIdApplyNotByOtherFunctions OK\n";
}

// living() -- func_spec.c: "int living(object default: F__THIS_OBJECT);",
// backed by the same O_ENABLE_COMMANDS flag enable_commands()/
// disable_commands() maintain (add_action.c's f_living()). Surfaced live:
// std/Object.c's own move() gates move_object() behind
// "living(this_object()) && living(ob)".
static void testLivingReflectsEnableCommandsStateAndDefaultsToCurrentObject() {
    ObjectVarHarness harness;
    harness.writeFile("/lv_probe.c",
        "int before_enable() { return living(this_object()); }\n"
        "int after_enable() { enable_commands(); return living(this_object()); }\n"
        "int after_disable() { disable_commands(); return living(this_object()); }\n"
        "int default_arg_after() { return living(); }\n"
        "int living_of(object ob) { return living(ob); }\n");
    harness.writeFile("/lv_bystander.c", "int probe() { return 1; }\n");

    auto probe = harness.objects.cloneObject("/lv_probe");
    auto bystander = harness.objects.cloneObject("/lv_bystander");
    assert(probe != nullptr && bystander != nullptr);

    lpcdriver::Value before = harness.vm.callFunction(probe, "before_enable", {});
    assert(std::get<int64_t>(before.data) == 0);

    lpcdriver::Value after = harness.vm.callFunction(probe, "after_enable", {});
    assert(std::get<int64_t>(after.data) == 1);

    // The default (no-argument) form means this_object(), matching
    // func_spec.c's "object default: F__THIS_OBJECT" -- probe's own flag
    // is on from after_enable() above, so this must read 1 too.
    lpcdriver::Value defaultArg = harness.vm.callFunction(probe, "default_arg_after", {});
    assert(std::get<int64_t>(defaultArg.data) == 1);

    // A second, unrelated object never had enable_commands() called on
    // it, so living() on it must read 0 even while probe's own flag is
    // still on -- the flag is per-object, not global.
    lpcdriver::Value bystanderLiving =
        harness.vm.callFunction(probe, "living_of", {lpcdriver::Value(bystander)});
    assert(std::get<int64_t>(bystanderLiving.data) == 0);

    lpcdriver::Value disabled = harness.vm.callFunction(probe, "after_disable", {});
    assert(std::get<int64_t>(disabled.data) == 0);

    std::cout << "testLivingReflectsEnableCommandsStateAndDefaultsToCurrentObject OK\n";
}

static void testAddActionExactVerbMatchDispatchesWithRemainderAsArgumentAndDeclinesUnknownVerbs() {
    ObjectVarHarness harness;
    harness.writeFile("/av_room.c",
        "string last_arg;\n"
        "void init() { add_action(\"cmd_look\", \"look\"); }\n"
        "int cmd_look(string arg) { last_arg = arg; return 1; }\n"
        "string query_last_arg() { return last_arg; }\n");
    harness.writeFile("/av_mover.c", "void go(object dest) { enable_commands(); move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/av_room");
    auto mover = harness.objects.cloneObject("/av_mover");
    assert(room != nullptr && mover != nullptr);
    harness.vm.callFunction(mover, "go", {lpcdriver::Value(room)});

    assert(harness.vm.dispatchCommand(mover, "look at sign") == true);
    lpcdriver::Value arg = harness.vm.callFunction(room, "query_last_arg", {});
    assert(std::holds_alternative<std::string>(arg.data));
    assert(std::get<std::string>(arg.data) == "at sign");

    // A verb the room never registered does not match anything.
    assert(harness.vm.dispatchCommand(mover, "dance") == false);

    std::cout << "testAddActionExactVerbMatchDispatchesWithRemainderAsArgumentAndDeclinesUnknownVerbs OK\n";
}

static void testAddActionCatchAllShortFlagReceivesRemainderAndQueryVerbReturnsFullTypedWord() {
    // Mirrors std/living.c's own real "add_action(\"cmd_hook\", \"\", 1)"
    // shape exactly: an empty verb with flag 1 matches every typed word
    // (real V_SHORT semantics), the handler receives only the text after
    // the first word, and query_verb() still returns the full first word,
    // not the (empty) matched prefix -- confirmed against add_action.c's
    // f_add_action() flag handling and this driver's own CLAUDE.md-
    // documented "flag 1 to add_action is V_SHORT" gotcha.
    ObjectVarHarness harness;
    harness.writeFile("/ch_mover.c",
        "string seen_verb;\n"
        "string seen_arg;\n"
        "void register_hook() { add_action(\"cmd_hook\", \"\", 1); }\n"
        "int cmd_hook(string arg) {\n"
        "    seen_verb = query_verb();\n"
        "    seen_arg = arg;\n"
        "    return 1;\n"
        "}\n"
        "string query_seen_verb() { return seen_verb; }\n"
        "string query_seen_arg() { return seen_arg; }\n");
    auto mover = harness.objects.cloneObject("/ch_mover");
    assert(mover != nullptr);

    // add_action() outside any dispatch/move context needs an explicit
    // command_giver -- mirrors the real live sequence (secure/std/
    // login.c's own "exec(__Player, this_object()); ... __Player->
    // setup();", confirmed by direct reading: exec() rebinds the
    // connection to the new player *before* setup() -> init_living() ->
    // add_action() ever runs, so command_giver already resolves to the
    // player itself by then) without needing a real Connection/
    // OutputContext here. current_object must equal command_giver for
    // add_action()'s own nearness check to pass, so the registration
    // happens inside a function called *on* mover, not from the test
    // driver directly.
    harness.vm.pushCommandGiver(mover);
    harness.vm.callFunction(mover, "register_hook", {});
    harness.vm.popCommandGiver();

    assert(harness.vm.dispatchCommand(mover, "smile warmly") == true);

    lpcdriver::Value verb = harness.vm.callFunction(mover, "query_seen_verb", {});
    assert(std::holds_alternative<std::string>(verb.data));
    assert(std::get<std::string>(verb.data) == "smile");

    lpcdriver::Value arg = harness.vm.callFunction(mover, "query_seen_arg", {});
    assert(std::holds_alternative<std::string>(arg.data));
    assert(std::get<std::string>(arg.data) == "warmly");

    std::cout << "testAddActionCatchAllShortFlagReceivesRemainderAndQueryVerbReturnsFullTypedWord OK\n";
}

static void testDispatchCommandTriesNextMatchWhenFirstHandlerReturnsFalsy() {
    // real add_action.c's own doc comment: "If it was the wrong command,
    // the parser will continue searching for another command, until one
    // returns true." A more-recently-added (checked-first) handler that
    // declines must not stop the search.
    ObjectVarHarness harness;
    // add_action() always prepends (real add_action.c: "adding to the top
    // of the list doesn't harm anything"), so registering cmd_accept
    // *first* and cmd_decline *second* puts cmd_decline at the front,
    // checked first -- exactly the ordering that would silently mask
    // this bug if the search stopped at the first match instead of
    // continuing past a falsy return.
    harness.writeFile("/dc_room.c",
        "void init() {\n"
        "    add_action(\"cmd_accept\", \"go\");\n"
        "    add_action(\"cmd_decline\", \"go\");\n"
        "}\n"
        "int cmd_decline(string arg) { return 0; }\n"
        "int cmd_accept(string arg) { return 1; }\n");
    harness.writeFile("/dc_mover.c", "void go(object dest) { enable_commands(); move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/dc_room");
    auto mover = harness.objects.cloneObject("/dc_mover");
    assert(room != nullptr && mover != nullptr);
    harness.vm.callFunction(mover, "go", {lpcdriver::Value(room)});

    // cmd_decline is checked first (most recently added), returns 0;
    // the search must fall through to cmd_accept, not stop there.
    assert(harness.vm.dispatchCommand(mover, "go north") == true);

    std::cout << "testDispatchCommandTriesNextMatchWhenFirstHandlerReturnsFalsy OK\n";
}

static void testThisPlayerReturnsCommandGiverDuringDispatch() {
    ObjectVarHarness harness;
    harness.writeFile("/tp_room.c",
        "void init() { add_action(\"cmd_who_am_i\", \"whoami\"); }\n"
        "object cmd_who_am_i(string arg) { return this_player(); }\n");
    harness.writeFile("/tp_mover.c", "void go(object dest) { enable_commands(); move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/tp_room");
    auto mover = harness.objects.cloneObject("/tp_mover");
    assert(room != nullptr && mover != nullptr);
    harness.vm.callFunction(mover, "go", {lpcdriver::Value(room)});

    // dispatchCommand() itself does not hand back the handler's return
    // value, so stash it on the room object instead, the same pattern
    // testAddActionExactVerbMatchDispatchesWithRemainderAsArgument uses.
    harness.writeFile("/tp_room2.c",
        "object last_caller;\n"
        "void init() { add_action(\"cmd_who_am_i\", \"whoami\"); }\n"
        "int cmd_who_am_i(string arg) { last_caller = this_player(); return 1; }\n"
        "object query_last_caller() { return last_caller; }\n");
    auto room2 = harness.objects.cloneObject("/tp_room2");
    auto mover2 = harness.objects.cloneObject("/tp_mover");
    assert(room2 != nullptr && mover2 != nullptr);
    harness.vm.callFunction(mover2, "go", {lpcdriver::Value(room2)});

    assert(harness.vm.dispatchCommand(mover2, "whoami") == true);
    lpcdriver::Value caller = harness.vm.callFunction(room2, "query_last_caller", {});
    auto* callerPtr = std::get_if<std::shared_ptr<lpcdriver::LpcObject>>(&caller.data);
    assert(callerPtr != nullptr && *callerPtr == mover2);

    std::cout << "testThisPlayerReturnsCommandGiverDuringDispatch OK\n";
}

static void testQueryVerbReturnsZeroOutsideOfDispatch() {
    ObjectVarHarness harness;
    harness.writeFile("/qv_lone.c", "mixed probe() { return query_verb(); }\n");
    auto ob = harness.objects.cloneObject("/qv_lone");
    assert(ob != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::monostate>(result.data));
    std::cout << "testQueryVerbReturnsZeroOutsideOfDispatch OK\n";
}

// ---------------------------------------------------------------------
// set_privs()/query_privs() (real object_t::privs). Surfaced live
// compiling/running std/living.c and std/money.c.
// ---------------------------------------------------------------------

static void testQueryPrivsReturnsZeroWhenNeverSet() {
    ObjectVarHarness harness;
    harness.writeFile("/priv_lone.c", "mixed probe() { return query_privs(this_object()); }\n");
    auto ob = harness.objects.cloneObject("/priv_lone");
    assert(ob != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::monostate>(result.data));
    std::cout << "testQueryPrivsReturnsZeroWhenNeverSet OK\n";
}

static void testSetPrivsThenQueryPrivsRoundTripsAndClearsOnNonStringArgument() {
    ObjectVarHarness harness;
    harness.writeFile("/priv_set.c",
        "void set_it(string s) { set_privs(this_object(), s); }\n"
        "void clear_it() { set_privs(this_object(), 0); }\n"
        "mixed probe() { return query_privs(this_object()); }\n");
    auto ob = harness.objects.cloneObject("/priv_set");
    assert(ob != nullptr);

    harness.vm.callFunction(ob, "set_it", {lpcdriver::Value(std::string("wizards:thurtea"))});
    lpcdriver::Value set = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::string>(set.data));
    assert(std::get<std::string>(set.data) == "wizards:thurtea");

    harness.vm.callFunction(ob, "clear_it", {});
    lpcdriver::Value cleared = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::monostate>(cleared.data));

    std::cout << "testSetPrivsThenQueryPrivsRoundTripsAndClearsOnNonStringArgument OK\n";
}

// ---------------------------------------------------------------------
// Object variable initializer VM execution ("$objvarinit", see
// CodeGen::generate()'s own comment). Surfaced live compiling secure/
// daemon/wiztools.c's own "string *REISSUED_TOOLS = ({ ... });".
// ---------------------------------------------------------------------

static void testObjectVarInitializerRunsBeforeCreate() {
    ObjectVarHarness harness;
    harness.writeFile("/vi_lone.c",
        "int x = 5;\n"
        "int seen_at_create;\n"
        "void create() { seen_at_create = x; }\n"
        "int query_x() { return x; }\n"
        "int query_seen_at_create() { return seen_at_create; }\n");
    auto ob = harness.objects.cloneObject("/vi_lone");
    assert(ob != nullptr);

    lpcdriver::Value x = harness.vm.callFunction(ob, "query_x", {});
    assert(std::holds_alternative<int64_t>(x.data));
    assert(std::get<int64_t>(x.data) == 5);

    // create() read the already-initialized value, not 0/void -- proves
    // ordering, not just that the initializer eventually ran at all.
    lpcdriver::Value seen = harness.vm.callFunction(ob, "query_seen_at_create", {});
    assert(std::holds_alternative<int64_t>(seen.data));
    assert(std::get<int64_t>(seen.data) == 5);

    std::cout << "testObjectVarInitializerRunsBeforeCreate OK\n";
}

static void testObjectVarInitializerRunsOnFileWithNoCreateAtAll() {
    // secure/daemon/wiztools.c's own real shape: an initializer with no
    // create() function in the file at all.
    ObjectVarHarness harness;
    harness.writeFile("/vi_no_create.c",
        "string *tools = ({ \"a\", \"b\", \"c\" });\n"
        "int query_count() { return sizeof(tools); }\n");
    auto ob = harness.objects.cloneObject("/vi_no_create");
    assert(ob != nullptr);
    lpcdriver::Value count = harness.vm.callFunction(ob, "query_count", {});
    assert(std::holds_alternative<int64_t>(count.data));
    assert(std::get<int64_t>(count.data) == 3);

    std::cout << "testObjectVarInitializerRunsOnFileWithNoCreateAtAll OK\n";
}

static void testObjectVarInitializerParentRunsBeforeChild() {
    ObjectVarHarness harness;
    harness.writeFile("/vi_parent.c", "string tag = \"parent\";\n");
    harness.writeFile("/vi_child.c",
        "inherit \"/vi_parent\";\n"
        "string child_tag = tag + \"-child\";\n"
        "string query_tag() { return tag; }\n"
        "string query_child_tag() { return child_tag; }\n");
    auto ob = harness.objects.cloneObject("/vi_child");
    assert(ob != nullptr);

    lpcdriver::Value tag = harness.vm.callFunction(ob, "query_tag", {});
    assert(std::holds_alternative<std::string>(tag.data));
    assert(std::get<std::string>(tag.data) == "parent");

    // child_tag's own initializer read the parent's already-initialized
    // "tag" -- proves the parent's own "$objvarinit" ran first.
    lpcdriver::Value childTag = harness.vm.callFunction(ob, "query_child_tag", {});
    assert(std::holds_alternative<std::string>(childTag.data));
    assert(std::get<std::string>(childTag.data) == "parent-child");

    std::cout << "testObjectVarInitializerParentRunsBeforeChild OK\n";
}

// int undefinedp(mixed) / int nullp(mixed). Surfaced live: daemon/
// multi.c's own query_prevent_login().
static void testUndefinedpTrueOnlyForVoidNotZeroOrOtherTypes() {
    // A bare unassigned local/object variable is real LPC's own 0
    // default now (see LpcObject.cpp's own comment), not this driver's
    // separate monostate sentinel -- environment() on an object with no
    // environment set is a genuine, reliable source of monostate to
    // test undefinedp()/nullp() against instead.
    lpcdriver::Value result = runProbe(
        "mixed never_set;\n"
        "never_set = environment(this_object());\n"
        "return undefinedp(never_set) * 1000 + nullp(never_set) * 100 +\n"
        "       undefinedp(0) * 10 + undefinedp(\"\");\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // never_set (environment() with nothing set -> monostate) is
    // undefined by both names; a real 0 and an empty string are not.
    assert(std::get<int64_t>(result.data) == 1100);
    std::cout << "testUndefinedpTrueOnlyForVoidNotZeroOrOtherTypes OK\n";
}

// ---------------------------------------------------------------------
// OpCode::Call's fallback to obj->program() when a bare call resolves
// against neither the currently-executing file's own functions nor its
// own inherited chain. Surfaced live: std/user/nmsh.c's own
// process_input() calling the bare name "query_client", a function only
// std/user.c (which inherits nmsh.c) defines -- real LPC compiles each
// file independently, so a parent cannot know at its own compile time
// that some future child will provide a name it references; this can
// only resolve at runtime, against whatever the real running object
// turns out to be.
// ---------------------------------------------------------------------

static void testParentCallToFunctionOnlyChildDefinesResolvesAtRuntime() {
    ObjectVarHarness harness;
    harness.writeFile("/pc_parent.c",
        "string probe() { return only_in_child(); }\n");
    harness.writeFile("/pc_child.c",
        "inherit \"/pc_parent\";\n"
        "string only_in_child() { return \"from child\"; }\n");
    auto obj = harness.objects.cloneObject("/pc_child");
    assert(obj != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "from child");

    std::cout << "testParentCallToFunctionOnlyChildDefinesResolvesAtRuntime OK\n";
}

static void testBareCallFromParentReachesChildsOverrideNotItsOwnLexicalDefinition() {
    // Corrected expectation (was
    // testParentCallStillPrefersItsOwnLexicalDefinitionOverChildsOverride,
    // which encoded the opposite, now-disproven claim as its own name and
    // comment): confirmed against fluffos-2.9-ds2.08/compiler.c's
    // define_new_function() ("It was either an undefined but used
    // function, or an inherited function ... we now consider this to be
    // THE new definition") -- real LPC flattens an entire inherit tree
    // into one shared function table per object; when a child redefines
    // a name an ancestor already provided, the child's definition
    // replaces that one shared entry for the *whole* object, so every
    // unqualified call to that name -- including one written inside the
    // ancestor's own source -- resolves to the override. This is the
    // standard Nightmare/LPC idiom of an ancestor providing a
    // placeholder a real subclass overrides (std/user/nmsh.c's own
    // query_name() stub vs. std/user.c's real one is the live case that
    // surfaced this). Only "::helper()" (OpCode::CallParent) reaches the
    // parent's own shadowed definition instead -- see
    // testQualifiedParentCallMatchesInheritPathBasename and
    // testBareParentCallInvokesInheritedFunctionNotLocalOverride for that
    // explicit-qualifier path, which this change does not touch.
    ObjectVarHarness harness;
    harness.writeFile("/pl_parent.c",
        "string helper() { return \"parent helper\"; }\n"
        "string probe() { return helper(); }\n");
    harness.writeFile("/pl_child.c",
        "inherit \"/pl_parent\";\n"
        "string helper() { return \"child helper\"; }\n");
    auto obj = harness.objects.cloneObject("/pl_child");
    assert(obj != nullptr);

    // probe() is only defined in the parent, and its own internal bare
    // call to helper() must resolve to the child's override -- the same
    // result a direct "obj->helper()" call_other would reach -- not the
    // parent's own lexically-local definition.
    lpcdriver::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "child helper");

    std::cout << "testBareCallFromParentReachesChildsOverrideNotItsOwnLexicalDefinition OK\n";
}

// ---------------------------------------------------------------------
// Array subtraction ("arr1 - arr2", real set difference) and monostate
// participating in arithmetic as a real 0. Both surfaced live compiling/
// running std/user.c's own register_channels() and std/living.c's own
// query_stats().
// ---------------------------------------------------------------------

static void testArraySubtractionRemovesEveryMatchingElementPreservingOrder() {
    lpcdriver::Value result = runProbe(
        "mixed *a;\n"
        "mixed *b;\n"
        "mixed *r;\n"
        "a = ({ 1, 2, 3, 2, 4 });\n"
        "b = ({ 2 });\n"
        "r = a - b;\n"
        "return sizeof(r) * 1000 + r[0] * 100 + r[1] * 10 + r[2];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // Every "2" removed (both occurrences), order and the rest preserved:
    // ({ 1, 3, 4 }).
    assert(std::get<int64_t>(result.data) == 3134);
    std::cout << "testArraySubtractionRemovesEveryMatchingElementPreservingOrder OK\n";
}

static void testArraySubtractionOnEmptyRightOperandLeavesLeftUnchanged() {
    lpcdriver::Value result = runProbe(
        "mixed *a;\n"
        "a = ({ 1, 2, 3 }) - ({});\n"
        "return sizeof(a) * 100 + a[0] * 10 + a[2];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 313);
    std::cout << "testArraySubtractionOnEmptyRightOperandLeavesLeftUnchanged OK\n";
}

static void testMonostateParticipatesInArithmeticAsRealZero() {
    // A missing mapping key (see OpCode::Index's own comment) is
    // monostate, not a real int64_t 0 -- but real LPC has no such
    // distinction for ordinary arithmetic, only for undefinedp()/
    // nullp(). Confirmed live: std/living.c's own query_stats() doing
    // "stats[stat] + x" where stats[stat] is a missing key for any stat
    // never rolled yet.
    lpcdriver::Value result = runProbe(
        "mapping stats;\n"
        "int x;\n"
        "stats = ([]);\n"
        "x = 0;\n"
        "return (stats[\"strength\"] + x) * 1000 +\n"
        "       (5 - stats[\"strength\"]) * 100 +\n"
        "       (stats[\"strength\"] * 3) * 10 +\n"
        "       undefinedp(stats[\"strength\"]);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // 0 + 0 = 0; 5 - 0 = 5; 0 * 3 = 0; undefinedp(missing key) = 1.
    assert(std::get<int64_t>(result.data) == 501);
    std::cout << "testMonostateParticipatesInArithmeticAsRealZero OK\n";
}

// ---------------------------------------------------------------------
// OpCode::Add's string+number/number+string/object+string branches --
// confirmed against real interpret.c's F_ADD (case T_STRING's own
// T_NUMBER/T_REAL/T_OBJECT sub-branches, and the symmetric T_NUMBER/
// T_REAL/T_OBJECT cases' own T_STRING sub-branch). Surfaced live:
// daemon/terminal.c's own ESC(p) macro ("%c"+(p), called with a bare
// int argument in several of its own table entries) threw "Add:
// unsupported operand types" and silently broke TERMINAL_D, in turn
// silently breaking std/user.c's own setup() past that point -- the
// exact class of bug the new catch()-logging (see testCatchLogsTrapped
// ErrorToStderrByDefault) surfaced instead of staying invisible.
// ---------------------------------------------------------------------

static void testStringPlusIntAppendsDecimalDigits() {
    lpcdriver::Value result = runProbe("return \"count:\" + 42;\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "count:42");
    std::cout << "testStringPlusIntAppendsDecimalDigits OK\n";
}

static void testIntPlusStringPrependsDecimalDigits() {
    lpcdriver::Value result = runProbe("return 42 + \":count\";\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "42:count");
    std::cout << "testIntPlusStringPrependsDecimalDigits OK\n";
}

// A missing mapping key (monostate, see asArithmeticOperand's own
// comment) participating in string+number concatenation as a real 0 --
// same treatment it already gets in plain numeric arithmetic. Surfaced
// live: std/money.c's own query_money() ("return money[str];") called
// on a fresh character's not-yet-populated money mapping, then
// concatenated by std/user.c's own setup() ("... + query_money(
// \"platinum\") + \" pl, \" + ...").
static void testStringPlusMissingMappingKeyFormatsAsZero() {
    lpcdriver::Value result = runProbe(
        "mapping m;\n"
        "m = ([]);\n"
        "return \"pl:\" + m[\"platinum\"];\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "pl:0");
    std::cout << "testStringPlusMissingMappingKeyFormatsAsZero OK\n";
}

static void testObjectPlusStringPrependsItsFilename() {
    ObjectVarHarness harness;
    harness.writeFile("/obj_add_test.c",
        "string probe() { return this_object() + \"::tail\"; }\n");
    auto ob = harness.objects.cloneObject("/obj_add_test");
    assert(ob != nullptr);
    lpcdriver::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "/obj_add_test::tail");
    std::cout << "testObjectPlusStringPrependsItsFilename OK\n";
}

// int random(int) -- confirmed against real efuns_main.c's f_random():
// n <= 0 always yields plain 0 (not an error), and the result is always
// in [0, n). Surfaced live: domains/Praxis/setter.c's own roll_d6()
// (Palladium 3d6 attribute rolling).
static void testRandomOfNonPositiveArgumentIsZero() {
    lpcdriver::Value result = runProbe("return random(0) + random(-5);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);
    std::cout << "testRandomOfNonPositiveArgumentIsZero OK\n";
}

static void testRandomStaysWithinZeroToNExclusiveAcrossManyDraws() {
    lpcdriver::Value result = runProbe(
        "int i;\n"
        "int bad;\n"
        "for(i = 0; i < 200; i++) {\n"
        "    int r;\n"
        "    r = random(6);\n"
        "    if(r < 0 || r >= 6) bad = 1;\n"
        "}\n"
        "return bad;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);
    std::cout << "testRandomStaysWithinZeroToNExclusiveAcrossManyDraws OK\n";
}

// set_heart_beat()/query_heart_beat() -- surfaced live: std/user.c's own
// setup() calling set_heart_beat(1) unconditionally.
static void testSetHeartBeatThenQueryHeartBeatRoundTrips() {
    ObjectVarHarness harness;
    harness.writeFile("/hb_lone.c",
        "void enable_it() { set_heart_beat(1); }\n"
        "void disable_it() { set_heart_beat(0); }\n"
        "int probe() { return query_heart_beat(this_object()); }\n");
    auto ob = harness.objects.cloneObject("/hb_lone");
    assert(ob != nullptr);

    lpcdriver::Value before = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(before.data));
    assert(std::get<int64_t>(before.data) == 0);

    harness.vm.callFunction(ob, "enable_it", {});
    lpcdriver::Value after = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(after.data));
    assert(std::get<int64_t>(after.data) == 1);

    harness.vm.callFunction(ob, "disable_it", {});
    lpcdriver::Value cleared = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(cleared.data));
    assert(std::get<int64_t>(cleared.data) == 0);

    std::cout << "testSetHeartBeatThenQueryHeartBeatRoundTrips OK\n";
}

// ---------------------------------------------------------------------
// map_array()/map() and filter_array()/filter() (string-function-name-
// plus-object-target form). Surfaced live: std/user/nmsh.c's own
// do_nickname() ("map_array(explode(str, \" \"), \"replace_nickname\",
// this_object())").
// ---------------------------------------------------------------------

static void testMapArrayWithStringFunctionNameCallsMethodOnTargetForEachElement() {
    ObjectVarHarness harness;
    harness.writeFile("/ma_target.c",
        "string shout(string s) { return s + \"!\"; }\n");
    harness.writeFile("/ma_caller.c",
        "mixed probe(object target) {\n"
        "    return map_array(({ \"a\", \"b\", \"c\" }), \"shout\", target);\n"
        "}\n");
    auto target = harness.objects.cloneObject("/ma_target");
    auto caller = harness.objects.cloneObject("/ma_caller");
    assert(target != nullptr && caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {lpcdriver::Value(target)});
    auto* arrPtr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&result.data);
    assert(arrPtr != nullptr && *arrPtr != nullptr);
    assert((*arrPtr)->items.size() == 3);
    assert(std::get<std::string>((*arrPtr)->items[0].data) == "a!");
    assert(std::get<std::string>((*arrPtr)->items[1].data) == "b!");
    assert(std::get<std::string>((*arrPtr)->items[2].data) == "c!");

    std::cout << "testMapArrayWithStringFunctionNameCallsMethodOnTargetForEachElement OK\n";
}

static void testFilterArrayWithStringFunctionNameKeepsOnlyTruthyElements() {
    ObjectVarHarness harness;
    harness.writeFile("/fa_target.c",
        "int is_even(int n) { return n % 2 == 0; }\n");
    harness.writeFile("/fa_caller.c",
        "mixed probe(object target) {\n"
        "    return filter_array(({ 1, 2, 3, 4, 5, 6 }), \"is_even\", target);\n"
        "}\n");
    auto target = harness.objects.cloneObject("/fa_target");
    auto caller = harness.objects.cloneObject("/fa_caller");
    assert(target != nullptr && caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {lpcdriver::Value(target)});
    auto* arrPtr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&result.data);
    assert(arrPtr != nullptr && *arrPtr != nullptr);
    assert((*arrPtr)->items.size() == 3);
    assert(std::get<int64_t>((*arrPtr)->items[0].data) == 2);
    assert(std::get<int64_t>((*arrPtr)->items[1].data) == 4);
    assert(std::get<int64_t>((*arrPtr)->items[2].data) == 6);

    std::cout << "testFilterArrayWithStringFunctionNameKeepsOnlyTruthyElements OK\n";
}

// sort_array() (string-function-name-plus-object-target form). Surfaced
// live: secure/daemon/player.c's own add_player_info(), "sort_array(
// player_list, \"sort_list\", this_object())".
static void testSortArrayWithStringFunctionNameOrdersByComparatorResult() {
    ObjectVarHarness harness;
    harness.writeFile("/sa_target.c",
        "int cmp(int a, int b) {\n"
        "    if(a > b) return -1;\n"
        "    if(a < b) return 1;\n"
        "    return 0;\n"
        "}\n");
    harness.writeFile("/sa_caller.c",
        "mixed probe(object target) {\n"
        "    return sort_array(({ 3, 1, 4, 1, 5 }), \"cmp\", target);\n"
        "}\n");
    auto target = harness.objects.cloneObject("/sa_target");
    auto caller = harness.objects.cloneObject("/sa_caller");
    assert(target != nullptr && caller != nullptr);

    lpcdriver::Value result = harness.vm.callFunction(caller, "probe", {lpcdriver::Value(target)});
    auto* arrPtr = std::get_if<std::shared_ptr<lpcdriver::Array>>(&result.data);
    assert(arrPtr != nullptr && *arrPtr != nullptr);
    assert((*arrPtr)->items.size() == 5);
    // Descending, per cmp()'s own convention (a > b returns -1, a sorts first).
    assert(std::get<int64_t>((*arrPtr)->items[0].data) == 5);
    assert(std::get<int64_t>((*arrPtr)->items[1].data) == 4);
    assert(std::get<int64_t>((*arrPtr)->items[2].data) == 3);
    assert(std::get<int64_t>((*arrPtr)->items[3].data) == 1);
    assert(std::get<int64_t>((*arrPtr)->items[4].data) == 1);

    std::cout << "testSortArrayWithStringFunctionNameOrdersByComparatorResult OK\n";
}

// implode() -- surfaced live: std/user/nmsh.c's own do_alias()/
// do_nickname() joining word arrays back into a line with " ".
static void testImplodeJoinsStringArrayWithSeparator() {
    lpcdriver::Value result = runProbe(
        "string *words;\n"
        "words = ({ \"go\", \"north\" });\n"
        "return implode(words, \" \");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "go north");
    std::cout << "testImplodeJoinsStringArrayWithSeparator OK\n";
}

static void testImplodeOnEmptyArrayReturnsEmptyString() {
    lpcdriver::Value result = runProbe("return implode(({}), \" \");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "");
    std::cout << "testImplodeOnEmptyArrayReturnsEmptyString OK\n";
}

// sprintf's "%c" -- surfaced live: daemon/terminal.c's own ANSI(p)/ESC(p)
// macros, "sprintf(\"%c[\"+(p)+\"m\", 27)", building a raw ESC (ASCII 27)
// byte ahead of an ANSI escape sequence. Confirmed against
// fluffos-2.9-ds2.08/sprintf.c: INFO_T_CHAR requires a T_NUMBER (int)
// argument, not a string.
static void testSprintfPercentCEmitsSingleCharacterFromIntArgument() {
    lpcdriver::Value result = runProbe("return sprintf(\"%c[31m\", 27);\n");
    assert(std::holds_alternative<std::string>(result.data));
    std::string expected;
    expected += static_cast<char>(27);
    expected += "[31m";
    assert(std::get<std::string>(result.data) == expected);
    std::cout << "testSprintfPercentCEmitsSingleCharacterFromIntArgument OK\n";
}

static void testSprintfPercentCThrowsOnNonIntArgument() {
    bool threw = false;
    try {
        runProbe("return sprintf(\"%c\", \"x\");\n");
    } catch (const lpcdriver::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testSprintfPercentCThrowsOnNonIntArgument OK\n";
}

// sprintf field-width modifiers ("-" left-adjust, leading-zero pad) --
// confirmed against real sprintf.c's own documented modifier set.
// Surfaced live: domains/Praxis/setter.c's own show_rolled_attributes(),
// "%-3d" for each rolled Palladium attribute.
static void testSprintfLeftJustifiedFieldWidthPadsWithSpaces() {
    lpcdriver::Value result = runProbe("return sprintf(\"[%-3d]\", 7);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[7  ]");
    std::cout << "testSprintfLeftJustifiedFieldWidthPadsWithSpaces OK\n";
}

static void testSprintfRightJustifiedFieldWidthPadsWithSpaces() {
    lpcdriver::Value result = runProbe("return sprintf(\"[%3d]\", 7);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[  7]");
    std::cout << "testSprintfRightJustifiedFieldWidthPadsWithSpaces OK\n";
}

static void testSprintfZeroPaddedFieldWidthPadsWithZeros() {
    lpcdriver::Value result = runProbe("return sprintf(\"[%03d]\", 7);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[007]");
    std::cout << "testSprintfZeroPaddedFieldWidthPadsWithZeros OK\n";
}

static void testSprintfFieldWidthDoesNotTruncateAWiderValue() {
    lpcdriver::Value result = runProbe("return sprintf(\"[%-3d]\", 12345);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[12345]");
    std::cout << "testSprintfFieldWidthDoesNotTruncateAWiderValue OK\n";
}

static void testSprintfStringFieldWidthLeftJustifies() {
    lpcdriver::Value result = runProbe("return sprintf(\"[%-5s]\", \"ab\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[ab   ]");
    std::cout << "testSprintfStringFieldWidthLeftJustifies OK\n";
}

// "%%" and the ":" field-size-and-precision modifier -- surfaced live:
// secure/SimulEfun/strings.c's own arrange_string(), which builds a
// second format string via sprintf("%%:-%ds", x) (needing "%%" to
// resolve to a literal "%") and then uses that built string ("%:-Ns")
// as a real format itself (needing ":" to mean "field size AND
// precision", truncating a %s argument longer than the field).
static void testSprintfDoublePercentEmitsLiteralPercentAndConsumesNoArgument() {
    lpcdriver::Value result = runProbe("return sprintf(\"100%%\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "100%");
    std::cout << "testSprintfDoublePercentEmitsLiteralPercentAndConsumesNoArgument OK\n";
}

static void testSprintfColonFieldWidthPadsAShorterStringLeftJustified() {
    lpcdriver::Value result = runProbe("return sprintf(\"[%:-5s]\", \"ab\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[ab   ]");
    std::cout << "testSprintfColonFieldWidthPadsAShorterStringLeftJustified OK\n";
}

static void testSprintfColonFieldWidthTruncatesALongerString() {
    lpcdriver::Value result = runProbe("return sprintf(\"[%:-5s]\", \"abcdefgh\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[abcde]");
    std::cout << "testSprintfColonFieldWidthTruncatesALongerString OK\n";
}

// The exact live shape: build "%:-Ns" via a first sprintf("%%:-%ds", x)
// call, then use the result as a real format string in a second call.
static void testSprintfBuildingAndThenUsingADynamicColonFormatString() {
    lpcdriver::Value result = runProbe(
        "string fmt;\n"
        "fmt = sprintf(\"%%:-%ds\", 6);\n"
        "return sprintf(fmt, \"hi\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "hi    ");
    std::cout << "testSprintfBuildingAndThenUsingADynamicColonFormatString OK\n";
}

int main() {
    // Real efuns (sizeof, write, etc.) are only registered here, in this
    // test binary, so the VM-level tests below can call them. Names like
    // "nonexistent_marker_efun" used by the short-circuit tests are not
    // among them, and stay correctly undefined.
    lpcdriver::registerCoreEfuns();

    testBasicTokenize();
    testArrowTokenizes();
    testComparisonOperatorsTokenize();
    testCallOtherParsesToCallOtherExpr();
    testArrowOperatorParsesToCallOtherExpr();
    testCodegenEmitsCallEfunForCallOther();
    testMultiFunctionProgramParses();
    testFunctionWithParameterParses();
    testVarDeclAndAssignParse();
    testCodegenBindsParamAndEchoesRuntimeValue();
    testIfElseParsesToIfStmt();
    testWhileParsesToWhileStmt();
    testCodegenEmitsJumpOpcodesForIf();
    testPrototypeThenDefinitionParsesAndCodegenEmitsOnlyOne();
    testTwoModifiersBeforeReturnTypeParseAsPrototype();
    testSingleModifierBeforeReturnTypeParsesAsPrototype();
    testUnrecognizedCharacterThrows();
    testFunctionTypeParameterParsesAsPrototype();
    testAsteriskParameterTypeParsesWithIsArrayTrue();
    testPlainParameterTypeDefaultsIsArrayFalse();
    testArrayCheckObjectParsesAndCodegens();
    testLogicalOperatorsTokenize();
    testGuardConditionShapeParsesWithCorrectPrecedence();
    testCodegenEmitsDupAndNotForLogicalAndUnary();
    testLogicalOrShortCircuitsWhenLeftTruthy();
    testLogicalOrEvaluatesRightWhenLeftFalsy();
    testLogicalAndShortCircuitsWhenLeftFalsy();
    testLogicalAndEvaluatesRightWhenLeftTruthy();
    testUnaryNotNegatesTruthiness();
    testGuardConditionDoesNotCrashOnEmptyArray();
    testCharLiteralsTokenizeToCorrectAsciiValue();
    testCharLiteralEscapeSequenceTokenizes();
    testMalformedCharLiteralThrows();
    testCharLiteralParsesAsIntLiteral();
    testStringIndexingReturnsByteValue();
    testStringIndexingOutOfBoundsThrows();
    testGuardConditionShapeWithRealStringIndexing();
    testDivisionAndModuloTokenizeAsSymbols();
    testCommentsStillWorkAfterSlashWhitelisting();
    testDivisionAndModuloParseToCorrectBinOp();
    testUnaryMinusParsesAsNegExpr();
    testMultiplicativeBindsTighterThanAdditive();
    testCodegenEmitsDivAndModOpcodes();
    testArithmeticVmExecution();
    testFloatLiteralsTokenizeAndVmExecute();
    testTrailingCommaInArrayAndMappingLiteralsParses();
    testArithmeticOnNonNumericOperandThrows();
    testDivisionAndModuloByZeroThrow();
    testRealBlockingLineArithmetic();
    testRangeDotDotTokenizes();
    testRangeIndexParsesWithRangeEndSet();
    testSingleIndexStillParsesWithNullRangeEnd();
    testRealShapeRangeIndexBindsToNameNotConcatenation();
    testCodegenEmitsRangeIndexOnlyWhenRangeEndPresent();
    testStringRangeIndexExecutesWithCorrectBounds();
    testStringRangeIndexNegativeStartThrows();
    testArrayRangeIndexExecutesWithCorrectBounds();
    testRealBlockingLineRangeIndex();
    testTernaryQuestionMarkTokenizes();
    testTernaryRealShapeTokenizes();
    testTernaryParsesToTernaryExpr();
    testParenthesizedTernaryParsesUnchanged();
    testTernaryRightAssociativity();
    testTernaryThenBranchNesting();
    testCodegenEmitsJumpOpcodesForTernary();
    testTernaryVmExecutesCorrectBranch();
    testTernaryOnlyEvaluatesTakenBranch();
    testRealBlockingLineTernary();
    testObjectVarDeclParsesSingleDeclaration();
    testObjectVarDeclParsesCommaSeparatedNames();
    testObjectVarDeclThenFunctionBothParse();
    testFunctionDeclarationStillParsesAfterPrefixRefactor();
    testObjectVarDeclArrayStarParses();
    testObjectVarDeclParsesInitializerExpression();
    testObjectVarDeclParsesInitializerInCommaList();
    testRealBlockingLinesObjectVarDecl();
    testCodegenEmitsPushObjectVarForRead();
    testCodegenEmitsStoreObjectVarForWrite();
    testCodegenLocalShadowsObjectVariableOfSameName();
    testCodegenDuplicateObjectVariableThrows();
    testCodegenUndeclaredVariableStillThrows();
    testObjectVariablePersistsAcrossSeparateCalls();
    testObjectVariableIsPerInstanceNotSharedAcrossObjects();
    testObjectVariableShadowedByLocalAtRuntime();
    testRealShapeMappingObjectVariableReadWrite();
    testObjectVariableReentrancySafeAcrossNestedCloneObject();
    testLocalVarDeclCommaSeparatedNamesParse();
    testLocalVarDeclCommaListVmExecution();
    testSiblingBlocksMayReuseALocalNameNeitherNestedInTheOther();
    testNameDeclaredInABlockIsUndeclaredOnceThatBlockEnds();
    testForLoopParsesToForStmt();
    testForLoopEmptyClausesParse();
    testForLoopWithAssignInitVmSumsExpectedTotal();
    testForLoopWithDeclInitAndIncDecUpdateVmExecution();
    testIncDecOperatorsTokenize();
    testPrefixAndPostfixIncDecParseToIncDecExpr();
    testPostfixIncDecOnRangeIndexTargetThrows();
    testPostfixIncDecOnIndexedTargetParsesToIndexedIncDecExpr();
    testIndexedPostfixIncDecVmExecutionReturnsOldValueAndMutates();
    testIndexedPrefixIncDecVmExecutionReturnsNewValueAndMutates();
    testPrefixIncrementVmExecutionReturnsNewValueAndMutates();
    testPostfixIncrementVmExecutionReturnsOldValueAndMutates();
    testBareCallToLocalFunctionEmitsCallOpcode();
    testSameObjectBareCallInvokesLocalFunctionAtRuntime();
    testBareCallFallsBackToEfunWhenNoLocalFunctionMatches();
    testUndefinedBareCallThrowsClearError();
    testSscanfParsesToSscanfExprWithVarNames();
    testSscanfNonIdentifierOutputArgThrows();
    testSscanfVmMatchesLiteralDelimitedTokens();
    testSscanfVmMatchesIntegerSpecifier();
    testSscanfVmSkipModifierDoesNotConsumeOutputSlot();
    testSscanfVmPartialMatchLeavesLaterVarsUntouchedAndReturnsPartialCount();
    testInheritStatementParsesPathAndConcatenation();
    testInheritedFunctionFallbackInvokedAtRuntime();
    testInheritedObjectVariableSlotsShareStorageWithParent();
    testInheritCycleDetectedAsCompileFailure();
    testTypeCastParsesAsNoOpWrappingInnerExpr();
    testTypeCastVmExecutionIsNoOp();
    testStringLiteralMatchingOperatorTextParsesAsLiteralNotOperator();
    testCallOtherWithVariableFunctionNameVmExecution();
    testIndexThenCallOtherOnResultVmExecution();
    testEfunOverrideBypassesLocalFunctionOfSameName();
    testCompoundAssignOperatorsTokenize();
    testCompoundAssignParsesToCompoundAssignExpr();
    testCompoundAssignVmExecutionOnIntAndArray();
    testBitAndParsesToBinaryExprWithBitAndOp();
    testBitAndVmExecutionOnInts();
    testBitAndVmExecutionOnArraysIsIntersection();
    testBitOrAndBitXorVmExecutionOnInts();
    testCatchEvaluatesToErrorMessageStringWhenGuardedExprThrows();
    testCatchEvaluatesToZeroAndDiscardsGuardedExprValueOnSuccess();
    testExecutionContinuesNormallyAfterCatchTrapsAnError();
    testNestedCatchInnerFailureDoesNotTriggerOuterCatch();
    testCatchLogsTrappedErrorToStderrByDefault();
    testCatchTrapsErrorThrownInsideCalledFunctionWithNoCatchOfItsOwn();
    testCatchInlineInsideIfConditionMatchesMasterConnectShape();
    testAdjacentStringLiteralsParseAsSingleConcatenatedLiteral();
    testAdjacentStringLiteralsVmExecution();
    testBreakAndContinueParseToDedicatedStmtNodes();
    testBreakOutsideLoopThrowsAtCodegen();
    testBreakStopsForLoopEarlyVmExecution();
    testContinueSkipsRestOfForLoopBodyVmExecution();
    testContinueInWhileLoopSkipsToConditionRecheckVmExecution();
    testBreakInInnerLoopDoesNotAffectOuterLoopVmExecution();
    testNullStatementAsLoopBodyParsesAndExecutesAsNoOp();
    testReadFileReturnsFileContentAndFalsyForMissingFile();
    testWriteFileThenReadFileRoundTrips();
    testCreateRuntimeErrorFailsLoadInsteadOfCrashing();
    testAbsoluteIncludePathResolvesAgainstMudlibRoot();
    testCppWarningsDoNotFailPreprocessing();
    testSimulEfunResolvesUnknownBareCallToSimulEfunObject();
    testLocalFunctionShadowsSimulEfunOfSameName();
    testHeredocTokenizesToStringWithLiteralContent();
    testHeredocVmExecutionMatchesRealShape();
    testUnterminatedHeredocThrows();
    testForeachSingleVarParsesToForeachStmt();
    testForeachTwoVarParsesWithValueVar();
    testForeachOverArraySumsElementsVmExecution();
    testForeachOverMappingSingleVarIteratesKeysVmExecution();
    testForeachOverMappingTwoVarGivesKeyValuePairsVmExecution();
    testBreakAndContinueInsideForeachVmExecution();
    testNestedForeachLoopsDoNotCollideVmExecution();
    testArrayUsableAsParameterNameNotReservedAsType();
    testTrailingVarargsEllipsisParsesAndIsDiscarded();
    testOpenEndedRangeIndexVmExecution();
    testForLoopCommaExprChainInInitAndUpdateVmExecution();
    testSwitchParsesToSwitchStmtWithInterleavedLabels();
    testSwitchRangeCaseLabelThrowsNotImplemented();
    testSwitchMatchingCaseVmExecution();
    testSwitchDefaultCaseVmExecution();
    testSwitchFallthroughWithoutBreakVmExecution();
    testContinueInsideSwitchInsideLoopTargetsLoopVmExecution();
    testEmbeddedAssignmentInsideLogicalAndBindsToImmediateVariable();
    testInputToRegistersPendingHandlerWithExtraArgsAndTargetObject();
    testInputToNumericFlagArgumentIsSkippedNotTreatedAsExtraArg();
    testInputToReturnsZeroWithNoActiveConnection();
    testDispatchLinePrefersPendingInputToHandlerOverProcessInput();
    testInputToCanReRegisterFromWithinDispatchedHandler();
    testLogonSendsBannerAndRegistersInputToPrompt();
    testCallOutAcceptsRealArgumentShapeAndReturnsHandle();
    testRemoveCallOutAlwaysReturnsMinusOneSinceNothingIsEverScheduled();
    testCallOtherWithStringTargetResolvesAlreadyLoadedObject();
    testCallOtherWithStringTargetAutoCompilesAndLoadsOnFirstUse();
    testCallOtherWithStringTargetToNonexistentFileThrows();
    testConvertNameMudlibFunctionWorksWithNewLowerCaseAndReplaceStringEfuns();
    testBareParentCallInvokesInheritedFunctionNotLocalOverride();
    testQualifiedParentCallMatchesInheritPathBasename();
    testParentCallOnFileWithNoInheritThrows();
    testClosureLiteralParsesToClosureLiteralExprBareForm();
    testClosureLiteralParsesToClosureLiteralExprWithBoundArgs();
    testClosureLiteralVmExecutionProducesClosureValueWithOwnerAndBoundArgs();
    testEvaluateInvokesEfunBoundClosureWithBoundArgsBeforeExtraArgs();
    testFuncallIsAnAliasOfEvaluate();
    testEvaluateOnNonFunctionValueIsSilentNoOp();
    testEvaluateInvokesLocalFunctionBoundClosure();
    testEvaluateThrowsWhenClosureOwnerDestructed();
    testCallOutAcceptsClosureAsFirstArgument();
    testPreviousObjectReturnsCallerAcrossCallOther();
    testPreviousObjectDoesNotChangeAcrossSameObjectLocalCall();
    testPreviousObjectMinusOneReturnsFullChain();
    testUnguardedClosureRoundTripsThroughSecurityAndMasterShape();
    testSaveObjectRestoreObjectRoundTripsNestedMappingsAndArrays();
    testEvaluateOfEfunBoundClosureSetsCurrentObjectToClosureOwnerNotCaller();
    testLoadObjectFallsBackToCompileObjectOnMissingSourceFile();
    testLoadVirtualObjectRebindsFilenameToVirtualPath();
    testLoadObjectCachesVirtualObjectAcrossRepeatedCalls();
    testLoadObjectReturnsNullWhenCompileObjectDeclines();
    testLoadObjectAndCloneObjectAutoPopulatePrivsFromMasterPrivsFile();
    testExplodeStripsAllLeadingSeparatorsAndNeverEmitsTrailingEmpty();
    testLoadObjectWithNoMasterLoadedFailsCleanlyOnMissingFile();
    testNewEfunIsAnAliasOfCloneObject();
    testStatusTypeKeywordParsesAsPlainIntSynonym();
    testFunctionDeclWithOnlyModifiersAndNoTypeParses();
    testBareBlockStatementScopesLocalsAndExecutesInline();
    testFromEndSingleIndexOnStringAndArray();
    testFromEndOpenRangeMatchesRealUserCShape();
    testFromEndBothBoundsOnRangeIndex();
    testFromEndStartBeyondLengthClampsToZeroInsteadOfThrowing();
    testCompoundIndexAssignOnSingleLevelMapping();
    testCompoundIndexAssignOnChainedNestedMappingMatchesRealUserCShape();
    testCompoundIndexAssignOnArrayElement();
    testIndexAssignAsSubExpressionParsesToIndexAssignExpr();
    testIndexAssignAsSubExpressionVmExecutionMatchesMoreCShape();
    testIndexAssignAsSubExpressionFallsThroughOnEmptyString();
    testInlineLambdaWithCallExpressionFirstOperandParsesAsInlineLambdaExpr();
    testInlineLambdaBareStringConstantParsesAsInlineLambdaExpr();
    testInlineLambdaVmExecutionEvaluatesBodyAtCallTimeNotConstructionTime();
    testFunctionPointerCallThroughParsesToForcedEvaluateCall();
    testFunctionPointerCallThroughOnIndexedTargetParses();
    testFunctionPointerCallThroughVmExecutionCallsClosure();
    testToIntPassesThroughAnInt();
    testToIntTruncatesAFloatTowardZero();
    testToIntParsesALeadingIntegerFromAStringIgnoringTrailingGarbage();
    testToIntReturnsZeroForAStringWithNoLeadingNumber();
    testPrivateObjectVariableDoesNotCollideWithChildsOwnSameNamedVariable();
    testSiblingLeafObjectVariablesDoNotAliasEachOther();
    testObjectVariableOffsetsComposeAcrossMultiLevelInheritChain();
    testEnvironmentDefaultsToNullBeforeAnyMove();
    testMoveObjectUpdatesEnvironmentAndInventory();
    testEnableCommandsGatesWhetherMoveObjectRegistersDestinationActions();
    testAllInventoryReturnsDirectChildrenOnlyNotGrandchildren();
    testDeepInventoryRecursesThroughNestedChildren();
    testStrcmpMatchesRealCComparisonSemantics();
    testMapDeleteRemovesKeyInPlaceAndLeavesOthersIntact();
    testCloneObjectAcceptsPathWithTrailingDotCWithoutDoublingExtension();
    testIntpTrueOnlyForIntNotStringObjectOrUnsetVariable();
    testRepeatStringConcatenatesNTimesAndEmptyForZeroOrNegative();
    testPresentFindsInventoryItemByIdApplyNotByOtherFunctions();
    testLivingReflectsEnableCommandsStateAndDefaultsToCurrentObject();
    testAddActionExactVerbMatchDispatchesWithRemainderAsArgumentAndDeclinesUnknownVerbs();
    testAddActionCatchAllShortFlagReceivesRemainderAndQueryVerbReturnsFullTypedWord();
    testDispatchCommandTriesNextMatchWhenFirstHandlerReturnsFalsy();
    testThisPlayerReturnsCommandGiverDuringDispatch();
    testQueryVerbReturnsZeroOutsideOfDispatch();
    testQueryPrivsReturnsZeroWhenNeverSet();
    testSetPrivsThenQueryPrivsRoundTripsAndClearsOnNonStringArgument();
    testObjectVarInitializerRunsBeforeCreate();
    testObjectVarInitializerRunsOnFileWithNoCreateAtAll();
    testObjectVarInitializerParentRunsBeforeChild();
    testUndefinedpTrueOnlyForVoidNotZeroOrOtherTypes();
    testParentCallToFunctionOnlyChildDefinesResolvesAtRuntime();
    testBareCallFromParentReachesChildsOverrideNotItsOwnLexicalDefinition();
    testArraySubtractionRemovesEveryMatchingElementPreservingOrder();
    testArraySubtractionOnEmptyRightOperandLeavesLeftUnchanged();
    testMonostateParticipatesInArithmeticAsRealZero();
    testStringPlusIntAppendsDecimalDigits();
    testIntPlusStringPrependsDecimalDigits();
    testStringPlusMissingMappingKeyFormatsAsZero();
    testObjectPlusStringPrependsItsFilename();
    testRandomOfNonPositiveArgumentIsZero();
    testRandomStaysWithinZeroToNExclusiveAcrossManyDraws();
    testSetHeartBeatThenQueryHeartBeatRoundTrips();
    testMapArrayWithStringFunctionNameCallsMethodOnTargetForEachElement();
    testFilterArrayWithStringFunctionNameKeepsOnlyTruthyElements();
    testSortArrayWithStringFunctionNameOrdersByComparatorResult();
    testImplodeJoinsStringArrayWithSeparator();
    testImplodeOnEmptyArrayReturnsEmptyString();
    testSprintfPercentCEmitsSingleCharacterFromIntArgument();
    testSprintfPercentCThrowsOnNonIntArgument();
    testSprintfLeftJustifiedFieldWidthPadsWithSpaces();
    testSprintfRightJustifiedFieldWidthPadsWithSpaces();
    testSprintfZeroPaddedFieldWidthPadsWithZeros();
    testSprintfFieldWidthDoesNotTruncateAWiderValue();
    testSprintfStringFieldWidthLeftJustifies();
    testSprintfDoublePercentEmitsLiteralPercentAndConsumesNoArgument();
    testSprintfColonFieldWidthPadsAShorterStringLeftJustified();
    testSprintfColonFieldWidthTruncatesALongerString();
    testSprintfBuildingAndThenUsingADynamicColonFormatString();
    std::cout << "all tests passed\n";
    return 0;
}
