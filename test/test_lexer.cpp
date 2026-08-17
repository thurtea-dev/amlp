#include "amlp/compiler/Lexer.hpp"
#include "amlp/compiler/Parser.hpp"
#include "amlp/compiler/CodeGen.hpp"
#include "amlp/compiler/Ast.hpp"
#include "amlp/vm/Bytecode.hpp"
#include "amlp/vm/Value.hpp"
#include "amlp/vm/VM.hpp"
#include "amlp/object/LpcObject.hpp"
#include "amlp/object/ObjectManager.hpp"
#include "amlp/config/Config.hpp"
#include "amlp/efun/EfunTable.hpp"
#include "amlp/core/Errors.hpp"
#include "amlp/net/Connection.hpp"
#include "amlp/net/OutputContext.hpp"
#include "amlp/net/Server.hpp"
#include "amlp/net/InteractiveRegistry.hpp"
#include "amlp/net/SocketRegistry.hpp"
#include "amlp/scheduler/Scheduler.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <fstream>
#include <sys/stat.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sstream>
#include <chrono>
#include <thread>
#include <functional>
#include <cstring>

static void testBasicTokenize() {
    std::string src =
        "void create() {\n"
        "    write(\"Hello from simple_login.c create()!\");\n"
        "}\n";

    amlp::Lexer lexer(src);
    auto tokens = lexer.tokenize();

    assert(tokens.size() == 12);
    assert(tokens[0].type == amlp::TokenType::Keyword && tokens[0].text == "void");
    assert(tokens[1].type == amlp::TokenType::Ident && tokens[1].text == "create");
    assert(tokens[7].type == amlp::TokenType::String);
    assert(tokens[7].text == "Hello from simple_login.c create()!");

    std::cout << "testBasicTokenize OK\n";
}

static void testArrowTokenizes() {
    amlp::Lexer lexer("ob->greet();");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 7);
    assert(tokens[1].type == amlp::TokenType::Symbol && tokens[1].text == "->");
    std::cout << "testArrowTokenizes OK\n";
}

static void testComparisonOperatorsTokenize() {
    amlp::Lexer lexer("a == b != c <= d >= e < f > g");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"==", "!=", "<=", ">=", "<", ">"};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == amlp::TokenType::Symbol) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* callOther = dynamic_cast<amlp::CallOtherExpr*>(exprStmt->expr.get());
    assert(callOther != nullptr);
    // Literal call_other(target, "name", ...) still parses its function
    // name down to a StringLiteral, same as before this became a general
    // expression (see Ast.hpp's CallOtherExpr comment).
    auto* funcLit = dynamic_cast<amlp::StringLiteral*>(callOther->function.get());
    assert(funcLit != nullptr);
    assert(funcLit->value == "greet");

    std::cout << "testCallOtherParsesToCallOtherExpr OK\n";
}

static void testArrowOperatorParsesToCallOtherExpr() {
    std::string src =
        "void create() {\n"
        "    clone_object(\"/obj/x\")->greet_again();\n"
        "}\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    auto* callOther = dynamic_cast<amlp::CallOtherExpr*>(exprStmt->expr.get());
    assert(callOther != nullptr);
    auto* funcLit = dynamic_cast<amlp::StringLiteral*>(callOther->function.get());
    assert(funcLit != nullptr);
    assert(funcLit->value == "greet_again");

    std::cout << "testArrowOperatorParsesToCallOtherExpr OK\n";
}

static void testCodegenEmitsCallEfunForCallOther() {
    std::string src =
        "void create() {\n"
        "    call_other(clone_object(\"/obj/x\"), \"greet\");\n"
        "}\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    // clone_object(...) is a plain bare call, so it compiles to OpCode::Call
    // now (resolved local-function-first, efun-fallback at run time -- see
    // the same-object-calls slice); call_other() is always forced to a
    // real CallEfun regardless of any local function named "call_other",
    // since it is a compiler-level construct, not an ordinary efun lookup.
    bool sawCloneObject = false;
    bool sawCallOther = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::Call) {
            const std::string& name = compiled.stringPool[instr.operand];
            if (name == "clone_object") sawCloneObject = true;
        }
        if (instr.op == amlp::OpCode::CallEfun) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    assert(program->functions.size() == 1);
    assert(program->functions[0]->params.size() == 1);
    assert(program->functions[0]->params[0].type == "string");
    assert(program->functions[0]->params[0].name == "msg");

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* call = dynamic_cast<amlp::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr);
    auto* ref = dynamic_cast<amlp::VarRefExpr*>(call->args[0].get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 3);

    auto* decl = dynamic_cast<amlp::VarDeclStmt*>(body[0].get());
    assert(decl != nullptr);
    assert(decl->type == "string");
    assert(decl->name == "s");
    assert(decl->initializer == nullptr);

    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[1].get());
    assert(assign != nullptr);
    assert(assign->name == "s");
    auto* rhsRef = dynamic_cast<amlp::VarRefExpr*>(assign->value.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    assert(compiled.functions.size() == 1);
    assert(compiled.functions[0].numArgs == 1);
    assert(compiled.functions[0].numLocals == 2);

    bool sawPushLocal = false;
    bool sawStoreLocal = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::PushLocal) sawPushLocal = true;
        if (instr.op == amlp::OpCode::StoreLocal) sawStoreLocal = true;
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 1);
    auto* ifStmt = dynamic_cast<amlp::IfStmt*>(body[0].get());
    assert(ifStmt != nullptr);
    assert(ifStmt->thenBranch != nullptr);
    assert(ifStmt->elseBranch != nullptr);

    auto* cond = dynamic_cast<amlp::BinaryExpr*>(ifStmt->condition.get());
    assert(cond != nullptr);
    assert(cond->op == amlp::BinOp::Eq);

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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 3);
    auto* whileStmt = dynamic_cast<amlp::WhileStmt*>(body[2].get());
    assert(whileStmt != nullptr);

    auto* cond = dynamic_cast<amlp::BinaryExpr*>(whileStmt->condition.get());
    assert(cond != nullptr);
    assert(cond->op == amlp::BinOp::Lt);

    std::cout << "testWhileParsesToWhileStmt OK\n";
}

// "do body while (condition);" -- real LPC/C post-test loop, previously
// entirely missing (not in the lexer's own keyword list at all). Parse
// shape only here, mirroring testWhileParsesToWhileStmt above; VM-
// execution coverage (the one thing that actually distinguishes a
// do-while from a while -- the body always runs at least once) lives
// alongside the while-loop break/continue VM-execution tests further
// down, since it needs the runProbe() helper defined below this point.
static void testDoWhileParsesToDoWhileStmt() {
    std::string src =
        "void create() {\n"
        "    int i;\n"
        "    i = 0;\n"
        "    do {\n"
        "        i = i;\n"
        "    } while (i < 3);\n"
        "}\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 3);
    auto* doWhileStmt = dynamic_cast<amlp::DoWhileStmt*>(body[2].get());
    assert(doWhileStmt != nullptr);

    auto* cond = dynamic_cast<amlp::BinaryExpr*>(doWhileStmt->condition.get());
    assert(cond != nullptr);
    assert(cond->op == amlp::BinOp::Lt);

    std::cout << "testDoWhileParsesToDoWhileStmt OK\n";
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawEq = false, sawJumpIfFalse = false, sawJump = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::Eq) sawEq = true;
        if (instr.op == amlp::OpCode::JumpIfFalse) sawJumpIfFalse = true;
        if (instr.op == amlp::OpCode::Jump) sawJump = true;
    }
    assert(sawEq);
    assert(sawJumpIfFalse);
    assert(sawJump);

    // Every jump target must be a valid in-range instruction index.
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::Jump || instr.op == amlp::OpCode::JumpIfFalse) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 2);
    assert(program->functions[0]->name == "create");
    assert(program->functions[0]->body == nullptr);
    assert(program->functions[1]->name == "create");
    assert(program->functions[1]->body != nullptr);

    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    assert(compiled.functions.size() == 1);
    assert(compiled.functions[0].name == "create");

    // write(...) is a plain bare call -> OpCode::Call (see
    // testCodegenEmitsCallEfunForCallOther's comment on the same-object
    // calls slice).
    bool sawWrite = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::Call) {
            const std::string& name = compiled.stringPool[instr.operand];
            if (name == "write") sawWrite = true;
        }
    }
    assert(sawWrite);

    std::cout << "testPrototypeThenDefinitionParsesAndCodegenEmitsOnlyOne OK\n";
}

static void testTwoModifiersBeforeReturnTypeParseAsPrototype() {
    std::string src = "static private void load_access(string cfg, mapping ref);\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    bool threw = false;
    try {
        lexer.tokenize();
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("unrecognized character") != std::string::npos);
    }
    assert(threw);
    std::cout << "testUnrecognizedCharacterThrows OK\n";
}

static void testFunctionTypeParameterParsesAsPrototype() {
    std::string src = "mixed apply_unguarded(function f);\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 10);

    auto* itemsDecl = dynamic_cast<amlp::VarDeclStmt*>(body[0].get());
    assert(itemsDecl != nullptr);
    assert(itemsDecl->isArray == true);
    assert(itemsDecl->type == "mixed");
    assert(itemsDecl->name == "items");

    auto* itemsAssign = dynamic_cast<amlp::AssignStmt*>(body[3].get());
    assert(itemsAssign != nullptr);
    auto* arrLit = dynamic_cast<amlp::ArrayLiteralExpr*>(itemsAssign->value.get());
    assert(arrLit != nullptr);
    assert(arrLit->elements.size() == 3);

    auto* scoresAssign = dynamic_cast<amlp::AssignStmt*>(body[4].get());
    assert(scoresAssign != nullptr);
    auto* mapLit = dynamic_cast<amlp::MappingLiteralExpr*>(scoresAssign->value.get());
    assert(mapLit != nullptr);
    assert(mapLit->entries.empty());

    auto* indexAssign = dynamic_cast<amlp::IndexAssignStmt*>(body[5].get());
    assert(indexAssign != nullptr);

    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawMakeArray = false, sawMakeMapping = false, sawIndex = false, sawIndexAssign = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::MakeArray) sawMakeArray = true;
        if (instr.op == amlp::OpCode::MakeMapping) sawMakeMapping = true;
        if (instr.op == amlp::OpCode::Index) sawIndex = true;
        if (instr.op == amlp::OpCode::IndexAssign) sawIndexAssign = true;
    }
    assert(sawMakeArray);
    assert(sawMakeMapping);
    assert(sawIndex);
    assert(sawIndexAssign);

    std::cout << "testArrayCheckObjectParsesAndCodegens OK\n";
}

static void testLogicalOperatorsTokenize() {
    amlp::Lexer lexer("a || b && c");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"||", "&&"};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == amlp::TokenType::Symbol) {
            assert(t.text == expectedOps[opIdx]);
            ++opIdx;
        }
    }
    assert(opIdx == expectedOps.size());

    // A lone '|' or '&' (not doubled) still lexes fine as its own
    // one-character Symbol token, same as a lone '-' already does today.
    amlp::Lexer loneLexer("a | b & c");
    auto loneTokens = loneLexer.tokenize();
    std::vector<std::string> expectedLoneOps = {"|", "&"};
    size_t loneOpIdx = 0;
    for (const auto& t : loneTokens) {
        if (t.type == amlp::TokenType::Symbol) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ifStmt = dynamic_cast<amlp::IfStmt*>(body[2].get());
    assert(ifStmt != nullptr);

    // Outermost operator must be Or: "(!lines[i] || lines[i] == "") || (lines[i] == "#")".
    auto* outer = dynamic_cast<amlp::BinaryExpr*>(ifStmt->condition.get());
    assert(outer != nullptr);
    assert(outer->op == amlp::BinOp::Or);

    // Its left side is itself an Or, whose left side is a UnaryExpr(Not)
    // over an IndexExpr, and whose right side is an Eq comparison. This
    // confirms == binds tighter than ||, and ! binds only to its
    // immediate operand.
    auto* innerOr = dynamic_cast<amlp::BinaryExpr*>(outer->left.get());
    assert(innerOr != nullptr);
    assert(innerOr->op == amlp::BinOp::Or);

    auto* notExpr = dynamic_cast<amlp::UnaryExpr*>(innerOr->left.get());
    assert(notExpr != nullptr);
    assert(notExpr->op == amlp::UnaryOp::Not);
    auto* notOperand = dynamic_cast<amlp::IndexExpr*>(notExpr->operand.get());
    assert(notOperand != nullptr);

    auto* firstEq = dynamic_cast<amlp::BinaryExpr*>(innerOr->right.get());
    assert(firstEq != nullptr);
    assert(firstEq->op == amlp::BinOp::Eq);

    auto* secondEq = dynamic_cast<amlp::BinaryExpr*>(outer->right.get());
    assert(secondEq != nullptr);
    assert(secondEq->op == amlp::BinOp::Eq);

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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawDup = false, sawNot = false, sawJumpIfFalse = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::Dup) sawDup = true;
        if (instr.op == amlp::OpCode::Not) sawNot = true;
        if (instr.op == amlp::OpCode::JumpIfFalse) sawJumpIfFalse = true;
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
        if (instr.op == amlp::OpCode::Jump || instr.op == amlp::OpCode::JumpIfFalse) {
            assert(instr.operand >= 0);
            assert(static_cast<size_t>(instr.operand) <= compiled.code.size());
            if (instr.op == amlp::OpCode::JumpIfFalse &&
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
static amlp::Value runProbe(const std::string& probeBody) {
    std::string src = "int probe() {\n" + probeBody + "\n}\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = std::make_shared<amlp::CompiledProgram>(codegen.generate(*program));

    auto obj = std::make_shared<amlp::LpcObject>("probe_object", compiled);
    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    return vm.callFunction(obj, "probe", {});
}

static void testLogicalOrShortCircuitsWhenLeftTruthy() {
    // If || evaluated its right operand unconditionally, calling this
    // nonexistent efun would throw "undefined efun" and fail the test.
    // It must never run because "x" (1) already decides the result.
    amlp::Value result = runProbe(
        "int x;\n"
        "x = 1;\n"
        "return x || nonexistent_marker_efun();\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testLogicalOrShortCircuitsWhenLeftTruthy OK\n";
}

static void testLogicalOrEvaluatesRightWhenLeftFalsy() {
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
        "int x;\n"
        "x = 0;\n"
        "return x && nonexistent_marker_efun();\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testLogicalAndShortCircuitsWhenLeftFalsy OK\n";
}

static void testLogicalAndEvaluatesRightWhenLeftTruthy() {
    amlp::Value result = runProbe(
        "int x;\n"
        "x = 1;\n"
        "return x && 5;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 5);

    std::cout << "testLogicalAndEvaluatesRightWhenLeftTruthy OK\n";
}

static void testUnaryNotNegatesTruthiness() {
    amlp::Value resultOfFalsy = runProbe(
        "int x;\n"
        "x = 0;\n"
        "return !x;\n");
    assert(std::holds_alternative<int64_t>(resultOfFalsy.data));
    assert(std::get<int64_t>(resultOfFalsy.data) == 1);

    amlp::Value resultOfTruthy = runProbe(
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
    amlp::Value result = runProbe(
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
        amlp::Lexer lexer(c.src);
        auto tokens = lexer.tokenize();
        assert(tokens.size() == 2); // the literal, then End
        assert(tokens[0].type == amlp::TokenType::Number);
        assert(tokens[0].text == c.expected);
    }

    std::cout << "testCharLiteralsTokenizeToCorrectAsciiValue OK\n";
}

static void testCharLiteralEscapeSequenceTokenizes() {
    // Not present anywhere in master.c today, but the lexChar() design
    // mirrors lexString()'s escape table, so this confirms it actually
    // works, not just that the plain-character case does.
    amlp::Lexer lexer("'\\n'");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 2);
    assert(tokens[0].type == amlp::TokenType::Number);
    assert(tokens[0].text == "10");

    std::cout << "testCharLiteralEscapeSequenceTokenizes OK\n";
}

static void testMalformedCharLiteralThrows() {
    bool threwForEmpty = false;
    try {
        amlp::Lexer lexer("''");
        lexer.tokenize();
    } catch (const amlp::LpcRuntimeError&) {
        threwForEmpty = true;
    }
    assert(threwForEmpty);

    bool threwForTwoChars = false;
    try {
        amlp::Lexer lexer("'ab'");
        lexer.tokenize();
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ifStmt = dynamic_cast<amlp::IfStmt*>(body[1].get());
    assert(ifStmt != nullptr);

    auto* cond = dynamic_cast<amlp::BinaryExpr*>(ifStmt->condition.get());
    assert(cond != nullptr);
    assert(cond->op == amlp::BinOp::Eq);

    // No new AST node for character literals: the right-hand side must
    // be a plain IntLiteral, same as any other integer literal, per the
    // plan's decision to reuse the existing pipeline instead of adding
    // a dedicated CharLiteral node.
    auto* rightLit = dynamic_cast<amlp::IntLiteral*>(cond->right.get());
    assert(rightLit != nullptr);
    assert(rightLit->value == 35);

    std::cout << "testCharLiteralParsesAsIntLiteral OK\n";
}

static void testStringIndexingReturnsByteValue() {
    amlp::Value matchResult = runProbe(
        "string s;\n"
        "s = \"#comment\";\n"
        "return s[0] == '#';\n");
    assert(std::holds_alternative<int64_t>(matchResult.data));
    assert(std::get<int64_t>(matchResult.data) == 1);

    amlp::Value noMatchResult = runProbe(
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
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Value skipOnComment = runProbe(
        "string s;\n"
        "s = \"#comment\";\n"
        "if (!s || s == \"\" || s[0] == '#') {\n"
        "    return 1;\n"
        "}\n"
        "return 0;\n");
    assert(std::holds_alternative<int64_t>(skipOnComment.data));
    assert(std::get<int64_t>(skipOnComment.data) == 1);

    amlp::Value skipOnEmpty = runProbe(
        "string s;\n"
        "s = \"\";\n"
        "if (!s || s == \"\" || s[0] == '#') {\n"
        "    return 1;\n"
        "}\n"
        "return 0;\n");
    assert(std::holds_alternative<int64_t>(skipOnEmpty.data));
    assert(std::get<int64_t>(skipOnEmpty.data) == 1);

    amlp::Value keepNormalLine = runProbe(
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
    amlp::Lexer lexer("t/60 t%60");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"/", "%"};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == amlp::TokenType::Symbol) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* divAssign = dynamic_cast<amlp::AssignStmt*>(body[3].get());
    assert(divAssign != nullptr);
    auto* divExpr = dynamic_cast<amlp::BinaryExpr*>(divAssign->value.get());
    assert(divExpr != nullptr);
    assert(divExpr->op == amlp::BinOp::Div);

    auto* modAssign = dynamic_cast<amlp::AssignStmt*>(body[4].get());
    assert(modAssign != nullptr);
    auto* modExpr = dynamic_cast<amlp::BinaryExpr*>(modAssign->value.get());
    assert(modExpr != nullptr);
    assert(modExpr->op == amlp::BinOp::Mod);

    std::cout << "testDivisionAndModuloParseToCorrectBinOp OK\n";
}

static void testUnaryMinusParsesAsNegExpr() {
    std::string src =
        "void create() {\n"
        "    int x;\n"
        "    x = -1;\n"
        "}\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[1].get());
    assert(assign != nullptr);

    // No literal-only special case: -1 must parse through the general
    // unary path as a UnaryExpr(Neg) wrapping a plain IntLiteral(1),
    // not as some dedicated negative-literal token.
    auto* negExpr = dynamic_cast<amlp::UnaryExpr*>(assign->value.get());
    assert(negExpr != nullptr);
    assert(negExpr->op == amlp::UnaryOp::Neg);

    auto* innerLit = dynamic_cast<amlp::IntLiteral*>(negExpr->operand.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[4].get());
    assert(assign != nullptr);

    auto* outer = dynamic_cast<amlp::BinaryExpr*>(assign->value.get());
    assert(outer != nullptr);
    assert(outer->op == amlp::BinOp::Sub);

    auto* right = dynamic_cast<amlp::BinaryExpr*>(outer->right.get());
    assert(right != nullptr);
    assert(right->op == amlp::BinOp::Mul);

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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawDiv = false, sawMod = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::Div) sawDiv = true;
        if (instr.op == amlp::OpCode::Mod) sawMod = true;
    }
    assert(sawDiv);
    assert(sawMod);

    std::cout << "testCodegenEmitsDivAndModOpcodes OK\n";
}

static void testArithmeticVmExecution() {
    amlp::Value subResult = runProbe("return 10 - 3;\n");
    assert(std::holds_alternative<int64_t>(subResult.data));
    assert(std::get<int64_t>(subResult.data) == 7);

    amlp::Value mulResult = runProbe("return 4 * 5;\n");
    assert(std::holds_alternative<int64_t>(mulResult.data));
    assert(std::get<int64_t>(mulResult.data) == 20);

    amlp::Value divResult = runProbe("return 17 / 5;\n");
    assert(std::holds_alternative<int64_t>(divResult.data));
    assert(std::get<int64_t>(divResult.data) == 3);

    amlp::Value modResult = runProbe("return 17 % 5;\n");
    assert(std::holds_alternative<int64_t>(modResult.data));
    assert(std::get<int64_t>(modResult.data) == 2);

    amlp::Value negResult = runProbe("return -1;\n");
    assert(std::holds_alternative<int64_t>(negResult.data));
    assert(std::get<int64_t>(negResult.data) == -1);

    std::cout << "testArithmeticVmExecution OK\n";
}

// Found compiling the real secure/SimulEfun/SimulEfun.c (misc.c's own
// "return 0.0;" and "percent()"'s trailing-digit/leading-dot mix): float
// literals were not lexed at all -- lexNumber() only ever consumed the
// integer part, leaving a bare '.' for the caller to trip over.
static void testFloatLiteralsTokenizeAndVmExecute() {
    amlp::Value trailing = runProbe("return 1.5;\n");
    assert(std::holds_alternative<double>(trailing.data));
    assert(std::get<double>(trailing.data) == 1.5);

    amlp::Value leading = runProbe("return .5;\n");
    assert(std::holds_alternative<double>(leading.data));
    assert(std::get<double>(leading.data) == 0.5);

    amlp::Value zero = runProbe("return 0.0;\n");
    assert(std::holds_alternative<double>(zero.data));
    assert(std::get<double>(zero.data) == 0.0);

    // Mixed int/float arithmetic promotes to float, and a float local
    // variable round-trips through PushObjectVar/StoreObjectVar-style
    // local storage the same as any other Value.
    amlp::Value mixed = runProbe(
        "float f;\n"
        "f = 1.5 + 2;\n"
        "return f;\n");
    assert(std::holds_alternative<double>(mixed.data));
    assert(std::get<double>(mixed.data) == 3.5);

    // The real code's own range-terminated shape ("arr[a..b]") must still
    // tokenize as the ".." range operator, not misfire as a float, now
    // that '.' can also start a number.
    amlp::Value rangeStillWorks = runProbe(
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
    amlp::Value arr = runProbe(
        "mixed *items;\n"
        "items = ({ 1, 2, 3, });\n"
        "return sizeof(items);\n");
    assert(std::holds_alternative<int64_t>(arr.data));
    assert(std::get<int64_t>(arr.data) == 3);

    amlp::Value map = runProbe(
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
    } catch (const amlp::LpcRuntimeError&) {
        subThrew = true;
    }
    assert(subThrew);

    bool mulThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return s * 1;\n");
    } catch (const amlp::LpcRuntimeError&) {
        mulThrew = true;
    }
    assert(mulThrew);

    bool divThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return s / 1;\n");
    } catch (const amlp::LpcRuntimeError&) {
        divThrew = true;
    }
    assert(divThrew);

    bool modThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return s % 1;\n");
    } catch (const amlp::LpcRuntimeError&) {
        modThrew = true;
    }
    assert(modThrew);

    bool negThrew = false;
    try {
        runProbe(
            "string s;\n"
            "s = \"x\";\n"
            "return -s;\n");
    } catch (const amlp::LpcRuntimeError&) {
        negThrew = true;
    }
    assert(negThrew);

    std::cout << "testArithmeticOnNonNumericOperandThrows OK\n";
}

static void testDivisionAndModuloByZeroThrow() {
    bool divThrew = false;
    try {
        runProbe("return 5 / 0;\n");
    } catch (const amlp::LpcRuntimeError&) {
        divThrew = true;
    }
    assert(divThrew);

    bool modThrew = false;
    try {
        runProbe("return 5 % 0;\n");
    } catch (const amlp::LpcRuntimeError&) {
        modThrew = true;
    }
    assert(modThrew);

    std::cout << "testDivisionAndModuloByZeroThrow OK\n";
}

static void testRealBlockingLineArithmetic() {
    // Reproduces the exact arithmetic from the real blocking line:
    //   write("("+(t/60)+"."+(t%60)+")\n");
    // with t = 125, matching 2 minutes and 5 seconds.
    amlp::Value divResult = runProbe(
        "int t;\n"
        "t = 125;\n"
        "return t / 60;\n");
    assert(std::holds_alternative<int64_t>(divResult.data));
    assert(std::get<int64_t>(divResult.data) == 2);

    amlp::Value modResult = runProbe(
        "int t;\n"
        "t = 125;\n"
        "return t % 60;\n");
    assert(std::holds_alternative<int64_t>(modResult.data));
    assert(std::get<int64_t>(modResult.data) == 5);

    std::cout << "testRealBlockingLineArithmetic OK\n";
}

static void testRangeDotDotTokenizes() {
    amlp::Lexer lexer("..");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 2); // the ".." symbol, then End
    assert(tokens[0].type == amlp::TokenType::Symbol);
    assert(tokens[0].text == "..");

    // The exact real shape, name[0..0], must tokenize as a single ".."
    // symbol between the two Number tokens, not two separate "." tokens
    // (which is not even a valid symbol on its own in this lexer).
    amlp::Lexer shapeLexer("name[0..0]");
    auto shapeTokens = shapeLexer.tokenize();
    std::vector<std::pair<amlp::TokenType, std::string>> expected = {
        {amlp::TokenType::Ident, "name"},
        {amlp::TokenType::Symbol, "["},
        {amlp::TokenType::Number, "0"},
        {amlp::TokenType::Symbol, ".."},
        {amlp::TokenType::Number, "0"},
        {amlp::TokenType::Symbol, "]"},
        {amlp::TokenType::End, ""},
    };
    assert(shapeTokens.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(shapeTokens[i].type == expected[i].first);
        assert(shapeTokens[i].text == expected[i].second);
    }

    // Regression check: an ordinary single index, name[0], must be
    // completely unaffected, no ".." token appears anywhere.
    amlp::Lexer singleLexer("name[0]");
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[2].get());
    assert(assign != nullptr);

    auto* idx = dynamic_cast<amlp::IndexExpr*>(assign->value.get());
    assert(idx != nullptr);
    assert(idx->rangeEnd != nullptr);

    auto* startLit = dynamic_cast<amlp::IntLiteral*>(idx->index.get());
    assert(startLit != nullptr);
    assert(startLit->value == 0);

    auto* endLit = dynamic_cast<amlp::IntLiteral*>(idx->rangeEnd.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[2].get());
    assert(assign != nullptr);

    auto* idx = dynamic_cast<amlp::IndexExpr*>(assign->value.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[2].get());
    assert(assign != nullptr);

    auto* outerAdd = dynamic_cast<amlp::BinaryExpr*>(assign->value.get());
    assert(outerAdd != nullptr);
    assert(outerAdd->op == amlp::BinOp::Add);

    auto* rangeIdx = dynamic_cast<amlp::IndexExpr*>(outerAdd->right.get());
    assert(rangeIdx != nullptr);
    assert(rangeIdx->rangeEnd != nullptr);

    auto* nameRef = dynamic_cast<amlp::VarRefExpr*>(rangeIdx->target.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawRangeIndex = false, sawPlainIndex = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::RangeIndex) sawRangeIndex = true;
        if (instr.op == amlp::OpCode::Index) sawPlainIndex = true;
    }
    assert(sawRangeIndex);
    assert(sawPlainIndex);

    std::cout << "testCodegenEmitsRangeIndexOnlyWhenRangeEndPresent OK\n";
}

static void testStringRangeIndexExecutesWithCorrectBounds() {
    amlp::Value firstChar = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[0..0];\n");
    assert(std::holds_alternative<std::string>(firstChar.data));
    assert(std::get<std::string>(firstChar.data) == "h");

    amlp::Value middle = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[1..3];\n");
    assert(std::holds_alternative<std::string>(middle.data));
    assert(std::get<std::string>(middle.data) == "ell");

    amlp::Value whole = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[0..4];\n");
    assert(std::holds_alternative<std::string>(whole.data));
    assert(std::get<std::string>(whole.data) == "hello");

    // End beyond the string's length is clamped, not an error.
    amlp::Value clamped = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[0..99];\n");
    assert(std::holds_alternative<std::string>(clamped.data));
    assert(std::get<std::string>(clamped.data) == "hello");

    // An inverted range names no characters, and returns "" rather
    // than throwing.
    amlp::Value inverted = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[3..1];\n");
    assert(std::holds_alternative<std::string>(inverted.data));
    assert(std::get<std::string>(inverted.data).empty());

    // An empty target at [0..0] also names no characters.
    amlp::Value emptyTarget = runProbe(
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
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testStringRangeIndexNegativeStartThrows OK\n";
}

static void testArrayRangeIndexExecutesWithCorrectBounds() {
    amlp::Value middleSlice = runProbe(
        "mixed *items;\n"
        "mixed *sliced;\n"
        "items = ({ 1, 2, 3, 4 });\n"
        "sliced = items[1..2];\n"
        "return sizeof(sliced) == 2 && sliced[0] == 2 && sliced[1] == 3;\n");
    assert(std::holds_alternative<int64_t>(middleSlice.data));
    assert(std::get<int64_t>(middleSlice.data) == 1);

    // End beyond the array's length is clamped, not an error.
    amlp::Value clampedSlice = runProbe(
        "mixed *items;\n"
        "mixed *sliced;\n"
        "items = ({ 1, 2, 3 });\n"
        "sliced = items[0..99];\n"
        "return sizeof(sliced) == 3 && sliced[0] == 1 && sliced[2] == 3;\n");
    assert(std::holds_alternative<int64_t>(clampedSlice.data));
    assert(std::get<int64_t>(clampedSlice.data) == 1);

    // An inverted range yields an empty array, not an error.
    amlp::Value invertedSlice = runProbe(
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
    amlp::Value shard = runProbe(
        "string name;\n"
        "name = \"thurtea\";\n"
        "return name[0..0];\n");
    assert(std::holds_alternative<std::string>(shard.data));
    assert(std::get<std::string>(shard.data) == "t");

    std::cout << "testRealBlockingLineRangeIndex OK\n";
}

static void testTernaryQuestionMarkTokenizes() {
    amlp::Lexer lexer("?");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 2); // the "?" symbol, then End
    assert(tokens[0].type == amlp::TokenType::Symbol);
    assert(tokens[0].text == "?");

    std::cout << "testTernaryQuestionMarkTokenizes OK\n";
}

static void testTernaryRealShapeTokenizes() {
    // Real shape from secure/daemon/master.c raw line 433, minus its
    // surrounding parens: caught ? "catch" : "runtime". Confirms '?'
    // and the already-whitelisted ':' both come through correctly in
    // sequence.
    amlp::Lexer lexer("caught ? \"catch\" : \"runtime\"");
    auto tokens = lexer.tokenize();
    std::vector<std::pair<amlp::TokenType, std::string>> expected = {
        {amlp::TokenType::Ident, "caught"},
        {amlp::TokenType::Symbol, "?"},
        {amlp::TokenType::String, "catch"},
        {amlp::TokenType::Symbol, ":"},
        {amlp::TokenType::String, "runtime"},
        {amlp::TokenType::End, ""},
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[4].get());
    assert(assign != nullptr);

    auto* tern = dynamic_cast<amlp::TernaryExpr*>(assign->value.get());
    assert(tern != nullptr);

    auto* cond = dynamic_cast<amlp::VarRefExpr*>(tern->condition.get());
    assert(cond != nullptr);
    assert(cond->name == "a");

    auto* thenRef = dynamic_cast<amlp::VarRefExpr*>(tern->thenBranch.get());
    assert(thenRef != nullptr);
    assert(thenRef->name == "b");

    auto* elseRef = dynamic_cast<amlp::VarRefExpr*>(tern->elseBranch.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ret = dynamic_cast<amlp::ReturnStmt*>(body[1].get());
    assert(ret != nullptr);

    auto* tern = dynamic_cast<amlp::TernaryExpr*>(ret->expr.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[6].get());
    assert(assign != nullptr);

    auto* outer = dynamic_cast<amlp::TernaryExpr*>(assign->value.get());
    assert(outer != nullptr);

    auto* outerCond = dynamic_cast<amlp::VarRefExpr*>(outer->condition.get());
    assert(outerCond != nullptr);
    assert(outerCond->name == "a");

    auto* outerThen = dynamic_cast<amlp::VarRefExpr*>(outer->thenBranch.get());
    assert(outerThen != nullptr);
    assert(outerThen->name == "b");

    // a ? b : (c ? d : e), not a left-associative (incorrect) grouping.
    auto* inner = dynamic_cast<amlp::TernaryExpr*>(outer->elseBranch.get());
    assert(inner != nullptr);

    auto* innerCond = dynamic_cast<amlp::VarRefExpr*>(inner->condition.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assign = dynamic_cast<amlp::AssignStmt*>(body[6].get());
    assert(assign != nullptr);

    auto* outer = dynamic_cast<amlp::TernaryExpr*>(assign->value.get());
    assert(outer != nullptr);

    auto* inner = dynamic_cast<amlp::TernaryExpr*>(outer->thenBranch.get());
    assert(inner != nullptr);

    auto* innerCond = dynamic_cast<amlp::VarRefExpr*>(inner->condition.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    int jumpIfFalseIdx = -1, jumpIdx = -1;
    for (size_t i = 0; i < compiled.code.size(); ++i) {
        if (compiled.code[i].op == amlp::OpCode::JumpIfFalse) {
            assert(jumpIfFalseIdx == -1); // exactly one
            jumpIfFalseIdx = static_cast<int>(i);
        }
        if (compiled.code[i].op == amlp::OpCode::Jump) {
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
    amlp::Value truthy = runProbe("return 1 ? 10 : 20;\n");
    assert(std::holds_alternative<int64_t>(truthy.data));
    assert(std::get<int64_t>(truthy.data) == 10);

    amlp::Value falsy = runProbe("return 0 ? 10 : 20;\n");
    assert(std::holds_alternative<int64_t>(falsy.data));
    assert(std::get<int64_t>(falsy.data) == 20);

    amlp::Value fromComparison = runProbe("return (5 == 5) ? \"yes\" : \"no\";\n");
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
    amlp::Value result = runProbe("return 1 ? 1 : nonexistent_marker_efun();\n");
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
    amlp::Value nullCase = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.size() == 1);
    assert(program->objectVars[0]->name == "x");
    auto* init = dynamic_cast<amlp::IntLiteral*>(program->objectVars[0]->initializer.get());
    assert(init != nullptr && init->value == 5);

    std::cout << "testObjectVarDeclParsesInitializerExpression OK\n";
}

static void testObjectVarDeclParsesInitializerInCommaList() {
    // Same shape, but for the second-or-later name in a comma-separated
    // list, not just the first: "a" has no initializer, "b" does.
    std::string src = "int a, b = 5;\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->objectVars.size() == 2);
    assert(program->objectVars[0]->name == "a");
    assert(program->objectVars[0]->initializer == nullptr);
    assert(program->objectVars[1]->name == "b");
    auto* init = dynamic_cast<amlp::IntLiteral*>(program->objectVars[1]->initializer.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    assert(compiled.objectVarNames.size() == 1);
    assert(compiled.objectVarNames[0] == "ob");

    bool sawPushObjectVar = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::PushObjectVar) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawStoreObjectVar = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::StoreObjectVar) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    // The inner "x" is a local declared inside probe(), which must
    // shadow the object variable "x" of the same name: assigning to it
    // must emit StoreLocal, never StoreObjectVar, matching real LPC's
    // local-wins-over-global precedence (confirmed against the FluffOS
    // reference driver's compiler.c/grammar.y).
    bool sawStoreLocal = false, sawStoreObjectVar = false;
    for (const auto& instr : compiled.code) {
        if (instr.op == amlp::OpCode::StoreLocal) sawStoreLocal = true;
        if (instr.op == amlp::OpCode::StoreObjectVar) sawStoreObjectVar = true;
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;

    bool threw = false;
    try {
        codegen.generate(*program);
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;

    bool threw = false;
    try {
        codegen.generate(*program);
    } catch (const amlp::LpcRuntimeError&) {
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
static std::shared_ptr<amlp::LpcObject> compileProgramObject(const std::string& src) {
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = std::make_shared<amlp::CompiledProgram>(codegen.generate(*program));
    return std::make_shared<amlp::LpcObject>("program_object", compiled);
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    // Before any write, the slot holds real LPC's own default: the
    // integer 0 (see LpcObject.cpp's own comment -- not this driver's
    // separate monostate "no value" sentinel, which real LPC has no
    // equivalent of at the ordinary-declared-variable level).
    amlp::Value before = vm.callFunction(obj, "read_counter", {});
    assert(std::holds_alternative<int64_t>(before.data));
    assert(std::get<int64_t>(before.data) == 0);

    vm.callFunction(obj, "write_counter",
                     std::vector<amlp::Value>{amlp::Value(static_cast<int64_t>(7))});

    // A separate, later call must observe the earlier call's write:
    // object-variable state lives on the LpcObject instance, not in any
    // one call's locals.
    amlp::Value after = vm.callFunction(obj, "read_counter", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    vm.callFunction(objA, "write_counter",
                     std::vector<amlp::Value>{amlp::Value(static_cast<int64_t>(99))});

    amlp::Value aResult = vm.callFunction(objA, "read_counter", {});
    assert(std::holds_alternative<int64_t>(aResult.data));
    assert(std::get<int64_t>(aResult.data) == 99);

    // objB is a separate LpcObject instance (even though compiled from
    // the same source), so its own storage must be untouched -- still at
    // real LPC's own default (0), not objA's write.
    amlp::Value bResult = vm.callFunction(objB, "read_counter", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    vm.callFunction(obj, "set_object_x",
                     std::vector<amlp::Value>{amlp::Value(static_cast<int64_t>(1))});

    // shadow_probe()'s own local "x" must shadow the object variable: it
    // returns the local's value, not the object variable's.
    amlp::Value shadowed = vm.callFunction(obj, "shadow_probe", {});
    assert(std::holds_alternative<int64_t>(shadowed.data));
    assert(std::get<int64_t>(shadowed.data) == 999);

    // The object variable itself must be untouched by shadow_probe()'s
    // local, confirmed by a separate function with no local named "x".
    amlp::Value objectX = vm.callFunction(obj, "read_object_x", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    vm.callFunction(obj, "set_groups", {});
    amlp::Value result = vm.callFunction(obj, "get_a", {});
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
    amlp::Config config;
    amlp::ObjectManager objects;
    amlp::VM vm;

    // extraConfigLines: raw "key: value\n" lines appended after the
    // fixed defaults below, for the rare test that needs a config key
    // none of the other ~500 tests using this same harness do (e.g.
    // "global_include_file: <name.h>\n") -- empty by default, so every
    // existing call site (ObjectVarHarness() with no arguments) is
    // completely unaffected.
    explicit ObjectVarHarness(const std::string& extraConfigLines = "")
        : objects(config), vm(objects, config) {
        objects.setVM(&vm);

        char dirTemplate[] = "/tmp/amlp_objvar_test_XXXXXX";
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
        cfg << extraConfigLines;
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
    amlp::Value result = harness.vm.callFunction(outer, "probe", {});
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    assert(body.size() == 1);
    auto* block = dynamic_cast<amlp::Block*>(body[0].get());
    assert(block != nullptr);
    assert(block->statements.size() == 3);

    std::vector<std::string> names;
    for (auto& stmt : block->statements) {
        auto* decl = dynamic_cast<amlp::VarDeclStmt*>(stmt.get());
        assert(decl != nullptr);
        names.push_back(decl->name);
    }
    assert(names[0] == "file");
    assert(names[1] == "fl");
    assert(names[2] == "ac");

    std::cout << "testLocalVarDeclCommaSeparatedNamesParse OK\n";
}

static void testLocalVarDeclCommaListVmExecution() {
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* forStmt = dynamic_cast<amlp::ForStmt*>(body[0].get());
    assert(forStmt != nullptr);
    assert(dynamic_cast<amlp::VarDeclStmt*>(forStmt->init.get()) != nullptr);
    assert(forStmt->condition != nullptr);
    assert(dynamic_cast<amlp::AssignExpr*>(forStmt->update.get()) != nullptr);
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* forStmt = dynamic_cast<amlp::ForStmt*>(body[0].get());
    assert(forStmt != nullptr);
    assert(forStmt->init == nullptr);
    assert(forStmt->condition == nullptr);
    assert(forStmt->update == nullptr);

    std::cout << "testForLoopEmptyClausesParse OK\n";
}

static void testForLoopWithAssignInitVmSumsExpectedTotal() {
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer("i++ ++i i-- --i");
    auto tokens = lexer.tokenize();
    int plusPlusCount = 0, minusMinusCount = 0;
    for (auto& t : tokens) {
        if (t.type == amlp::TokenType::Symbol && t.text == "++") ++plusPlusCount;
        if (t.type == amlp::TokenType::Symbol && t.text == "--") ++minusMinusCount;
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* prefixStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    auto* prefixExpr = dynamic_cast<amlp::IncDecExpr*>(prefixStmt->expr.get());
    assert(prefixExpr != nullptr);
    assert(prefixExpr->prefix == true);
    assert(prefixExpr->op == amlp::IncDecOp::Inc);
    assert(prefixExpr->name == "a");

    auto* postfixStmt = dynamic_cast<amlp::ExprStmt*>(body[1].get());
    auto* postfixExpr = dynamic_cast<amlp::IncDecExpr*>(postfixStmt->expr.get());
    assert(postfixExpr != nullptr);
    assert(postfixExpr->prefix == false);
    assert(postfixExpr->op == amlp::IncDecOp::Dec);
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());

    bool threw = false;
    try {
        parser.parseProgram();
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* stmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(stmt != nullptr);
    auto* incDec = dynamic_cast<amlp::IncDecExpr*>(stmt->expr.get());
    assert(incDec != nullptr);
    assert(incDec->prefix == false);
    assert(incDec->op == amlp::IncDecOp::Dec);
    assert(incDec->name.empty());
    assert(incDec->indexTarget != nullptr);
    auto* target = dynamic_cast<amlp::VarRefExpr*>(incDec->indexTarget.get());
    assert(target != nullptr && target->name == "healing");
    auto* key = dynamic_cast<amlp::StringLiteral*>(incDec->indexKey.get());
    assert(key != nullptr && key->value == "intox");

    std::cout << "testPostfixIncDecOnIndexedTargetParsesToIndexedIncDecExpr OK\n";
}

static void testIndexedPostfixIncDecVmExecutionReturnsOldValueAndMutates() {
    // Mirrors testPostfixIncrementVmExecutionReturnsOldValueAndMutates
    // below, but on a mapping-indexed target -- std/living.c's own real
    // shape ("healing[\"intox\"]--"), confirmed against the reference
    // driver's grammar.y restricted "lvalue" nonterminal covering both
    // a bare variable and an indexed target.
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;
    auto compiled = codegen.generate(*program);

    bool sawCall = false;
    for (auto& instr : compiled.code) {
        // A plain bare call must never fall back to the old
        // always-CallEfun routing -- resolution now happens at run time.
        assert(instr.op != amlp::OpCode::CallEfun);
        if (instr.op == amlp::OpCode::Call) {
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
    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "caller", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testSameObjectBareCallInvokesLocalFunctionAtRuntime OK\n";
}

static void testBareCallFallsBackToEfunWhenNoLocalFunctionMatches() {
    amlp::Value result = runProbe(
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
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    // body[0] is the "string a, b;" comma-decl Block; the sscanf() call
    // itself is body[1].
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[1].get());
    assert(exprStmt != nullptr);
    auto* sscanfExpr = dynamic_cast<amlp::SscanfExpr*>(exprStmt->expr.get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());

    bool threw = false;
    try {
        parser.parseProgram();
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testSscanfNonIdentifierOutputArgThrows OK\n";
}

static void testSscanfVmMatchesLiteralDelimitedTokens() {
    // Mirrors secure/daemon/master.c's own
    // "sscanf(lines[i], \"(%s) %s\", fl, ac)" shape.
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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

// "%x", "%f", and "%s" directly adjacent to another specifier with no
// literal text between them -- general LPC-compliance additions, ported
// from fluffos-2.9-ds2.08/interpret.c's own inter_sscanf(), not driven by
// a new mudlib call site this time.

static void testSscanfVmMatchesHexSpecifier() {
    amlp::Value result = runProbe(
        "int x;\n"
        "int n;\n"
        "n = sscanf(\"ff\", \"%x\", x);\n"
        "return n == 1 && x == 255;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testSscanfVmMatchesHexSpecifier OK\n";
}

static void testSscanfVmHexSpecifierAcceptsLeading0xPrefix() {
    amlp::Value result = runProbe(
        "int x;\n"
        "int n;\n"
        "n = sscanf(\"0x1A\", \"%x\", x);\n"
        "return n == 1 && x == 26;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testSscanfVmHexSpecifierAcceptsLeading0xPrefix OK\n";
}

static void testSscanfVmMatchesFloatSpecifier() {
    amlp::Value result = runProbe(
        "float x;\n"
        "int n;\n"
        "n = sscanf(\"3.5\", \"%f\", x);\n"
        "return n == 1 && x == 3.5;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testSscanfVmMatchesFloatSpecifier OK\n";
}

static void testSscanfVmAdjacentSThenDWithNoLiteralBetween() {
    // "%s%d" with no literal separator: %s must scan ahead to find where
    // the digits start, matching real inter_sscanf()'s own lookahead
    // rather than throwing.
    amlp::Value result = runProbe(
        "string a;\n"
        "int b;\n"
        "int n;\n"
        "n = sscanf(\"abc123\", \"%s%d\", a, b);\n"
        "return n == 2 && a == \"abc\" && b == 123;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testSscanfVmAdjacentSThenDWithNoLiteralBetween OK\n";
}

static void testSscanfVmAdjacentSThenXWithNoLiteralBetween() {
    amlp::Value result = runProbe(
        "string a;\n"
        "int b;\n"
        "int n;\n"
        "n = sscanf(\"name0xFF\", \"%s%x\", a, b);\n"
        "return n == 2 && a == \"name\" && b == 255;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testSscanfVmAdjacentSThenXWithNoLiteralBetween OK\n";
}

static void testSscanfVmAdjacentSThenLiteralPercentWithNoLiteralBetween() {
    // "%s%%": %s adjacent to a literal "%" character in the input (not
    // another specifier) -- real inter_sscanf()'s own "case '%':" lookahead.
    amlp::Value result = runProbe(
        "string a;\n"
        "int n;\n"
        "n = sscanf(\"75%\", \"%s%%\", a);\n"
        "return n == 2 && a == \"75\";\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testSscanfVmAdjacentSThenLiteralPercentWithNoLiteralBetween OK\n";
}

static void testSscanfVmTwoAdjacentSSpecifiersThrows() {
    // Real inter_sscanf(): "Illegal to have 2 adjacent %s's in format
    // string in sscanf()" -- a genuine LPC-level format error, not an
    // unimplemented-feature stub.
    bool threw = false;
    try {
        runProbe(
            "string a, b;\n"
            "sscanf(\"abcdef\", \"%s%s\", a, b);\n"
            "return 0;\n");
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testSscanfVmTwoAdjacentSSpecifiersThrows OK\n";
}

// --- inherit ----------------------------------------------------------

static void testInheritStatementParsesPathAndConcatenation() {
    std::string src =
        "inherit \"/secure/std/daemon\";\n"
        "inherit \"/secure/daemon\" + \"/refs\";\n"
        "void create() {}\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
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
    amlp::Value result = harness.vm.callFunction(child, "probe", {});
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
        std::vector<amlp::Value>{amlp::Value(static_cast<int64_t>(55))});
    amlp::Value result = harness.vm.callFunction(child, "read_shared", {});
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    // The cast contributes nothing to the AST: the expression is exactly
    // the bare VarRefExpr it wrapped, not some CastExpr wrapper.
    auto* ref = dynamic_cast<amlp::VarRefExpr*>(exprStmt->expr.get());
    assert(ref != nullptr);
    assert(ref->name == "x");

    std::cout << "testTypeCastParsesAsNoOpWrappingInnerExpr OK\n";
}

static void testTypeCastVmExecutionIsNoOp() {
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe("return (string)\"!\" + \"x\";\n");
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 3); // real sizeof(), not the local shadow

    std::cout << "testEfunOverrideBypassesLocalFunctionOfSameName OK\n";
}

// --- compound assignment -----------------------------------------------
// Hit immediately after the dynamic call_other fix while re-attempting the
// real master.c boot (line 115: "files += ({ lines[i] });").

static void testCompoundAssignOperatorsTokenize() {
    amlp::Lexer lexer("a += b -= c *= d /= e %= f");
    auto tokens = lexer.tokenize();
    std::vector<std::string> expectedOps = {"+=", "-=", "*=", "/=", "%="};
    size_t opIdx = 0;
    for (const auto& t : tokens) {
        if (t.type == amlp::TokenType::Symbol) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* assign = dynamic_cast<amlp::AssignExpr*>(exprStmt->expr.get());
    assert(assign != nullptr);
    assert(assign->isCompound);
    assert(assign->compoundOp == amlp::BinOp::Add);
    assert(assign->name == "x");

    std::cout << "testCompoundAssignParsesToCompoundAssignExpr OK\n";
}

static void testCompoundAssignVmExecutionOnIntAndArray() {
    amlp::Value intResult = runProbe(
        "int x;\n"
        "x = 5;\n"
        "x += 3;\n"
        "return x;\n");
    assert(std::holds_alternative<int64_t>(intResult.data));
    assert(std::get<int64_t>(intResult.data) == 8);

    // Mirrors master.c's own "files += ({ lines[i] });" shape: "+="
    // reuses the Add opcode's existing array-concatenation behavior.
    amlp::Value arrResult = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* bin = dynamic_cast<amlp::BinaryExpr*>(exprStmt->expr.get());
    assert(bin != nullptr);
    assert(bin->op == amlp::BinOp::BitAnd);

    std::cout << "testBitAndParsesToBinaryExprWithBitAndOp OK\n";
}

static void testBitAndVmExecutionOnInts() {
    amlp::Value result = runProbe("return 6 & 3;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 2); // 110 & 011 == 010

    std::cout << "testBitAndVmExecutionOnInts OK\n";
}

static void testBitAndVmExecutionOnArraysIsIntersection() {
    // Mirrors master.c's own "sizeof(privs & ok)" shape.
    amlp::Value result = runProbe(
        "mixed *privs, *ok;\n"
        "privs = ({ \"a\", \"b\", \"c\" });\n"
        "ok = ({ \"b\", \"c\", \"d\" });\n"
        "return sizeof(privs & ok);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 2); // "b" and "c" are shared

    amlp::Value empty = runProbe(
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
    amlp::Value orResult = runProbe("return 1 | 2;\n");
    assert(std::holds_alternative<int64_t>(orResult.data));
    assert(std::get<int64_t>(orResult.data) == 3);

    amlp::Value xorResult = runProbe("return 6 ^ 3;\n");
    assert(std::holds_alternative<int64_t>(xorResult.data));
    assert(std::get<int64_t>(xorResult.data) == 5);

    // "||" must still tokenize and parse as logical-or, not two adjacent
    // "|" bitwise-or tokens.
    amlp::Value orElse = runProbe("return 0 || 5;\n");
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
        "mixed innerErr, outerErr;\n"
        "outerErr = catch(innerErr = catch(totally_undefined_thing_xyz()));\n"
        "return outerErr;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0); // outer: no error reached it

    // The inner catch, checked via a second probe, still caught its own
    // error and produced the message.
    amlp::Value innerResult = runProbe(
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
    amlp::Value result = runProbe(
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
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

    amlp::Value failureResult =
        harness.vm.callFunction(caller, "probe", {amlp::Value(static_cast<int64_t>(1))});
    assert(std::holds_alternative<int64_t>(failureResult.data));
    assert(std::get<int64_t>(failureResult.data) == 1); // error branch taken

    amlp::Value successResult =
        harness.vm.callFunction(caller, "probe", {amlp::Value(static_cast<int64_t>(0))});
    assert(std::holds_alternative<int64_t>(successResult.data));
    assert(std::get<int64_t>(successResult.data) == 7); // ob correctly assigned, marker() reachable

    std::cout << "testCatchInlineInsideIfConditionMatchesMasterConnectShape OK\n";
}

// --- throw() -----------------------------------------------------------
// Real FluffOS: a real efun (func_spec.c: "void throw(mixed);"), not
// special grammar the way catch() is -- see EfunTable.cpp's own
// registration comment. Unlike an ordinary runtime error, which catch()
// always sees as a string message, throw() hands back the *exact* value
// it was given, of any type -- confirmed against interpret.c's own
// catch_value svalue_t (not a string-typed field).

static void testThrowIntIsCaughtVerbatimByCatch() {
    amlp::Value result = runProbe(
        "mixed err;\n"
        "err = catch(throw(42));\n"
        "return err;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testThrowIntIsCaughtVerbatimByCatch OK\n";
}

static void testThrowStringIsCaughtVerbatimByCatch() {
    amlp::Value result = runProbe(
        "mixed err;\n"
        "err = catch(throw(\"custom error\"));\n"
        "return err;\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "custom error");

    std::cout << "testThrowStringIsCaughtVerbatimByCatch OK\n";
}

static void testThrowArrayValueIsCaughtVerbatimByCatch() {
    // The actual point of the feature: an ordinary runtime error can
    // only ever produce a string, but throw() can hand back any value,
    // including a structured one -- a real, common LPC idiom for
    // signaling an error code plus data together.
    amlp::Value result = runProbe(
        "mixed err;\n"
        "err = catch(throw(({ \"ERR_CODE\", 7 })));\n"
        "return err;\n");
    assert(std::holds_alternative<std::shared_ptr<amlp::Array>>(result.data));
    auto arr = std::get<std::shared_ptr<amlp::Array>>(result.data);
    assert(arr != nullptr);
    assert(arr->items.size() == 2);
    assert(std::holds_alternative<std::string>(arr->items[0].data));
    assert(std::get<std::string>(arr->items[0].data) == "ERR_CODE");
    assert(std::holds_alternative<int64_t>(arr->items[1].data));
    assert(std::get<int64_t>(arr->items[1].data) == 7);

    std::cout << "testThrowArrayValueIsCaughtVerbatimByCatch OK\n";
}

static void testThrowWithWrongArgCountThrowsLpcRuntimeError() {
    bool threw = false;
    try {
        runProbe("throw();\nreturn 0;\n");
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testThrowWithWrongArgCountThrowsLpcRuntimeError OK\n";
}

// The critical case: throw() inside a called function that has no
// catch() of its own must still reach the *caller*'s catch() with the
// exact original value intact, not flattened into a rewrapped string
// LpcRuntimeError -- confirmed by reading VM::run()'s own no-active-
// catch-frame branch, which does exactly that flattening for an
// ordinary LpcRuntimeError (adding a "file::function(): " prefix to the
// message) and would silently destroy a thrown array/int the same way
// without its own dedicated LpcThrownValue branch ahead of it.
static void testThrowInsideCalledFunctionWithNoCatchOfItsOwnReachesCallersCatchIntact() {
    ObjectVarHarness harness;

    harness.writeFile("/callee.c",
        "int explode_now() {\n"
        "    throw(({ \"ERR_CODE\", 99 }));\n"
        "    return 0;\n"
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::Array>>(result.data));
    auto arr = std::get<std::shared_ptr<amlp::Array>>(result.data);
    assert(arr != nullptr);
    assert(arr->items.size() == 2);
    assert(std::get<std::string>(arr->items[0].data) == "ERR_CODE");
    assert(std::get<int64_t>(arr->items[1].data) == 99);

    std::cout << "testThrowInsideCalledFunctionWithNoCatchOfItsOwnReachesCallersCatchIntact OK\n";
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* lit = dynamic_cast<amlp::StringLiteral*>(exprStmt->expr.get());
    assert(lit != nullptr);
    assert(lit->value == "foobar");

    std::cout << "testAdjacentStringLiteralsParseAsSingleConcatenatedLiteral OK\n";
}

static void testAdjacentStringLiteralsVmExecution() {
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* whileStmt = dynamic_cast<amlp::WhileStmt*>(body[0].get());
    assert(whileStmt != nullptr);
    assert(dynamic_cast<amlp::BreakStmt*>(whileStmt->body->statements[0].get()) != nullptr);
    assert(dynamic_cast<amlp::ContinueStmt*>(whileStmt->body->statements[1].get()) != nullptr);

    std::cout << "testBreakAndContinueParseToDedicatedStmtNodes OK\n";
}

static void testBreakOutsideLoopThrowsAtCodegen() {
    std::string src = "void probe() { break; }\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();
    amlp::CodeGen codegen;

    bool threw = false;
    try {
        codegen.generate(*program);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testBreakOutsideLoopThrowsAtCodegen OK\n";
}

static void testBreakStopsForLoopEarlyVmExecution() {
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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

// --- do-while loop (VM execution) ---------------------------------------
// See testDoWhileParsesToDoWhileStmt above for the parse-level coverage.
// The one thing that actually distinguishes a do-while from a while: the
// body always runs at least once, checked only *after* the first
// iteration -- plus break/continue behaving the same as they do for a
// while loop's own body.

static void testDoWhileExecutesBodyAtLeastOnceEvenWhenConditionFalseVmExecution() {
    amlp::Value result = runProbe(
        "int i;\n"
        "int count;\n"
        "i = 0;\n"
        "count = 0;\n"
        "do {\n"
        "    count = count + 1;\n"
        "} while (i > 0);\n"  // false from the very first check
        "return count;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testDoWhileExecutesBodyAtLeastOnceEvenWhenConditionFalseVmExecution OK\n";
}

static void testDoWhileLoopsWhileConditionTrueVmExecution() {
    amlp::Value result = runProbe(
        "int i;\n"
        "int sum;\n"
        "i = 0;\n"
        "sum = 0;\n"
        "do {\n"
        "    i = i + 1;\n"
        "    sum = sum + i;\n"
        "} while (i < 5);\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 15); // 1+2+3+4+5
    std::cout << "testDoWhileLoopsWhileConditionTrueVmExecution OK\n";
}

static void testContinueInDoWhileLoopSkipsToConditionRecheckVmExecution() {
    // Mirrors testContinueInWhileLoopSkipsToConditionRecheckVmExecution:
    // continue must still reach the condition check (real do-while
    // semantics), not restart the body from its own top unconditionally.
    amlp::Value result = runProbe(
        "int i;\n"
        "int sum;\n"
        "i = 0;\n"
        "sum = 0;\n"
        "do {\n"
        "    i = i + 1;\n"
        "    if (i == 3) continue;\n"
        "    sum = sum + i;\n"
        "} while (i < 5);\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 12); // 1+2+4+5, skipping 3
    std::cout << "testContinueInDoWhileLoopSkipsToConditionRecheckVmExecution OK\n";
}

static void testBreakStopsDoWhileLoopEarlyVmExecution() {
    amlp::Value result = runProbe(
        "int i;\n"
        "int sum;\n"
        "i = 0;\n"
        "sum = 0;\n"
        "do {\n"
        "    i = i + 1;\n"
        "    if (i == 3) break;\n"
        "    sum = sum + i;\n"
        "} while (i < 5);\n"
        "return sum;\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 3); // 1+2, stops before adding 3
    std::cout << "testBreakStopsDoWhileLoopEarlyVmExecution OK\n";
}

static void testBreakInInnerLoopDoesNotAffectOuterLoopVmExecution() {
    // Nested loops: break/continue must resolve against the innermost
    // enclosing loop only (CodeGen's loopStack_ push/pop per loop).
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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

    amlp::Value content = harness.vm.callFunction(obj, "read_it", {});
    assert(std::holds_alternative<std::string>(content.data));
    assert(std::get<std::string>(content.data) == "line one\nline two\n");

    amlp::Value missing = harness.vm.callFunction(obj, "read_missing", {});
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

    amlp::Value writeResult = harness.vm.callFunction(obj, "write_it", {});
    assert(std::holds_alternative<int64_t>(writeResult.data));
    assert(std::get<int64_t>(writeResult.data) == 1);

    amlp::Value content = harness.vm.callFunction(obj, "read_it", {});
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

    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
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

    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testCppWarningsDoNotFailPreprocessing OK\n";
}

static void testIncludeDirConfigSupportsColonSeparatedListLikeRealMudosCfg() {
    // Real FluffOS's own "include directories" mudos.cfg setting is a
    // colon-separated *list*, confirmed directly against
    // fluffos-2.9-ds2.08's own rc.c ("CONFIG_STR(__INCLUDE_DIRS__)") and
    // main.c's own "set_inc_list(INCLUDE_DIRS)", and against this actual
    // target mudlib's own historical bin/mudos.cfg: "include directories
    // : /secure/include:/include" -- a "#include <...>" lookup searches
    // every listed directory, not just the first. See ObjectManager.cpp's
    // own splitIncludeDirs() comment for the real call site this closes:
    // this mudlib's own std/rifts_vehicle.c "#include <vehicle.h>"
    // previously failed outright ("No such file or directory") since
    // this driver's Config only ever passed a single -I to cpp.
    //
    // Built by hand rather than via ObjectVarHarness (whose own
    // constructor hardcodes a single include_dir) specifically to prove
    // two *separate* directories, each holding one of the two headers
    // needed, are both actually searched -- not just that some include
    // path works at all.
    char dirTemplate[] = "/tmp/amlp_incdir_test_XXXXXX";
    char* created = mkdtemp(dirTemplate);
    assert(created != nullptr);
    std::string tempDir = created;

    assert(::mkdir((tempDir + "/hdrs_a").c_str(), 0755) == 0);
    assert(::mkdir((tempDir + "/hdrs_b").c_str(), 0755) == 0);

    std::ofstream headerA(tempDir + "/hdrs_a/a.h");
    headerA << "#define FROM_A 1\n";
    headerA.close();
    std::ofstream headerB(tempDir + "/hdrs_b/b.h");
    headerB << "#define FROM_B 2\n";
    headerB.close();

    std::ofstream probeFile(tempDir + "/probe.c");
    probeFile << "#include <a.h>\n#include <b.h>\n"
                 "int probe() { return FROM_A + FROM_B; }\n";
    probeFile.close();

    std::string cfgPath = tempDir + "/driver.cfg";
    std::ofstream cfg(cfgPath);
    cfg << "mudlib_root: " << tempDir << "\n";
    cfg << "master_file: /unused\n";
    cfg << "include_dir: " << tempDir << "/hdrs_a:" << tempDir << "/hdrs_b\n";
    cfg << "port: 0\n";
    cfg.close();

    amlp::Config config;
    assert(config.loadFromFile(cfgPath));
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);
    objects.setVM(&vm);

    auto obj = objects.loadObject("/probe");
    assert(obj != nullptr);

    amlp::Value result = vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 3);

    std::cout << "testIncludeDirConfigSupportsColonSeparatedListLikeRealMudosCfg OK\n";
}

static void testIncludeDirSingleEntryWithNoColonStillWorks() {
    // Backward-compatibility check: every existing driver.cfg (this
    // project's own etc/driver.cfg among them) sets
    // "include_dir" to one plain path with no ':' at all -- confirms
    // splitIncludeDirs() still resolves a real "#include <...>" against
    // that one entry, not zero entries and not a spurious empty second
    // one from over-eager splitting.
    ObjectVarHarness harness;
    harness.writeFile("/single_incdir.h", "#define SINGLE_INCDIR_VALUE 9\n");
    harness.writeFile("/single_incdir_probe.c",
        "#include <single_incdir.h>\n"
        "int probe() { return SINGLE_INCDIR_VALUE; }\n");
    auto obj = harness.objects.cloneObject("/single_incdir_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 9);
    std::cout << "testIncludeDirSingleEntryWithNoColonStillWorks OK\n";
}

// --- __FILE__ / __DIR__ predefines ---------------------------------------
// Real lex.c's own start_new_file(): __FILE__ is "/" + the compiled
// object's own mudlib-relative path *with* its ".c" extension, __DIR__ is
// that same string truncated right after its own last '/'. See
// ObjectManager.cpp's own buildPredefinedMacroFlags() comment for the
// real citation and the two distinct symptoms this closed: __DIR__ was a
// bare undefined identifier before this (a hard compile failure wherever
// used unguarded -- lil_0.3's own single/tests/efuns/shadow.c "new(__DIR__
// \"badshad\", 1)"), and __FILE__ silently resolved to gcc's own built-in
// value (this driver's real host filesystem source path) instead of the
// LPC-visible one, compiling but wrong -- confirmed live against 15 other
// lil_0.3 test files that reference __FILE__.

static void testFileDunderPredefineResolvesToRealLpcPathNotHostFilesystemPath() {
    ObjectVarHarness harness;
    harness.writeFile("/filedunder_probe.c", "mixed probe() { return __FILE__; }\n");
    auto obj = harness.objects.cloneObject("/filedunder_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    // Not an absolute host path (no leading tempDir, no leftover
    // /tmp/amlp_src_XXXXXX staging path) -- exactly the real LPC
    // in-mudlib path, "/" + filename + ".c".
    assert(std::get<std::string>(result.data) == "/filedunder_probe.c");
    std::cout << "testFileDunderPredefineResolvesToRealLpcPathNotHostFilesystemPath OK\n";
}

static void testDirDunderPredefineTruncatesAfterLastSlashWithMultipleSegments() {
    // A nested directory (not just "/") to actually exercise the
    // truncate-after-last-slash logic, matching the real shape
    // ("/single/tests/efuns/") rather than the degenerate root case.
    char dirTemplate[] = "/tmp/amlp_dirdunder_test_XXXXXX";
    char* created = mkdtemp(dirTemplate);
    assert(created != nullptr);
    std::string tempDir = created;
    assert(::mkdir((tempDir + "/sub").c_str(), 0755) == 0);

    std::ofstream probeFile(tempDir + "/sub/dirdunder_probe.c");
    probeFile << "mixed probe() { return __DIR__; }\n";
    probeFile.close();

    std::string cfgPath = tempDir + "/driver.cfg";
    std::ofstream cfg(cfgPath);
    cfg << "mudlib_root: " << tempDir << "\n";
    cfg << "master_file: /unused\n";
    cfg << "include_dir: " << tempDir << "\n";
    cfg << "port: 0\n";
    cfg.close();

    amlp::Config config;
    assert(config.loadFromFile(cfgPath));
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);
    objects.setVM(&vm);

    auto obj = objects.loadObject("/sub/dirdunder_probe");
    assert(obj != nullptr);
    amlp::Value result = vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "/sub/");
    std::cout << "testDirDunderPredefineTruncatesAfterLastSlashWithMultipleSegments OK\n";
}

static void testDirDunderAdjacentToStringLiteralMatchesRealShadowTestShape() {
    // The exact real failing shape: "__DIR__ \"badshad\"" -- an
    // undefined-until-now predefine directly adjacent to a hand-written
    // string literal, relying on this driver's own already-working
    // adjacent-string-literal concatenation (Parser.cpp's parsePrimary())
    // to combine the two. Before this fix __DIR__ was a bare identifier,
    // so this driver's parser never even reached the concatenation logic
    // -- it choked immediately on the unexpected string literal
    // following it.
    ObjectVarHarness harness;
    harness.writeFile("/dirstr_probe.c",
        "mixed probe() { return __DIR__ \"badshad\"; }\n");
    auto obj = harness.objects.cloneObject("/dirstr_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "/badshad");
    std::cout << "testDirDunderAdjacentToStringLiteralMatchesRealShadowTestShape OK\n";
}

// --- Unnamed function parameters ("string foo(string, int) { ... }") ----
// Real grammar.y's own "new_arg: arg_type optional_star" alternative
// (confirmed by direct reading): a parameter may declare just its type,
// with no identifier at all, if the function body never needs to refer
// to it -- add_local_name("", type) in the real driver, still a real
// argument slot, just permanently unreachable by name. Found live
// sweeping the lil_0.3 reference testsuite mudlib standalone against
// this driver: its own single/master.c ("staticf void crash(string,
// object, object)") and single/simul_efun.c ("string domain_file(string)
// { return ROOT_UID; }") both use this shape, and neither the master nor
// the simul_efun object -- i.e. nothing in that entire mudlib -- could
// compile at all without it. inherit/master/valid.c (part of the real
// master object's own inherit chain) uses the same shape six more times
// (valid_seteuid, valid_socket, valid_write, valid_read, etc).

static void testUnnamedFunctionParameterParsesAndDoesNotBreakOtherLocals() {
    // A single unnamed parameter, matching single/simul_efun.c's own
    // "domain_file(string)" shape exactly, plus a real named local
    // declared afterward -- proves the unnamed slot doesn't leave the
    // local-slot counter out of sync with a normal declared local
    // sharing the same function.
    ObjectVarHarness harness;
    harness.writeFile("/unnamed_single_probe.c",
        "string domain_file(string) {\n"
        "    int x;\n"
        "    x = 42;\n"
        "    return \"root\";\n"
        "}\n"
        "mixed probe() { return domain_file(\"ignored\"); }\n");
    auto obj = harness.objects.cloneObject("/unnamed_single_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "root");
    std::cout << "testUnnamedFunctionParameterParsesAndDoesNotBreakOtherLocals OK\n";
}

static void testMultipleUnnamedParametersInOneFunctionDoNotCollide() {
    // The exact real single/master.c shape: three unnamed parameters in
    // one function. Before this fix, CodeGen's declareLocal() would have
    // thrown "variable \"\" already declared in this scope" on the
    // second one even if the parser accepted the syntax at all, since
    // every unnamed parameter would otherwise share the same empty-string
    // key in the name-keyed locals_ map.
    ObjectVarHarness harness;
    harness.writeFile("/unnamed_multi_probe.c",
        "void crash(string, object, object) {\n"
        "}\n"
        "int probe() {\n"
        "    crash(\"x\", this_object(), this_object());\n"
        "    return 1;\n"
        "}\n");
    auto obj = harness.objects.cloneObject("/unnamed_multi_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testMultipleUnnamedParametersInOneFunctionDoNotCollide OK\n";
}

static void testUnnamedParameterMixedWithNamedOnesStaysPositionallyCorrect() {
    // A named parameter declared *after* an unnamed one must still land
    // in the right slot and read back the right call-time argument --
    // not just "doesn't crash", the actual value has to be correct, the
    // same standard this project already holds compiler-level fixes to.
    ObjectVarHarness harness;
    harness.writeFile("/unnamed_mixed_probe.c",
        "int helper(string, int keep) {\n"
        "    return keep;\n"
        "}\n"
        "int probe() {\n"
        "    return helper(\"ignored\", 99);\n"
        "}\n");
    auto obj = harness.objects.cloneObject("/unnamed_mixed_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 99);
    std::cout << "testUnnamedParameterMixedWithNamedOnesStaysPositionallyCorrect OK\n";
}

// --- Phase 0.13 efun growth: to_float, typeof, rename, rmdir, math --------

static void testToFloatIntArgConvertsToDouble() {
    // to_float(int) → the same value as a float, confirmed against real
    // f__to_float()'s "(double) sp->u.number" cast.
    ObjectVarHarness harness;
    harness.writeFile("/tf.c",
        "float probe_int() { return to_float(42); }\n"
        "float probe_neg() { return to_float(-3); }\n"
        "float probe_zero() { return to_float(0); }\n");
    auto obj = harness.objects.cloneObject("/tf");
    assert(obj != nullptr);

    amlp::Value r1 = harness.vm.callFunction(obj, "probe_int", {});
    assert(std::holds_alternative<double>(r1.data));
    assert(std::get<double>(r1.data) == 42.0);

    amlp::Value r2 = harness.vm.callFunction(obj, "probe_neg", {});
    assert(std::holds_alternative<double>(r2.data));
    assert(std::get<double>(r2.data) == -3.0);

    amlp::Value r3 = harness.vm.callFunction(obj, "probe_zero", {});
    assert(std::holds_alternative<double>(r3.data));
    assert(std::get<double>(r3.data) == 0.0);

    std::cout << "testToFloatIntArgConvertsToDouble OK\n";
}

static void testToFloatStringArgParsesLeadingFloat() {
    // to_float(string) → sscanf "%lf" result; unparseable → 0.0.
    ObjectVarHarness harness;
    harness.writeFile("/tf2.c",
        "float probe_str() { return to_float(\"3.14\"); }\n"
        "float probe_bad() { return to_float(\"hello\"); }\n");
    auto obj = harness.objects.cloneObject("/tf2");
    assert(obj != nullptr);

    amlp::Value r1 = harness.vm.callFunction(obj, "probe_str", {});
    assert(std::holds_alternative<double>(r1.data));
    // Allow small floating-point epsilon
    assert(std::get<double>(r1.data) > 3.13 && std::get<double>(r1.data) < 3.15);

    amlp::Value r2 = harness.vm.callFunction(obj, "probe_bad", {});
    assert(std::holds_alternative<double>(r2.data));
    assert(std::get<double>(r2.data) == 0.0);

    std::cout << "testToFloatStringArgParsesLeadingFloat OK\n";
}

static void testToFloatFloatArgPassesThrough() {
    // to_float(float) → same float unchanged.
    ObjectVarHarness harness;
    harness.writeFile("/tf3.c",
        "float probe() { return to_float(2.5); }\n");
    auto obj = harness.objects.cloneObject("/tf3");
    assert(obj != nullptr);

    amlp::Value r = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<double>(r.data));
    assert(std::get<double>(r.data) == 2.5);
    std::cout << "testToFloatFloatArgPassesThrough OK\n";
}

static void testTypeofReturnsCorrectTypeStringForEachKind() {
    // typeof(x) → confirmed type names from interpret.c type_names[].
    ObjectVarHarness harness;
    harness.writeFile("/tyof.c",
        "string probe_int()    { return typeof(42); }\n"
        "string probe_float()  { return typeof(1.5); }\n"
        "string probe_str()    { return typeof(\"hi\"); }\n"
        "string probe_arr()    { return typeof(({1,2})); }\n"
        "string probe_map()    { return typeof(([\"a\":1])); }\n");
    auto obj = harness.objects.cloneObject("/tyof");
    assert(obj != nullptr);

    auto checkStr = [&](const std::string& fn, const std::string& expected) {
        amlp::Value r = harness.vm.callFunction(obj, fn, {});
        assert(std::holds_alternative<std::string>(r.data));
        assert(std::get<std::string>(r.data) == expected);
    };
    checkStr("probe_int",   "int");
    checkStr("probe_float", "float");
    checkStr("probe_str",   "string");
    checkStr("probe_arr",   "array");
    checkStr("probe_map",   "mapping");
    std::cout << "testTypeofReturnsCorrectTypeStringForEachKind OK\n";
}

static void testRenameFileAndVerifyViaReadFile() {
    // rename(from, to) → 0 on success (real do_rename() return value);
    // the renamed file can then be read at its new path.
    ObjectVarHarness harness;
    harness.writeFile("/src_for_rename.txt", "rename me\n");
    harness.writeFile("/renamer.c",
        "int do_rename() {\n"
        "    return rename(\"/src_for_rename.txt\", \"/dst_renamed.txt\");\n"
        "}\n"
        "string read_dst() { return read_file(\"/dst_renamed.txt\"); }\n");
    auto obj = harness.objects.cloneObject("/renamer");
    assert(obj != nullptr);

    amlp::Value rv = harness.vm.callFunction(obj, "do_rename", {});
    assert(std::holds_alternative<int64_t>(rv.data));
    assert(std::get<int64_t>(rv.data) == 0); // 0 = success

    amlp::Value content = harness.vm.callFunction(obj, "read_dst", {});
    assert(std::holds_alternative<std::string>(content.data));
    assert(std::get<std::string>(content.data) == "rename me\n");
    std::cout << "testRenameFileAndVerifyViaReadFile OK\n";
}

static void testRmdirRemovesEmptyDirectoryAndFailsOnNonEmpty() {
    // rmdir() → 1 on success, 0 on failure (non-empty dir or missing).
    ObjectVarHarness harness;
    // Create an empty subdir to remove.
    ::mkdir((harness.tempDir + "/emptydir").c_str(), 0755);
    // Create a non-empty subdir.
    ::mkdir((harness.tempDir + "/nonempty").c_str(), 0755);
    harness.writeFile("/nonempty/file.txt", "x");

    harness.writeFile("/rmdirer.c",
        "int rm_empty()    { return rmdir(\"/emptydir\"); }\n"
        "int rm_nonempty() { return rmdir(\"/nonempty\"); }\n"
        "int rm_missing()  { return rmdir(\"/does_not_exist_dir\"); }\n");
    auto obj = harness.objects.cloneObject("/rmdirer");
    assert(obj != nullptr);

    amlp::Value r1 = harness.vm.callFunction(obj, "rm_empty", {});
    assert(std::holds_alternative<int64_t>(r1.data));
    assert(std::get<int64_t>(r1.data) == 1); // success

    amlp::Value r2 = harness.vm.callFunction(obj, "rm_nonempty", {});
    assert(std::holds_alternative<int64_t>(r2.data));
    assert(std::get<int64_t>(r2.data) == 0); // non-empty: fail

    amlp::Value r3 = harness.vm.callFunction(obj, "rm_missing", {});
    assert(std::holds_alternative<int64_t>(r3.data));
    assert(std::get<int64_t>(r3.data) == 0); // missing: fail
    std::cout << "testRmdirRemovesEmptyDirectoryAndFailsOnNonEmpty OK\n";
}

static void testAbsReturnsPositiveForNegativeIntAndFloat() {
    // abs(int|float) → same type, positive. Confirmed against contrib.c
    // f_abs(): negates negative value in-place, keeps positive unchanged.
    ObjectVarHarness harness;
    harness.writeFile("/abstest.c",
        "int probe_neg_int()    { return abs(-7); }\n"
        "int probe_pos_int()    { return abs(5); }\n"
        "float probe_neg_float() { return abs(-2.5); }\n"
        "float probe_pos_float() { return abs(3.0); }\n");
    auto obj = harness.objects.cloneObject("/abstest");
    assert(obj != nullptr);

    amlp::Value r1 = harness.vm.callFunction(obj, "probe_neg_int", {});
    assert(std::holds_alternative<int64_t>(r1.data));
    assert(std::get<int64_t>(r1.data) == 7);

    amlp::Value r2 = harness.vm.callFunction(obj, "probe_pos_int", {});
    assert(std::holds_alternative<int64_t>(r2.data));
    assert(std::get<int64_t>(r2.data) == 5);

    amlp::Value r3 = harness.vm.callFunction(obj, "probe_neg_float", {});
    assert(std::holds_alternative<double>(r3.data));
    assert(std::get<double>(r3.data) == 2.5);

    amlp::Value r4 = harness.vm.callFunction(obj, "probe_pos_float", {});
    assert(std::holds_alternative<double>(r4.data));
    assert(std::get<double>(r4.data) == 3.0);
    std::cout << "testAbsReturnsPositiveForNegativeIntAndFloat OK\n";
}

static void testMaxAndMinReturnCorrectElementFromIntArray() {
    // max({arr}) → largest; min({arr}) → smallest.
    // Second arg != 0 → index rather than value.
    ObjectVarHarness harness;
    harness.writeFile("/minmaxtest.c",
        "int probe_max()       { return max(({3,1,4,1,5,9,2,6})); }\n"
        "int probe_min()       { return min(({3,1,4,1,5,9,2,6})); }\n"
        "int probe_max_idx()   { return max(({3,1,4,1,5,9,2,6}), 1); }\n"
        "int probe_min_idx()   { return min(({3,1,4,1,5,9,2,6}), 1); }\n");
    auto obj = harness.objects.cloneObject("/minmaxtest");
    assert(obj != nullptr);

    amlp::Value maxVal = harness.vm.callFunction(obj, "probe_max", {});
    assert(std::holds_alternative<int64_t>(maxVal.data));
    assert(std::get<int64_t>(maxVal.data) == 9);

    amlp::Value minVal = harness.vm.callFunction(obj, "probe_min", {});
    assert(std::holds_alternative<int64_t>(minVal.data));
    assert(std::get<int64_t>(minVal.data) == 1);

    amlp::Value maxIdx = harness.vm.callFunction(obj, "probe_max_idx", {});
    assert(std::holds_alternative<int64_t>(maxIdx.data));
    assert(std::get<int64_t>(maxIdx.data) == 5); // index of 9

    amlp::Value minIdx = harness.vm.callFunction(obj, "probe_min_idx", {});
    assert(std::holds_alternative<int64_t>(minIdx.data));
    assert(std::get<int64_t>(minIdx.data) == 1); // first occurrence of 1
    std::cout << "testMaxAndMinReturnCorrectElementFromIntArray OK\n";
}

static void testMathEfunsSqrtFloorCeilCosExpLog() {
    // One test exercising the real math package shapes this mudlib uses:
    // sqrt(4.0) == 2.0, floor(2.9) == 2.0, ceil(2.1) == 3.0,
    // cos(0.0) == 1.0, exp(0.0) == 1.0, log(1.0) == 0.0.
    // All confirmed against packages/math.c directly.
    ObjectVarHarness harness;
    harness.writeFile("/mathtest.c",
        "float probe_sqrt()  { return sqrt(4.0); }\n"
        "float probe_floor() { return floor(2.9); }\n"
        "float probe_ceil()  { return ceil(2.1); }\n"
        "float probe_cos()   { return cos(0.0); }\n"
        "float probe_exp()   { return exp(0.0); }\n"
        "float probe_log()   { return log(1.0); }\n"
        "float probe_pow()   { return pow(2.0, 10.0); }\n"
        // int arg auto-promoted: sqrt of perfect square via int arg
        "float probe_sqrt_int() { return sqrt(9); }\n");
    auto obj = harness.objects.cloneObject("/mathtest");
    assert(obj != nullptr);

    auto checkApprox = [&](const std::string& fn, double expected) {
        amlp::Value r = harness.vm.callFunction(obj, fn, {});
        assert(std::holds_alternative<double>(r.data));
        double got = std::get<double>(r.data);
        assert(got > expected - 1e-9 && got < expected + 1e-9);
    };

    checkApprox("probe_sqrt",  2.0);
    checkApprox("probe_floor", 2.0);
    checkApprox("probe_ceil",  3.0);
    checkApprox("probe_cos",   1.0);
    checkApprox("probe_exp",   1.0);
    checkApprox("probe_log",   0.0);
    checkApprox("probe_pow",   1024.0);
    checkApprox("probe_sqrt_int", 3.0);
    std::cout << "testMathEfunsSqrtFloorCeilCosExpLog OK\n";
}

// Phase 0 row 0.12 audit: sin/tan/asin/acos/atan/log10 were registered
// (packages/math.c/math_spec.c) but never actually called by name
// anywhere in this suite -- cos/exp/log/sqrt/pow above all had real
// coverage, these six did not. Same checkApprox() shape as the test
// just above, known exact values so no reference-implementation
// re-verification is needed here.
static void testTrigAndLog10EfunsMatchKnownExactValues() {
    ObjectVarHarness harness;
    harness.writeFile("/trigtest.c",
        "float probe_sin()   { return sin(0.0); }\n"
        "float probe_tan()   { return tan(0.0); }\n"
        "float probe_atan()  { return atan(0.0); }\n"
        "float probe_asin()  { return asin(0.0); }\n"
        "float probe_acos()  { return acos(1.0); }\n"
        "float probe_log10() { return log10(100.0); }\n");
    auto obj = harness.objects.cloneObject("/trigtest");
    assert(obj != nullptr);

    auto checkApprox = [&](const std::string& fn, double expected) {
        amlp::Value r = harness.vm.callFunction(obj, fn, {});
        assert(std::holds_alternative<double>(r.data));
        double got = std::get<double>(r.data);
        assert(got > expected - 1e-9 && got < expected + 1e-9);
    };
    checkApprox("probe_sin",   0.0);
    checkApprox("probe_tan",   0.0);
    checkApprox("probe_atan",  0.0);
    checkApprox("probe_asin",  0.0);
    checkApprox("probe_acos",  0.0);
    checkApprox("probe_log10", 2.0);

    std::cout << "testTrigAndLog10EfunsMatchKnownExactValues OK\n";
}

static void testAsinAcosThrowOutsideDomainButAtanDoesNot() {
    // asin/acos guard |x| > 1.0 (EfunTable.cpp's own comment: "f_asin()
    // and f_acos() throw if |x| > 1.0"); atan has no such guard anywhere
    // in the real math package, matching testMathEfunsSqrtFloorCeilCosExpLog's
    // own sibling sqrt-negative-throws test's style.
    ObjectVarHarness harness;
    harness.writeFile("/trigerr.c",
        "float probe_asin_bad() { return asin(2.0); }\n"
        "float probe_acos_bad() { return acos(-2.0); }\n"
        "float probe_atan_big() { return atan(1000000.0); }\n");
    auto obj = harness.objects.cloneObject("/trigerr");
    assert(obj != nullptr);

    bool asinThrew = false;
    try { harness.vm.callFunction(obj, "probe_asin_bad", {}); }
    catch (const amlp::LpcRuntimeError&) { asinThrew = true; }
    assert(asinThrew);

    bool acosThrew = false;
    try { harness.vm.callFunction(obj, "probe_acos_bad", {}); }
    catch (const amlp::LpcRuntimeError&) { acosThrew = true; }
    assert(acosThrew);

    // Must not throw: well within float range, no domain restriction.
    amlp::Value atanResult = harness.vm.callFunction(obj, "probe_atan_big", {});
    assert(std::holds_alternative<double>(atanResult.data));

    std::cout << "testAsinAcosThrowOutsideDomainButAtanDoesNot OK\n";
}

// --- regexp / regexplode / reg_assoc (Phase 0 row 0.11, PCRE2-backed) ----

static void testRegexpBasicMatchReturnsOneAndNoMatchReturnsZero() {
    // Single-string form: plain int 1/0, confirmed against
    // efuns_main.c's f_regexp() -> match_single_regexp() (a bare
    // regexec() truth value, not the array-of-strings form below).
    ObjectVarHarness harness;
    harness.writeFile("/regprobe.c",
        "int probe(string s, string pat) { return regexp(s, pat); }\n");
    auto ob = harness.objects.cloneObject("/regprobe");
    assert(ob != nullptr);

    amlp::Value matched = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("the quick brown fox")),
         amlp::Value(std::string("qu[a-z]+"))});
    assert(std::holds_alternative<int64_t>(matched.data));
    assert(std::get<int64_t>(matched.data) == 1);

    amlp::Value noMatch = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("the quick brown fox")),
         amlp::Value(std::string("^slow"))});
    assert(std::holds_alternative<int64_t>(noMatch.data));
    assert(std::get<int64_t>(noMatch.data) == 0);

    std::cout << "testRegexpBasicMatchReturnsOneAndNoMatchReturnsZero OK\n";
}

static void testRegexpThirdArgIllegalForStringFormThrows() {
    // Real f_regexp(): "3rd argument illegal for regexp(string,
    // string)" -- the flag argument only makes sense against the
    // array-of-strings form below.
    ObjectVarHarness harness;
    harness.writeFile("/regflagerr.c",
        "int probe() { return regexp(\"abc\", \"a\", 1); }\n");
    auto ob = harness.objects.cloneObject("/regflagerr");
    assert(ob != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(ob, "probe", {});
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testRegexpThirdArgIllegalForStringFormThrows OK\n";
}

static void testRegexpArrayFormSelectsMatchingLinesWithIndexAndInvertFlags() {
    // Array-of-strings form: match_regexp() returns the MATCHING
    // ELEMENTS themselves (not a bool array), in original order.
    // flag&1 interleaves each match's own 1-based index right after
    // it; flag&2 inverts the selection to non-matching elements.
    // Confirmed by hand-tracing match_regexp()'s backward-filling loop
    // in array.c against a concrete example.
    ObjectVarHarness harness;
    harness.writeFile("/regarr.c",
        "mixed *plain(mixed *lines, string pat) { return regexp(lines, pat); }\n"
        "mixed *withIndex(mixed *lines, string pat) { return regexp(lines, pat, 1); }\n"
        "mixed *inverted(mixed *lines, string pat) { return regexp(lines, pat, 2); }\n");
    auto ob = harness.objects.cloneObject("/regarr");
    assert(ob != nullptr);

    auto lines = std::make_shared<amlp::Array>();
    lines->items.push_back(amlp::Value(std::string("apple")));
    lines->items.push_back(amlp::Value(std::string("banana")));
    lines->items.push_back(amlp::Value(std::string("cherry")));
    lines->items.push_back(amlp::Value(std::string("date")));

    amlp::Value plain = harness.vm.callFunction(ob, "plain",
        {amlp::Value(lines), amlp::Value(std::string("an"))});
    auto* plainArr = std::get_if<std::shared_ptr<amlp::Array>>(&plain.data);
    assert(plainArr != nullptr && (*plainArr)->items.size() == 1);
    assert(std::get<std::string>((*plainArr)->items[0].data) == "banana");

    amlp::Value withIdx = harness.vm.callFunction(ob, "withIndex",
        {amlp::Value(lines), amlp::Value(std::string("^[ab]"))});
    auto* withIdxArr = std::get_if<std::shared_ptr<amlp::Array>>(&withIdx.data);
    assert(withIdxArr != nullptr && (*withIdxArr)->items.size() == 4);
    assert(std::get<std::string>((*withIdxArr)->items[0].data) == "apple");
    assert(std::get<int64_t>((*withIdxArr)->items[1].data) == 1);
    assert(std::get<std::string>((*withIdxArr)->items[2].data) == "banana");
    assert(std::get<int64_t>((*withIdxArr)->items[3].data) == 2);

    amlp::Value inv = harness.vm.callFunction(ob, "inverted",
        {amlp::Value(lines), amlp::Value(std::string("^[ab]"))});
    auto* invArr = std::get_if<std::shared_ptr<amlp::Array>>(&inv.data);
    assert(invArr != nullptr && (*invArr)->items.size() == 2);
    assert(std::get<std::string>((*invArr)->items[0].data) == "cherry");
    assert(std::get<std::string>((*invArr)->items[1].data) == "date");

    std::cout << "testRegexpArrayFormSelectsMatchingLinesWithIndexAndInvertFlags OK\n";
}

static void testRegexpBadPatternThrows() {
    ObjectVarHarness harness;
    harness.writeFile("/regbadpat.c",
        "int probe() { return regexp(\"abc\", \"[unterminated\"); }\n");
    auto ob = harness.objects.cloneObject("/regbadpat");
    assert(ob != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(ob, "probe", {});
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testRegexpBadPatternThrows OK\n";
}

static void testRegexplodeSplitsStringOnPatternMatches() {
    // string *regexplode(str, pat) -- alternating [text, match, text,
    // match, ..., text], one more text segment than matches.
    ObjectVarHarness harness;
    harness.writeFile("/regexplodeprobe.c",
        "mixed *probe(string s, string pat) { return regexplode(s, pat); }\n");
    auto ob = harness.objects.cloneObject("/regexplodeprobe");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("ab12cd34ef")),
         amlp::Value(std::string("[0-9]+"))});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 5);
    assert(std::get<std::string>((*arr)->items[0].data) == "ab");
    assert(std::get<std::string>((*arr)->items[1].data) == "12");
    assert(std::get<std::string>((*arr)->items[2].data) == "cd");
    assert(std::get<std::string>((*arr)->items[3].data) == "34");
    assert(std::get<std::string>((*arr)->items[4].data) == "ef");

    std::cout << "testRegexplodeSplitsStringOnPatternMatches OK\n";
}

static void testRegexplodeWithCaptureGroupPatternUsesFullMatchNotGroupText() {
    // A pattern with a capturing group must still split on the WHOLE
    // match, not just the captured subgroup text -- regexplode() has
    // no capture-group output at all, matching reg_assoc()'s own
    // real behavior below (neither efun exposes subgroups).
    ObjectVarHarness harness;
    harness.writeFile("/regexplodegroup.c",
        "mixed *probe(string s, string pat) { return regexplode(s, pat); }\n");
    auto ob = harness.objects.cloneObject("/regexplodegroup");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("xx[42]yy[7]zz")),
         amlp::Value(std::string("\\[([0-9]+)\\]"))});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 5);
    assert(std::get<std::string>((*arr)->items[0].data) == "xx");
    assert(std::get<std::string>((*arr)->items[1].data) == "[42]");
    assert(std::get<std::string>((*arr)->items[2].data) == "yy");
    assert(std::get<std::string>((*arr)->items[3].data) == "[7]");
    assert(std::get<std::string>((*arr)->items[4].data) == "zz");

    std::cout << "testRegexplodeWithCaptureGroupPatternUsesFullMatchNotGroupText OK\n";
}

static void testRegAssocMatchesRealDocCommentExample() {
    // Reproduces array.c's own worked example verbatim, straight from
    // reg_assoc()'s doc comment:
    //   reg_assoc("testhahatest", ({"haha","te"}), ({2,3}), 4)
    //   == ({({"","te","st","haha","","te","st"}),
    //        ({  4,   3,   4,    2,  4,   3,   4})})
    ObjectVarHarness harness;
    harness.writeFile("/regassocprobe.c",
        "mixed *probe(string s, mixed *pats, mixed *toks, mixed def) { "
        "return reg_assoc(s, pats, toks, def); }\n");
    auto ob = harness.objects.cloneObject("/regassocprobe");
    assert(ob != nullptr);

    auto pats = std::make_shared<amlp::Array>();
    pats->items.push_back(amlp::Value(std::string("haha")));
    pats->items.push_back(amlp::Value(std::string("te")));
    auto toks = std::make_shared<amlp::Array>();
    toks->items.push_back(amlp::Value(int64_t{2}));
    toks->items.push_back(amlp::Value(int64_t{3}));

    amlp::Value result = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("testhahatest")),
         amlp::Value(pats), amlp::Value(toks), amlp::Value(int64_t{4})});
    auto* outer = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(outer != nullptr && (*outer)->items.size() == 2);

    auto* texts = std::get_if<std::shared_ptr<amlp::Array>>(&(*outer)->items[0].data);
    assert(texts != nullptr && (*texts)->items.size() == 7);
    const char* expectedTexts[7] = {"", "te", "st", "haha", "", "te", "st"};
    for (int i = 0; i < 7; ++i) {
        assert(std::get<std::string>((*texts)->items[i].data) == expectedTexts[i]);
    }

    auto* tokens = std::get_if<std::shared_ptr<amlp::Array>>(&(*outer)->items[1].data);
    assert(tokens != nullptr && (*tokens)->items.size() == 7);
    const int64_t expectedTokens[7] = {4, 3, 4, 2, 4, 3, 4};
    for (int i = 0; i < 7; ++i) {
        assert(std::get<int64_t>((*tokens)->items[i].data) == expectedTokens[i]);
    }

    std::cout << "testRegAssocMatchesRealDocCommentExample OK\n";
}

static void testRegAssocZeroPatternsReturnsWholeStringWithDefaultToken() {
    // Real array.c's own "else / Default match" branch: an empty
    // pattern array is a real, explicit special case, not an error --
    // the whole string comes back unmatched, paired with one default
    // token.
    ObjectVarHarness harness;
    harness.writeFile("/regassocempty.c",
        "mixed *probe(string s, mixed *pats, mixed *toks, mixed def) { "
        "return reg_assoc(s, pats, toks, def); }\n");
    auto ob = harness.objects.cloneObject("/regassocempty");
    assert(ob != nullptr);

    auto emptyArr = std::make_shared<amlp::Array>();

    amlp::Value result = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("untouched")),
         amlp::Value(emptyArr), amlp::Value(emptyArr), amlp::Value(int64_t{9})});
    auto* outer = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(outer != nullptr && (*outer)->items.size() == 2);

    auto* texts = std::get_if<std::shared_ptr<amlp::Array>>(&(*outer)->items[0].data);
    assert(texts != nullptr && (*texts)->items.size() == 1);
    assert(std::get<std::string>((*texts)->items[0].data) == "untouched");

    auto* tokens = std::get_if<std::shared_ptr<amlp::Array>>(&(*outer)->items[1].data);
    assert(tokens != nullptr && (*tokens)->items.size() == 1);
    assert(std::get<int64_t>((*tokens)->items[0].data) == 9);

    std::cout << "testRegAssocZeroPatternsReturnsWholeStringWithDefaultToken OK\n";
}

// --- Phase 0.13 Tier 1 batch (m_delete, classp, all_previous_objects, ---
// --- localtime, stat, read_bytes, write_bytes, link, unique_array) -----

static void testMapDeleteAndMDeleteAliasBothRemoveTheKey() {
    // map_delete/m_delete mutate the mapping in place; a missing key is
    // a silent no-op, matching real mapping_delete().
    ObjectVarHarness harness;
    harness.writeFile("/mapdel.c",
        "mapping probe_map_delete() {\n"
        "    mapping m = ([\"a\": 1, \"b\": 2]);\n"
        "    map_delete(m, \"a\");\n"
        "    map_delete(m, \"nope\");\n"
        "    return m;\n"
        "}\n"
        "mapping probe_m_delete() {\n"
        "    mapping m = ([\"a\": 1, \"b\": 2]);\n"
        "    m_delete(m, \"b\");\n"
        "    return m;\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/mapdel");
    assert(ob != nullptr);

    amlp::Value r1 = harness.vm.callFunction(ob, "probe_map_delete", {});
    auto* map1 = std::get_if<std::shared_ptr<amlp::Mapping>>(&r1.data);
    assert(map1 != nullptr && (*map1)->entries.size() == 1);
    assert(std::get<std::string>((*map1)->entries[0].first.data) == "b");

    amlp::Value r2 = harness.vm.callFunction(ob, "probe_m_delete", {});
    auto* map2 = std::get_if<std::shared_ptr<amlp::Mapping>>(&r2.data);
    assert(map2 != nullptr && (*map2)->entries.size() == 1);
    assert(std::get<std::string>((*map2)->entries[0].first.data) == "a");

    std::cout << "testMapDeleteAndMDeleteAliasBothRemoveTheKey OK\n";
}

// Phase 0 row 0.12 audit: allocate/allocate_mapping/copy/values were all
// registered but never called by name anywhere in this suite.
static void testAllocateAllocateMappingCopyAndValues() {
    ObjectVarHarness harness;
    harness.writeFile("/alloc_probe.c",
        "mixed *probe_allocate() { return allocate(3); }\n"
        "mixed *probe_allocate_init() { return allocate(3, \"x\"); }\n"
        "mapping probe_allocate_mapping() { return allocate_mapping(10); }\n"
        "mixed *copy_and_mutate(mixed *src) {\n"
        "    mixed *dup;\n"
        "    dup = copy(src);\n"
        "    dup[0] = 999;\n"
        "    return ({ src, dup });\n"
        "}\n"
        "mixed *probe_values(mapping m) { return values(m); }\n");
    auto ob = harness.objects.cloneObject("/alloc_probe");
    assert(ob != nullptr);
    auto& vm = harness.vm;

    // allocate(3): three elements, each defaulting to int 0 (real
    // func_spec.c's own default).
    amlp::Value a1 = vm.callFunction(ob, "probe_allocate", {});
    auto* arr1 = std::get_if<std::shared_ptr<amlp::Array>>(&a1.data);
    assert(arr1 != nullptr && (*arr1)->items.size() == 3);
    for (auto& item : (*arr1)->items) {
        assert(std::holds_alternative<int64_t>(item.data) && std::get<int64_t>(item.data) == 0);
    }

    // allocate(3, "x"): every slot initialized to the given value instead.
    amlp::Value a2 = vm.callFunction(ob, "probe_allocate_init", {});
    auto* arr2 = std::get_if<std::shared_ptr<amlp::Array>>(&a2.data);
    assert(arr2 != nullptr && (*arr2)->items.size() == 3);
    for (auto& item : (*arr2)->items) {
        assert(std::holds_alternative<std::string>(item.data) && std::get<std::string>(item.data) == "x");
    }

    // allocate_mapping(10): a real, empty mapping (the capacity hint has
    // no observable effect on this driver's own Mapping, per
    // EfunTable.cpp's own comment).
    amlp::Value m = vm.callFunction(ob, "probe_allocate_mapping", {});
    auto* mapPtr = std::get_if<std::shared_ptr<amlp::Mapping>>(&m.data);
    assert(mapPtr != nullptr && *mapPtr != nullptr && (*mapPtr)->entries.empty());

    // copy(): a real deep copy -- mutating the copy must not affect the
    // original (the key behavior that distinguishes copy() from a plain
    // reference/alias).
    auto srcArr = std::make_shared<amlp::Array>();
    srcArr->items.push_back(amlp::Value(static_cast<int64_t>(111)));
    amlp::Value copyResult = vm.callFunction(ob, "copy_and_mutate", {amlp::Value(srcArr)});
    auto* pair = std::get_if<std::shared_ptr<amlp::Array>>(&copyResult.data);
    assert(pair != nullptr && (*pair)->items.size() == 2);
    auto* originalAfter = std::get_if<std::shared_ptr<amlp::Array>>(&(*pair)->items[0].data);
    auto* dupAfter = std::get_if<std::shared_ptr<amlp::Array>>(&(*pair)->items[1].data);
    assert(std::get<int64_t>((*originalAfter)->items[0].data) == 111);
    assert(std::get<int64_t>((*dupAfter)->items[0].data) == 999);

    // values(): every value in insertion order, matching keys()'s own
    // already-tested ordering.
    auto srcMap = std::make_shared<amlp::Mapping>();
    srcMap->entries.push_back({amlp::Value(std::string("a")), amlp::Value(static_cast<int64_t>(1))});
    srcMap->entries.push_back({amlp::Value(std::string("b")), amlp::Value(static_cast<int64_t>(2))});
    amlp::Value valsResult = vm.callFunction(ob, "probe_values", {amlp::Value(srcMap)});
    auto* valsArr = std::get_if<std::shared_ptr<amlp::Array>>(&valsResult.data);
    assert(valsArr != nullptr && (*valsArr)->items.size() == 2);
    assert(std::get<int64_t>((*valsArr)->items[0].data) == 1);
    assert(std::get<int64_t>((*valsArr)->items[1].data) == 2);

    std::cout << "testAllocateAllocateMappingCopyAndValues OK\n";
}

static void testClasspAlwaysReturnsFalseSinceNoClassTypeExists() {
    ObjectVarHarness harness;
    harness.writeFile("/classptest.c",
        "int probe_int() { return classp(5); }\n"
        "int probe_string() { return classp(\"hi\"); }\n"
        "int probe_array() { return classp(({1,2,3})); }\n");
    auto ob = harness.objects.cloneObject("/classptest");
    assert(ob != nullptr);

    amlp::Value r1 = harness.vm.callFunction(ob, "probe_int", {});
    assert(std::get<int64_t>(r1.data) == 0);
    amlp::Value r2 = harness.vm.callFunction(ob, "probe_string", {});
    assert(std::get<int64_t>(r2.data) == 0);
    amlp::Value r3 = harness.vm.callFunction(ob, "probe_array", {});
    assert(std::get<int64_t>(r3.data) == 0);

    std::cout << "testClasspAlwaysReturnsFalseSinceNoClassTypeExists OK\n";
}

static void testAllPreviousObjectsReturnsSameArrayAsPreviousObjectMinusOne() {
    ObjectVarHarness harness;
    harness.writeFile("/allprevcallee.c",
        "mixed *probe() { return all_previous_objects(); }\n"
        "mixed *probe_via_flag() { return previous_object(-1); }\n");
    auto callee = harness.objects.loadObject("/allprevcallee");
    assert(callee != nullptr);

    harness.writeFile("/allprevcaller.c",
        "mixed *call_probe() { return call_other(\"/allprevcallee\", \"probe\"); }\n"
        "mixed *call_probe_via_flag() { return call_other(\"/allprevcallee\", \"probe_via_flag\"); }\n");

    auto caller = harness.objects.cloneObject("/allprevcaller");
    assert(caller != nullptr);

    amlp::Value r1 = harness.vm.callFunction(caller, "call_probe", {});
    auto* arr1 = std::get_if<std::shared_ptr<amlp::Array>>(&r1.data);
    assert(arr1 != nullptr && (*arr1)->items.size() == 1);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*arr1)->items[0].data) == caller);

    amlp::Value r2 = harness.vm.callFunction(caller, "call_probe_via_flag", {});
    auto* arr2 = std::get_if<std::shared_ptr<amlp::Array>>(&r2.data);
    assert(arr2 != nullptr && (*arr2)->items.size() == 1);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*arr2)->items[0].data) == caller);

    std::cout << "testAllPreviousObjectsReturnsSameArrayAsPreviousObjectMinusOne OK\n";
}

static void testLocaltimeReturnsElevenElementArrayMatchingKnownEpochInstant() {
    // 2005-03-18 01:58:31 UTC = epoch 1111111111 -- a well-known,
    // easy-to-eyeball test instant. TZ is not pinned here (localtime()
    // is genuinely local-timezone-dependent, matching the real efun),
    // so only the fields that do not shift with timezone are checked
    // exactly; array shape and the derived fields are checked for
    // internal consistency instead.
    ObjectVarHarness harness;
    harness.writeFile("/localtimetest.c",
        "mixed *probe(int clock) { return localtime(clock); }\n");
    auto ob = harness.objects.cloneObject("/localtimetest");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe",
        {amlp::Value(int64_t{1111111111})});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 11);
    for (int i = 0; i < 8; ++i) {
        assert(std::holds_alternative<int64_t>((*arr)->items[i].data));
    }
    int64_t year = std::get<int64_t>((*arr)->items[5].data);
    assert(year == 2005);
    int64_t wday = std::get<int64_t>((*arr)->items[6].data);
    assert(wday >= 0 && wday <= 6);
    assert(std::holds_alternative<std::string>((*arr)->items[9].data));

    std::cout << "testLocaltimeReturnsElevenElementArrayMatchingKnownEpochInstant OK\n";
}

// Phase 0 row 0.12 audit: time()/ctime() were registered (localtime()
// already had its own coverage just above) but never called by name
// anywhere in this suite.
static void testTimeReturnsPlausibleCurrentEpochAndCtimeFormatsAKnownInstant() {
    ObjectVarHarness harness;
    harness.writeFile("/timetest.c",
        "int probe_time() { return time(); }\n"
        "string probe_ctime(int clock) { return ctime(clock); }\n");
    auto ob = harness.objects.cloneObject("/timetest");
    assert(ob != nullptr);

    std::time_t before = std::time(nullptr);
    amlp::Value t = harness.vm.callFunction(ob, "probe_time", {});
    std::time_t after = std::time(nullptr);
    assert(std::holds_alternative<int64_t>(t.data));
    int64_t got = std::get<int64_t>(t.data);
    assert(got >= static_cast<int64_t>(before) - 1 && got <= static_cast<int64_t>(after) + 1);

    // Same known instant as testLocaltimeReturnsElevenElementArrayMatchingKnownEpochInstant
    // (2005-03-18 01:58:31 UTC), safely mid-day-ish in every real timezone
    // so the year digits below cannot roll to an adjacent year. Real
    // ctime() format is always exactly 25 characters, trailing newline
    // included ("Www Mmm dd hh:mm:ss yyyy\n").
    amlp::Value c = harness.vm.callFunction(ob, "probe_ctime", {amlp::Value(int64_t{1111111111})});
    assert(std::holds_alternative<std::string>(c.data));
    const std::string& s = std::get<std::string>(c.data);
    assert(s.size() == 25);
    assert(s.back() == '\n');
    assert(s.find("2005") != std::string::npos);

    std::cout << "testTimeReturnsPlausibleCurrentEpochAndCtimeFormatsAKnownInstant OK\n";
}

static void testStatOnRegularFileReturnsSizeAndMtimeArray() {
    ObjectVarHarness harness;
    harness.writeFile("/stattest.c",
        "mixed *probe() { return stat(\"/statfile.txt\"); }\n");
    harness.writeFile("/statfile.txt", "12345");
    auto ob = harness.objects.cloneObject("/stattest");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 3);
    assert(std::get<int64_t>((*arr)->items[0].data) == 5);
    assert(std::holds_alternative<int64_t>((*arr)->items[1].data));
    assert(std::get<int64_t>((*arr)->items[2].data) == 0);

    std::cout << "testStatOnRegularFileReturnsSizeAndMtimeArray OK\n";
}

static void testReadBytesReadsSubrangeAndHandlesNegativeStartAndMissingFile() {
    ObjectVarHarness harness;
    harness.writeFile("/readbytestest.c",
        "mixed probe_range() { return read_bytes(\"/rb.txt\", 2, 3); }\n"
        "mixed probe_neg_start() { return read_bytes(\"/rb.txt\", -4); }\n"
        "mixed probe_missing() { return read_bytes(\"/nope.txt\"); }\n");
    harness.writeFile("/rb.txt", "abcdefghij");
    auto ob = harness.objects.cloneObject("/readbytestest");
    assert(ob != nullptr);

    amlp::Value r1 = harness.vm.callFunction(ob, "probe_range", {});
    assert(std::holds_alternative<std::string>(r1.data));
    assert(std::get<std::string>(r1.data) == "cde");

    amlp::Value r2 = harness.vm.callFunction(ob, "probe_neg_start", {});
    assert(std::holds_alternative<std::string>(r2.data));
    assert(std::get<std::string>(r2.data) == "ghij");

    amlp::Value r3 = harness.vm.callFunction(ob, "probe_missing", {});
    assert(std::holds_alternative<int64_t>(r3.data));
    assert(std::get<int64_t>(r3.data) == 0);

    std::cout << "testReadBytesReadsSubrangeAndHandlesNegativeStartAndMissingFile OK\n";
}

static void testWriteBytesOverwritesAtOffsetThenReadBytesConfirmsIt() {
    ObjectVarHarness harness;
    harness.writeFile("/writebytestest.c",
        "int probe_write() { return write_bytes(\"/wb.txt\", 2, \"XY\"); }\n"
        "mixed probe_read() { return read_bytes(\"/wb.txt\"); }\n");
    harness.writeFile("/wb.txt", "abcdefgh");
    auto ob = harness.objects.cloneObject("/writebytestest");
    assert(ob != nullptr);

    amlp::Value writeResult = harness.vm.callFunction(ob, "probe_write", {});
    assert(std::holds_alternative<int64_t>(writeResult.data));
    assert(std::get<int64_t>(writeResult.data) == 1);

    amlp::Value readResult = harness.vm.callFunction(ob, "probe_read", {});
    assert(std::holds_alternative<std::string>(readResult.data));
    assert(std::get<std::string>(readResult.data) == "abXYefgh");

    std::cout << "testWriteBytesOverwritesAtOffsetThenReadBytesConfirmsIt OK\n";
}

static void testLinkCreatesASecondNameForTheSameFileContent() {
    ObjectVarHarness harness;
    harness.writeFile("/linktest.c",
        "int probe_link() { return link(\"/orig.txt\", \"/linked.txt\"); }\n"
        "mixed probe_read_linked() { return read_file(\"/linked.txt\"); }\n");
    harness.writeFile("/orig.txt", "shared content\n");
    auto ob = harness.objects.cloneObject("/linktest");
    assert(ob != nullptr);

    amlp::Value linkResult = harness.vm.callFunction(ob, "probe_link", {});
    assert(std::holds_alternative<int64_t>(linkResult.data));
    // Real link() shares rename()'s own inverted convention: 0 = success.
    assert(std::get<int64_t>(linkResult.data) == 0);

    amlp::Value readResult = harness.vm.callFunction(ob, "probe_read_linked", {});
    assert(std::holds_alternative<std::string>(readResult.data));
    assert(std::get<std::string>(readResult.data) == "shared content\n");

    std::cout << "testLinkCreatesASecondNameForTheSameFileContent OK\n";
}

static void testUniqueArrayGroupsElementsByClosureResultAndExcludesSkipValue() {
    ObjectVarHarness harness;
    harness.writeFile("/uniqarrtest.c",
        "mixed *probe(mixed *arr) {\n"
        "    return unique_array(arr, (: $1 % 3 :));\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/uniqarrtest");
    assert(ob != nullptr);

    auto arr = std::make_shared<amlp::Array>();
    for (int64_t v : {1, 2, 3, 4, 5, 6, 9}) arr->items.emplace_back(v);
    // 1%3=1, 2%3=2, 3%3=0(skip default), 4%3=1, 5%3=2, 6%3=0(skip), 9%3=0(skip)
    // Groups expected (order not asserted beyond membership): {1,4} and {2,5}.
    // 3, 6, 9 are excluded (classifier result 0 == default skip value).

    amlp::Value result = harness.vm.callFunction(ob, "probe", {amlp::Value(arr)});
    auto* groups = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(groups != nullptr && (*groups)->items.size() == 2);

    std::vector<std::vector<int64_t>> actual;
    for (const auto& g : (*groups)->items) {
        auto* gArr = std::get_if<std::shared_ptr<amlp::Array>>(&g.data);
        assert(gArr != nullptr);
        std::vector<int64_t> nums;
        for (const auto& item : (*gArr)->items) nums.push_back(std::get<int64_t>(item.data));
        actual.push_back(nums);
    }
    bool foundOneFour = false, foundTwoFive = false;
    for (const auto& g : actual) {
        if (g.size() == 2 && g[0] == 1 && g[1] == 4) foundOneFour = true;
        if (g.size() == 2 && g[0] == 2 && g[1] == 5) foundTwoFive = true;
    }
    assert(foundOneFour);
    assert(foundTwoFive);

    std::cout << "testUniqueArrayGroupsElementsByClosureResultAndExcludesSkipValue OK\n";
}

// --- Corrected Tier 1 batch (base_name, debug_message, rusage, command, ---
// --- shutdown, uptime, in_edit, in_input, match_path, call_out_info) -----

static void testBaseNameReturnsSameAsFileNameSinceNoCloneSuffixExistsHere() {
    ObjectVarHarness harness;
    harness.writeFile("/basenametest.c",
        "string probe_self() { return base_name(); }\n"
        "string probe_arg(object ob) { return base_name(ob); }\n"
        "string probe_file_name(object ob) { return file_name(ob); }\n");
    auto ob = harness.objects.cloneObject("/basenametest");
    assert(ob != nullptr);

    amlp::Value self = harness.vm.callFunction(ob, "probe_self", {});
    assert(std::holds_alternative<std::string>(self.data));
    assert(std::get<std::string>(self.data) == ob->filename());

    amlp::Value viaArg = harness.vm.callFunction(ob, "probe_arg", {amlp::Value(ob)});
    amlp::Value viaFileName = harness.vm.callFunction(ob, "probe_file_name", {amlp::Value(ob)});
    assert(std::get<std::string>(viaArg.data) == std::get<std::string>(viaFileName.data));

    std::cout << "testBaseNameReturnsSameAsFileNameSinceNoCloneSuffixExistsHere OK\n";
}

static void testDebugMessageAcceptsAStringArgumentAndDoesNotThrow() {
    ObjectVarHarness harness;
    harness.writeFile("/debugmsgtest.c",
        "int probe() { debug_message(\"test diagnostic line\\n\"); return 1; }\n");
    auto ob = harness.objects.cloneObject("/debugmsgtest");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1);

    std::cout << "testDebugMessageAcceptsAStringArgumentAndDoesNotThrow OK\n";
}

static void testUptimeIsNonNegativeAndNonDecreasingAcrossTwoCalls() {
    ObjectVarHarness harness;
    harness.writeFile("/uptimetest.c",
        "int probe() { return uptime(); }\n");
    auto ob = harness.objects.cloneObject("/uptimetest");
    assert(ob != nullptr);

    amlp::Value first = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(first.data));
    assert(std::get<int64_t>(first.data) >= 0);

    amlp::Value second = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<int64_t>(second.data) >= std::get<int64_t>(first.data));

    std::cout << "testUptimeIsNonNegativeAndNonDecreasingAcrossTwoCalls OK\n";
}

static void testRusageReturnsMappingWithExpectedKeysAndNonNegativeValues() {
    ObjectVarHarness harness;
    harness.writeFile("/rusagetest.c",
        "mapping probe() { return rusage(); }\n");
    auto ob = harness.objects.cloneObject("/rusagetest");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* map = std::get_if<std::shared_ptr<amlp::Mapping>>(&result.data);
    assert(map != nullptr && *map);
    assert((*map)->entries.size() == 16);

    bool foundUtime = false, foundMaxrss = false;
    for (const auto& entry : (*map)->entries) {
        assert(std::holds_alternative<std::string>(entry.first.data));
        assert(std::holds_alternative<int64_t>(entry.second.data));
        assert(std::get<int64_t>(entry.second.data) >= 0);
        const std::string& key = std::get<std::string>(entry.first.data);
        if (key == "utime") foundUtime = true;
        if (key == "maxrss") foundMaxrss = true;
    }
    assert(foundUtime);
    assert(foundMaxrss);

    std::cout << "testRusageReturnsMappingWithExpectedKeysAndNonNegativeValues OK\n";
}

static void testCommandDispatchesToCurrentObjectsOwnActionTableAndReturnsTruthy() {
    ObjectVarHarness harness;
    harness.writeFile("/commandtest.c",
        "int wasForced;\n"
        "int cmd_force_action(string arg) { wasForced = 1; return 1; }\n"
        "void register_hook() { add_action(\"cmd_force_action\", \"force_action\"); }\n"
        "int probe_hit() { return command(\"force_action now\"); }\n"
        "int probe_miss() { return command(\"totally_unrecognized_verb\"); }\n"
        "int query_was_forced() { return wasForced; }\n");
    auto ob = harness.objects.cloneObject("/commandtest");
    assert(ob != nullptr);

    // add_action() outside any dispatch/move context needs an explicit
    // command_giver, and current_object must equal command_giver for its
    // own nearness check to pass -- same setup as the established
    // precedent elsewhere in this file for registering an action outside
    // a real move/dispatch context (see
    // testAddActionCatchAllShortFlagReceivesRemainderAndQueryVerbReturnsFullTypedWord's
    // own comment).
    harness.vm.pushCommandGiver(ob);
    harness.vm.callFunction(ob, "register_hook", {});
    harness.vm.popCommandGiver();

    amlp::Value hit = harness.vm.callFunction(ob, "probe_hit", {});
    assert(std::holds_alternative<int64_t>(hit.data));
    assert(std::get<int64_t>(hit.data) == 1);

    amlp::Value forced = harness.vm.callFunction(ob, "query_was_forced", {});
    assert(std::get<int64_t>(forced.data) == 1);

    amlp::Value miss = harness.vm.callFunction(ob, "probe_miss", {});
    assert(std::get<int64_t>(miss.data) == 0);

    std::cout << "testCommandDispatchesToCurrentObjectsOwnActionTableAndReturnsTruthy OK\n";
}

static void testShutdownSetsSchedulerRequestFlag() {
    ObjectVarHarness harness;
    harness.writeFile("/shutdowntest.c",
        "void probe() { shutdown(); }\n");
    auto ob = harness.objects.cloneObject("/shutdowntest");
    assert(ob != nullptr);

    assert(!amlp::Scheduler::isShutdownRequested());
    harness.vm.callFunction(ob, "probe", {});
    assert(amlp::Scheduler::isShutdownRequested());

    std::cout << "testShutdownSetsSchedulerRequestFlag OK\n";
}

static void testInEditAlwaysReturnsFalseSinceEdIsNotImplemented() {
    ObjectVarHarness harness;
    harness.writeFile("/ineditest.c",
        "mixed probe() { return in_edit(); }\n");
    auto ob = harness.objects.cloneObject("/ineditest");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testInEditAlwaysReturnsFalseSinceEdIsNotImplemented OK\n";
}

static void testInInputReflectsPendingInputToStateOnAConnectedObject() {
    ObjectVarHarness harness;
    harness.writeFile("/ininputtest.c",
        "void start() { input_to(\"get_name\"); }\n"
        "void get_name(string str) {}\n"
        "int probe() { return in_input(); }\n");
    auto ob = harness.objects.cloneObject("/ininputtest");
    assert(ob != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(ob);

    amlp::Value before = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<int64_t>(before.data) == 0);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(ob, "start", {});
    amlp::OutputContext::set(nullptr);

    amlp::Value after = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<int64_t>(after.data) == 1);

    ::close(fds[1]);
    std::cout << "testInInputReflectsPendingInputToStateOnAConnectedObject OK\n";
}

static void testMatchPathReturnsDeepestMatchingPrefix() {
    ObjectVarHarness harness;
    harness.writeFile("/matchpathtest.c",
        "mixed probe(mapping m, string path) { return match_path(m, path); }\n");
    auto ob = harness.objects.cloneObject("/matchpathtest");
    assert(ob != nullptr);

    auto m = std::make_shared<amlp::Mapping>();
    m->entries.emplace_back(amlp::Value(std::string("/")), amlp::Value(std::string("low")));
    m->entries.emplace_back(amlp::Value(std::string("/domains/")), amlp::Value(std::string("mid")));
    m->entries.emplace_back(amlp::Value(std::string("/domains/Praxis/")), amlp::Value(std::string("high")));

    amlp::Value deep = harness.vm.callFunction(ob, "probe",
        {amlp::Value(m), amlp::Value(std::string("/domains/Praxis/room.c"))});
    assert(std::holds_alternative<std::string>(deep.data));
    assert(std::get<std::string>(deep.data) == "high");

    amlp::Value mid = harness.vm.callFunction(ob, "probe",
        {amlp::Value(m), amlp::Value(std::string("/domains/other.c"))});
    assert(std::get<std::string>(mid.data) == "mid");

    amlp::Value none = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::make_shared<amlp::Mapping>()), amlp::Value(std::string("/anything"))});
    assert(std::holds_alternative<int64_t>(none.data));
    assert(std::get<int64_t>(none.data) == 0);

    std::cout << "testMatchPathReturnsDeepestMatchingPrefix OK\n";
}

static void testCallOutInfoListsPendingEntryWithOwnerFunctionAndDelay() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/coinfotest.c",
        "int probe() { return call_out(\"idle\", 60); }\n"
        "mixed *probe_info() { return call_out_info(); }\n"
        "void idle() {}\n");
    auto ob = harness.objects.cloneObject("/coinfotest");
    assert(ob != nullptr);

    harness.vm.callFunction(ob, "probe", {});
    amlp::Value infoResult = harness.vm.callFunction(ob, "probe_info", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&infoResult.data);
    assert(arr != nullptr && (*arr)->items.size() == 1);

    auto* entry = std::get_if<std::shared_ptr<amlp::Array>>(&(*arr)->items[0].data);
    assert(entry != nullptr && (*entry)->items.size() == 3);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*entry)->items[0].data) == ob);
    assert(std::get<std::string>((*entry)->items[1].data) == "idle");
    assert(std::holds_alternative<int64_t>((*entry)->items[2].data));
    int64_t delay = std::get<int64_t>((*entry)->items[2].data);
    assert(delay >= 0 && delay <= 60);

    std::cout << "testCallOutInfoListsPendingEntryWithOwnerFunctionAndDelay OK\n";
}

// --- shadow support (Phase 0.6) -------------------------------------------
// All shadow tests use a master object defining a real (permissive)
// valid_shadow(), matching the established set_hide()-style harness
// pattern (loadMasterObject() + "/unused.c") -- real shadow() always
// denies without one, confirmed against this mudlib's own master.c
// (which defines no valid_shadow() at all), so a permissive test master
// is the only way to exercise the attach path at all, on this driver or
// on real FluffOS against this exact mudlib.

static void testShadowAttachInterceptsCallOtherWhenShadowDefinesFunction() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh_victim.c",
        "string greet() { return \"victim\"; }\n");
    harness.writeFile("/sh_shadow.c",
        "string greet() { return \"shadow\"; }\n"
        "object attach(object victim) { return shadow(victim, 1); }\n");
    harness.writeFile("/sh_caller.c",
        "string probe(object victim) { return victim->greet(); }\n");

    auto victim = harness.objects.cloneObject("/sh_victim");
    auto shadowOb = harness.objects.cloneObject("/sh_shadow");
    auto caller = harness.objects.cloneObject("/sh_caller");
    assert(victim != nullptr && shadowOb != nullptr && caller != nullptr);

    amlp::Value attachResult = harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(attachResult.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(attachResult.data) == victim);

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(victim)});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "shadow");

    std::cout << "testShadowAttachInterceptsCallOtherWhenShadowDefinesFunction OK\n";
}

static void testShadowFallsThroughToVictimWhenFunctionUndefinedOnShadowRegardlessOfReturnValue() {
    // The real, easy-to-get-backwards detail: apply_low()'s own shadow
    // retry is gated on whether the function is *defined* on a given
    // link, never on the truthiness of what it returns. A shadow that
    // DOES define the function, even returning a falsy 0, is still the
    // final answer -- only an *undefined* function falls through.
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh2_victim.c",
        "string only_on_victim() { return \"victim-answer\"; }\n"
        "int falsy_on_both() { return 0; }\n");
    harness.writeFile("/sh2_shadow.c",
        "int falsy_on_both() { return 0; }\n"
        "object attach(object victim) { return shadow(victim, 1); }\n");
    harness.writeFile("/sh2_caller.c",
        "mixed probe_undefined(object victim) { return victim->only_on_victim(); }\n"
        "mixed probe_falsy(object victim) { return victim->falsy_on_both(); }\n");

    auto victim = harness.objects.cloneObject("/sh2_victim");
    auto shadowOb = harness.objects.cloneObject("/sh2_shadow");
    auto caller = harness.objects.cloneObject("/sh2_caller");
    assert(victim != nullptr && shadowOb != nullptr && caller != nullptr);
    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});

    // Not defined on the shadow at all -- falls through to the victim's
    // own definition, matching real "function not found, retry".
    amlp::Value undef = harness.vm.callFunction(caller, "probe_undefined", {amlp::Value(victim)});
    assert(std::holds_alternative<std::string>(undef.data));
    assert(std::get<std::string>(undef.data) == "victim-answer");

    // Defined on the shadow, returns a falsy 0 -- the call still
    // resolves and returns cleanly (a falsy value is not a resolution
    // error), no fall-through attempted. The next test below proves
    // this is genuinely the shadow's own copy running, not the
    // victim's, using bodies that return distinguishable values.
    amlp::Value falsy = harness.vm.callFunction(caller, "probe_falsy", {amlp::Value(victim)});
    assert(std::holds_alternative<int64_t>(falsy.data));
    assert(std::get<int64_t>(falsy.data) == 0);

    std::cout << "testShadowFallsThroughToVictimWhenFunctionUndefinedOnShadowRegardlessOfReturnValue OK\n";
}

static void testShadowDefinedFunctionReturningFalsyIsStillFinalNotAFallThroughTrigger() {
    // Same real point as the test above, made unambiguous: the shadow's
    // own falsy_on_both() and the victim's own return DIFFERENT falsy
    // values (0 vs "", both falsy, but distinguishable by type), so
    // getting back the shadow's own falsy int 0 (not the victim's own
    // falsy empty string) is the only way this test passes.
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh3_victim.c",
        "mixed answer() { return \"\"; }\n");
    harness.writeFile("/sh3_shadow.c",
        "mixed answer() { return 0; }\n"
        "object attach(object victim) { return shadow(victim, 1); }\n");
    harness.writeFile("/sh3_caller.c",
        "mixed probe(object victim) { return victim->answer(); }\n");

    auto victim = harness.objects.cloneObject("/sh3_victim");
    auto shadowOb = harness.objects.cloneObject("/sh3_shadow");
    auto caller = harness.objects.cloneObject("/sh3_caller");
    assert(victim != nullptr && shadowOb != nullptr && caller != nullptr);
    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(victim)});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testShadowDefinedFunctionReturningFalsyIsStillFinalNotAFallThroughTrigger OK\n";
}

static void testShadowGuardsAgainstReenteringItselfViaCurrentObjectCheck() {
    // Real "ob->shadowed != current_object" guard: when the shadow's own
    // code calls back into the victim (e.g. to reach the real,
    // unshadowed implementation), that call must not immediately
    // re-intercept itself, since the shadow IS current_object at that
    // point.
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh4_victim.c",
        "string greet() { return \"victim\"; }\n");
    harness.writeFile("/sh4_shadow.c",
        "string greet() { return \"shadow\"; }\n"
        "string call_victim_directly(object victim) { return victim->greet(); }\n"
        "object attach(object victim) { return shadow(victim, 1); }\n");

    auto victim = harness.objects.cloneObject("/sh4_victim");
    auto shadowOb = harness.objects.cloneObject("/sh4_shadow");
    assert(victim != nullptr && shadowOb != nullptr);
    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});

    // Called directly ON the shadow object (so current_object during the
    // nested victim->greet() call other is the shadow itself) -- must
    // reach the victim's own real greet(), not re-intercept itself.
    amlp::Value result = harness.vm.callFunction(shadowOb, "call_victim_directly", {amlp::Value(victim)});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "victim");

    std::cout << "testShadowGuardsAgainstReenteringItselfViaCurrentObjectCheck OK\n";
}

static void testShadowDestructingVictimCascadesEntireChain() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh5_victim.c", "void create() {}\n");
    harness.writeFile("/sh5_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n");

    auto victim = harness.objects.cloneObject("/sh5_victim");
    auto shadowOb = harness.objects.cloneObject("/sh5_shadow");
    assert(victim != nullptr && shadowOb != nullptr);
    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});
    assert(!victim->isDestructed());
    assert(!shadowOb->isDestructed());

    harness.objects.destructObject(victim);
    assert(victim->isDestructed());
    // Real destruct_object(): destructing the base victim of a chain
    // cascades to destruct every shadow above it too.
    assert(shadowOb->isDestructed());

    std::cout << "testShadowDestructingVictimCascadesEntireChain OK\n";
}

static void testShadowDestructingShadowSplicesItOutLeavingVictimIntact() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh6_victim.c", "void create() {}\n");
    harness.writeFile("/sh6_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n");

    auto victim = harness.objects.cloneObject("/sh6_victim");
    auto shadowOb = harness.objects.cloneObject("/sh6_shadow");
    assert(victim != nullptr && shadowOb != nullptr);
    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});

    harness.objects.destructObject(shadowOb);
    assert(shadowOb->isDestructed());
    // Real destruct_object(): destructing a shadow (not the base
    // victim) just splices it out of the chain; the victim itself is
    // untouched and no longer shows anyone shadowing it.
    assert(!victim->isDestructed());
    assert(!victim->shadowedBy().lock());

    std::cout << "testShadowDestructingShadowSplicesItOutLeavingVictimIntact OK\n";
}

static void testShadowDeniedWhenMasterHasNoValidShadowApproval() {
    // Real semantics, confirmed against this mudlib's own master.c
    // (which defines no valid_shadow() at all): attach-mode shadow() is
    // correctly, always denied by default without an approving master --
    // the real MASTER_APPROVED() gate, not a driver-side no_shadow()
    // efun (no such separate efun exists in real FluffOS).
    ObjectVarHarness noMaster;
    noMaster.writeFile("/sh7_victim.c", "void create() {}\n");
    noMaster.writeFile("/sh7_shadow.c",
        "mixed attach(object victim) { return shadow(victim, 1); }\n");
    auto victim1 = noMaster.objects.cloneObject("/sh7_victim");
    auto shadow1 = noMaster.objects.cloneObject("/sh7_shadow");
    assert(victim1 != nullptr && shadow1 != nullptr);
    amlp::Value r1 = noMaster.vm.callFunction(shadow1, "attach", {amlp::Value(victim1)});
    assert(std::holds_alternative<std::monostate>(r1.data));
    assert(!victim1->shadowedBy().lock());

    ObjectVarHarness rejecting;
    rejecting.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 0; }\n");
    assert(rejecting.objects.loadMasterObject());
    rejecting.writeFile("/sh7b_victim.c", "void create() {}\n");
    rejecting.writeFile("/sh7b_shadow.c",
        "mixed attach(object victim) { return shadow(victim, 1); }\n");
    auto victim2 = rejecting.objects.cloneObject("/sh7b_victim");
    auto shadow2 = rejecting.objects.cloneObject("/sh7b_shadow");
    assert(victim2 != nullptr && shadow2 != nullptr);
    amlp::Value r2 = rejecting.vm.callFunction(shadow2, "attach", {amlp::Value(victim2)});
    assert(std::holds_alternative<std::monostate>(r2.data));
    assert(!victim2->shadowedBy().lock());

    std::cout << "testShadowDeniedWhenMasterHasNoValidShadowApproval OK\n";
}

static void testShadowQueryFormAndQueryShadowingReturnBothDirectionsOrZero() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh8_victim.c",
        "mixed who_shadows_me() { return shadow(this_object(), 0); }\n");
    harness.writeFile("/sh8_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n"
        "mixed who_do_i_shadow() { return query_shadowing(this_object()); }\n");
    auto victim = harness.objects.cloneObject("/sh8_victim");
    auto shadowOb = harness.objects.cloneObject("/sh8_shadow");
    assert(victim != nullptr && shadowOb != nullptr);

    // Before attaching: neither direction reports a relationship.
    amlp::Value beforeVictim = harness.vm.callFunction(victim, "who_shadows_me", {});
    assert(std::holds_alternative<std::monostate>(beforeVictim.data));
    amlp::Value beforeShadow = harness.vm.callFunction(shadowOb, "who_do_i_shadow", {});
    assert(std::holds_alternative<std::monostate>(beforeShadow.data));

    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});

    // shadow(victim, 0): who is currently shadowing victim?
    amlp::Value afterVictim = harness.vm.callFunction(victim, "who_shadows_me", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(afterVictim.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(afterVictim.data) == shadowOb);

    // query_shadowing(shadowOb): who does shadowOb itself shadow?
    amlp::Value afterShadow = harness.vm.callFunction(shadowOb, "who_do_i_shadow", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(afterShadow.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(afterShadow.data) == victim);

    std::cout << "testShadowQueryFormAndQueryShadowingReturnBothDirectionsOrZero OK\n";
}

static void testShadowRejectsSelfShadowAlreadyShadowingAndAlreadyShadowed() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sh9_a.c",
        "mixed shadow_self() { return shadow(this_object(), 1); }\n");
    auto selfOb = harness.objects.cloneObject("/sh9_a");
    assert(selfOb != nullptr);
    bool threwSelf = false;
    try {
        harness.vm.callFunction(selfOb, "shadow_self", {});
    } catch (const amlp::LpcRuntimeError&) {
        threwSelf = true;
    }
    assert(threwSelf);

    harness.writeFile("/sh9_v1.c", "void create() {}\n");
    harness.writeFile("/sh9_v2.c", "void create() {}\n");
    harness.writeFile("/sh9_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n");
    auto v1 = harness.objects.cloneObject("/sh9_v1");
    auto v2 = harness.objects.cloneObject("/sh9_v2");
    auto sh = harness.objects.cloneObject("/sh9_shadow");
    assert(v1 != nullptr && v2 != nullptr && sh != nullptr);
    harness.vm.callFunction(sh, "attach", {amlp::Value(v1)});

    // sh is already shadowing v1 -- attaching to v2 as well must throw.
    bool threwAlreadyShadowing = false;
    try {
        harness.vm.callFunction(sh, "attach", {amlp::Value(v2)});
    } catch (const amlp::LpcRuntimeError&) {
        threwAlreadyShadowing = true;
    }
    assert(threwAlreadyShadowing);

    // v1 is already shadowed (by sh) -- v1 itself then trying to shadow
    // a third object must throw. v1's own file needs an attach()
    // function to drive this from v1 as current_object, so a fresh
    // "victim that can also attach" object stands in for it here rather
    // than reusing v1 (whose file never declared one).
    harness.writeFile("/sh9_v1b.c",
        "void create() {}\n"
        "mixed attach(object victim) { return shadow(victim, 1); }\n");
    harness.writeFile("/sh9_v3.c", "void create() {}\n");
    auto v1b = harness.objects.cloneObject("/sh9_v1b");
    auto v3 = harness.objects.cloneObject("/sh9_v3");
    auto shForV1b = harness.objects.cloneObject("/sh9_shadow");
    assert(v1b != nullptr && v3 != nullptr && shForV1b != nullptr);
    harness.vm.callFunction(shForV1b, "attach", {amlp::Value(v1b)});
    bool threwAlreadyShadowed = false;
    try {
        harness.vm.callFunction(v1b, "attach", {amlp::Value(v3)});
    } catch (const amlp::LpcRuntimeError&) {
        threwAlreadyShadowed = true;
    }
    assert(threwAlreadyShadowed);

    std::cout << "testShadowRejectsSelfShadowAlreadyShadowingAndAlreadyShadowed OK\n";
}

// --- snoop() / query_snoop() / query_snooping() (Phase 0.13, snoop family) -
// Confirmed directly against fluffos-2.9-ds2.08's own f_snoop()/
// new_set_snoop()/query_snoop()/query_snooping() (comm.c) before writing
// any of this -- see EfunTable.cpp's own registration comment for the
// full real-semantics writeup, including the one place these tests
// deliberately deviate from this row's own original task description:
// there is no master()->valid_snoop() apply anywhere in real FluffOS
// (confirmed exhaustively against applies.h -- APPLY_VALID_SHADOW exists,
// there is no APPLY_VALID_SNOOP at all), so "denial without master
// approval" is covered here by the two real denial paths that do exist --
// a non-interactive victim (a hard throw) and the anti-loop walk (a
// silent 0) -- rather than a fabricated gate.

static void testSnoopStartLinksBothDirectionsAndQueryReflectsThem() {
    ObjectVarHarness harness;
    harness.writeFile("/sn1_victim.c",
        "void create() {}\n"
        "mixed who_snoops_me() { return query_snoop(this_object()); }\n");
    harness.writeFile("/sn1_snooper.c",
        "mixed start(object victim) { return snoop(this_object(), victim); }\n"
        "mixed who_am_i_snooping() { return query_snooping(this_object()); }\n");
    auto victim = harness.objects.cloneObject("/sn1_victim");
    auto snooper = harness.objects.cloneObject("/sn1_snooper");
    assert(victim != nullptr && snooper != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(victim);

    amlp::Value started = harness.vm.callFunction(snooper, "start", {amlp::Value(victim)});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(started.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(started.data) == victim);

    amlp::Value who = harness.vm.callFunction(victim, "who_snoops_me", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(who.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(who.data) == snooper);

    amlp::Value whoIAmSnooping = harness.vm.callFunction(snooper, "who_am_i_snooping", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(whoIAmSnooping.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(whoIAmSnooping.data) == victim);

    ::close(fds[1]);
    std::cout << "testSnoopStartLinksBothDirectionsAndQueryReflectsThem OK\n";
}

static void testSnoopOutputDuplicationCallsReceiveSnoopOnSnooperWithMatchingText() {
    ObjectVarHarness harness;
    harness.writeFile("/sn2_victim.c",
        "void create() {}\n"
        "void speak(string s) { write(s); }\n");
    harness.writeFile("/sn2_snooper.c",
        "string got = \"\";\n"
        "mixed start(object victim) { return snoop(this_object(), victim); }\n"
        "void receive_snoop(string s) { got += s; }\n"
        "string get_got() { return got; }\n");
    auto victim = harness.objects.cloneObject("/sn2_victim");
    auto snooper = harness.objects.cloneObject("/sn2_snooper");
    assert(victim != nullptr && snooper != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(victim);

    harness.vm.callFunction(snooper, "start", {amlp::Value(victim)});

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(victim, "speak", {amlp::Value(std::string("hello there\n"))});
    amlp::OutputContext::set(nullptr);

    // The victim's own connection still receives the text normally --
    // snoop duplicates output, it never diverts it.
    char buf[64];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    assert(std::string(buf, static_cast<size_t>(n)) == "hello there\n");

    amlp::Value got = harness.vm.callFunction(snooper, "get_got", {});
    assert(std::holds_alternative<std::string>(got.data));
    assert(std::get<std::string>(got.data) == "hello there\n");

    ::close(fds[1]);
    std::cout << "testSnoopOutputDuplicationCallsReceiveSnoopOnSnooperWithMatchingText OK\n";
}

static void testSnoopDeniesNotInteractiveThrowsAndLoopReturnsFalsy() {
    ObjectVarHarness harness;
    harness.writeFile("/sn3_plain.c", "void create() {}\n");
    harness.writeFile("/sn3_snooper.c",
        "mixed start(object victim) { return snoop(this_object(), victim); }\n");
    auto plain = harness.objects.cloneObject("/sn3_plain");
    auto snooper = harness.objects.cloneObject("/sn3_snooper");
    assert(plain != nullptr && snooper != nullptr);

    // Real "if (!victim->interactive) error(...)" -- a hard, catchable
    // throw, not a silent 0. plain was never attached to any Connection.
    bool threw = false;
    try {
        harness.vm.callFunction(snooper, "start", {amlp::Value(plain)});
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    // Self-snoop: interactive, so the not-interactive check passes, but
    // the anti-loop walk's very first step (tmp starts at by) already
    // equals victim -- denied silently (0), never a throw.
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(snooper);
    amlp::Value selfResult = harness.vm.callFunction(snooper, "start", {amlp::Value(snooper)});
    assert(std::holds_alternative<std::monostate>(selfResult.data));

    ::close(fds[1]);
    std::cout << "testSnoopDeniesNotInteractiveThrowsAndLoopReturnsFalsy OK\n";
}

static void testSnoopChainCycleDeniedByAntiLoopWalk() {
    ObjectVarHarness harness;
    harness.writeFile("/sn4_a.c",
        "mixed start(object victim) { return snoop(this_object(), victim); }\n");
    harness.writeFile("/sn4_b.c",
        "mixed start(object victim) { return snoop(this_object(), victim); }\n"
        "mixed who_snoops_me() { return query_snoop(this_object()); }\n");
    auto a = harness.objects.cloneObject("/sn4_a");
    auto b = harness.objects.cloneObject("/sn4_b");
    assert(a != nullptr && b != nullptr);

    int fdsA[2];
    int fdsB[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsA) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsB) == 0);
    amlp::Connection connA(fdsA[0]);
    amlp::Connection connB(fdsB[0]);
    connA.attach(a);
    connB.attach(b);

    // A snoops B first.
    amlp::Value r1 = harness.vm.callFunction(a, "start", {amlp::Value(b)});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(r1.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(r1.data) == b);

    // B now tries to snoop A -- would create a 2-cycle (A watching B, B
    // watching A back). The anti-loop walk starting at B finds A one hop
    // up (B->snoopedBy() == A) and denies.
    amlp::Value r2 = harness.vm.callFunction(b, "start", {amlp::Value(a)});
    assert(std::holds_alternative<std::monostate>(r2.data));

    // The original, legitimate snoop survives the denied attempt intact.
    amlp::Value stillA = harness.vm.callFunction(b, "who_snoops_me", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(stillA.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(stillA.data) == a);

    ::close(fdsA[1]);
    ::close(fdsB[1]);
    std::cout << "testSnoopChainCycleDeniedByAntiLoopWalk OK\n";
}

static void testSnoopStopFormUnlinksAndReturnsByItself() {
    ObjectVarHarness harness;
    harness.writeFile("/sn5_victim.c", "void create() {}\n");
    harness.writeFile("/sn5_snooper.c",
        "mixed start(object victim) { return snoop(this_object(), victim); }\n"
        "mixed stop() { return snoop(this_object()); }\n"
        "mixed who_am_i_snooping() { return query_snooping(this_object()); }\n");
    auto victim = harness.objects.cloneObject("/sn5_victim");
    auto snooper = harness.objects.cloneObject("/sn5_snooper");
    assert(victim != nullptr && snooper != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(victim);

    harness.vm.callFunction(snooper, "start", {amlp::Value(victim)});
    amlp::Value stopped = harness.vm.callFunction(snooper, "stop", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(stopped.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(stopped.data) == snooper);

    amlp::Value afterStop = harness.vm.callFunction(snooper, "who_am_i_snooping", {});
    assert(std::holds_alternative<std::monostate>(afterStop.data));

    ::close(fds[1]);
    std::cout << "testSnoopStopFormUnlinksAndReturnsByItself OK\n";
}

static void testSnoopVictimDisconnectClearsBothSidesOfTheRelationship() {
    ObjectVarHarness harness;
    harness.writeFile("/sn6_victim.c", "void create() {}\n");
    harness.writeFile("/sn6_snooper.c",
        "mixed start(object victim) { return snoop(this_object(), victim); }\n"
        "mixed who_am_i_snooping() { return query_snooping(this_object()); }\n");
    auto victim = harness.objects.cloneObject("/sn6_victim");
    auto snooper = harness.objects.cloneObject("/sn6_snooper");
    assert(victim != nullptr && snooper != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(victim);

    harness.vm.callFunction(snooper, "start", {amlp::Value(victim)});
    assert(victim->snoopedBy().lock() == snooper);

    // Real remove_interactive(): closing the victim's connection --
    // whether a genuine net death or a destruct()-driven close -- always
    // unlinks whoever was snooping it, on both sides.
    conn.close();

    assert(!victim->snoopedBy().lock());
    amlp::Value afterClose = harness.vm.callFunction(snooper, "who_am_i_snooping", {});
    assert(std::holds_alternative<std::monostate>(afterClose.data));

    ::close(fds[1]);
    std::cout << "testSnoopVictimDisconnectClearsBothSidesOfTheRelationship OK\n";
}

static void testSnoopSnooperDestructedClearsVictimsSnoopedBy() {
    ObjectVarHarness harness;
    harness.writeFile("/sn7_victim.c",
        "void create() {}\n"
        "mixed who_snoops_me() { return query_snoop(this_object()); }\n");
    harness.writeFile("/sn7_snooper.c",
        "mixed start(object victim) { return snoop(this_object(), victim); }\n");
    auto victim = harness.objects.cloneObject("/sn7_victim");
    auto snooper = harness.objects.cloneObject("/sn7_snooper");
    assert(victim != nullptr && snooper != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(victim);

    harness.vm.callFunction(snooper, "start", {amlp::Value(victim)});
    amlp::Value before = harness.vm.callFunction(victim, "who_snoops_me", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(before.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(before.data) == snooper);

    // Real destruct_object(): "if (ob->flags & O_SNOOP) { scan all_users,
    // clear snooped_by == ob }" -- destructing the snooper unlinks
    // whoever it was watching too, not just the snooper's own state.
    harness.vm.destructObject(snooper);

    amlp::Value after = harness.vm.callFunction(victim, "who_snoops_me", {});
    assert(std::holds_alternative<std::monostate>(after.data));

    ::close(fds[1]);
    std::cout << "testSnoopSnooperDestructedClearsVictimsSnoopedBy OK\n";
}

// --- telnet IAC negotiation, echo suppression, NAWS (Phase 0.8) ----------
// AF_UNIX socketpair, same convention as the rest of this file's net
// tests: fds[0] is what Connection reads from, fds[1] stands in for the
// remote client -- writing to fds[1] simulates bytes arriving from the
// client, reading fds[1] observes what the driver sent back.

namespace {
// Connection::pollLines()'s own read() loop treats EAGAIN/EWOULDBLOCK as
// "no more data right now", matching real non-blocking socket handling --
// but a plain socketpair() fd is blocking by default, so pollLines() would
// hang on its own second read() once the first drains whatever was
// written, unless the fd is put in non-blocking mode first, exactly like
// Server::onNewConnection() already does for a real accepted connection
// (setNonBlocking()) before ever constructing a Connection over it.
void makeNonBlocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::string readAvailable(int fd) {
    // A plain socketpair() fd is blocking by default -- without this, a
    // "nothing was sent back" assertion would hang forever waiting for
    // bytes that are never coming, rather than correctly observing zero.
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    std::string out;
    char buf[256];
    for (;;) {
        ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        out.append(buf, static_cast<size_t>(n));
    }
    return out;
}
}  // namespace

static void testIacSequencesAreStrippedAndNeverReachDispatchedLines() {
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);

    // IAC WILL <99 = unsupported> "hel" IAC DO <99> "lo\n"
    std::string raw;
    raw += '\xff'; raw += '\xfb'; raw += '\x63';  // IAC WILL 99
    raw += "hel";
    raw += '\xff'; raw += '\xfd'; raw += '\x63';  // IAC DO 99
    raw += "lo\n";
    ::write(fds[1], raw.data(), raw.size());

    auto lines = conn.pollLines();
    assert(lines.size() == 1);
    assert(lines[0] == "hello");
    for (unsigned char c : lines[0]) assert(c != 0xff);

    std::cout << "testIacSequencesAreStrippedAndNeverReachDispatchedLines OK\n";
}

static void testIacIacIsAnEscapedLiteral0xffDataByteNotACommand() {
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);

    std::string raw = "a";
    raw += '\xff'; raw += '\xff';  // IAC IAC -> literal 0xff
    raw += "b\n";
    ::write(fds[1], raw.data(), raw.size());

    auto lines = conn.pollLines();
    assert(lines.size() == 1);
    assert(lines[0].size() == 3);
    assert(lines[0][0] == 'a');
    assert(static_cast<unsigned char>(lines[0][1]) == 0xff);
    assert(lines[0][2] == 'b');

    std::cout << "testIacIacIsAnEscapedLiteral0xffDataByteNotACommand OK\n";
}

static void testTelnetWillEchoAndNawsAreSilentlyAcceptedOtherOptionsRefused() {
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);

    // IAC WILL ECHO, IAC WILL NAWS: both silently accepted, no reply.
    std::string raw1;
    raw1 += '\xff'; raw1 += '\xfb'; raw1 += '\x01';  // IAC WILL ECHO(1)
    raw1 += '\xff'; raw1 += '\xfb'; raw1 += '\x1f';  // IAC WILL NAWS(31)
    ::write(fds[1], raw1.data(), raw1.size());
    conn.pollLines();
    assert(readAvailable(fds[1]).empty());

    // IAC WILL <99>: unsupported, real default branch refuses with DONT.
    std::string raw2;
    raw2 += '\xff'; raw2 += '\xfb'; raw2 += '\x63';  // IAC WILL 99
    ::write(fds[1], raw2.data(), raw2.size());
    conn.pollLines();
    std::string reply = readAvailable(fds[1]);
    assert(reply.size() == 3);
    assert(static_cast<unsigned char>(reply[0]) == 0xff);
    assert(static_cast<unsigned char>(reply[1]) == 254);  // DONT
    assert(static_cast<unsigned char>(reply[2]) == 99);

    std::cout << "testTelnetWillEchoAndNawsAreSilentlyAcceptedOtherOptionsRefused OK\n";
}

static void testTelnetDoEchoIsSilentlyAcceptedOtherOptionsRefusedWithWont() {
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);

    std::string raw1;
    raw1 += '\xff'; raw1 += '\xfd'; raw1 += '\x01';  // IAC DO ECHO
    ::write(fds[1], raw1.data(), raw1.size());
    conn.pollLines();
    assert(readAvailable(fds[1]).empty());

    std::string raw2;
    raw2 += '\xff'; raw2 += '\xfd'; raw2 += '\x63';  // IAC DO 99
    ::write(fds[1], raw2.data(), raw2.size());
    conn.pollLines();
    std::string reply = readAvailable(fds[1]);
    assert(reply.size() == 3);
    assert(static_cast<unsigned char>(reply[0]) == 0xff);
    assert(static_cast<unsigned char>(reply[1]) == 252);  // WONT
    assert(static_cast<unsigned char>(reply[2]) == 99);

    std::cout << "testTelnetDoEchoIsSilentlyAcceptedOtherOptionsRefusedWithWont OK\n";
}

static void testNawsSubnegotiationUpdatesTerminalWidthAndHeight() {
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);
    assert(conn.terminalWidth() == 0 && conn.terminalHeight() == 0);

    // IAC SB NAWS <w1><w2><h1><h2> IAC SE -- 132x43, matching a real
    // client's own typical negotiated value, not a round/suspicious one.
    std::string raw;
    raw += '\xff'; raw += '\xfa'; raw += '\x1f';  // IAC SB NAWS
    raw += static_cast<char>(0);  raw += static_cast<char>(132);  // width = 132
    raw += static_cast<char>(0);  raw += static_cast<char>(43);   // height = 43
    raw += '\xff'; raw += '\xf0';  // IAC SE
    ::write(fds[1], raw.data(), raw.size());
    conn.pollLines();

    assert(conn.terminalWidth() == 132);
    assert(conn.terminalHeight() == 43);

    std::cout << "testNawsSubnegotiationUpdatesTerminalWidthAndHeight OK\n";
}

static void testNawsSubnegotiationSplitAcrossTwoReadsStillParsesCorrectly() {
    // Persistent state machine check: a real TCP stream can split any
    // telnet sequence across separate read()s. Splitting mid-subnegotiation
    // is the sharpest case (SB, IAC, and SE are all real, distinct states).
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);

    std::string part1;
    part1 += '\xff'; part1 += '\xfa'; part1 += '\x1f';  // IAC SB NAWS
    part1 += static_cast<char>(0); part1 += static_cast<char>(80);  // width = 80
    ::write(fds[1], part1.data(), part1.size());
    conn.pollLines();  // no line yet, but state must persist
    assert(conn.terminalWidth() == 0);  // subneg not finished yet

    std::string part2;
    part2 += static_cast<char>(0); part2 += static_cast<char>(24);  // height = 24
    part2 += '\xff'; part2 += '\xf0';  // IAC SE
    ::write(fds[1], part2.data(), part2.size());
    conn.pollLines();

    assert(conn.terminalWidth() == 80);
    assert(conn.terminalHeight() == 24);

    std::cout << "testNawsSubnegotiationSplitAcrossTwoReadsStillParsesCorrectly OK\n";
}

static void testQueryScreenWidthAndHeightReturnNegotiatedValues() {
    ObjectVarHarness harness;
    harness.writeFile("/qswtest.c",
        "int probe_w(object ob) { return query_screen_width(ob); }\n"
        "int probe_h(object ob) { return query_screen_height(ob); }\n");
    auto ob = harness.objects.cloneObject("/qswtest");
    assert(ob != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);
    conn.attach(ob);

    std::string raw;
    raw += '\xff'; raw += '\xfa'; raw += '\x1f';
    raw += static_cast<char>(0); raw += static_cast<char>(100);
    raw += static_cast<char>(0); raw += static_cast<char>(40);
    raw += '\xff'; raw += '\xf0';
    ::write(fds[1], raw.data(), raw.size());
    conn.pollLines();

    amlp::Value w = harness.vm.callFunction(ob, "probe_w", {amlp::Value(ob)});
    amlp::Value h = harness.vm.callFunction(ob, "probe_h", {amlp::Value(ob)});
    assert(std::get<int64_t>(w.data) == 100);
    assert(std::get<int64_t>(h.data) == 40);

    std::cout << "testQueryScreenWidthAndHeightReturnNegotiatedValues OK\n";
}

static void testInputToNoEchoFlagSendsIacWillEchoImmediately() {
    ObjectVarHarness harness;
    harness.writeFile("/noechotest.c",
        "void start() { input_to(\"get_pw\", 1); }\n"
        "void get_pw(string str) {}\n");
    auto ob = harness.objects.cloneObject("/noechotest");
    assert(ob != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);
    conn.attach(ob);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(ob, "start", {});
    amlp::OutputContext::set(nullptr);

    std::string reply = readAvailable(fds[1]);
    assert(reply.size() == 3);
    assert(static_cast<unsigned char>(reply[0]) == 0xff);
    assert(static_cast<unsigned char>(reply[1]) == 251);  // WILL
    assert(static_cast<unsigned char>(reply[2]) == 1);    // ECHO

    std::cout << "testInputToNoEchoFlagSendsIacWillEchoImmediately OK\n";
}

static void testEchoReenabledWithIacWontEchoWhenAwaitedLineArrives() {
    ObjectVarHarness harness;
    harness.writeFile("/noecho2test.c",
        "void start() { input_to(\"get_pw\", 1); }\n"
        "void get_pw(string str) {}\n");
    auto ob = harness.objects.cloneObject("/noecho2test");
    assert(ob != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);
    conn.attach(ob);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(ob, "start", {});
    amlp::OutputContext::set(nullptr);
    readAvailable(fds[1]);  // drain the WILL ECHO from registration

    ::write(fds[1], "secret\n", 7);
    auto lines = conn.pollLines();
    assert(lines.size() == 1 && lines[0] == "secret");

    std::string reply = readAvailable(fds[1]);
    assert(reply.size() == 3);
    assert(static_cast<unsigned char>(reply[0]) == 0xff);
    assert(static_cast<unsigned char>(reply[1]) == 252);  // WONT
    assert(static_cast<unsigned char>(reply[2]) == 1);    // ECHO

    std::cout << "testEchoReenabledWithIacWontEchoWhenAwaitedLineArrives OK\n";
}

static void testWindowSizeUpdateFlagSetOnNawsAndConsumedOnce() {
    // Connection-level check of the one-shot flag Server::handleConnection()
    // consumes to decide whether to fire window_size() (APPLY_WINDOW_SIZE) --
    // mirrors testNawsSubnegotiationUpdatesTerminalWidthAndHeight()'s own
    // byte sequence, just also asserting the flag side of handleSubnegotiation().
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);

    // Nothing has happened yet: no update pending.
    assert(conn.takeWindowSizeUpdate() == false);

    std::string raw;
    raw += '\xff'; raw += '\xfa'; raw += '\x1f';  // IAC SB NAWS
    raw += static_cast<char>(0);  raw += static_cast<char>(90);   // width = 90
    raw += static_cast<char>(0);  raw += static_cast<char>(30);   // height = 30
    raw += '\xff'; raw += '\xf0';  // IAC SE
    ::write(fds[1], raw.data(), raw.size());
    conn.pollLines();

    // Set exactly once by handleSubnegotiation() -- take it and confirm the
    // flag is consumed (a second take returns false), matching the same
    // "optional one-shot value" contract takePendingInputTo()/
    // takePendingNotifyFail() already use.
    assert(conn.takeWindowSizeUpdate() == true);
    assert(conn.takeWindowSizeUpdate() == false);

    std::cout << "testWindowSizeUpdateFlagSetOnNawsAndConsumedOnce OK\n";
}

static void testWindowSizeUpdateFlagNotSetByPlainDataLines() {
    // Ordinary line traffic (no telnet subnegotiation at all) must never
    // spuriously flag a window-size update -- window_size() firing is
    // strictly tied to a NAWS subnegotiation actually being parsed.
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    makeNonBlocking(fds[0]);

    ::write(fds[1], "hello\n", 6);
    auto lines = conn.pollLines();
    assert(lines.size() == 1 && lines[0] == "hello");
    assert(conn.takeWindowSizeUpdate() == false);

    std::cout << "testWindowSizeUpdateFlagNotSetByPlainDataLines OK\n";
}

// Not tested directly: the actual window_size() apply firing inside
// Server::handleConnection() ("if (conn.takeWindowSizeUpdate() && obj) ...
// vm_.callFunction(obj, \"window_size\", ...)"), for the same reason
// documented just below for the IAC DO NAWS trigger -- handleConnection()
// is private, outside this driver's established test seam
// (Server::dispatchLine()/fireNetDeadIfLinkDead() are the two methods
// pulled out static and public specifically so tests do not need a live
// accept loop). The flag Server::handleConnection() reads is fully covered
// by the two Connection-level tests just above; only the one call site in
// Server.cpp itself (mirroring fireNetDeadIfLinkDead()'s own well-tested
// shape) is unverified by a regression test.

// Not tested directly: the "IAC DO NAWS sent from Server::onNewConnection()"
// trigger itself. onNewConnection() is private, deliberately outside this
// driver's established test seam (Server::dispatchLine()/
// fireNetDeadIfLinkDead() are the two methods pulled out static and public
// specifically so tests do not need a live accept loop -- see their own
// comments in Server.hpp). The byte sequence and subnegotiation parsing it
// triggers are fully covered by the Connection-level tests above; only the
// one-line call site in Server.cpp itself is unverified by a regression
// test.

// --- terminal_colour() (Phase 0.8, net/instruct.md item 4) --------------
// Algorithm grounded against real daemon/terminal.c's no_colours() and
// std/user.c's message() (both explode(str, "%^") and substitute/strip
// per-segment) -- see EfunTable.cpp's own comment on the efun registration
// for the full citation trail; terminal_colour() itself has no reference
// C implementation anywhere in the vendored fluffos-2.9-ds2.08 tree.

static void testTerminalColourSubstitutesRecognizedTokensWithMaxColorsOn() {
    ObjectVarHarness harness;
    harness.writeFile("/tcolour1.c",
        "string probe() {\n"
        "    mapping colours = ([ \"RED\": \"\x1b[31m\", \"RESET\": \"\x1b[0m\" ]);\n"
        "    return terminal_colour(\"%^RED%^hello%^RESET%^\", colours, 1);\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/tcolour1");
    assert(ob != nullptr);

    amlp::Value r = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<std::string>(r.data) == "\x1b[31mhello\x1b[0m");

    std::cout << "testTerminalColourSubstitutesRecognizedTokensWithMaxColorsOn OK\n";
}

static void testTerminalColourStripsRecognizedTokensWithMaxColorsOff() {
    ObjectVarHarness harness;
    harness.writeFile("/tcolour2.c",
        "string probe() {\n"
        "    mapping colours = ([ \"RED\": \"\x1b[31m\", \"RESET\": \"\x1b[0m\" ]);\n"
        "    return terminal_colour(\"%^RED%^hello%^RESET%^\", colours, 0);\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/tcolour2");
    assert(ob != nullptr);

    amlp::Value r = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<std::string>(r.data) == "hello");

    std::cout << "testTerminalColourStripsRecognizedTokensWithMaxColorsOff OK\n";
}

static void testTerminalColourLeavesUnrecognizedTokensAndPlainTextAsIs() {
    ObjectVarHarness harness;
    harness.writeFile("/tcolour3.c",
        "string probe() {\n"
        "    mapping colours = ([ \"RED\": \"\x1b[31m\" ]);\n"
        "    return terminal_colour(\"plain %^BOGUS%^ text\", colours, 1);\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/tcolour3");
    assert(ob != nullptr);

    amlp::Value r = harness.vm.callFunction(ob, "probe", {});
    // "BOGUS" is not a key in colours at all -- left exactly as literal
    // text, same as real no_colours()/message() leave any unrecognized
    // segment (including ordinary plain text) completely untouched.
    assert(std::get<std::string>(r.data) == "plain BOGUS text");

    std::cout << "testTerminalColourLeavesUnrecognizedTokensAndPlainTextAsIs OK\n";
}

static void testTerminalColourWithNoMarkupReturnsStringUnchanged() {
    ObjectVarHarness harness;
    harness.writeFile("/tcolour4.c",
        "string probe() {\n"
        "    mapping colours = ([ \"RED\": \"\x1b[31m\" ]);\n"
        "    return terminal_colour(\"just plain text\", colours, 1);\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/tcolour4");
    assert(ob != nullptr);

    amlp::Value r = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<std::string>(r.data) == "just plain text");

    std::cout << "testTerminalColourWithNoMarkupReturnsStringUnchanged OK\n";
}

static void testTerminalColourMultipleRealCodesInOneString() {
    ObjectVarHarness harness;
    harness.writeFile("/tcolour5.c",
        "string probe() {\n"
        "    mapping colours = ([\n"
        "        \"BOLD\": \"\x1b[1m\", \"GREEN\": \"\x1b[32m\",\n"
        "        \"CYAN\": \"\x1b[36m\", \"RESET\": \"\x1b[0m\"\n"
        "    ]);\n"
        "    return terminal_colour(\"%^BOLD%^%^GREEN%^ok%^RESET%^ %^CYAN%^bye%^RESET%^\",\n"
        "                           colours, 1);\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/tcolour5");
    assert(ob != nullptr);

    amlp::Value r = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<std::string>(r.data) ==
           "\x1b[1m\x1b[32mok\x1b[0m \x1b[36mbye\x1b[0m");

    std::cout << "testTerminalColourMultipleRealCodesInOneString OK\n";
}

// Phase 0 row 0.12 audit: query_ip_number/query_ip_name/socket_status
// were all registered but never called by name anywhere in this suite.
// query_ip_number()/query_ip_name() read OutputContext::current()'s own
// fd via getpeername() (EfunTable.cpp's own comment), which needs a real
// AF_INET peer -- the socketpair(AF_UNIX, ...) fd pairs every other net
// test in this file uses will not produce a real IPv4 getpeername()
// result, so this one small helper spins up a genuine loopback TCP pair
// instead (synchronous/blocking is fine here, this is just test setup,
// not exercising the non-blocking accept-loop machinery itself).
static void makeLoopbackTcpPair(int& serverSide, int& clientSide) {
    int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(listenFd >= 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert(::bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    assert(::listen(listenFd, 1) == 0);
    socklen_t len = sizeof(addr);
    assert(::getsockname(listenFd, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    int port = ntohs(addr.sin_port);

    clientSide = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(clientSide >= 0);
    sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    caddr.sin_port = htons(static_cast<uint16_t>(port));
    assert(::connect(clientSide, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr)) == 0);

    serverSide = ::accept(listenFd, nullptr, nullptr);
    assert(serverSide >= 0);
    ::close(listenFd);
}

static void testQueryIpNumberAndQueryIpNameReturnLoopbackAddressForCurrentConnection() {
    ObjectVarHarness harness;
    harness.writeFile("/ip_probe.c",
        "string probe_num() { return query_ip_number(); }\n"
        "string probe_name() { return query_ip_name(); }\n");
    auto ob = harness.objects.cloneObject("/ip_probe");
    assert(ob != nullptr);

    int serverFd, clientFd;
    makeLoopbackTcpPair(serverFd, clientFd);
    amlp::Connection conn(serverFd);
    conn.attach(ob);

    amlp::OutputContext::set(&conn);
    amlp::Value numResult = harness.vm.callFunction(ob, "probe_num", {});
    // No DNS resolution in this driver (a blocking reverse lookup would
    // stall every other connection) -- query_ip_name() always falls back
    // to the same numeric IP query_ip_number() returns, per its own
    // EfunTable.cpp comment.
    amlp::Value nameResult = harness.vm.callFunction(ob, "probe_name", {});
    amlp::OutputContext::set(nullptr);

    assert(std::get<std::string>(numResult.data) == "127.0.0.1");
    assert(std::get<std::string>(nameResult.data) == "127.0.0.1");

    ::close(clientFd);
    std::cout << "testQueryIpNumberAndQueryIpNameReturnLoopbackAddressForCurrentConnection OK\n";
}

static void testQueryIpNumberReturnsZeroWithNoCurrentConnection() {
    ObjectVarHarness harness;
    harness.writeFile("/ip_probe2.c", "mixed probe() { return query_ip_number(); }\n");
    auto ob = harness.objects.cloneObject("/ip_probe2");
    assert(ob != nullptr);

    amlp::OutputContext::set(nullptr);
    amlp::Value r = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::monostate>(r.data));

    std::cout << "testQueryIpNumberReturnsZeroWithNoCurrentConnection OK\n";
}

static void testSocketStatusReturnsRealShapeArrayForKnownFdAndIncludesItInTheAllForm() {
    ObjectVarHarness harness;
    harness.writeFile("/sockstatus_probe.c",
        "int fd;\n"
        "int make() { fd = socket_create(2, \"noop\", 0); return socket_bind(fd, 0); }\n"
        "void noop(int f, string m, string a) {}\n"
        "mixed *probe_one() { return socket_status(fd); }\n"
        "mixed *probe_all() { return socket_status(); }\n"
        "int get_fd() { return fd; }\n");
    auto ob = harness.objects.cloneObject("/sockstatus_probe");
    assert(ob != nullptr);
    auto& vm = harness.vm;

    amlp::Value bindResult = vm.callFunction(ob, "make", {});
    assert(std::get<int64_t>(bindResult.data) == amlp::SocketErr::Success);
    int fd = static_cast<int>(std::get<int64_t>(vm.callFunction(ob, "get_fd", {}).data));

    // Real shape (lib/packages/sockets_spec.c's own doc comment):
    // [fd, state, mode, local addr, remote addr, owner].
    amlp::Value one = vm.callFunction(ob, "probe_one", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&one.data);
    assert(arr != nullptr && (*arr)->items.size() == 6);
    assert(std::get<int64_t>((*arr)->items[0].data) == fd);
    assert(std::get<std::string>((*arr)->items[1].data) == "BOUND");
    assert(std::get<std::string>((*arr)->items[2].data) == "DATAGRAM");

    amlp::Value all = vm.callFunction(ob, "probe_all", {});
    auto* allArr = std::get_if<std::shared_ptr<amlp::Array>>(&all.data);
    assert(allArr != nullptr);
    bool found = false;
    for (auto& entry : (*allArr)->items) {
        auto* entryArr = std::get_if<std::shared_ptr<amlp::Array>>(&entry.data);
        if (entryArr && *entryArr && !(*entryArr)->items.empty() &&
            std::holds_alternative<int64_t>((*entryArr)->items[0].data) &&
            std::get<int64_t>((*entryArr)->items[0].data) == fd) {
            found = true;
        }
    }
    assert(found);

    // Cleanup: this socket must not linger in the global SocketRegistry
    // (shared across every test in this binary) for later tests to trip
    // over.
    amlp::SocketRegistry::forceRemove(fd);
    std::cout << "testSocketStatusReturnsRealShapeArrayForKnownFdAndIncludesItInTheAllForm OK\n";
}

// --- socket_* efun family (ROADMAP row 0.10) -----------------------------
// Real signatures/state machine/error codes/callback argument conventions
// verified against fluffos-2.9-ds2.08/socket_efuns.c and socket_efuns.h --
// see EfunTable.cpp's own comment on the registration block for the full
// citation trail and net/instruct.md corrections. Server::pollSockets()
// is static and takes only a VM& (SocketRegistry is a global registry,
// same as InteractiveRegistry), so these tests drive the async
// read/write/accept/close callback machinery directly, with no live
// accept loop or listening Server instance needed -- the same test seam
// dispatchLine()/fireNetDeadIfLinkDead() already established.

// Real sockets are non-blocking and event-driven even on loopback --
// pollSockets() needs to be called repeatedly until the OS actually
// delivers the event (a loopback connect()/accept() is normally near-
// instant, but never synchronous). Bounded so a genuine regression hangs
// the test suite for at most ~1 second rather than forever.
static void pollSocketsUntil(amlp::VM& vm, const std::function<bool()>& done) {
    for (int i = 0; i < 200 && !done(); ++i) {
        amlp::Server::pollSockets(vm);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

static void testSocketCreateRejectsUnsupportedModesAndReturnsIncreasingHandles() {
    ObjectVarHarness harness;
    harness.writeFile("/socktest_modes.c",
        "int make(int mode) { return socket_create(mode, \"cb\", 0); }\n"
        "void cb(int fd, string msg) {}\n");
    auto ob = harness.objects.cloneObject("/socktest_modes");
    assert(ob != nullptr);

    // MUD (0), STREAM_BINARY (3), DATAGRAM_BINARY (4) are real modes
    // (socket_efuns.h's own enum) but unimplemented here -- see
    // LpcSocket.hpp's own SocketMode comment. Each must reject with
    // SocketErr::EModeNotSupp (-12), matching real socket_create()'s own
    // "default: return EEMODENOTSUPP;" for any mode outside its switch.
    amlp::Value mud = harness.vm.callFunction(ob, "make", {amlp::Value(static_cast<int64_t>(0))});
    amlp::Value streamBinary = harness.vm.callFunction(ob, "make", {amlp::Value(static_cast<int64_t>(3))});
    amlp::Value datagramBinary = harness.vm.callFunction(ob, "make", {amlp::Value(static_cast<int64_t>(4))});
    assert(std::get<int64_t>(mud.data) == amlp::SocketErr::EModeNotSupp);
    assert(std::get<int64_t>(streamBinary.data) == amlp::SocketErr::EModeNotSupp);
    assert(std::get<int64_t>(datagramBinary.data) == amlp::SocketErr::EModeNotSupp);

    // STREAM (1) is real and implemented -- handles are a monotonic
    // counter, never reused, per net/instruct.md's own explicit "Key
    // invariants" for this row.
    amlp::Value h1 = harness.vm.callFunction(ob, "make", {amlp::Value(static_cast<int64_t>(1))});
    amlp::Value h2 = harness.vm.callFunction(ob, "make", {amlp::Value(static_cast<int64_t>(1))});
    assert(std::get<int64_t>(h1.data) >= 0);
    assert(std::get<int64_t>(h2.data) > std::get<int64_t>(h1.data));

    std::cout << "testSocketCreateRejectsUnsupportedModesAndReturnsIncreasingHandles OK\n";
}

static void testSocketWriteOnUnknownHandleReturnsFdRangeAndErrorTextMatchesReal() {
    ObjectVarHarness harness;
    harness.writeFile("/socktest_err.c",
        "int write_to(int fd, string msg) { return socket_write(fd, msg); }\n"
        "string err(int e) { return socket_error(e); }\n");
    auto ob = harness.objects.cloneObject("/socktest_err");
    assert(ob != nullptr);

    amlp::Value r = harness.vm.callFunction(ob, "write_to",
        {amlp::Value(static_cast<int64_t>(99999)), amlp::Value(std::string("x"))});
    assert(std::get<int64_t>(r.data) == amlp::SocketErr::EFdRange);

    // Real error_strings[] (socket_err.c), same text, same "-(error+1)"
    // index formula, confirmed directly.
    amlp::Value s1 = harness.vm.callFunction(ob, "err",
        {amlp::Value(static_cast<int64_t>(amlp::SocketErr::EFdRange))});
    assert(std::get<std::string>(s1.data) == "Descriptor out of range");
    amlp::Value s2 = harness.vm.callFunction(ob, "err",
        {amlp::Value(static_cast<int64_t>(amlp::SocketErr::EModeNotSupp))});
    assert(std::get<std::string>(s2.data) == "Socket mode not supported");

    std::cout << "testSocketWriteOnUnknownHandleReturnsFdRangeAndErrorTextMatchesReal OK\n";
}

static void testSocketStreamCreateBindListenAcceptConnectWriteReadCloseRoundTrip() {
    ObjectVarHarness harness;
    harness.writeFile("/socktest_stream.c",
        "int server_fd; int accepted_fd = -1; int accept_fired;\n"
        "string server_received = \"\";\n"
        "int client_fd; int client_write_fired; string client_received = \"\";\n"
        "\n"
        "int start_server(int port) {\n"
        "    server_fd = socket_create(1, \"on_listen_readable\", 0);\n"
        "    if (server_fd < 0) return server_fd;\n"
        "    if (socket_bind(server_fd, port) != 1) return -100;\n"
        "    if (socket_listen(server_fd, \"on_listen_readable\") != 1) return -101;\n"
        "    return server_fd;\n"
        "}\n"
        "void on_listen_readable(int fd) { accept_fired = 1; }\n"
        "int do_accept() {\n"
        "    accepted_fd = socket_accept(server_fd, \"on_server_read\", \"on_server_write\");\n"
        "    return accepted_fd;\n"
        "}\n"
        "void on_server_read(int fd, string msg) { server_received = msg; }\n"
        "void on_server_write(int fd) {}\n"
        "int start_client(string addr) {\n"
        "    client_fd = socket_create(1, \"on_client_read\", 0);\n"
        "    return socket_connect(client_fd, addr, \"on_client_read\", \"on_client_write\");\n"
        "}\n"
        "void on_client_write(int fd) { client_write_fired = 1; }\n"
        "void on_client_read(int fd, string msg) { client_received = msg; }\n"
        "int send_client_msg(string msg) { return socket_write(client_fd, msg); }\n"
        "int send_server_reply(string msg) { return socket_write(accepted_fd, msg); }\n"
        "int close_client() { return socket_close(client_fd); }\n"
        "int close_server() { return socket_close(server_fd); }\n"
        "int get_accept_fired() { return accept_fired; }\n"
        "int get_client_write_fired() { return client_write_fired; }\n"
        "string get_server_received() { return server_received; }\n"
        "string get_client_received() { return client_received; }\n"
        "int get_accepted_fd() { return accepted_fd; }\n");
    auto ob = harness.objects.cloneObject("/socktest_stream");
    assert(ob != nullptr);
    auto& vm = harness.vm;

    // port 0: let the OS pick a free ephemeral port, then read the real
    // bound port straight back out of the registry -- no fixed port
    // number to collide with anything else on the test machine.
    amlp::Value serverResult = vm.callFunction(ob, "start_server", {amlp::Value(static_cast<int64_t>(0))});
    int serverFd = static_cast<int>(std::get<int64_t>(serverResult.data));
    assert(serverFd >= 0);
    auto serverSock = amlp::SocketRegistry::find(serverFd);
    assert(serverSock != nullptr);
    assert(serverSock->localPort > 0);

    std::string addr = "127.0.0.1 " + std::to_string(serverSock->localPort);
    amlp::Value connectResult = vm.callFunction(ob, "start_client", {amlp::Value(addr)});
    assert(std::get<int64_t>(connectResult.data) == amlp::SocketErr::Success);

    pollSocketsUntil(vm, [&] {
        return std::get<int64_t>(vm.callFunction(ob, "get_accept_fired", {}).data) == 1;
    });
    assert(std::get<int64_t>(vm.callFunction(ob, "get_accept_fired", {}).data) == 1);

    amlp::Value acceptResult = vm.callFunction(ob, "do_accept", {});
    int acceptedFd = static_cast<int>(std::get<int64_t>(acceptResult.data));
    assert(acceptedFd >= 0);

    // The client's own write_callback is real FluffOS's own "connect
    // complete" signal (see SocketRegistry::connect()'s own comment) --
    // socket_write() must wait for it, exactly like real LPC code does
    // (see lib/secure/std/client.c's own eventReadCallback/write-after-
    // connect pattern).
    pollSocketsUntil(vm, [&] {
        return std::get<int64_t>(vm.callFunction(ob, "get_client_write_fired", {}).data) == 1;
    });
    assert(std::get<int64_t>(vm.callFunction(ob, "get_client_write_fired", {}).data) == 1);

    amlp::Value writeResult = vm.callFunction(ob, "send_client_msg", {amlp::Value(std::string("hello server"))});
    assert(std::get<int64_t>(writeResult.data) == amlp::SocketErr::Success);

    pollSocketsUntil(vm, [&] {
        return !std::get<std::string>(vm.callFunction(ob, "get_server_received", {}).data).empty();
    });
    assert(std::get<std::string>(vm.callFunction(ob, "get_server_received", {}).data) == "hello server");

    amlp::Value replyResult = vm.callFunction(ob, "send_server_reply", {amlp::Value(std::string("hello client"))});
    assert(std::get<int64_t>(replyResult.data) == amlp::SocketErr::Success);

    pollSocketsUntil(vm, [&] {
        return !std::get<std::string>(vm.callFunction(ob, "get_client_received", {}).data).empty();
    });
    assert(std::get<std::string>(vm.callFunction(ob, "get_client_received", {}).data) == "hello client");

    // Real plain LPC-initiated socket_close(fd): succeeds (EESUCCESS),
    // and the fd is gone from the registry entirely afterward.
    amlp::Value closeClientResult = vm.callFunction(ob, "close_client", {});
    assert(std::get<int64_t>(closeClientResult.data) == amlp::SocketErr::Success);
    amlp::Value closeServerResult = vm.callFunction(ob, "close_server", {});
    assert(std::get<int64_t>(closeServerResult.data) == amlp::SocketErr::Success);

    // The accepted socket's own peer (the client) just closed -- a poll-
    // detected EOF must eventually remove it from the registry too
    // (Server::pollSockets()'s own closeSocketAndFireCallback() path).
    pollSocketsUntil(vm, [&] {
        return amlp::SocketRegistry::find(acceptedFd) == nullptr;
    });
    assert(amlp::SocketRegistry::find(acceptedFd) == nullptr);

    // A write to a now-fully-closed handle is exactly the unknown-handle
    // case: SocketErr::EFdRange.
    amlp::Value postCloseWrite = vm.callFunction(ob, "send_client_msg", {amlp::Value(std::string("x"))});
    assert(std::get<int64_t>(postCloseWrite.data) == amlp::SocketErr::EFdRange);

    std::cout << "testSocketStreamCreateBindListenAcceptConnectWriteReadCloseRoundTrip OK\n";
}

static void testSocketDatagramWriteAndReadCallbackCarriesSenderAddress() {
    ObjectVarHarness harness;
    harness.writeFile("/socktest_dgram.c",
        "int a_fd; int b_fd; string b_received = \"\"; string b_from = \"\";\n"
        "int make_a() { a_fd = socket_create(2, \"noop\", 0); return socket_bind(a_fd, 0); }\n"
        "int make_b() { b_fd = socket_create(2, \"on_b_read\", 0); return socket_bind(b_fd, 0); }\n"
        "void noop(int fd, string msg, string addr) {}\n"
        "void on_b_read(int fd, string msg, string addr) { b_received = msg; b_from = addr; }\n"
        "int send_to_b(string addr, string msg) { return socket_write(a_fd, msg, addr); }\n"
        "string get_b_received() { return b_received; }\n"
        "string get_b_from() { return b_from; }\n"
        "int get_b_fd() { return b_fd; }\n");
    auto ob = harness.objects.cloneObject("/socktest_dgram");
    assert(ob != nullptr);
    auto& vm = harness.vm;

    amlp::Value makeA = vm.callFunction(ob, "make_a", {});
    assert(std::get<int64_t>(makeA.data) == amlp::SocketErr::Success);
    amlp::Value makeB = vm.callFunction(ob, "make_b", {});
    assert(std::get<int64_t>(makeB.data) == amlp::SocketErr::Success);

    // b's actual bound ephemeral port is only known to SocketRegistry
    // (never returned to LPC by socket_bind() itself, matching real
    // socket_bind()'s own int-status-only return) -- read it straight
    // back out via b's own handle (both a_fd and b_fd are datagram
    // sockets bound to their own ephemeral ports, so this must key off
    // b's specific handle rather than scanning for "a bound datagram
    // socket", which could just as easily match a_fd).
    int bFd = static_cast<int>(std::get<int64_t>(vm.callFunction(ob, "get_b_fd", {}).data));
    auto bSock = amlp::SocketRegistry::find(bFd);
    assert(bSock != nullptr);
    int bPort = bSock->localPort;
    assert(bPort > 0);

    std::string addr = "127.0.0.1 " + std::to_string(bPort);
    amlp::Value sendResult = vm.callFunction(ob, "send_to_b",
        {amlp::Value(addr), amlp::Value(std::string("ping"))});
    assert(std::get<int64_t>(sendResult.data) == amlp::SocketErr::Success);

    pollSocketsUntil(vm, [&] {
        return !std::get<std::string>(vm.callFunction(ob, "get_b_received", {}).data).empty();
    });
    assert(std::get<std::string>(vm.callFunction(ob, "get_b_received", {}).data) == "ping");
    // Real DATAGRAM read_callback's own third argument: "host port",
    // built from recvfrom()'s own sender address -- confirmed here as a
    // loopback address string, not asserting the exact ephemeral source
    // port (which is real, but not deterministic across runs).
    std::string from = std::get<std::string>(vm.callFunction(ob, "get_b_from", {}).data);
    assert(from.rfind("127.0.0.1 ", 0) == 0);

    std::cout << "testSocketDatagramWriteAndReadCallbackCarriesSenderAddress OK\n";
}

// Phase 0 row 0.12 audit: regexp_assoc/remove_action/rm/set_eval_limit/
// map/query_once_interactive were all registered but never called by
// name anywhere in this suite.

// regexp_assoc is a real registered alias of reg_assoc (EfunTable.cpp's
// own comment: "added for the same reason: the task's own spec
// explicitly calls for it as a second name for reg_assoc"), sharing the
// exact same lambda -- reg_assoc's own algorithm already has full
// coverage (testRegAssocMatchesRealDocCommentExample and its sibling),
// so this only needs to confirm the alias name itself actually resolves
// and produces the identical result, not a second full algorithm
// re-verification.
static void testRegexpAssocAliasProducesSameResultAsRegAssoc() {
    ObjectVarHarness harness;
    harness.writeFile("/regexpassocprobe.c",
        "mixed *probe(string s, mixed *pats, mixed *toks, mixed def) { "
        "return regexp_assoc(s, pats, toks, def); }\n");
    auto ob = harness.objects.cloneObject("/regexpassocprobe");
    assert(ob != nullptr);

    auto pats = std::make_shared<amlp::Array>();
    pats->items.push_back(amlp::Value(std::string("ha")));
    auto toks = std::make_shared<amlp::Array>();
    toks->items.push_back(amlp::Value(int64_t{1}));

    // "xhax" against pattern "ha": exactly one match (position 1..3),
    // giving a clean 3-element split (before/match/after) -- "haha"
    // itself would match twice here (real reg_assoc's own genuinely
    // global scan), which is already what
    // testRegAssocMatchesRealDocCommentExample verifies in more depth.
    amlp::Value result = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("xhax")), amlp::Value(pats),
         amlp::Value(toks), amlp::Value(int64_t{0})});
    auto* outer = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(outer != nullptr && (*outer)->items.size() == 2);
    auto* texts = std::get_if<std::shared_ptr<amlp::Array>>(&(*outer)->items[0].data);
    assert(texts != nullptr && (*texts)->items.size() == 3);
    assert(std::get<std::string>((*texts)->items[0].data) == "x");
    assert(std::get<std::string>((*texts)->items[1].data) == "ha");
    assert(std::get<std::string>((*texts)->items[2].data) == "x");

    std::cout << "testRegexpAssocAliasProducesSameResultAsRegAssoc OK\n";
}

// remove_action(act, verb): removes an action previously registered by
// the calling object on the resolved command giver's own table (real
// add_action.c's own remove_action()). Must run from the same "current
// object that originally registered it, with the same command giver
// resolution" context add_action itself needs (EfunTable.cpp's own
// resolveCommandGiver() comment: VM::commandGiver() is set explicitly
// during move_object()'s init()-calling sequence and during
// dispatchCommand()'s own handler calls) -- so this test removes the
// registration from inside a second dispatched command, not via a bare
// vm.callFunction(), the same way add_action's own registration only
// works from inside init().
static void testRemoveActionRemovesPreviouslyAddedActionAndReturnsZeroWhenNothingToRemove() {
    ObjectVarHarness harness;
    harness.writeFile("/ra_room.c",
        "void init() {\n"
        "    add_action(\"cmd_look\", \"look\");\n"
        "    add_action(\"cmd_forget\", \"forget\");\n"
        "}\n"
        "int cmd_look(string arg) { return 1; }\n"
        "int cmd_forget(string arg) { return remove_action(\"cmd_look\", \"look\"); }\n");
    harness.writeFile("/ra_mover.c", "void go(object dest) { enable_commands(); move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/ra_room");
    auto mover = harness.objects.cloneObject("/ra_mover");
    assert(room != nullptr && mover != nullptr);
    harness.vm.callFunction(mover, "go", {amlp::Value(room)});

    assert(harness.vm.dispatchCommand(mover, "look") == true);

    // cmd_forget's own return value IS remove_action()'s return value
    // (1 == removed) -- a truthy handler return is exactly what makes
    // dispatchCommand() itself report "claimed" here.
    assert(harness.vm.dispatchCommand(mover, "forget") == true);

    // The removed action no longer matches anything.
    assert(harness.vm.dispatchCommand(mover, "look") == false);

    // A second remove_action() call for the same (act, verb) finds
    // nothing left to remove -- cmd_forget returns the real 0, which
    // is itself falsy, so this second "forget" dispatch is correctly
    // reported as not claimed (no other handler answers "forget" either).
    assert(harness.vm.dispatchCommand(mover, "forget") == false);

    std::cout << "testRemoveActionRemovesPreviouslyAddedActionAndReturnsZeroWhenNothingToRemove OK\n";
}

// int rm(string path) -- 1 on success, 0 on failure (missing file),
// mirroring testRenameFileAndVerifyViaReadFile's/
// testRmdirRemovesEmptyDirectoryAndFailsOnNonEmpty's own established
// style for this driver's file efuns.
static void testRmDeletesFileAndReturnsZeroForMissingPath() {
    ObjectVarHarness harness;
    harness.writeFile("/rm_target.txt", "delete me\n");
    harness.writeFile("/rm_probe.c",
        "int do_rm(string path) { return rm(path); }\n"
        "mixed read_it() { return read_file(\"/rm_target.txt\"); }\n");
    auto ob = harness.objects.cloneObject("/rm_probe");
    assert(ob != nullptr);

    amlp::Value before = harness.vm.callFunction(ob, "read_it", {});
    assert(std::holds_alternative<std::string>(before.data));

    amlp::Value rmResult = harness.vm.callFunction(ob, "do_rm",
        {amlp::Value(std::string("/rm_target.txt"))});
    assert(std::get<int64_t>(rmResult.data) == 1);

    // Real read_file() on a now-missing path returns falsy (0), matching
    // testReadFileReturnsFileContentAndFalsyForMissingFile's own
    // established expectation.
    amlp::Value after = harness.vm.callFunction(ob, "read_it", {});
    assert(std::holds_alternative<int64_t>(after.data) && std::get<int64_t>(after.data) == 0);

    amlp::Value rmMissing = harness.vm.callFunction(ob, "do_rm",
        {amlp::Value(std::string("/rm_target.txt"))});
    assert(std::get<int64_t>(rmMissing.data) == 0);

    std::cout << "testRmDeletesFileAndReturnsZeroForMissingPath OK\n";
}

// VM::setMaxEvalCost(int): the raw ceiling-overwrite primitive (real
// f_set_eval_limit()'s own "default: max_cost = sp->u.number;" switch
// branch) -- verified here directly rather than trusted, by actually
// tripping a very low limit against an infinite loop and confirming
// EvalCostError fires, then raising the ceiling back to a generous
// explicit value (there is no "-1 restores the default" sentinel in real
// FluffOS at all, confirmed directly against efuns_main.c's own switch --
// see VM.hpp's own corrected comment on maxEvalCost_) so the same kind of
// short loop completes normally again.
static void testSetEvalLimitActuallyChangesTheEnforcedCeiling() {
    ObjectVarHarness harness;
    harness.writeFile("/evallimit_probe.c",
        "int spin() { while(1) {} return 1; }\n"
        "int short_loop() { int i; for (i = 0; i < 5; i++) {} return i; }\n");
    auto ob = harness.objects.cloneObject("/evallimit_probe");
    assert(ob != nullptr);

    harness.vm.resetEvalCost();
    harness.vm.setMaxEvalCost(50);
    bool threw = false;
    try {
        harness.vm.callFunction(ob, "spin", {});
    } catch (const amlp::EvalCostError&) {
        threw = true;
    }
    assert(threw);

    // Raised back to a generous explicit ceiling -- high enough that this
    // short, genuinely-terminating loop runs to completion without
    // tripping it. A mudlib restoring an earlier ceiling has to remember
    // and re-set the actual value the same way, real set_eval_limit() has
    // no built-in "restore" of its own.
    harness.vm.setMaxEvalCost(1000000);
    harness.vm.resetEvalCost();
    amlp::Value shortResult = harness.vm.callFunction(ob, "short_loop", {});
    assert(std::get<int64_t>(shortResult.data) == 5);

    std::cout << "testSetEvalLimitActuallyChangesTheEnforcedCeiling OK\n";
}

// map is a real registered alias of map_array (func_spec.c: "mixed
// *map_array map(...)"), sharing the exact same lambda -- map_array's
// own two call shapes already have full coverage, so this only needs to
// confirm the bare "map" name itself resolves and works, not a second
// full re-verification.
static void testMapAliasCallsMethodOnTargetForEachElementSameAsMapArray() {
    ObjectVarHarness harness;
    harness.writeFile("/map_target.c",
        "string shout(string s) { return s + \"!\"; }\n");
    harness.writeFile("/map_caller.c",
        "mixed probe(object target) { return map(({ \"a\", \"b\" }), \"shout\", target); }\n");
    auto target = harness.objects.cloneObject("/map_target");
    auto caller = harness.objects.cloneObject("/map_caller");
    assert(target != nullptr && caller != nullptr);

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(target)});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 2);
    assert(std::get<std::string>((*arr)->items[0].data) == "a!");
    assert(std::get<std::string>((*arr)->items[1].data) == "b!");

    std::cout << "testMapAliasCallsMethodOnTargetForEachElementSameAsMapArray OK\n";
}

// query_once_interactive is a real registered alias of userp (real
// O_ONCE_INTERACTIVE), sharing the exact same lambda -- userp()'s own
// sticky-after-disconnect semantics already have coverage elsewhere;
// this only confirms the alias name itself works.
static void testQueryOnceInteractiveAliasMatchesUserp() {
    ObjectVarHarness harness;
    harness.writeFile("/qoi_probe.c",
        "int check(object ob) { return query_once_interactive(ob); }\n");
    auto ob = harness.objects.cloneObject("/qoi_probe");
    assert(ob != nullptr);

    amlp::Value before = harness.vm.callFunction(ob, "check", {amlp::Value(ob)});
    assert(std::get<int64_t>(before.data) == 0);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(ob);

    amlp::Value after = harness.vm.callFunction(ob, "check", {amlp::Value(ob)});
    assert(std::get<int64_t>(after.data) == 1);

    ::close(fds[1]);
    std::cout << "testQueryOnceInteractiveAliasMatchesUserp OK\n";
}

// Phase 0 row 0.13 efun-growth batch (2026-08-20 corrected Tier 1 pass,
// see src/efun/instruct.md): objects()/livings() needed a new
// LiveObjectRegistry that did not exist before this batch.

static void testObjectsReturnsEveryLiveObjectAndOmitsDestructedOnes() {
    ObjectVarHarness harness;
    harness.writeFile("/objs_probe.c",
        "int contains(mixed *arr, object me) {\n"
        "    int i;\n"
        "    for (i = 0; i < sizeof(arr); i++) if (arr[i] == me) return 1;\n"
        "    return 0;\n"
        "}\n"
        "mixed *probe() { return objects(); }\n");
    auto probe = harness.objects.cloneObject("/objs_probe");
    assert(probe != nullptr);
    harness.writeFile("/objs_a.c", "void create() {}\n");
    auto a = harness.objects.cloneObject("/objs_a");
    assert(a != nullptr);

    amlp::Value all1 = harness.vm.callFunction(probe, "probe", {});
    amlp::Value hasA = harness.vm.callFunction(probe, "contains", {all1, amlp::Value(a)});
    assert(std::get<int64_t>(hasA.data) == 1);
    amlp::Value hasProbe = harness.vm.callFunction(probe, "contains", {all1, amlp::Value(probe)});
    assert(std::get<int64_t>(hasProbe.data) == 1);

    harness.objects.destructObject(a);
    amlp::Value all2 = harness.vm.callFunction(probe, "probe", {});
    amlp::Value hasADestructed = harness.vm.callFunction(probe, "contains", {all2, amlp::Value(a)});
    assert(std::get<int64_t>(hasADestructed.data) == 0);

    std::cout << "testObjectsReturnsEveryLiveObjectAndOmitsDestructedOnes OK\n";
}

static void testObjectsWithStringFilterExcludesFalsyResultsAndAbortsOnMissingFunction() {
    ObjectVarHarness harness;
    harness.writeFile("/objsf_probe.c",
        "int contains(mixed *arr, object me) {\n"
        "    int i;\n"
        "    for (i = 0; i < sizeof(arr); i++) if (arr[i] == me) return 1;\n"
        "    return 0;\n"
        "}\n"
        "int keep(object candidate) { return candidate != this_object(); }\n"
        "mixed *probe_filtered() { return objects(\"keep\"); }\n"
        "mixed *probe_missing() { return objects(\"totally_undefined_objects_filter\"); }\n");
    auto probe = harness.objects.cloneObject("/objsf_probe");
    assert(probe != nullptr);

    amlp::Value filtered = harness.vm.callFunction(probe, "probe_filtered", {});
    amlp::Value hasSelf = harness.vm.callFunction(probe, "contains", {filtered, amlp::Value(probe)});
    assert(std::get<int64_t>(hasSelf.data) == 0);

    // Real f_objects(): the callback itself failing to exist aborts the
    // whole call to an empty array, not just a per-candidate exclusion.
    amlp::Value missing = harness.vm.callFunction(probe, "probe_missing", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&missing.data);
    assert(arr != nullptr && (*arr)->items.empty());

    std::cout << "testObjectsWithStringFilterExcludesFalsyResultsAndAbortsOnMissingFunction OK\n";
}

static void testLivingsReturnsOnlyObjectsWithCommandsEnabled() {
    ObjectVarHarness harness;
    harness.writeFile("/liv_enabled.c", "void create() { enable_commands(); }\n");
    harness.writeFile("/liv_disabled.c", "void create() {}\n");
    auto enabled = harness.objects.cloneObject("/liv_enabled");
    auto disabled = harness.objects.cloneObject("/liv_disabled");
    assert(enabled != nullptr && disabled != nullptr);

    harness.writeFile("/liv_probe.c",
        "int contains(mixed *arr, object me) {\n"
        "    int i;\n"
        "    for (i = 0; i < sizeof(arr); i++) if (arr[i] == me) return 1;\n"
        "    return 0;\n"
        "}\n"
        "mixed *probe() { return livings(); }\n");
    auto probe = harness.objects.cloneObject("/liv_probe");
    assert(probe != nullptr);

    amlp::Value result = harness.vm.callFunction(probe, "probe", {});
    amlp::Value hasEnabled = harness.vm.callFunction(probe, "contains", {result, amlp::Value(enabled)});
    amlp::Value hasDisabled = harness.vm.callFunction(probe, "contains", {result, amlp::Value(disabled)});
    assert(std::get<int64_t>(hasEnabled.data) == 1);
    assert(std::get<int64_t>(hasDisabled.data) == 0);

    std::cout << "testLivingsReturnsOnlyObjectsWithCommandsEnabled OK\n";
}

static void testShallowAndDeepInheritListWalkARealThreeLevelChain() {
    ObjectVarHarness harness;
    harness.writeFile("/gtest_base.c", "void create() {}\n");
    harness.writeFile("/gtest_mid.c", "inherit \"/gtest_base\";\nvoid create() {}\n");
    harness.writeFile("/gtest_top.c", "inherit \"/gtest_mid\";\nvoid create() {}\n");
    auto top = harness.objects.cloneObject("/gtest_top");
    assert(top != nullptr);

    harness.writeFile("/gtest_probe.c",
        "mixed *probe_shallow(object ob) { return shallow_inherit_list(ob); }\n"
        "mixed *probe_alias(object ob) { return inherit_list(ob); }\n"
        "mixed *probe_deep(object ob) { return deep_inherit_list(ob); }\n");
    auto probe = harness.objects.cloneObject("/gtest_probe");
    assert(probe != nullptr);

    amlp::Value shallow = harness.vm.callFunction(probe, "probe_shallow", {amlp::Value(top)});
    auto* shallowArr = std::get_if<std::shared_ptr<amlp::Array>>(&shallow.data);
    assert(shallowArr != nullptr && (*shallowArr)->items.size() == 1);
    assert(std::get<std::string>((*shallowArr)->items[0].data) == "gtest_mid.c");

    // Real inherit_list is the exact same efun as shallow_inherit_list
    // (efun_defs.c's own F_ALIAS_FLAG), not a second implementation.
    amlp::Value alias = harness.vm.callFunction(probe, "probe_alias", {amlp::Value(top)});
    auto* aliasArr = std::get_if<std::shared_ptr<amlp::Array>>(&alias.data);
    assert(aliasArr != nullptr && (*aliasArr)->items.size() == 1);
    assert(std::get<std::string>((*aliasArr)->items[0].data) == "gtest_mid.c");

    amlp::Value deep = harness.vm.callFunction(probe, "probe_deep", {amlp::Value(top)});
    auto* deepArr = std::get_if<std::shared_ptr<amlp::Array>>(&deep.data);
    assert(deepArr != nullptr && (*deepArr)->items.size() == 2);
    assert(std::get<std::string>((*deepArr)->items[0].data) == "gtest_mid.c");
    assert(std::get<std::string>((*deepArr)->items[1].data) == "gtest_base.c");

    std::cout << "testShallowAndDeepInheritListWalkARealThreeLevelChain OK\n";
}

static void testClonepTrueForCloneFalseForBlueprintAndNonObject() {
    ObjectVarHarness harness;
    harness.writeFile("/cp_target.c", "void create() {}\n");
    auto blueprint = harness.objects.loadObject("/cp_target");
    assert(blueprint != nullptr);
    auto clone = harness.objects.cloneObject("/cp_target");
    assert(clone != nullptr);

    harness.writeFile("/cp_probe.c", "int probe(mixed ob) { return clonep(ob); }\n");
    auto probe = harness.objects.cloneObject("/cp_probe");
    assert(probe != nullptr);

    amlp::Value cloneResult = harness.vm.callFunction(probe, "probe", {amlp::Value(clone)});
    assert(std::get<int64_t>(cloneResult.data) == 1);
    amlp::Value blueprintResult = harness.vm.callFunction(probe, "probe", {amlp::Value(blueprint)});
    assert(std::get<int64_t>(blueprintResult.data) == 0);
    amlp::Value nonObjectResult = harness.vm.callFunction(probe, "probe", {amlp::Value(static_cast<int64_t>(5))});
    assert(std::get<int64_t>(nonObjectResult.data) == 0);

    std::cout << "testClonepTrueForCloneFalseForBlueprintAndNonObject OK\n";
}

static void testVirtualpTrueOnlyForACompileObjectResultAndDefaultsToThisObject() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "object compile_object(string str) { return clone_object(\"/vp_target\"); }\n");
    harness.writeFile("/vp_target.c",
        "void create() {}\n"
        "int probe_self() { return virtualp(); }\n"
        "int probe_arg(object ob) { return virtualp(ob); }\n");
    bool masterLoaded = harness.objects.loadMasterObject();
    assert(masterLoaded);

    auto virtualOb = harness.objects.loadObject("/secure/save/users/t/vptestchar");
    assert(virtualOb != nullptr);
    auto realClone = harness.objects.cloneObject("/vp_target");
    assert(realClone != nullptr);

    amlp::Value selfResult = harness.vm.callFunction(virtualOb, "probe_self", {});
    assert(std::get<int64_t>(selfResult.data) == 1);
    amlp::Value notVirtual = harness.vm.callFunction(realClone, "probe_arg", {amlp::Value(realClone)});
    assert(std::get<int64_t>(notVirtual.data) == 0);

    std::cout << "testVirtualpTrueOnlyForACompileObjectResultAndDefaultsToThisObject OK\n";
}

static void testCallStackMode1ReturnsObjectsCurrentFirstWalkingOutward() {
    ObjectVarHarness harness;
    harness.writeFile("/cs_callee.c",
        "mixed *probe() { return call_stack(1); }\n"
        "mixed *probe_filenames() { return call_stack(0); }\n");
    harness.writeFile("/cs_caller.c",
        "mixed *start(object callee) { return callee->probe(); }\n"
        "mixed *start_filenames(object callee) { return callee->probe_filenames(); }\n");
    auto callee = harness.objects.cloneObject("/cs_callee");
    auto caller = harness.objects.cloneObject("/cs_caller");
    assert(callee != nullptr && caller != nullptr);

    amlp::Value result = harness.vm.callFunction(caller, "start", {amlp::Value(callee)});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    // Real call_stack(1): current frame (callee) first, walking outward
    // (caller), then the test harness's own top-level dispatch frame.
    assert(arr != nullptr && (*arr)->items.size() >= 2);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*arr)->items[0].data) == callee);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*arr)->items[1].data) == caller);

    amlp::Value fileResult = harness.vm.callFunction(caller, "start_filenames", {amlp::Value(callee)});
    auto* fileArr = std::get_if<std::shared_ptr<amlp::Array>>(&fileResult.data);
    assert(fileArr != nullptr && (*fileArr)->items.size() >= 2);
    assert(std::get<std::string>((*fileArr)->items[0].data) == callee->filename());
    assert(std::get<std::string>((*fileArr)->items[1].data) == caller->filename());

    std::cout << "testCallStackMode1ReturnsObjectsCurrentFirstWalkingOutward OK\n";
}

static void testCallStackModes2And3ThrowNotImplementedButModesOutOfRangeAlsoThrow() {
    ObjectVarHarness harness;
    harness.writeFile("/cs_err.c",
        "mixed *probe_fn() { return call_stack(2); }\n"
        "mixed *probe_origin() { return call_stack(3); }\n"
        "mixed *probe_bad() { return call_stack(9); }\n");
    auto ob = harness.objects.cloneObject("/cs_err");
    assert(ob != nullptr);

    bool threwFn = false;
    try { harness.vm.callFunction(ob, "probe_fn", {}); }
    catch (const amlp::LpcRuntimeError&) { threwFn = true; }
    assert(threwFn);

    bool threwOrigin = false;
    try { harness.vm.callFunction(ob, "probe_origin", {}); }
    catch (const amlp::LpcRuntimeError&) { threwOrigin = true; }
    assert(threwOrigin);

    bool threwBad = false;
    try { harness.vm.callFunction(ob, "probe_bad", {}); }
    catch (const amlp::LpcRuntimeError&) { threwBad = true; }
    assert(threwBad);

    std::cout << "testCallStackModes2And3ThrowNotImplementedButModesOutOfRangeAlsoThrow OK\n";
}

static void testCommandsReturnsRegisteredActionsOnTheCommandGiverItself() {
    // Mirrors testAddActionExactVerbMatchDispatchesWithRemainderAsArgumentAndDeclinesUnknownVerbs's
    // own setup exactly: real commands() reads current_object's own
    // sentence list, which for a player is built by moveObject()'s own
    // setup_new_commands() propagation, not the registering room's own
    // action table directly.
    ObjectVarHarness harness;
    harness.writeFile("/cmds_room.c",
        "void init() { add_action(\"cmd_look\", \"look\"); }\n"
        "int cmd_look(string arg) { return 1; }\n");
    harness.writeFile("/cmds_mover.c",
        "void go(object dest) { enable_commands(); move_object(dest); }\n"
        "mixed *get_commands() { return commands(); }\n");
    auto room = harness.objects.cloneObject("/cmds_room");
    auto mover = harness.objects.cloneObject("/cmds_mover");
    assert(room != nullptr && mover != nullptr);
    harness.vm.callFunction(mover, "go", {amlp::Value(room)});

    amlp::Value result = harness.vm.callFunction(mover, "get_commands", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 1);
    auto* entry = std::get_if<std::shared_ptr<amlp::Array>>(&(*arr)->items[0].data);
    assert(entry != nullptr && (*entry)->items.size() == 4);
    assert(std::get<std::string>((*entry)->items[0].data) == "look");
    assert(std::get<int64_t>((*entry)->items[1].data) == 0);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*entry)->items[2].data) == room);
    assert(std::get<std::string>((*entry)->items[3].data) == "cmd_look");

    std::cout << "testCommandsReturnsRegisteredActionsOnTheCommandGiverItself OK\n";
}

static void testSocketAddressForInteractiveObjectReturnsPeerAddrAndPortOrZero() {
    ObjectVarHarness harness;
    harness.writeFile("/sa_probe.c",
        "string probe(object ob) { return socket_address(ob, 0); }\n");
    auto ob = harness.objects.cloneObject("/sa_probe");
    assert(ob != nullptr);

    // Not (yet) interactive: real socket_address() returns 0/falsy.
    amlp::Value beforeAttach = harness.vm.callFunction(ob, "probe", {amlp::Value(ob)});
    assert(std::holds_alternative<int64_t>(beforeAttach.data) && std::get<int64_t>(beforeAttach.data) == 0);

    int serverFd, clientFd;
    makeLoopbackTcpPair(serverFd, clientFd);
    amlp::Connection conn(serverFd);
    conn.attach(ob);

    amlp::Value afterAttach = harness.vm.callFunction(ob, "probe", {amlp::Value(ob)});
    assert(std::holds_alternative<std::string>(afterAttach.data));
    const std::string& addr = std::get<std::string>(afterAttach.data);
    assert(addr.rfind("127.0.0.1 ", 0) == 0);

    ::close(clientFd);
    std::cout << "testSocketAddressForInteractiveObjectReturnsPeerAddrAndPortOrZero OK\n";
}

static void testSocketAddressForHandleDistinguishesLocalFromRemote() {
    ObjectVarHarness harness;
    harness.writeFile("/sa_sock_probe.c",
        "int fd;\n"
        "int make() { fd = socket_create(1, \"noop\", 0); return socket_bind(fd, 0, \"127.0.0.1\"); }\n"
        "void noop(int f, string m) {}\n"
        "string probe_local() { return socket_address(fd, 1); }\n"
        "string probe_remote() { return socket_address(fd, 0); }\n"
        "int probe_missing_local() { return sizeof(socket_address(99999, 1)); }\n");
    auto ob = harness.objects.cloneObject("/sa_sock_probe");
    assert(ob != nullptr);

    amlp::Value bindResult = harness.vm.callFunction(ob, "make", {});
    assert(std::get<int64_t>(bindResult.data) == amlp::SocketErr::Success);

    amlp::Value local = harness.vm.callFunction(ob, "probe_local", {});
    assert(std::holds_alternative<std::string>(local.data));
    assert(std::get<std::string>(local.data).rfind("127.0.0.1 ", 0) == 0);

    // Never connected: real remote/peer address is empty (this driver's
    // own LpcSocket::remoteAddr default), matching an unbound peer side.
    amlp::Value remote = harness.vm.callFunction(ob, "probe_remote", {});
    assert(std::holds_alternative<std::string>(remote.data));
    assert(std::get<std::string>(remote.data).rfind(" 0", 0) == 0);

    amlp::Value missing = harness.vm.callFunction(ob, "probe_missing_local", {});
    assert(std::get<int64_t>(missing.data) == 0);

    std::cout << "testSocketAddressForHandleDistinguishesLocalFromRemote OK\n";
}

// socket_release()/socket_acquire(): real socket_efuns.c's own object-
// to-object efun socket handoff. Previously filed as "Tier 3, out of
// basics scope"; re-verified real, tractable, and worth implementing
// this session (SocketRegistry.hpp/.cpp's own comments carry the full
// citation).

static void testSocketReleaseAndAcquireHandOffOwnershipAndCallbacks() {
    ObjectVarHarness harness;
    harness.writeFile("/sr_target.c",
        "int acquired;\n"
        "void on_handoff(int fd, object ob) {\n"
        "    acquired = (socket_acquire(fd, \"on_read2\", \"on_write2\", \"on_close2\") == 1);\n"
        "}\n"
        "void on_read2(int f, string m, string a) {}\n"
        "void on_write2(int f) {}\n"
        "void on_close2(int f) {}\n"
        "int did_acquire() { return acquired; }\n");
    harness.writeFile("/sr_owner.c",
        "int make() { return socket_create(2, \"on_read\", 0); }\n"
        "void on_read(int f, string m, string a) {}\n"
        "int release_to(int fd, object target) { return socket_release(fd, target, \"on_handoff\"); }\n");
    auto owner = harness.objects.cloneObject("/sr_owner");
    auto target = harness.objects.cloneObject("/sr_target");
    assert(owner != nullptr && target != nullptr);

    amlp::Value madeFd = harness.vm.callFunction(owner, "make", {});
    assert(std::get<int64_t>(madeFd.data) >= 0);
    int fd = static_cast<int>(std::get<int64_t>(madeFd.data));

    amlp::Value rc = harness.vm.callFunction(owner, "release_to",
        { amlp::Value(static_cast<int64_t>(fd)), amlp::Value(target) });
    assert(std::get<int64_t>(rc.data) == amlp::SocketErr::Success);
    assert(std::get<int64_t>(harness.vm.callFunction(target, "did_acquire", {}).data) == 1);

    // Real ownership genuinely moved -- socket_status()'s own owner slot
    // (index 5, LpcSocket::owner) now reads target, not the original
    // owner, confirming socket_acquire()'s own "lpc_socks[fd].owner_ob =
    // current_object;" actually ran, not just returned success.
    std::vector<amlp::Value> statusArgs{ amlp::Value(int64_t{fd}) };
    amlp::Value status = amlp::EfunTable::instance().call("socket_status", harness.vm, statusArgs);
    auto statusArr = std::get<std::shared_ptr<amlp::Array>>(status.data);
    assert(!statusArr->items.empty());
    auto ownerInStatus = std::get<std::shared_ptr<amlp::LpcObject>>(statusArr->items[5].data);
    assert(ownerInStatus == target);

    std::cout << "testSocketReleaseAndAcquireHandOffOwnershipAndCallbacks OK\n";
}

static void testSocketReleaseRevertsWhenNeverAcquiredAndRejectsTheWrongCaller() {
    ObjectVarHarness harness;
    // A third object the release target itself tries (and must fail) to
    // hand the fd off to, from inside its own callback -- proving
    // socket_acquire()'s own "release_ob != current_object" security
    // check is a genuine identity check, not just "was this fd released
    // at all", and exercised through a real call_other() so
    // current_object during that nested acquire attempt is genuinely
    // this third object, not the release target itself.
    harness.writeFile("/sr_wrong.c",
        "int try_acquire(int fd) { return socket_acquire(fd, \"x\", \"y\", \"z\"); }\n");
    harness.writeFile("/sr_target2.c",
        "int wrong_rc;\n"
        "void on_handoff2(int fd, object ob) {\n"
        "    object wrong = clone_object(\"/sr_wrong\");\n"
        "    wrong_rc = wrong->try_acquire(fd);\n"
        "}\n"
        "int get_wrong_rc() { return wrong_rc; }\n");
    harness.writeFile("/sr_owner3.c",
        "int make() { return socket_create(2, \"on_read\", 0); }\n"
        "void on_read(int f, string m, string a) {}\n"
        "int release_to(int fd, object target) { return socket_release(fd, target, \"on_handoff2\"); }\n");
    auto owner3 = harness.objects.cloneObject("/sr_owner3");
    auto target2 = harness.objects.cloneObject("/sr_target2");
    assert(owner3 != nullptr && target2 != nullptr);

    amlp::Value madeFd = harness.vm.callFunction(owner3, "make", {});
    assert(std::get<int64_t>(madeFd.data) >= 0);
    int fd = static_cast<int>(std::get<int64_t>(madeFd.data));

    amlp::Value rc = harness.vm.callFunction(owner3, "release_to",
        { amlp::Value(static_cast<int64_t>(fd)), amlp::Value(target2) });
    // target2's own on_handoff2 never itself calls socket_acquire() --
    // only the wrong object tries, and fails -- so the outer
    // socket_release() call reverts the release and reports
    // ESockNotRlsd, real socket_release()'s own "S_RELEASE still set
    // after the callback returns" fallback.
    assert(std::get<int64_t>(rc.data) == amlp::SocketErr::ESockNotRlsd);
    assert(std::get<int64_t>(harness.vm.callFunction(target2, "get_wrong_rc", {}).data)
           == amlp::SocketErr::ESecurity);

    // A stray socket_acquire() on this now-unreleased fd fails too --
    // there is nothing left to acquire.
    std::vector<amlp::Value> lateArgs{
        amlp::Value(int64_t{fd}), amlp::Value(std::string("x")),
        amlp::Value(std::string("y")), amlp::Value(std::string("z")) };
    amlp::Value late = amlp::EfunTable::instance().call("socket_acquire", harness.vm, lateArgs);
    assert(std::get<int64_t>(late.data) == amlp::SocketErr::ESockNotRlsd);

    std::cout << "testSocketReleaseRevertsWhenNeverAcquiredAndRejectsTheWrongCaller OK\n";
}

static void testQueryHostNameMatchesRealGethostname() {
    ObjectVarHarness harness;
    harness.writeFile("/qhn_probe.c", "string probe() { return query_host_name(); }\n");
    auto ob = harness.objects.cloneObject("/qhn_probe");
    assert(ob != nullptr);

    char buf[256];
    assert(::gethostname(buf, sizeof(buf)) == 0);
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == std::string(buf));

    std::cout << "testQueryHostNameMatchesRealGethostname OK\n";
}

static void testFlushMessagesIsANoOpThatNeverThrowsForEitherObjectKind() {
    ObjectVarHarness harness;
    harness.writeFile("/fm_probe.c",
        "void probe(object ob) { flush_messages(ob); }\n");
    auto ob = harness.objects.cloneObject("/fm_probe");
    assert(ob != nullptr);

    // Not interactive at all -- real flush_message() silently does
    // nothing for a non-interactive object.
    harness.vm.callFunction(ob, "probe", {amlp::Value(ob)});

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(ob);
    harness.vm.callFunction(ob, "probe", {amlp::Value(ob)});

    ::close(fds[1]);
    std::cout << "testFlushMessagesIsANoOpThatNeverThrowsForEitherObjectKind OK\n";
}

// ROADMAP row 0.14: "global include file" config support. Real lex.c's
// own start_new_file(): "if (*GLOBAL_INCLUDE_FILE) { ...; handle_include
// (gifile, 1); } else refill_buffer();" -- confirmed directly against
// reference/fluffos-2.9-ds2.08/lex.c before writing anything, not
// assumed. MY_QUAL mirrors real Lil's own inherit/base.c shape exactly
// (a bare macro, defined only in an auto-included header, used as an
// object-variable modifier immediately before a real type keyword) --
// the same real bug this row exists to close.

static void testGlobalIncludeFileMacroResolvesWhenConfigured() {
    ObjectVarHarness harness("global_include_file: <auto.h>\n");
    harness.writeFile("/auto.h", "#define MY_QUAL static\n");
    harness.writeFile("/gif_on.c",
        "MY_QUAL int counter = 42;\n"
        "int get_counter() { return counter; }\n");

    auto ob = harness.objects.cloneObject("/gif_on");
    assert(ob != nullptr);
    amlp::Value result = harness.vm.callFunction(ob, "get_counter", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    std::cout << "testGlobalIncludeFileMacroResolvesWhenConfigured OK\n";
}

static void testGlobalIncludeFileMacroUnresolvedWhenNotConfigured() {
    // Same MY_QUAL-dependent fixture, but the harness never sets
    // global_include_file (its own established default, matching every
    // other test in this file) -- MY_QUAL is genuinely undefined here,
    // and this compile must fail the exact same way real Lil's own
    // inherit/base.c failed against this driver before this row: an
    // unrecognized bare identifier immediately before a real type
    // keyword is parsed as the "type omitted" declaration shape (the
    // identifier itself becomes the variable's own name), leaving the
    // real type token stranded and triggering a clean parse error, not
    // a crash.
    ObjectVarHarness harness;
    harness.writeFile("/gif_off.c",
        "MY_QUAL int counter = 42;\n"
        "int get_counter() { return counter; }\n");

    auto ob = harness.objects.cloneObject("/gif_off");
    assert(ob == nullptr);

    std::cout << "testGlobalIncludeFileMacroUnresolvedWhenNotConfigured OK\n";
}

static void testGlobalIncludeFileIsANoOpForMudlibsThatNeverSetIt() {
    // The default (unset) case must not alter compilation for a mudlib
    // with no dependency on the feature at all -- true no-op, matching
    // real "if (*GLOBAL_INCLUDE_FILE) ... else refill_buffer();"'s own
    // else branch (nothing extra happens). This repo's own bundled Rifts
    // mudlib and mudlib_stub both fall in this category: neither sets
    // global_include_file, so this is exactly their own real-world path.
    ObjectVarHarness harness;
    harness.writeFile("/gif_unused.c",
        "int probe() { return 7; }\n");

    auto ob = harness.objects.cloneObject("/gif_unused");
    assert(ob != nullptr);
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<int64_t>(result.data) == 7);

    std::cout << "testGlobalIncludeFileIsANoOpForMudlibsThatNeverSetIt OK\n";
}

static void testSqrtNegativeArgThrows() {
    // sqrt(x < 0) must throw, matching real f_sqrt()'s own guard.
    ObjectVarHarness harness;
    harness.writeFile("/sqrterr.c",
        "float probe() { return sqrt(-1.0); }\n");
    auto obj = harness.objects.cloneObject("/sqrterr");
    assert(obj != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(obj, "probe", {});
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testSqrtNegativeArgThrows OK\n";
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
    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);

    // A simul_efun function's own object variables belong to the
    // simul_efun object itself, not whichever object called it.
    amlp::Value marker = harness.vm.callFunction(harness.objects.simulEfunObject(),
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

    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 1); // local wins, not the simul_efun's 100

    std::cout << "testLocalFunctionShadowsSimulEfunOfSameName OK\n";
}

// --- heredoc string literals ("@TERM ... TERM") ---------------------
// Hit loading the real secure/SimulEfun/SimulEfun.c (misc.c's
// dump_socket_status()).

static void testHeredocTokenizesToStringWithLiteralContent() {
    std::string src = "\"before\" @END\nline one\nline two\nEND;\n\"after\"";
    amlp::Lexer lexer(src);
    auto tokens = lexer.tokenize();
    // ["before"](String) ["\nline one\nline two\n"](String) [;](Symbol) ["after"](String) [End]
    assert(tokens.size() == 5);
    assert(tokens[0].type == amlp::TokenType::String && tokens[0].text == "before");
    assert(tokens[1].type == amlp::TokenType::String);
    assert(tokens[1].text == "line one\nline two\n");
    assert(tokens[2].type == amlp::TokenType::Symbol && tokens[2].text == ";");
    assert(tokens[3].type == amlp::TokenType::String && tokens[3].text == "after");

    std::cout << "testHeredocTokenizesToStringWithLiteralContent OK\n";
}

static void testHeredocVmExecutionMatchesRealShape() {
    // Mirrors secure/SimulEfun/misc.c's own "ret = @END ... END;" shape.
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer(src);
    bool threw = false;
    try {
        lexer.tokenize();
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* fe = dynamic_cast<amlp::ForeachStmt*>(body[0].get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* fe = dynamic_cast<amlp::ForeachStmt*>(body[0].get());
    assert(fe != nullptr);
    assert(fe->varName == "key");
    assert(fe->declareVar == false);
    assert(fe->hasValueVar == true);
    assert(fe->valueVarName == "val");
    assert(fe->declareValueVar == false);

    std::cout << "testForeachTwoVarParsesWithValueVar OK\n";
}

static void testForeachOverArraySumsElementsVmExecution() {
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    assert(program->functions[0]->params.size() == 3);
    assert(program->functions[0]->params[0].name == "array");

    std::cout << "testArrayUsableAsParameterNameNotReservedAsType OK\n";
}

static void testTrailingVarargsEllipsisParsesAndIsDiscarded() {
    // Mirrors secure/SimulEfun/misc.c's own "int true(mixed args...)".
    std::string src = "int true(mixed args...) { return 1; }\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    assert(program->functions.size() == 1);
    assert(program->functions[0]->params.size() == 1);
    assert(program->functions[0]->params[0].name == "args");

    std::cout << "testTrailingVarargsEllipsisParsesAndIsDiscarded OK\n";
}

static void testOpenEndedRangeIndexVmExecution() {
    // Mirrors secure/SimulEfun/misc.c's own
    // "str[strsrch(str, \"\\n\")+1..]" shape.
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* sw = dynamic_cast<amlp::SwitchStmt*>(body[0].get());
    assert(sw != nullptr);
    // [case 1, write("one"), case 2, write("two"), break, default, write("other")]
    assert(sw->body.size() == 7);
    auto* case1 = dynamic_cast<amlp::CaseLabel*>(sw->body[0].get());
    assert(case1 != nullptr && case1->value != nullptr);
    auto* defaultLabel = dynamic_cast<amlp::CaseLabel*>(sw->body[5].get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());

    bool threw = false;
    try {
        parser.parseProgram();
    } catch (const amlp::NotImplementedError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testSwitchRangeCaseLabelThrowsNotImplemented OK\n";
}

static void testSwitchMatchingCaseVmExecution() {
    // Mirrors secure/SimulEfun/alignment.c's own shape, including a
    // string subject and each case returning immediately.
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value real = runProbe(
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
    amlp::Connection conn(fds[0]);

    amlp::OutputContext::set(&conn);
    amlp::Value regResult = harness.vm.callFunction(loginObj, "start", {});
    (void)regResult;
    amlp::OutputContext::set(nullptr);

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
    amlp::Connection conn(fds[0]);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "start", {});
    amlp::OutputContext::set(nullptr);

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
    amlp::OutputContext::set(nullptr);
    amlp::Value result = harness.vm.callFunction(loginObj, "start", {});
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
    amlp::Connection conn(fds[0]);
    conn.attach(loginObj);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "start", {}); // registers input_to("get_name")
    assert(conn.hasPendingInputTo());

    // comm.c's process_user_command(): call_function_interactive() (the
    // pending input_to handler) is checked and consumed first; only when
    // it was NOT pending does process_input() run for that line.
    amlp::Server::dispatchLine(harness.vm, conn, "Bob");
    assert(!conn.hasPendingInputTo());

    amlp::Value called = harness.vm.callFunction(loginObj, "query_last_called", {});
    assert(std::get<std::string>(called.data) == "get_name");
    amlp::Value lineVal = harness.vm.callFunction(loginObj, "query_last_line", {});
    assert(std::get<std::string>(lineVal.data) == "Bob");

    // Second line: nothing pending this time, falls back to
    // process_input().
    amlp::Server::dispatchLine(harness.vm, conn, "look");
    amlp::OutputContext::set(nullptr);

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
    amlp::Connection conn(fds[0]);
    conn.attach(loginObj);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "start", {});
    amlp::Server::dispatchLine(harness.vm, conn, "bob");
    amlp::OutputContext::set(nullptr);

    // get_name() called input_to("get_password") from inside its own
    // dispatched invocation -- comm.c's own comment on why input_to's
    // fields are cleared *before* the apply(): "someone might want to
    // set up a new input_to()". Must be the new handler, not left
    // clear or stuck on the old registration.
    assert(conn.hasPendingInputTo());
    auto pending = conn.takePendingInputTo();
    assert(pending->function == "get_password");

    amlp::Value step = harness.vm.callFunction(loginObj, "query_step", {});
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
    amlp::Connection conn(fds[0]);
    conn.attach(loginObj);

    // Mirrors Server::onNewConnection()'s own logon() call: zero
    // arguments, OutputContext set to the connection for the duration.
    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(loginObj, "logon", {});
    amlp::OutputContext::set(nullptr);

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

// ---------------------------------------------------------------------
// Real link-death net_dead() apply (Server::fireNetDeadIfLinkDead())
// ---------------------------------------------------------------------
//
// Previously a connection whose peer simply vanished (EOF/read error via
// Connection::pollLines()) was torn down with no apply fired on its
// still-bound object at all -- real FluffOS's own remove_interactive()
// always calls net_dead() first for a still-alive object (comm.c: "if
// (!dested) safe_apply(APPLY_NET_DEAD, ob, 0, ORIGIN_DRIVER)"). Nothing
// in this driver exercised that gap before now: every prior connection-
// close test drove either the mid-dispatch runtime-error close path or
// the destruct() efun's own close, both of which correctly still skip
// it (see fireNetDeadIfLinkDead()'s own comment for why).

static void testFireNetDeadIfLinkDeadCallsApplyWhenPeerClosesConnection() {
    ObjectVarHarness harness;
    harness.writeFile("/nd_target.c",
        "int ran;\n"
        "void net_dead() { ran = 1; }\n");
    auto target = harness.objects.cloneObject("/nd_target");
    assert(target != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(target);

    ::close(fds[1]); // peer goes away -- real link death
    auto lines = conn.pollLines(); // read() returns 0 (EOF), sets closed_
    assert(lines.empty());
    assert(conn.closed());
    assert(conn.boundObject() == target); // close() itself hasn't run yet

    amlp::Server::fireNetDeadIfLinkDead(harness.vm, conn);

    amlp::Value ranVal = target->variables()[0];
    assert(std::holds_alternative<int64_t>(ranVal.data));
    assert(std::get<int64_t>(ranVal.data) == 1);

    std::cout << "testFireNetDeadIfLinkDeadCallsApplyWhenPeerClosesConnection OK\n";
}

static void testFireNetDeadIfLinkDeadIsNoOpWhileConnectionStillOpen() {
    ObjectVarHarness harness;
    harness.writeFile("/nd_open.c",
        "int ran;\n"
        "void net_dead() { ran = 1; }\n");
    auto target = harness.objects.cloneObject("/nd_open");
    assert(target != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(target);

    assert(!conn.closed());
    amlp::Server::fireNetDeadIfLinkDead(harness.vm, conn);

    amlp::Value ranVal = target->variables()[0];
    assert(std::get<int64_t>(ranVal.data) == 0); // never fired

    ::close(fds[1]);
    std::cout << "testFireNetDeadIfLinkDeadIsNoOpWhileConnectionStillOpen OK\n";
}

// Matches real remove_interactive()'s own "dested=1" skip: the destruct()
// efun already closes the connection itself (EfunTable.cpp), clearing
// boundObject() before fireNetDeadIfLinkDead could ever see it -- so an
// explicitly destructed interactive's own teardown must never reach
// net_dead(), same distinction the real reference driver makes.
static void testFireNetDeadIfLinkDeadSkipsAfterExplicitConnectionClose() {
    ObjectVarHarness harness;
    harness.writeFile("/nd_destructed.c",
        "int ran;\n"
        "void net_dead() { ran = 1; }\n");
    auto target = harness.objects.cloneObject("/nd_destructed");
    assert(target != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(target);

    conn.close(); // same effect as destruct()'s own conn->close() call
    assert(conn.closed());
    assert(conn.boundObject() == nullptr);

    amlp::Server::fireNetDeadIfLinkDead(harness.vm, conn);

    amlp::Value ranVal = target->variables()[0];
    assert(std::get<int64_t>(ranVal.data) == 0); // net_dead() never ran

    ::close(fds[1]);
    std::cout << "testFireNetDeadIfLinkDeadSkipsAfterExplicitConnectionClose OK\n";
}

// ---------------------------------------------------------------------
// destruct() efun: closes the destructed object's OWN connection, not
// whichever connection happens to be OutputContext::current()
// ---------------------------------------------------------------------
//
// Real, confirmed-live bug (see EfunTable.cpp's own comment on
// destruct()): destructing some other, still-connected object's player
// (an admin "boot"/kick command, one player's own code destructing a
// different player's object) used to remove it from InteractiveRegistry
// but leave its actual socket open, still bound to the now-destructed
// object -- the O_DESTRUCTED guard then made every further command on
// that connection a silent no-op rather than the connection ever
// actually closing.

static void testDestructEfunClosesTargetObjectsOwnConnectionNotCallersConnection() {
    ObjectVarHarness harness;
    harness.writeFile("/boot_actor.c",
        "void boot(object victim) { destruct(victim); }\n");
    auto actor = harness.objects.cloneObject("/boot_actor");
    assert(actor != nullptr);

    harness.writeFile("/boot_target.c", "void create() {}\n");
    auto target = harness.objects.cloneObject("/boot_target");
    assert(target != nullptr);

    int fdsActor[2];
    int fdsTarget[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsActor) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsTarget) == 0);
    amlp::Connection connActor(fdsActor[0]);
    amlp::Connection connTarget(fdsTarget[0]);
    connActor.attach(actor);
    connTarget.attach(target);

    // The admin's own connection is "current" -- not the target's, the
    // exact condition that broke this live (an admin "boot" command
    // driving the call from their own, still-open connection).
    amlp::OutputContext::set(&connActor);
    harness.vm.callFunction(actor, "boot", {amlp::Value(target)});
    amlp::OutputContext::set(nullptr);

    // Target's own connection is genuinely closed now, not just removed
    // from InteractiveRegistry.
    assert(!connTarget.isOpen());
    assert(connTarget.boundObject() == nullptr);
    assert(amlp::InteractiveRegistry::find(target) == nullptr);

    // The admin's own connection (the caller, "current" throughout) is
    // completely untouched.
    assert(connActor.isOpen());
    assert(connActor.boundObject() == actor);

    ::close(fdsActor[1]);
    ::close(fdsTarget[1]);
    std::cout << "testDestructEfunClosesTargetObjectsOwnConnectionNotCallersConnection OK\n";
}

// Regression coverage for the pre-existing, still-correct case this fix
// must not break: secure/std/login.c's own internal_remove() pattern,
// "destruct(this_object())" on the very object bound to the connection
// actually driving the call.
static void testDestructEfunStillClosesOwnConnectionWhenSelfDestructing() {
    ObjectVarHarness harness;
    harness.writeFile("/self_destruct.c",
        "void quit() { destruct(this_object()); }\n");
    auto self = harness.objects.cloneObject("/self_destruct");
    assert(self != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(self);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(self, "quit", {});
    amlp::OutputContext::set(nullptr);

    assert(!conn.isOpen());
    assert(conn.boundObject() == nullptr);
    assert(amlp::InteractiveRegistry::find(self) == nullptr);

    ::close(fds[1]);
    std::cout << "testDestructEfunStillClosesOwnConnectionWhenSelfDestructing OK\n";
}

// destruct() on a plain, never-interactive object must not crash looking
// for a connection that was never there (InteractiveRegistry::find()
// correctly returns null, matching real "if (ob->interactive)").
static void testDestructEfunOnNonInteractiveObjectDoesNotTouchAnyConnection() {
    ObjectVarHarness harness;
    harness.writeFile("/boot_actor2.c",
        "void boot(object victim) { destruct(victim); }\n");
    auto actor = harness.objects.cloneObject("/boot_actor2");
    assert(actor != nullptr);

    harness.writeFile("/plain_item.c", "void create() {}\n");
    auto item = harness.objects.cloneObject("/plain_item");
    assert(item != nullptr);

    int fdsActor[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsActor) == 0);
    amlp::Connection connActor(fdsActor[0]);
    connActor.attach(actor);

    amlp::OutputContext::set(&connActor);
    harness.vm.callFunction(actor, "boot", {amlp::Value(item)});
    amlp::OutputContext::set(nullptr);

    assert(connActor.isOpen());
    assert(connActor.boundObject() == actor);

    ::close(fdsActor[1]);
    std::cout << "testDestructEfunOnNonInteractiveObjectDoesNotTouchAnyConnection OK\n";
}

// Regression coverage for userp()/query_once_interactive() now being a
// real, sticky O_ONCE_INTERACTIVE-equivalent (LpcObject::wasEverInteractive(),
// set by Connection::attach()) instead of an alias of interactive()'s own
// "currently connected" check -- see EfunTable.cpp's own comment on the
// "userp" registration for the real mudlib call sites this was silently
// wrong for (any "userp(target) && !interactive(target)" or
// "is_player() && !interactive()" check, both extremely common real
// patterns, the moment a connected player goes link-dead).

static void testUserpAndInteractiveBothTrueWhileConnectionIsLive() {
    ObjectVarHarness harness;
    harness.writeFile("/userp_probe1.c",
        "int check_userp(object ob) { return userp(ob); }\n"
        "int check_interactive(object ob) { return interactive(ob); }\n");
    auto probe = harness.objects.cloneObject("/userp_probe1");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(probe);

    amlp::Value userpResult = harness.vm.callFunction(probe, "check_userp", {amlp::Value(probe)});
    assert(std::get<int64_t>(userpResult.data) == 1);
    amlp::Value interactiveResult = harness.vm.callFunction(probe, "check_interactive", {amlp::Value(probe)});
    assert(std::get<int64_t>(interactiveResult.data) == 1);

    ::close(fds[1]);
    std::cout << "testUserpAndInteractiveBothTrueWhileConnectionIsLive OK\n";
}

static void testUserpStaysTrueAfterDisconnectWhileInteractiveGoesFalse() {
    ObjectVarHarness harness;
    harness.writeFile("/userp_probe2.c",
        "int check_userp(object ob) { return userp(ob); }\n"
        "int check_interactive(object ob) { return interactive(ob); }\n");
    auto probe = harness.objects.cloneObject("/userp_probe2");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(probe);
    conn.close(); // real link death / quit -- probe stays in the world

    amlp::Value userpResult = harness.vm.callFunction(probe, "check_userp", {amlp::Value(probe)});
    assert(std::get<int64_t>(userpResult.data) == 1);
    amlp::Value interactiveResult = harness.vm.callFunction(probe, "check_interactive", {amlp::Value(probe)});
    assert(std::get<int64_t>(interactiveResult.data) == 0);

    ::close(fds[1]);
    std::cout << "testUserpStaysTrueAfterDisconnectWhileInteractiveGoesFalse OK\n";
}

static void testUserpReturnsFalseForObjectNeverBoundToAnyConnection() {
    ObjectVarHarness harness;
    harness.writeFile("/userp_probe3.c",
        "int check_userp(object ob) { return userp(ob); }\n");
    auto probe = harness.objects.cloneObject("/userp_probe3");
    assert(probe != nullptr);
    harness.writeFile("/plain_item2.c", "void create() {}\n");
    auto item = harness.objects.cloneObject("/plain_item2");
    assert(item != nullptr);

    amlp::Value result = harness.vm.callFunction(probe, "check_userp", {amlp::Value(item)});
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testUserpReturnsFalseForObjectNeverBoundToAnyConnection OK\n";
}

// Regression coverage for query_idle(object), a real efun for the first
// time -- see EfunTable.cpp's own comment on the "query_idle"
// registration for the real mechanism (comm.c's f_query_idle()) and the
// real mudlib call sites this closes (cmds/mortal/_who.c/_idle.c, and
// std/user.c's own heart_beat() auto-idle-logout, reachable every tick).

static void testQueryIdleIsZeroImmediatelyAfterConnectionEstablished() {
    // Real new_user() sets last_time to current_time at setup, before
    // any input has ever arrived -- Connection's constructor mirrors
    // that, so idle must read 0 the instant a connection is attached.
    ObjectVarHarness harness;
    harness.writeFile("/qi_probe1.c",
        "int check_idle(object ob) { return query_idle(ob); }\n");
    auto probe = harness.objects.cloneObject("/qi_probe1");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(probe);

    amlp::Value result = harness.vm.callFunction(probe, "check_idle", {amlp::Value(probe)});
    assert(std::get<int64_t>(result.data) == 0);

    ::close(fds[1]);
    std::cout << "testQueryIdleIsZeroImmediatelyAfterConnectionEstablished OK\n";
}

static void testQueryIdleReflectsMostRecentDispatchedLineNotJustConnectionTime() {
    // Proves query_idle() tracks Server::dispatchLine()'s own
    // touchActivity() call (get_user_command()'s real "ip->last_time =
    // current_time", re-set on every line, not just once at connect) --
    // a real, elapsed-wall-clock gap before the dispatch, then a reset
    // back to ~0 right after it.
    ObjectVarHarness harness;
    harness.writeFile("/qi_probe2.c",
        "int check_idle(object ob) { return query_idle(ob); }\n");
    auto probe = harness.objects.cloneObject("/qi_probe2");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(probe);

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    amlp::Value beforeDispatch = harness.vm.callFunction(probe, "check_idle", {amlp::Value(probe)});
    assert(std::get<int64_t>(beforeDispatch.data) >= 1);

    amlp::Server::dispatchLine(harness.vm, conn, "anything");

    amlp::Value afterDispatch = harness.vm.callFunction(probe, "check_idle", {amlp::Value(probe)});
    assert(std::get<int64_t>(afterDispatch.data) == 0);

    ::close(fds[1]);
    std::cout << "testQueryIdleReflectsMostRecentDispatchedLineNotJustConnectionTime OK\n";
}

static void testQueryIdleThrowsForObjectNeverBoundToAnyConnection() {
    // Real f_query_idle(): "if (!ob->interactive) error(...)" -- unlike
    // userp()/interactive(), which both quietly return 0 for a
    // non-interactive argument, this efun throws.
    ObjectVarHarness harness;
    harness.writeFile("/qi_probe3.c",
        "int check_idle(object ob) { return query_idle(ob); }\n");
    auto probe = harness.objects.cloneObject("/qi_probe3");
    assert(probe != nullptr);
    harness.writeFile("/plain_item3.c", "void create() {}\n");
    auto item = harness.objects.cloneObject("/plain_item3");
    assert(item != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(probe, "check_idle", {amlp::Value(item)});
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("query_idle") != std::string::npos);
    }
    assert(threw);

    std::cout << "testQueryIdleThrowsForObjectNeverBoundToAnyConnection OK\n";
}

// --- LivingNameRegistry: find_player()/find_living() -------------------
// See LivingNameRegistry.hpp's own comment for the real
// find_living_object(str, user) mechanism this mirrors: one shared
// function backing both efuns, keyed by set_living_name()'s own name,
// gated by O_ENABLE_COMMANDS always and O_ONCE_INTERACTIVE only for
// find_player(). Previously this driver approximated find_player() via
// InteractiveRegistry (currently-connected-only) and query_name(), and
// had no find_living() efun registered at all.

static void testFindPlayerFindsCurrentlyConnectedObjectByLivingName() {
    ObjectVarHarness harness;
    harness.writeFile("/lnr_player1.c",
        "void setup() { enable_commands(); set_living_name(\"alice\"); }\n"
        "object check() { return find_player(\"alice\"); }\n");
    auto probe = harness.objects.cloneObject("/lnr_player1");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(probe);

    harness.vm.callFunction(probe, "setup", {});
    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(result.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(result.data) == probe);

    ::close(fds[1]);
    std::cout << "testFindPlayerFindsCurrentlyConnectedObjectByLivingName OK\n";
}

static void testFindPlayerStillFindsObjectAfterDisconnectViaOnceInteractive() {
    // The actual bug: real find_player() gates on O_ONCE_INTERACTIVE,
    // not "currently connected" -- a link-dead-but-still-present player
    // must still be findable.
    ObjectVarHarness harness;
    harness.writeFile("/lnr_player2.c",
        "void setup() { enable_commands(); set_living_name(\"bob\"); }\n"
        "object check() { return find_player(\"bob\"); }\n");
    auto probe = harness.objects.cloneObject("/lnr_player2");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(probe);
    harness.vm.callFunction(probe, "setup", {});
    conn.close(); // real link death -- probe stays in the world

    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(result.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(result.data) == probe);

    ::close(fds[1]);
    std::cout << "testFindPlayerStillFindsObjectAfterDisconnectViaOnceInteractive OK\n";
}

static void testFindPlayerDoesNotMatchAnObjectThatWasNeverInteractive() {
    ObjectVarHarness harness;
    harness.writeFile("/lnr_npc1.c",
        "void setup() { enable_commands(); set_living_name(\"goblin\"); }\n"
        "object check() { return find_player(\"goblin\"); }\n");
    auto probe = harness.objects.cloneObject("/lnr_npc1");
    assert(probe != nullptr);

    harness.vm.callFunction(probe, "setup", {}); // never bound to any connection

    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(result.isVoid());

    std::cout << "testFindPlayerDoesNotMatchAnObjectThatWasNeverInteractive OK\n";
}

static void testFindLivingMatchesAnNpcThatWasNeverInteractive() {
    ObjectVarHarness harness;
    harness.writeFile("/lnr_npc2.c",
        "void setup() { enable_commands(); set_living_name(\"goblin2\"); }\n"
        "object check() { return find_living(\"goblin2\"); }\n");
    auto probe = harness.objects.cloneObject("/lnr_npc2");
    assert(probe != nullptr);

    harness.vm.callFunction(probe, "setup", {});

    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(result.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(result.data) == probe);

    std::cout << "testFindLivingMatchesAnNpcThatWasNeverInteractive OK\n";
}

static void testFindLivingReturnsNullWithoutEnableCommands() {
    ObjectVarHarness harness;
    harness.writeFile("/lnr_npc3.c",
        "void setup() { set_living_name(\"hermit\"); }\n" // no enable_commands()
        "object check() { return find_living(\"hermit\"); }\n");
    auto probe = harness.objects.cloneObject("/lnr_npc3");
    assert(probe != nullptr);

    harness.vm.callFunction(probe, "setup", {});

    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(result.isVoid());

    std::cout << "testFindLivingReturnsNullWithoutEnableCommands OK\n";
}

static void testFindLivingReturnsNullForUnknownName() {
    ObjectVarHarness harness;
    harness.writeFile("/lnr_probe4.c",
        "object check() { return find_living(\"nobody_named_this\"); }\n");
    auto probe = harness.objects.cloneObject("/lnr_probe4");
    assert(probe != nullptr);

    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(result.isVoid());

    std::cout << "testFindLivingReturnsNullForUnknownName OK\n";
}

static void testFindLivingDoesNotMatchADestructedObjectsFormerLivingName() {
    ObjectVarHarness harness;
    harness.writeFile("/lnr_npc4.c",
        "void setup() { enable_commands(); set_living_name(\"ghost\"); }\n");
    harness.writeFile("/lnr_finder.c",
        "object check() { return find_living(\"ghost\"); }\n");
    auto npc = harness.objects.cloneObject("/lnr_npc4");
    auto finder = harness.objects.cloneObject("/lnr_finder");
    assert(npc != nullptr && finder != nullptr);

    harness.vm.callFunction(npc, "setup", {});
    harness.vm.destructObject(npc); // npc stays alive via this local

    amlp::Value result = harness.vm.callFunction(finder, "check", {});
    assert(result.isVoid());

    std::cout << "testFindLivingDoesNotMatchADestructedObjectsFormerLivingName OK\n";
}

// Real, confirmed-live bug (see EfunTable.cpp's own comment on
// message()): this efun used to always write to whichever connection is
// "currently active" (OutputContext::current()), completely ignoring its
// own targets argument. Harmless on every path this driver had ever
// actually run before this slice (every real call site was
// "message(type, text, this_object())" where this_object() already was
// the active connection's own object) -- until call_out()/heart_beat()
// started genuinely firing with no active connection at all, and
// secure/SimulEfun/communications.c's own tell_object(ob, str) (which
// every delayed player-facing message in this mudlib goes through)
// produced nothing. This test drives two independent connections and
// confirms message() reaches the one actually named as its target, not
// whichever connection happens to be "current" (or none at all).
static void testMessageRoutesToTargetObjectsOwnConnectionNotCurrentOne() {
    ObjectVarHarness harness;
    harness.writeFile("/msg_probe.c",
        "void tell(object ob, string str) { message(\"tell\", str, ob); }\n");
    auto sender = harness.objects.cloneObject("/msg_probe");
    assert(sender != nullptr);

    harness.writeFile("/msg_target_a.c", "void create() {}\n");
    harness.writeFile("/msg_target_b.c", "void create() {}\n");
    auto targetA = harness.objects.cloneObject("/msg_target_a");
    auto targetB = harness.objects.cloneObject("/msg_target_b");
    assert(targetA != nullptr && targetB != nullptr);

    int fdsA[2];
    int fdsB[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsA) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsB) == 0);
    amlp::Connection connA(fdsA[0]);
    amlp::Connection connB(fdsB[0]);
    connA.attach(targetA);
    connB.attach(targetB);

    // Real call_out()/heart_beat() firing: no connection is "current" at
    // all (see Scheduler.cpp) -- confirmed the message must still reach
    // B by target, not by whatever OutputContext::current() happens to
    // hold (here, deliberately left null, the exact condition that broke
    // this live).
    amlp::OutputContext::set(nullptr);
    harness.vm.callFunction(sender, "tell",
        {amlp::Value(targetB), amlp::Value(std::string("hello B\n"))});

    char buf[256];
    ssize_t nB = ::recv(fdsB[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(nB > 0);
    assert(std::string(buf, static_cast<size_t>(nB)) == "hello B\n");

    ssize_t nA = ::recv(fdsA[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(nA < 0); // A's own connection received nothing at all

    // Also confirmed the other direction: A being "current" must not
    // redirect a message actually targeted at B.
    amlp::OutputContext::set(&connA);
    harness.vm.callFunction(sender, "tell",
        {amlp::Value(targetB), amlp::Value(std::string("hello B again\n"))});
    amlp::OutputContext::set(nullptr);

    ssize_t nB2 = ::recv(fdsB[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(nB2 > 0);
    assert(std::string(buf, static_cast<size_t>(nB2)) == "hello B again\n");
    ssize_t nA2 = ::recv(fdsA[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(nA2 < 0);

    ::close(fdsA[1]);
    ::close(fdsB[1]);
    std::cout << "testMessageRoutesToTargetObjectsOwnConnectionNotCurrentOne OK\n";
}

// Updated for the real Scheduler (previously exercised call_out()'s
// pre-Scheduler stub, which always returned a fixed handle of 1 no
// matter how many call_outs had already been made -- see the real
// registration/firing/removal tests further below for the bulk of this
// area's coverage now).
static void testCallOutAcceptsRealArgumentShapeAndReturnsHandle() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/callout_probe.c",
        "int probe() { return call_out(\"idle\", 180); }\n"
        "void idle() {}\n");
    auto obj = harness.objects.cloneObject("/callout_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    // First handle ever issued by a fresh Scheduler is 1 (see
    // Scheduler::newCallOutHandle()'s own comment on the monotonic
    // counter this driver uses in place of real call_out.c's ring-buffer-
    // slot-encoded handle).
    assert(std::get<int64_t>(result.data) == 1);
    std::cout << "testCallOutAcceptsRealArgumentShapeAndReturnsHandle OK\n";
}

// remove_call_out() on a name with nothing actually pending under it
// still correctly reports -1 (not found), the real "nothing to remove"
// outcome -- distinct from the old pre-Scheduler stub this test used to
// cover, which returned -1 unconditionally regardless of what was
// pending. See the removal tests further below for the "something IS
// pending" side of this efun.
static void testRemoveCallOutReturnsMinusOneWhenNothingPendingUnderThatName() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/rco_probe.c",
        "int probe() { return remove_call_out(\"idle\"); }\n"
        "void idle() {}\n");
    auto obj = harness.objects.cloneObject("/rco_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == -1);
    std::cout << "testRemoveCallOutReturnsMinusOneWhenNothingPendingUnderThatName OK\n";
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
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
    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
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
    } catch (const amlp::LpcRuntimeError& e) {
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
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "obriensmithjones");
    std::cout << "testConvertNameMudlibFunctionWorksWithNewLowerCaseAndReplaceStringEfuns OK\n";
}

// upper_case(string) -- real packages/contrib.c's own f_upper_case(),
// lower_case()'s exact mirror. See EfunTable.cpp's own comment on the
// "upper_case" registration for the real call sites this closes
// (cmds/mortal/_guild.c's guild create/join/leave/list commands,
// cmds/mortal/_setenv.c, cmds/adm/_repairchar.c, daemon/guild_d.c).

static void testUpperCaseFoldsLowercaseLettersAndLeavesEverythingElseUnchanged() {
    amlp::Value result = runProbe("return upper_case(\"Hello, World! 123\");");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "HELLO, WORLD! 123");
    std::cout << "testUpperCaseFoldsLowercaseLettersAndLeavesEverythingElseUnchanged OK\n";
}

static void testUpperCaseMatchesRealGuildTagUppercasingShape() {
    // daemon/guild_d.c's own real shape: "tag = upper_case(tag);" --
    // confirms the exact real call site's own input works, not just a
    // synthetic string.
    ObjectVarHarness harness;
    harness.writeFile("/guild_tag_probe.c",
        "string probe() {\n"
        "    string tag;\n"
        "    tag = \"thief\";\n"
        "    tag = upper_case(tag);\n"
        "    return tag;\n"
        "}\n");
    auto obj = harness.objects.cloneObject("/guild_tag_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "THIEF");
    std::cout << "testUpperCaseMatchesRealGuildTagUppercasingShape OK\n";
}

static void testUpperCaseThrowsOnNonStringArgument() {
    ObjectVarHarness harness;
    harness.writeFile("/uc_probe1.c", "mixed probe() { return upper_case(42); }\n");
    auto ob = harness.objects.cloneObject("/uc_probe1");
    assert(ob != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(ob, "probe", {});
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("upper_case") != std::string::npos);
    }
    assert(threw);

    std::cout << "testUpperCaseThrowsOnNonStringArgument OK\n";
}

// Phase 0 row 0.12 audit: capitalize/strlen/strstr were registered but
// never called by name anywhere in this suite. capitalize()'s own
// EfunTable.cpp comment: only a currently-lowercase first character is
// uppercased -- an already-uppercase or non-alphabetic first character is
// left alone, both edge cases checked here alongside the happy path.
static void testCapitalizeUppercasesOnlyALowercaseFirstCharacter() {
    ObjectVarHarness harness;
    harness.writeFile("/cap_probe.c",
        "string probe_lower() { return capitalize(\"hello\"); }\n"
        "string probe_upper() { return capitalize(\"Hello\"); }\n"
        "string probe_digit() { return capitalize(\"7up\"); }\n"
        "string probe_empty() { return capitalize(\"\"); }\n");
    auto ob = harness.objects.cloneObject("/cap_probe");
    assert(ob != nullptr);

    assert(std::get<std::string>(harness.vm.callFunction(ob, "probe_lower", {}).data) == "Hello");
    assert(std::get<std::string>(harness.vm.callFunction(ob, "probe_upper", {}).data) == "Hello");
    assert(std::get<std::string>(harness.vm.callFunction(ob, "probe_digit", {}).data) == "7up");
    assert(std::get<std::string>(harness.vm.callFunction(ob, "probe_empty", {}).data) == "");

    std::cout << "testCapitalizeUppercasesOnlyALowercaseFirstCharacter OK\n";
}

// strlen (a real alias of sizeof on a string, EfunTable.cpp's own
// "t.registerEfun(\"strlen\", sizeofImpl);") and strstr (a real alias of
// strsrch, "int strstr strsrch(...)") -- both registered efuns, neither
// name called directly anywhere else in this suite even though sizeof()
// and strsrch() themselves each have their own separate coverage.
static void testStrlenAndStrstrAliasesWorkByTheirOwnNames() {
    ObjectVarHarness harness;
    harness.writeFile("/strlen_probe.c",
        "int probe_len() { return strlen(\"hello\"); }\n"
        "int probe_find() { return strstr(\"hello world\", \"world\"); }\n"
        "int probe_find_from() { return strstr(\"aXaXa\", \"a\", 1); }\n"
        "int probe_not_found() { return strstr(\"hello\", \"xyz\"); }\n");
    auto ob = harness.objects.cloneObject("/strlen_probe");
    assert(ob != nullptr);

    assert(std::get<int64_t>(harness.vm.callFunction(ob, "probe_len", {}).data) == 5);
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "probe_find", {}).data) == 6);
    // strsrch/strstr's own "start" argument: first match at or after
    // index 1 -- the 'a' at index 0 must be skipped.
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "probe_find_from", {}).data) == 2);
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "probe_not_found", {}).data) == -1);

    std::cout << "testStrlenAndStrstrAliasesWorkByTheirOwnNames OK\n";
}

// string crypt(string str, string|int salt) -- confirmed live in
// secure/std/login.c's own confirm_password() (EfunTable.cpp's own
// comment). Happy path: a fixed >=2-char salt makes the result
// deterministic and reproducible (real crypt(3) semantics -- the salt is
// also always the output's own prefix). Edge case: an omitted/too-short
// salt still produces *some* valid hash rather than throwing (this
// driver generates a random one instead), covering the other real branch
// of the same efun.
static void testCryptWithExplicitSaltIsDeterministicAndSaltIsThePrefix() {
    ObjectVarHarness harness;
    harness.writeFile("/crypt_probe.c",
        "string probe(string pw, string salt) { return crypt(pw, salt); }\n"
        "string probe_no_salt(string pw) { return crypt(pw, 0); }\n");
    auto ob = harness.objects.cloneObject("/crypt_probe");
    assert(ob != nullptr);

    amlp::Value r1 = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("hunter2")), amlp::Value(std::string("ab"))});
    amlp::Value r2 = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("hunter2")), amlp::Value(std::string("ab"))});
    const std::string& hash1 = std::get<std::string>(r1.data);
    const std::string& hash2 = std::get<std::string>(r2.data);
    assert(hash1 == hash2);
    assert(hash1.rfind("ab", 0) == 0);

    amlp::Value r3 = harness.vm.callFunction(ob, "probe_no_salt", {amlp::Value(std::string("hunter2"))});
    assert(std::holds_alternative<std::string>(r3.data));
    assert(!std::get<std::string>(r3.data).empty());

    std::cout << "testCryptWithExplicitSaltIsDeterministicAndSaltIsThePrefix OK\n";
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
    amlp::Value result = harness.vm.callFunction(obj, "query_tag", {});
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
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* call = dynamic_cast<amlp::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr);
    assert(call->args.size() == 1);
    auto* closure = dynamic_cast<amlp::ClosureLiteralExpr*>(call->args[0].get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    auto* call = dynamic_cast<amlp::CallExpr*>(exprStmt->expr.get());
    auto* closure = dynamic_cast<amlp::ClosureLiteralExpr*>(call->args[0].get());
    assert(closure != nullptr);
    assert(closure->functionName == "file_size");
    assert(closure->boundArgs.size() == 2);
    auto* firstArg = dynamic_cast<amlp::VarRefExpr*>(closure->boundArgs[0].get());
    assert(firstArg != nullptr && firstArg->name == "p");
    auto* secondArg = dynamic_cast<amlp::StringLiteral*>(closure->boundArgs[1].get());
    assert(secondArg != nullptr && secondArg->value == "extra");

    std::cout << "testClosureLiteralParsesToClosureLiteralExprWithBoundArgs OK\n";
}

static void testClosureLiteralVmExecutionProducesClosureValueWithOwnerAndBoundArgs() {
    std::string src =
        "mixed make_closure(string p) {\n"
        "    return (: file_size, p :);\n"
        "}\n";
    auto obj = compileProgramObject(src);

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "make_closure", {amlp::Value(std::string("/some/path"))});
    auto* closurePtr = std::get_if<std::shared_ptr<amlp::Closure>>(&result.data);
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "probe", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "probe", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "probe", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "probe", {});
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

    amlp::Value closureVal = harness.vm.callFunction(owner, "make_closure", {});
    auto closurePtr = std::get<std::shared_ptr<amlp::Closure>>(closureVal.data);
    assert(closurePtr != nullptr);

    owner.reset(); // only the weak_ptr in the closure should be left

    bool threw = false;
    try {
        harness.vm.callClosure(closurePtr, {});
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("destructed") != std::string::npos);
    }
    assert(threw);

    std::cout << "testEvaluateThrowsWhenClosureOwnerDestructed OK\n";
}

// --- destructed-object guard --------------------------------------------
// Previously this driver had no O_DESTRUCTED-equivalent flag at all: a
// destructed LpcObject just kept working as an ordinary C++ object,
// reachable and callable through any shared_ptr still pointing at it
// (still sitting in a room's own inventory, since destruct() never
// unlinked it either), until the last shared_ptr happened to drop. The
// existing testEvaluateThrowsWhenClosureOwnerDestructed test above (and
// testCallOutSkipsDestructedTargetSilently further down) only ever
// exercised the *weak_ptr-expired* case (an explicit ".reset()" after
// destructObject()), not a destructed object that is still kept alive by
// another live reference -- a completely ordinary thing for destruct()
// to leave behind (an array, a mapping, another object's own variable).
// Each test below deliberately keeps a live shared_ptr past
// destructObject() to prove the new isDestructed() flag itself is what
// closes the gap, not weak_ptr expiry doing the work incidentally.

static void testDestructedObjectIsUnlinkedFromEnvironmentInventory() {
    ObjectVarHarness harness;
    harness.writeFile("/dg_room.c", "int probe() { return 0; }\n");
    harness.writeFile("/dg_item.c", "int probe() { return 0; }\n");

    auto room = harness.objects.cloneObject("/dg_room");
    auto item = harness.objects.cloneObject("/dg_item");
    assert(room != nullptr && item != nullptr);

    harness.vm.moveObject(item, room);
    assert(room->inventory().size() == 1);
    assert(item->environment().lock() == room);

    harness.vm.destructObject(item); // item stays alive via this local

    assert(room->inventory().empty());
    assert(item->environment().lock() == nullptr);

    std::cout << "testDestructedObjectIsUnlinkedFromEnvironmentInventory OK\n";
}

static void testCallOtherOnDestructedObjectIsSilentNoOp() {
    ObjectVarHarness harness;
    harness.writeFile("/dg_target.c",
        "int ran;\n"
        "int mark() { ran = 1; return 42; }\n");
    auto target = harness.objects.cloneObject("/dg_target");
    assert(target != nullptr);

    harness.vm.destructObject(target); // target stays alive via this local

    amlp::Value result = harness.vm.callFunction(target, "mark", {});
    assert(result.isVoid()); // real apply(): destructed target, no call made

    amlp::Value ranVal = target->variables()[0];
    assert(std::holds_alternative<int64_t>(ranVal.data));
    assert(std::get<int64_t>(ranVal.data) == 0); // mark() body never ran

    std::cout << "testCallOtherOnDestructedObjectIsSilentNoOp OK\n";
}

static void testCallClosureThrowsForDestructedOwnerEvenWhenStillReferenced() {
    ObjectVarHarness harness;
    harness.writeFile("/dg_owner.c",
        "mixed make_closure() { return (: file_size, \"/x\" :); }\n");
    auto owner = harness.objects.cloneObject("/dg_owner");
    assert(owner != nullptr);

    amlp::Value closureVal = harness.vm.callFunction(owner, "make_closure", {});
    auto closurePtr = std::get<std::shared_ptr<amlp::Closure>>(closureVal.data);
    assert(closurePtr != nullptr);

    harness.vm.destructObject(owner); // owner stays alive via this local

    bool threw = false;
    try {
        harness.vm.callClosure(closurePtr, {});
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("destructed") != std::string::npos);
    }
    assert(threw);

    std::cout << "testCallClosureThrowsForDestructedOwnerEvenWhenStillReferenced OK\n";
}

static void testDispatchCommandSkipsActionFromDestructedOwnerEvenWhenStillReferenced() {
    ObjectVarHarness harness;
    harness.writeFile("/dg_actor.c",
        "int ran;\n"
        "int cmd_poke(string arg) { ran = 1; return 1; }\n");
    auto actor = harness.objects.cloneObject("/dg_actor");
    assert(actor != nullptr);
    actor->setCommandsEnabled(true);
    amlp::LpcObject::ActionEntry entry;
    entry.verb = "poke";
    entry.functionName = "cmd_poke";
    entry.owner = actor;
    actor->addAction(entry);

    harness.vm.destructObject(actor); // actor stays alive via this local

    bool handled = harness.vm.dispatchCommand(actor, "poke");
    assert(!handled);

    amlp::Value ranVal = actor->variables()[0];
    assert(std::holds_alternative<int64_t>(ranVal.data));
    assert(std::get<int64_t>(ranVal.data) == 0); // cmd_poke() body never ran

    std::cout << "testDispatchCommandSkipsActionFromDestructedOwnerEvenWhenStillReferenced OK\n";
}

static void testCallOutSkipsDestructedTargetEvenWhenStillReferenced() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/dg_co_target.c",
        "int ran;\n"
        "void tick() { ran = 1; }\n");
    auto obj = harness.objects.cloneObject("/dg_co_target");
    assert(obj != nullptr);

    amlp::CallOutEntry entry;
    entry.target = obj; // weak_ptr
    entry.function = "tick";
    entry.dueAt = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    scheduler.addCallOut(std::move(entry));

    harness.vm.destructObject(obj); // obj stays alive via this local

    scheduler.tickCallOuts();

    amlp::Value ranVal = obj->variables()[0];
    assert(std::holds_alternative<int64_t>(ranVal.data));
    assert(std::get<int64_t>(ranVal.data) == 0); // tick() never actually ran

    std::cout << "testCallOutSkipsDestructedTargetEvenWhenStillReferenced OK\n";
}

// --- destructed-object read coercion (real F_LOCAL/F_GLOBAL/F_INDEX) ---
// See VM.cpp's own coerceIfDestructed() comment for the exact real
// mechanism this mirrors: every "read a value out of storage" opcode
// rewrites a destructed-object reference to a real int 0 in place,
// self-healing the storage so the same slot never needs re-checking.
// Each test below destructs an object while it is still referenced from
// LPC-visible storage (a local, an object variable, an array element, a
// mapping value) and confirms a later LPC-level read of that same
// storage location comes back as a genuine int 0 -- not just falsy, the
// exact type real destruct_object() leaves behind -- matching real
// semantics precisely rather than approximating "still callable but
// somehow falsy."

static void testDestructedObjectInLocalVariableReadsBackAsIntZero() {
    ObjectVarHarness harness;
    harness.writeFile("/drc_item1.c", "int probe() { return 0; }\n");
    harness.writeFile("/drc_probe1.c",
        "mixed check() {\n"
        "    object o;\n"
        "    o = clone_object(\"/drc_item1\");\n"
        "    destruct(o);\n"
        "    return o;\n"
        "}\n");
    auto probe = harness.objects.cloneObject("/drc_probe1");
    assert(probe != nullptr);

    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testDestructedObjectInLocalVariableReadsBackAsIntZero OK\n";
}

static void testDestructedObjectInObjectVariableReadsBackAsIntZero() {
    ObjectVarHarness harness;
    harness.writeFile("/drc_item2.c", "int probe() { return 0; }\n");
    harness.writeFile("/drc_probe2.c",
        "object stored;\n"
        "void set_and_destruct() {\n"
        "    stored = clone_object(\"/drc_item2\");\n"
        "    destruct(stored);\n"
        "}\n"
        "mixed check() { return stored; }\n");
    auto probe = harness.objects.cloneObject("/drc_probe2");
    assert(probe != nullptr);

    harness.vm.callFunction(probe, "set_and_destruct", {});
    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testDestructedObjectInObjectVariableReadsBackAsIntZero OK\n";
}

static void testDestructedObjectInArrayElementReadsBackAsIntZeroWhenIndexed() {
    ObjectVarHarness harness;
    harness.writeFile("/drc_item3.c", "int probe() { return 0; }\n");
    harness.writeFile("/drc_probe3.c",
        "mixed *arr;\n"
        "void set_and_destruct() {\n"
        "    object o;\n"
        "    o = clone_object(\"/drc_item3\");\n"
        "    arr = ({ o });\n"
        "    destruct(o);\n"
        "}\n"
        "mixed check() { return arr[0]; }\n");
    auto probe = harness.objects.cloneObject("/drc_probe3");
    assert(probe != nullptr);

    harness.vm.callFunction(probe, "set_and_destruct", {});
    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testDestructedObjectInArrayElementReadsBackAsIntZeroWhenIndexed OK\n";
}

static void testDestructedObjectInMappingValueReadsBackAsIntZeroWhenIndexed() {
    ObjectVarHarness harness;
    harness.writeFile("/drc_item4.c", "int probe() { return 0; }\n");
    harness.writeFile("/drc_probe4.c",
        "mapping m;\n"
        "void set_and_destruct() {\n"
        "    object o;\n"
        "    o = clone_object(\"/drc_item4\");\n"
        "    m = ([ \"k\": o ]);\n"
        "    destruct(o);\n"
        "}\n"
        "mixed check() { return m[\"k\"]; }\n");
    auto probe = harness.objects.cloneObject("/drc_probe4");
    assert(probe != nullptr);

    harness.vm.callFunction(probe, "set_and_destruct", {});
    amlp::Value result = harness.vm.callFunction(probe, "check", {});
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);

    std::cout << "testDestructedObjectInMappingValueReadsBackAsIntZeroWhenIndexed OK\n";
}

static void testNonDestructedObjectInVariableAndArrayStillReadsBackAsRealObject() {
    // Confirms coerceIfDestructed() only fires for an actually-destructed
    // object -- an ordinary, still-alive object reference stored the
    // same way must keep working exactly as before.
    ObjectVarHarness harness;
    harness.writeFile("/drc_item5.c", "int probe() { return 0; }\n");
    harness.writeFile("/drc_probe5.c",
        "object stored;\n"
        "mixed *arr;\n"
        "void set() {\n"
        "    object o;\n"
        "    o = clone_object(\"/drc_item5\");\n"
        "    stored = o;\n"
        "    arr = ({ o });\n"
        "}\n"
        "mixed check_var() { return stored; }\n"
        "mixed check_arr() { return arr[0]; }\n");
    auto probe = harness.objects.cloneObject("/drc_probe5");
    assert(probe != nullptr);

    harness.vm.callFunction(probe, "set", {});
    amlp::Value varResult = harness.vm.callFunction(probe, "check_var", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(varResult.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(varResult.data) != nullptr);
    amlp::Value arrResult = harness.vm.callFunction(probe, "check_arr", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(arrResult.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(arrResult.data) != nullptr);

    std::cout << "testNonDestructedObjectInVariableAndArrayStillReadsBackAsRealObject OK\n";
}

static void testCallOutAcceptsClosureAsFirstArgument() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/callout_closure_probe.c",
        "void idle() {}\n"
        "int probe() { return call_out((: idle :), 5); }\n");
    auto obj = harness.objects.cloneObject("/callout_closure_probe");
    assert(obj != nullptr);
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
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

    amlp::Value result = harness.vm.callFunction(callerObj, "run", {});
    auto* obPtr = std::get_if<std::shared_ptr<amlp::LpcObject>>(&result.data);
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

    amlp::Value result = harness.vm.callFunction(callerObj, "run", {});
    auto* obPtr = std::get_if<std::shared_ptr<amlp::LpcObject>>(&result.data);
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

    amlp::Value result = harness.vm.callFunction(a, "run", {});
    auto* arrPtr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arrPtr != nullptr && *arrPtr != nullptr);
    // Nearest first: whoever called level_c's probe() (level_b's own
    // loaded singleton), then whoever called level_b's run() (level_a).
    assert((*arrPtr)->items.size() == 2);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*arrPtr)->items[0].data) == b);
    assert(std::get<std::shared_ptr<amlp::LpcObject>>((*arrPtr)->items[1].data) == a);

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

    amlp::Value result = harness.vm.callFunction(accountObj, "probe", {amlp::Value(std::string("/does_not_exist"))});
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

    amlp::Value saveResult = harness.vm.callFunction(obj, "save", {});
    assert(std::holds_alternative<int64_t>(saveResult.data));
    assert(std::get<int64_t>(saveResult.data) == 1);

    harness.vm.callFunction(obj, "clear", {});
    amlp::Value nAfterClear = harness.vm.callFunction(obj, "query_n", {});
    assert(std::holds_alternative<int64_t>(nAfterClear.data));
    assert(std::get<int64_t>(nAfterClear.data) == 0);

    amlp::Value loadResult = harness.vm.callFunction(obj, "load", {});
    assert(std::holds_alternative<int64_t>(loadResult.data));
    assert(std::get<int64_t>(loadResult.data) == 1);

    amlp::Value n = harness.vm.callFunction(obj, "query_n", {});
    assert(std::get<int64_t>(n.data) == 42);
    amlp::Value s = harness.vm.callFunction(obj, "query_s", {});
    assert(std::get<std::string>(s.data) == "hello");
    amlp::Value mA = harness.vm.callFunction(obj, "query_m_a", {});
    assert(std::get<int64_t>(mA.data) == 1);
    amlp::Value mB1 = harness.vm.callFunction(obj, "query_m_b_1", {});
    assert(std::get<std::string>(mB1.data) == "y");
    amlp::Value nested = harness.vm.callFunction(obj, "query_nested", {});
    auto* nestedArr = std::get_if<std::shared_ptr<amlp::Array>>(&nested.data);
    assert(nestedArr != nullptr && *nestedArr != nullptr);
    assert((*nestedArr)->items.size() == 3);
    assert(std::get<int64_t>((*nestedArr)->items[0].data) == 1);
    assert(std::get<std::string>((*nestedArr)->items[1].data) == "two");
    auto* innerArr = std::get_if<std::shared_ptr<amlp::Array>>(&(*nestedArr)->items[2].data);
    assert(innerArr != nullptr && *innerArr != nullptr);
    assert((*innerArr)->items.size() == 2);
    assert(std::get<int64_t>((*innerArr)->items[0].data) == 3);
    assert(std::get<int64_t>((*innerArr)->items[1].data) == 4);

    std::cout << "testSaveObjectRestoreObjectRoundTripsNestedMappingsAndArrays OK\n";
}

static void testRestoreObjectParsesRealFluffosOnDiskFormatScalarsAndNesting() {
    // restore_object() previously only understood this driver's own
    // tab-delimited save format (see serializeValue()/deserializeValue()
    // in EfunTable.cpp) -- a real, pre-existing FluffOS save file (real
    // format: "varname value\n", value in plain LPC literal syntax, a
    // single space as the delimiter, not a tab) silently matched no
    // line at all and left every variable at its create()-time default,
    // with no error (see this file's own "Known stubs" entry for
    // save_object()/restore_object()). This writes a save file by hand
    // in the real on-disk shape (never touched by this driver's own
    // save_object()) and confirms restore_object() now actually loads
    // it: an int, a negative int, a float, a string, and a nested
    // array-of-{int,string,mapping} value, matching real save_svalue()'s
    // own grammar (fluffos-2.9-ds2.08's object.c).
    ObjectVarHarness harness;
    harness.writeFile("/real_probe.o",
        "n 42\n"
        "neg -7\n"
        "f 3.500000\n"
        "s \"hello\"\n"
        "data ({1,\"two\",([\"k\":3,]),})\n");
    harness.writeFile("/real_probe.c",
        "int n;\n"
        "int neg;\n"
        "float f;\n"
        "string s;\n"
        "mixed *data;\n"
        "int load() { return restore_object(\"/real_probe.o\"); }\n"
        "int query_n() { return n; }\n"
        "int query_neg() { return neg; }\n"
        "float query_f() { return f; }\n"
        "string query_s() { return s; }\n"
        "mixed *query_data() { return data; }\n");
    auto obj = harness.objects.cloneObject("/real_probe");
    assert(obj != nullptr);

    amlp::Value loadResult = harness.vm.callFunction(obj, "load", {});
    assert(std::holds_alternative<int64_t>(loadResult.data));
    assert(std::get<int64_t>(loadResult.data) == 1);

    amlp::Value n = harness.vm.callFunction(obj, "query_n", {});
    assert(std::get<int64_t>(n.data) == 42);
    amlp::Value neg = harness.vm.callFunction(obj, "query_neg", {});
    assert(std::get<int64_t>(neg.data) == -7);
    amlp::Value f = harness.vm.callFunction(obj, "query_f", {});
    assert(std::get<double>(f.data) == 3.5);
    amlp::Value s = harness.vm.callFunction(obj, "query_s", {});
    assert(std::get<std::string>(s.data) == "hello");

    amlp::Value data = harness.vm.callFunction(obj, "query_data", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&data.data);
    assert(arr != nullptr && *arr != nullptr);
    assert((*arr)->items.size() == 3);
    assert(std::get<int64_t>((*arr)->items[0].data) == 1);
    assert(std::get<std::string>((*arr)->items[1].data) == "two");
    auto* map = std::get_if<std::shared_ptr<amlp::Mapping>>(&(*arr)->items[2].data);
    assert(map != nullptr && *map != nullptr);
    assert((*map)->entries.size() == 1);
    assert(std::get<std::string>((*map)->entries[0].first.data) == "k");
    assert(std::get<int64_t>((*map)->entries[0].second.data) == 3);

    std::cout << "testRestoreObjectParsesRealFluffosOnDiskFormatScalarsAndNesting OK\n";
}

static void testRestoreObjectSkipsRealFormatCommentHeaderLineAndParsesEmptyContainers() {
    // Mirrors this project's own real, shipped example of a genuine
    // FluffOS save file (nightmare3_fluffos_v2/lib/daemon/save/
    // banish.o, confirmed against an untouched backup copy): a leading
    // "#/path/to/originating_file.c" comment line (real
    // restore_object_from_line()'s own "ignore 'comments'" case), then
    // several empty-array and one empty-mapping variables.
    ObjectVarHarness harness;
    harness.writeFile("/banish_probe.o",
        "#/daemon/banish.c\n"
        "__Names ({})\n"
        "__TmpBanish ([])\n");
    harness.writeFile("/banish_probe.c",
        "mixed *__Names;\n"
        "mapping __TmpBanish;\n"
        "int load() { return restore_object(\"/banish_probe.o\"); }\n"
        "int query_names_size() { return sizeof(__Names); }\n"
        "int query_tmpbanish_size() { return sizeof(__TmpBanish); }\n");
    auto obj = harness.objects.cloneObject("/banish_probe");
    assert(obj != nullptr);

    amlp::Value loadResult = harness.vm.callFunction(obj, "load", {});
    assert(std::holds_alternative<int64_t>(loadResult.data));
    assert(std::get<int64_t>(loadResult.data) == 1);

    amlp::Value namesSize = harness.vm.callFunction(obj, "query_names_size", {});
    assert(std::get<int64_t>(namesSize.data) == 0);
    amlp::Value tmpBanishSize = harness.vm.callFunction(obj, "query_tmpbanish_size", {});
    assert(std::get<int64_t>(tmpBanishSize.data) == 0);

    std::cout << "testRestoreObjectSkipsRealFormatCommentHeaderLineAndParsesEmptyContainers OK\n";
}

static void testRestoreObjectRealFormatStringEscapesAndEmbeddedNewline() {
    // Real save_svalue()'s own string writer (object.c) backslash-
    // escapes '"' and '\\', and encodes an embedded '\n' as a raw '\r'
    // byte on disk (so the save file itself stays one line per
    // variable) -- restore_string() undoes both. Confirms this
    // driver's real-format reader matches: a literal quote, a literal
    // backslash, and a raw CR byte translated back to '\n'.
    ObjectVarHarness harness;
    std::string onDisk = "s \"she said \\\"hi\\\" then a\\\\b then a";
    onDisk += '\r';
    onDisk += "newline\"\n";
    harness.writeFile("/escape_probe.o", onDisk);
    harness.writeFile("/escape_probe.c",
        "string s;\n"
        "int load() { return restore_object(\"/escape_probe.o\"); }\n"
        "string query_s() { return s; }\n");
    auto obj = harness.objects.cloneObject("/escape_probe");
    assert(obj != nullptr);

    amlp::Value loadResult = harness.vm.callFunction(obj, "load", {});
    assert(std::holds_alternative<int64_t>(loadResult.data));
    assert(std::get<int64_t>(loadResult.data) == 1);

    amlp::Value s = harness.vm.callFunction(obj, "query_s", {});
    assert(std::get<std::string>(s.data) == "she said \"hi\" then a\\b then a\nnewline");

    std::cout << "testRestoreObjectRealFormatStringEscapesAndEmbeddedNewline OK\n";
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

    amlp::Value closureVal = harness.vm.callFunction(owner, "make_closure", {});
    auto closurePtr = std::get<std::shared_ptr<amlp::Closure>>(closureVal.data);
    assert(closurePtr != nullptr);

    // middleman calls evaluate() on a closure it did not build --
    // vm.currentObject() during save_object()'s own execution must
    // still be "owner", not "middleman".
    amlp::Value saveResult = harness.vm.callFunction(middleman, "run", {closureVal});
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
    amlp::Value loaded = harness.vm.callFunction(restoreObj, "load", {});
    assert(std::holds_alternative<int64_t>(loaded.data));
    assert(std::get<int64_t>(loaded.data) == 1);
    amlp::Value marker = harness.vm.callFunction(restoreObj, "query_marker", {});
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

    amlp::Value marker = harness.vm.callFunction(ob, "query_marker", {});
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

    amlp::Value calls = harness.vm.callFunction(master, "query_calls", {});
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
    amlp::Value loadedPrivs = harness.vm.callFunction(loaded, "probe", {});
    auto* loadedStr = std::get_if<std::string>(&loadedPrivs.data);
    assert(loadedStr != nullptr && *loadedStr == "TestPriv");

    auto cloned = harness.objects.cloneObject("/priv_item");
    assert(cloned != nullptr);
    amlp::Value clonedPrivs = harness.vm.callFunction(cloned, "probe", {});
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
    amlp::Value leading = harness.vm.callFunction(ob, "leading",
        {amlp::Value(std::string("/domains/Praxis/rift_survivor"))});
    auto* leadingArr = std::get_if<std::shared_ptr<amlp::Array>>(&leading.data);
    assert(leadingArr != nullptr && (*leadingArr)->items.size() == 3);
    assert(std::get<std::string>((*leadingArr)->items[0].data) == "domains");
    assert(std::get<std::string>((*leadingArr)->items[1].data) == "Praxis");
    assert(std::get<std::string>((*leadingArr)->items[2].data) == "rift_survivor");

    // Trailing separator: no spurious trailing "" element.
    amlp::Value trailing = harness.vm.callFunction(ob, "trailing",
        {amlp::Value(std::string("line one\nline two\n"))});
    auto* trailingArr = std::get_if<std::shared_ptr<amlp::Array>>(&trailing.data);
    assert(trailingArr != nullptr && (*trailingArr)->items.size() == 2);
    assert(std::get<std::string>((*trailingArr)->items[1].data) == "line two");

    // A separator in the middle still produces an empty element there --
    // only LEADING runs are collapsed, nothing in the middle is special.
    amlp::Value middle = harness.vm.callFunction(ob, "middle",
        {amlp::Value(std::string("a//b"))});
    auto* middleArr = std::get_if<std::shared_ptr<amlp::Array>>(&middle.data);
    assert(middleArr != nullptr && (*middleArr)->items.size() == 3);
    assert(std::get<std::string>((*middleArr)->items[1].data) == "");

    // A string made entirely of the separator explodes to an empty array.
    amlp::Value allSeps = harness.vm.callFunction(ob, "all_seps",
        {amlp::Value(std::string("///"))});
    auto* allSepsArr = std::get_if<std::shared_ptr<amlp::Array>>(&allSeps.data);
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
    auto* obPtr = std::get_if<std::shared_ptr<amlp::LpcObject>>(&result.data);
    assert(obPtr != nullptr && *obPtr != nullptr);

    amlp::Value marker = harness.vm.callFunction(*obPtr, "query_marker", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    vm.callFunction(obj, "set_it", {});
    amlp::Value result = vm.callFunction(obj, "get_it", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "call_it", {});
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

    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    amlp::Value result = vm.callFunction(obj, "probe", {});
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
    amlp::Value strResult = runProbe(
        "string s;\n"
        "s = \"hello\";\n"
        "return s[<1];\n"); // last char: 'o' (111)
    assert(std::holds_alternative<int64_t>(strResult.data));
    assert(std::get<int64_t>(strResult.data) == 'o');

    amlp::Value arrResult = runProbe(
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
    amlp::Value result = runProbe(
        "string name;\n"
        "name = \"testchar.o\";\n"
        "return name[<2..];\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == ".o");

    std::cout << "testFromEndOpenRangeMatchesRealUserCShape OK\n";
}

static void testFromEndBothBoundsOnRangeIndex() {
    // "arr[<a..<b]" -- both bounds counted from the end.
    amlp::Value result = runProbe(
        "mixed *items;\n"
        "items = ({ 1, 2, 3, 4, 5 });\n"
        "return items[<4..<2];\n"); // indices 1..3 -> ({2,3,4})
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
        "mapping player_data;\n"
        "player_data = ([ \"general\": ([ \"quest points\": 3 ]) ]);\n"
        "player_data[\"general\"][\"quest points\"] += 7;\n"
        "return player_data[\"general\"][\"quest points\"];\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 10);

    std::cout << "testCompoundIndexAssignOnChainedNestedMappingMatchesRealUserCShape OK\n";
}

static void testCompoundIndexAssignOnArrayElement() {
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* ifStmt = dynamic_cast<amlp::IfStmt*>(body[1].get());
    assert(ifStmt != nullptr);
    auto* notExpr = dynamic_cast<amlp::UnaryExpr*>(ifStmt->condition.get());
    assert(notExpr != nullptr && notExpr->op == amlp::UnaryOp::Not);
    auto* idxAssign = dynamic_cast<amlp::IndexAssignExpr*>(notExpr->operand.get());
    assert(idxAssign != nullptr);
    assert(idxAssign->isCompound == false);
    auto* target = dynamic_cast<amlp::VarRefExpr*>(idxAssign->target.get());
    assert(target != nullptr && target->name == "m");
    auto* key = dynamic_cast<amlp::StringLiteral*>(idxAssign->index.get());
    assert(key != nullptr && key->value == "class");

    std::cout << "testIndexAssignAsSubExpressionParsesToIndexAssignExpr OK\n";
}

static void testIndexAssignAsSubExpressionVmExecutionMatchesMoreCShape() {
    // Mirrors std/user/more.c's own
    // "if(!(__More[\"class\"] = cl)) __More[\"class\"] = \"info\";"
    // exactly: a non-empty cl leaves the assigned value in place, and
    // the assignment's own value (not 0/1) is what the "!" tests.
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assignStmt = dynamic_cast<amlp::AssignStmt*>(body[1].get());
    assert(assignStmt != nullptr);
    auto* lambda = dynamic_cast<amlp::InlineLambdaExpr*>(assignStmt->value.get());
    assert(lambda != nullptr);
    assert(lambda->bodyExprs.size() == 2);
    auto* firstCall = dynamic_cast<amlp::CallExpr*>(lambda->bodyExprs[0].get());
    assert(firstCall != nullptr && firstCall->callee == "previous_object");
    auto* secondStr = dynamic_cast<amlp::StringLiteral*>(lambda->bodyExprs[1].get());
    assert(secondStr != nullptr && secondStr->value == "abort");

    std::cout << "testInlineLambdaWithCallExpressionFirstOperandParsesAsInlineLambdaExpr OK\n";
}

static void testInlineLambdaBareStringConstantParsesAsInlineLambdaExpr() {
    std::string src =
        "void probe() {\n"
        "    mixed f;\n"
        "    f = (: \"return_to_edit\" :);\n"
        "}\n";
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* assignStmt = dynamic_cast<amlp::AssignStmt*>(body[1].get());
    assert(assignStmt != nullptr);
    auto* lambda = dynamic_cast<amlp::InlineLambdaExpr*>(assignStmt->value.get());
    assert(lambda != nullptr);
    assert(lambda->bodyExprs.size() == 1);
    auto* str = dynamic_cast<amlp::StringLiteral*>(lambda->bodyExprs[0].get());
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
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
    assert(std::holds_alternative<int64_t>(result.data));
    // before == 0 (not yet run), side_effect == 1 (ran once evaluate()
    // called it), called == "abort" (the string, not a method call).
    assert(std::get<int64_t>(result.data) == 101);

    std::cout << "testInlineLambdaVmExecutionEvaluatesBodyAtCallTimeNotConstructionTime OK\n";
}

// "$N" inside a "(: ... :)" lambda body -- real lex.c's own L_PARAMETER
// token, an implicit reference to the closure's own Nth call-time
// argument. See Ast.hpp's LambdaParamExpr comment for the full citation:
// real, previously-unparseable (a bare "$" threw "lexer: unrecognized
// character"), and load-bearing -- secure/daemon/events.c's own
// unguarded "filter(users(), (: $1 && environment($1) :))" meant the
// whole EVENTS_D daemon could never compile at all, breaking every
// simul_efun in secure/SimulEfun/time.c and light.c that depends on it.

static void testDollarLambdaParamBindsClosuresOwnFirstCallTimeArgument() {
    amlp::Value result = runProbe("return funcall((: $1 + 1 :), 41);");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);
    std::cout << "testDollarLambdaParamBindsClosuresOwnFirstCallTimeArgument OK\n";
}

static void testDollarLambdaParamMultipleParametersBindPositionally() {
    // "$2" alone (with no "$1" at all) still forces a 2-parameter
    // closure -- real lex.c: num_parameters tracks the *highest* digit
    // seen, not a count of distinct ones used -- confirmed by also
    // exercising $1 out of order (referenced second) in the same body.
    amlp::Value result = runProbe("return funcall((: $2 * 10 + $1 :), 2, 4);");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);
    std::cout << "testDollarLambdaParamMultipleParametersBindPositionally OK\n";
}

static void testDollarLambdaParamFilterMatchesRealEventsDShape() {
    // The exact real call site's own shape, secure/daemon/events.c's
    // "filter(users(), (: $1 && environment($1) :))" -- filtering a list
    // of objects down to only the ones currently somewhere (a
    // non-null environment()), mirrored here with move_object()/
    // environment() instead of users() (this driver has no live
    // connections in a unit test, but the mechanism under test -- $1
    // binding to each filtered element -- is identical either way).
    ObjectVarHarness harness;
    harness.writeFile("/dlp_room.c", "void init() {}\n");
    harness.writeFile("/dlp_thing.c", "void create() {}\n");
    harness.writeFile("/dlp_probe.c",
        "mixed *probe(object a, object b) {\n"
        "    return filter(({ a, b }), (: $1 && environment($1) :));\n"
        "}\n");

    auto room = harness.objects.cloneObject("/dlp_room");
    auto placed = harness.objects.cloneObject("/dlp_thing");
    auto homeless = harness.objects.cloneObject("/dlp_thing");
    auto probe = harness.objects.cloneObject("/dlp_probe");
    assert(room != nullptr && placed != nullptr && homeless != nullptr && probe != nullptr);
    harness.vm.moveObject(placed, room);

    amlp::Value result = harness.vm.callFunction(
        probe, "probe", {amlp::Value(placed), amlp::Value(homeless)});
    auto* arrPtr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arrPtr != nullptr && *arrPtr != nullptr);
    assert((*arrPtr)->items.size() == 1);
    auto* keptPtr = std::get_if<std::shared_ptr<amlp::LpcObject>>(&(*arrPtr)->items[0].data);
    assert(keptPtr != nullptr && *keptPtr == placed);

    std::cout << "testDollarLambdaParamFilterMatchesRealEventsDShape OK\n";
}

static void testDollarParamOutsideLambdaBodyThrowsParseError() {
    // Real lex.c: "$var illegal outside of function pointer." -- a bare
    // "$1" in ordinary code (never inside a "(: ... :)") must not
    // silently parse as if it meant something.
    std::string src = "int probe() {\n    return $1;\n}\n";
    bool threw = false;
    try {
        amlp::Lexer lexer(src);
        amlp::Parser parser(lexer.tokenize());
        parser.parseProgram();
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("illegal outside of function pointer") != std::string::npos);
    }
    assert(threw);
    std::cout << "testDollarParamOutsideLambdaBodyThrowsParseError OK\n";
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    assert(exprStmt != nullptr);
    auto* call = dynamic_cast<amlp::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr);
    assert(call->callee == "evaluate");
    assert(call->forceEfun == true);
    assert(call->args.size() == 2);
    auto* fpArg = dynamic_cast<amlp::VarRefExpr*>(call->args[0].get());
    assert(fpArg != nullptr && fpArg->name == "cb");
    auto* argsArg = dynamic_cast<amlp::VarRefExpr*>(call->args[1].get());
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
    amlp::Lexer lexer(src);
    amlp::Parser parser(lexer.tokenize());
    auto program = parser.parseProgram();

    auto& body = program->functions[0]->body->statements;
    auto* exprStmt = dynamic_cast<amlp::ExprStmt*>(body[0].get());
    auto* call = dynamic_cast<amlp::CallExpr*>(exprStmt->expr.get());
    assert(call != nullptr && call->forceEfun && call->callee == "evaluate");
    assert(call->args.size() == 2);
    auto* fpArg = dynamic_cast<amlp::IndexExpr*>(call->args[0].get());
    assert(fpArg != nullptr);

    std::cout << "testFunctionPointerCallThroughOnIndexedTargetParses OK\n";
}

static void testFunctionPointerCallThroughVmExecutionCallsClosure() {
    // "sizeof" is a real efun registerCoreEfuns() actually registers
    // (unlike the short-circuit tests' deliberately-undefined marker
    // names), so a correct array-length result here confirms the
    // "(*f)(...)" desugaring genuinely reached
    // evaluate()->VM::callClosure(), not just that it parsed.
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe("return to_int(42);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 42);
    std::cout << "testToIntPassesThroughAnInt OK\n";
}

static void testToIntTruncatesAFloatTowardZero() {
    amlp::Value result = runProbe("return to_int(5.9) * 100 + to_int(-5.9);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // f__to_int()'s real "(long) sp->u.real" cast truncates toward
    // zero, not round-to-nearest or floor: 5.9 -> 5, -5.9 -> -5.
    assert(std::get<int64_t>(result.data) == 495);
    std::cout << "testToIntTruncatesAFloatTowardZero OK\n";
}

static void testToIntParsesALeadingIntegerFromAStringIgnoringTrailingGarbage() {
    amlp::Value result = runProbe("return to_int(\"10x\");\n");
    assert(std::holds_alternative<int64_t>(result.data));
    // Real f__to_int()'s own documented behavior: "to_int(\"10x\") == 10".
    assert(std::get<int64_t>(result.data) == 10);
    std::cout << "testToIntParsesALeadingIntegerFromAStringIgnoringTrailingGarbage OK\n";
}

static void testToIntReturnsZeroForAStringWithNoLeadingNumber() {
    amlp::Value result = runProbe("return to_int(\"nothing here\");\n");
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
    amlp::Value parentResult = harness.vm.callFunction(obj, "query_parent_tag", {});
    assert(std::holds_alternative<int64_t>(parentResult.data));
    assert(std::get<int64_t>(parentResult.data) == 1);

    amlp::Value childResult = harness.vm.callFunction(obj, "query_child_tag", {});
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

    amlp::Value a1 = harness.vm.callFunction(obj, "query_a1", {});
    amlp::Value a2 = harness.vm.callFunction(obj, "query_a2", {});
    amlp::Value a3 = harness.vm.callFunction(obj, "query_a3", {});
    amlp::Value b1 = harness.vm.callFunction(obj, "query_b1", {});
    amlp::Value b2 = harness.vm.callFunction(obj, "query_b2", {});
    amlp::Value b3 = harness.vm.callFunction(obj, "query_b3", {});
    amlp::Value b4 = harness.vm.callFunction(obj, "query_b4", {});

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

    amlp::Value rootVal = harness.vm.callFunction(obj, "query_root_val", {});
    amlp::Value midVal = harness.vm.callFunction(obj, "query_mid_val", {});
    amlp::Value sib1 = harness.vm.callFunction(obj, "query_sib1", {});
    amlp::Value sib2 = harness.vm.callFunction(obj, "query_sib2", {});
    amlp::Value leafVal = harness.vm.callFunction(obj, "query_leaf_val", {});

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
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
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

    harness.vm.callFunction(item, "go", {amlp::Value(room)});

    amlp::Value env = harness.vm.callFunction(item, "probe_env", {});
    auto* envPtr = std::get_if<std::shared_ptr<amlp::LpcObject>>(&env.data);
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
    harness.vm.callFunction(silent, "go", {amlp::Value(room1)});
    assert(harness.vm.dispatchCommand(silent, "look") == false);

    auto room2 = harness.objects.cloneObject("/ea_room");
    auto enabled = harness.objects.cloneObject("/ea_enabled_mover");
    assert(room2 != nullptr && enabled != nullptr);
    harness.vm.callFunction(enabled, "go", {amlp::Value(room2)});
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

    harness.vm.callFunction(outer, "go", {amlp::Value(room)});
    harness.vm.callFunction(inner, "go", {amlp::Value(outer)});

    amlp::Value result = harness.vm.callFunction(room, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 1);
    auto* childOb = std::get_if<std::shared_ptr<amlp::LpcObject>>(&(*arr)->items[0].data);
    assert(childOb != nullptr && *childOb == outer);

    // Default-argument form (this_object() when no argument given).
    harness.writeFile("/ai_default_probe.c",
        "object *probe() { return all_inventory(); }\n");
    auto defaultProbe = harness.objects.cloneObject("/ai_default_probe");
    harness.vm.callFunction(outer, "go", {amlp::Value(defaultProbe)});
    amlp::Value defResult = harness.vm.callFunction(defaultProbe, "probe", {});
    auto* defArr = std::get_if<std::shared_ptr<amlp::Array>>(&defResult.data);
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

    harness.vm.callFunction(outer, "go", {amlp::Value(room)});
    harness.vm.callFunction(inner, "go", {amlp::Value(outer)});

    amlp::Value result = harness.vm.callFunction(room, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 2);
    auto* first = std::get_if<std::shared_ptr<amlp::LpcObject>>(&(*arr)->items[0].data);
    auto* second = std::get_if<std::shared_ptr<amlp::LpcObject>>(&(*arr)->items[1].data);
    assert(first != nullptr && *first == outer);
    assert(second != nullptr && *second == inner);

    std::cout << "testDeepInventoryRecursesThroughNestedChildren OK\n";
}

// first_inventory(object|string)/next_inventory(object) -- func_spec.c.
// Confirmed against fluffos-2.9-ds2.08's own simulate.c/efuns_main.c:
// first_inventory(container) is the container's first child;
// next_inventory(ob) is ob's own next *sibling* (not ob's own first
// child). This driver's own LpcObject::inventory_ vector is populated by
// VM::moveObject()'s push_back() (oldest arrival first) rather than real
// FluffOS's own move_object() prepend (newest arrival first) -- a
// pre-existing divergence, confirmed here rather than assumed
// equivalent: item_a below (moved in first) is the one first_inventory()
// returns, and next_inventory() walks a -> b -> c, oldest to newest.
static void testFirstInventoryAndNextInventoryWalkChildrenInMoveOrder() {
    ObjectVarHarness harness;
    harness.writeFile("/fi_room.c", "void init() {}\n");
    harness.writeFile("/fi_item.c",
        "void go(object dest) { move_object(dest); }\n"
        "object probe_first(object ob) { return first_inventory(ob); }\n"
        "object probe_first_default() { return first_inventory(); }\n"
        "object probe_next(object ob) { return next_inventory(ob); }\n");

    auto room = harness.objects.cloneObject("/fi_room");
    auto itemA = harness.objects.cloneObject("/fi_item");
    auto itemB = harness.objects.cloneObject("/fi_item");
    auto itemC = harness.objects.cloneObject("/fi_item");
    assert(room != nullptr && itemA != nullptr && itemB != nullptr && itemC != nullptr);

    harness.vm.callFunction(itemA, "go", {amlp::Value(room)});
    harness.vm.callFunction(itemB, "go", {amlp::Value(room)});
    harness.vm.callFunction(itemC, "go", {amlp::Value(room)});

    // first_inventory(room) -- oldest occupant (itemA), not real
    // FluffOS's own newest-occupant answer, per this driver's own
    // append-order divergence noted above.
    amlp::Value first =
        harness.vm.callFunction(itemA, "probe_first", {amlp::Value(room)});
    auto* firstOb = std::get_if<std::shared_ptr<amlp::LpcObject>>(&first.data);
    assert(firstOb != nullptr && *firstOb == itemA);

    // next_inventory() walks siblings, oldest to newest: a -> b -> c -> 0.
    amlp::Value nextOfA =
        harness.vm.callFunction(itemA, "probe_next", {amlp::Value(itemA)});
    auto* nextOfAOb = std::get_if<std::shared_ptr<amlp::LpcObject>>(&nextOfA.data);
    assert(nextOfAOb != nullptr && *nextOfAOb == itemB);

    amlp::Value nextOfB =
        harness.vm.callFunction(itemA, "probe_next", {amlp::Value(itemB)});
    auto* nextOfBOb = std::get_if<std::shared_ptr<amlp::LpcObject>>(&nextOfB.data);
    assert(nextOfBOb != nullptr && *nextOfBOb == itemC);

    // itemC is the last sibling -- 0, not itemA (no wraparound).
    amlp::Value nextOfC =
        harness.vm.callFunction(itemA, "probe_next", {amlp::Value(itemC)});
    assert(std::holds_alternative<std::monostate>(nextOfC.data));

    // An item with no children of its own -- first_inventory() on it
    // returns 0, not an error.
    amlp::Value emptyFirst =
        harness.vm.callFunction(itemA, "probe_first", {amlp::Value(itemB)});
    assert(std::holds_alternative<std::monostate>(emptyFirst.data));

    // Default-argument form (this_object() when no argument given) --
    // called as itemA's own method, so this_object() is itemA, whose own
    // inventory is empty.
    amlp::Value defaultResult = harness.vm.callFunction(itemA, "probe_first_default", {});
    assert(std::holds_alternative<std::monostate>(defaultResult.data));

    std::cout << "testFirstInventoryAndNextInventoryWalkChildrenInMoveOrder OK\n";
}

// first_inventory()'s own object|string signature (func_spec.c) --
// confirmed against simulate.c's own first_inventory(): a string
// argument is resolved with find_object(), same as call_other()'s own
// string form; anything that resolves to no object throws (real
// bad_argument()), as does a first argument that is neither an object
// nor a string.
static void testFirstInventoryStringArgumentResolvesOrThrows() {
    ObjectVarHarness harness;
    harness.writeFile("/fis_room.c", "void init() {}\n");
    harness.writeFile("/fis_item.c", "void go(object dest) { move_object(dest); }\n");
    harness.writeFile("/fis_probe.c",
        "object probe_path(string path) { return first_inventory(path); }\n"
        "object probe_bad_arg() { return first_inventory(42); }\n");

    // find_object() (what first_inventory()'s own string form resolves
    // through) returns the cached, load-by-path object -- not a fresh,
    // unrelated instance the way cloneObject() would -- so room must be
    // obtained the same way, or "/fis_room" would resolve to a different
    // object than the one item is actually moved into.
    auto room = harness.objects.loadObject("/fis_room");
    auto item = harness.objects.cloneObject("/fis_item");
    auto probe = harness.objects.cloneObject("/fis_probe");
    assert(room != nullptr && item != nullptr && probe != nullptr);
    harness.vm.callFunction(item, "go", {amlp::Value(room)});

    amlp::Value result = harness.vm.callFunction(
        probe, "probe_path", {amlp::Value(std::string("/fis_room"))});
    auto* resultOb = std::get_if<std::shared_ptr<amlp::LpcObject>>(&result.data);
    assert(resultOb != nullptr && *resultOb == item);

    bool threwOnUnresolved = false;
    try {
        harness.vm.callFunction(
            probe, "probe_path", {amlp::Value(std::string("/does/not/exist"))});
    } catch (const amlp::LpcRuntimeError&) {
        threwOnUnresolved = true;
    }
    assert(threwOnUnresolved);

    bool threwOnBadArgType = false;
    try {
        harness.vm.callFunction(probe, "probe_bad_arg", {});
    } catch (const amlp::LpcRuntimeError&) {
        threwOnBadArgType = true;
    }
    assert(threwOnBadArgType);

    std::cout << "testFirstInventoryStringArgumentResolvesOrThrows OK\n";
}

// first_inventory()/next_inventory()'s real hidden-skip loop
// (efuns_main.c/simulate.c, only compiled when F_SET_HIDE is defined --
// it is, confirmed live): a hidden sibling is skipped unless the
// observer (current_object) is itself hidden, or master()->valid_hide()
// grants the observer permission -- the same master apply set_hide()
// itself is gated behind (see its own tests above).
static void testFirstInventoryAndNextInventorySkipHiddenSiblingsUnlessPermitted() {
    // valid_hide() here checks query_privs(ob) == "wiz" rather than
    // unconditionally permitting or denying everyone -- a uniformly
    // denying master would also block set_hide() itself (it is gated
    // behind the identical valid_hide() apply, checked against whichever
    // object is trying to hide), so hidden below would never actually
    // become hidden and this test would not be exercising the skip logic
    // at all. Real valid_hide() implementations gate on privilege the
    // same way (e.g. wizard-only); set_privs()/query_privs() already
    // exist here and give the two objects below genuinely different
    // permission without needing anything new.
    ObjectVarHarness denying;
    denying.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_hide(object ob) { return query_privs(ob) == \"wiz\"; }\n");
    assert(denying.objects.loadMasterObject());
    denying.writeFile("/hd_room.c", "void init() {}\n");
    denying.writeFile("/hd_item.c",
        "void go(object dest) { move_object(dest); }\n"
        "void grant_wiz() { set_privs(this_object(), \"wiz\"); }\n"
        "void hide() { set_hide(1); }\n"
        "object probe_first(object ob) { return first_inventory(ob); }\n"
        "object probe_next(object ob) { return next_inventory(ob); }\n");

    // Three siblings, oldest to newest: before, hidden, after -- so
    // next_inventory(before) genuinely has to skip over a *middle*
    // hidden sibling to reach after, not just fall off the end.
    auto room = denying.objects.cloneObject("/hd_room");
    auto before = denying.objects.cloneObject("/hd_item");
    auto hidden = denying.objects.cloneObject("/hd_item");
    auto after = denying.objects.cloneObject("/hd_item");
    assert(room != nullptr && before != nullptr && hidden != nullptr && after != nullptr);
    denying.vm.callFunction(before, "go", {amlp::Value(room)});
    denying.vm.callFunction(hidden, "go", {amlp::Value(room)});
    denying.vm.callFunction(after, "go", {amlp::Value(room)});
    // hidden has "wiz" privs -- valid_hide(hidden) permits it to hide
    // itself; before/after never get "wiz" privs, so valid_hide() checked
    // with either of them as the *observer* stays denied.
    denying.vm.callFunction(hidden, "grant_wiz", {});
    denying.vm.callFunction(hidden, "hide", {});
    assert(hidden->isHidden());

    // Observer is "before" (never hidden itself, no "wiz" privs) --
    // first_inventory(room) must skip the hidden occupant entirely and
    // land on "before" itself, the oldest non-hidden occupant.
    amlp::Value firstSeen =
        denying.vm.callFunction(before, "probe_first", {amlp::Value(room)});
    auto* firstSeenOb = std::get_if<std::shared_ptr<amlp::LpcObject>>(&firstSeen.data);
    assert(firstSeenOb != nullptr && *firstSeenOb == before);

    // next_inventory(before) from the same unprivileged observer must
    // skip straight over the hidden middle sibling and land on "after" --
    // confirms the same skip loop applies to next_inventory(), not just
    // first_inventory(), and that it skips a middle entry, not merely
    // the first one checked.
    amlp::Value nextAfterBefore =
        denying.vm.callFunction(before, "probe_next", {amlp::Value(before)});
    auto* nextAfterBeforeOb =
        std::get_if<std::shared_ptr<amlp::LpcObject>>(&nextAfterBefore.data);
    assert(nextAfterBeforeOb != nullptr && *nextAfterBeforeOb == after);

    // Permitting master: the same hidden occupant is now returned.
    ObjectVarHarness permitting;
    permitting.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_hide(object ob) { return 1; }\n");
    assert(permitting.objects.loadMasterObject());
    permitting.writeFile("/hd2_room.c", "void init() {}\n");
    permitting.writeFile("/hd2_item.c",
        "void go(object dest) { move_object(dest); }\n"
        "void hide() { set_hide(1); }\n"
        "object probe_first(object ob) { return first_inventory(ob); }\n");

    auto room2 = permitting.objects.cloneObject("/hd2_room");
    auto hidden2 = permitting.objects.cloneObject("/hd2_item");
    auto visible2 = permitting.objects.cloneObject("/hd2_item");
    assert(room2 != nullptr && hidden2 != nullptr && visible2 != nullptr);
    permitting.vm.callFunction(hidden2, "go", {amlp::Value(room2)});
    permitting.vm.callFunction(visible2, "go", {amlp::Value(room2)});
    permitting.vm.callFunction(hidden2, "hide", {});

    amlp::Value firstPermitted =
        permitting.vm.callFunction(visible2, "probe_first", {amlp::Value(room2)});
    auto* firstPermittedOb =
        std::get_if<std::shared_ptr<amlp::LpcObject>>(&firstPermitted.data);
    assert(firstPermittedOb != nullptr && *firstPermittedOb == hidden2);

    std::cout << "testFirstInventoryAndNextInventorySkipHiddenSiblingsUnlessPermitted OK\n";
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

    amlp::Value eq = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("abc")), amlp::Value(std::string("abc"))});
    assert(std::get<int64_t>(eq.data) == 0);

    amlp::Value lt = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("abc")), amlp::Value(std::string("abd"))});
    assert(std::get<int64_t>(lt.data) < 0);

    amlp::Value gt = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("abd")), amlp::Value(std::string("abc"))});
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

    amlp::Value before = harness.vm.callFunction(ob, "setup", {});
    assert(std::get<int64_t>(before.data) == 3);

    amlp::Value after = harness.vm.callFunction(ob, "after_delete", {});
    assert(std::get<int64_t>(after.data) == 2);

    amlp::Value hasB = harness.vm.callFunction(ob, "has_key",
        {amlp::Value(std::string("b"))});
    assert(std::get<int64_t>(hasB.data) == 0);

    amlp::Value stillA = harness.vm.callFunction(ob, "still_has",
        {amlp::Value(std::string("a"))});
    assert(std::get<int64_t>(stillA.data) == 1);
    amlp::Value stillC = harness.vm.callFunction(ob, "still_has",
        {amlp::Value(std::string("c"))});
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
    amlp::Value name = harness.vm.callFunction(withExt, "probe", {});
    auto* namePtr = std::get_if<std::string>(&name.data);
    assert(namePtr != nullptr);
    assert(namePtr->find(".c.c") == std::string::npos);

    auto withoutExt = harness.objects.cloneObject("/dotc_item");
    assert(withoutExt != nullptr);

    std::cout << "testCloneObjectAcceptsPathWithTrailingDotCWithoutDoublingExtension OK\n";
}

// get_dir(string path) -- real file.c's get_dir(): the directory
// portion of path is literal, only the final path component may carry
// a glob wildcard, matched against that directory's own entries.
// Found live blocking EVERY typed command in the mudlib, not
// discovered until this project's chargen live-testing finally reached
// a real room: daemon/command.c's own rehash() (behind find_cmd(),
// which cmd_hook() depends on for every verb) calls
// "get_dir(val[i]+\"/_*.c\")" -- a genuine glob this efun's earlier
// bare-directory-or-bare-file-only implementation could never match,
// so __Cmds was silently never populated and every command silently
// produced nothing (see EfunTable.cpp's own comment for the full
// symptom chain -- no exception, no dropped connection, nothing to
// suggest the cause without reading rehash() directly).
static void testGetDirMatchesGlobPatternInFinalPathComponentOnly() {
    ObjectVarHarness harness;
    harness.writeFile("/_look.c", "int cmd_look() { return 1; }\n");
    harness.writeFile("/_score.c", "int cmd_score() { return 1; }\n");
    harness.writeFile("/readme.txt", "not a command file\n");
    harness.writeFile("/probe.c",
        "mixed *glob() { return get_dir(\"/_*.c\"); }\n"
        "mixed *plain_dir() { return get_dir(\"/\"); }\n");
    auto ob = harness.objects.cloneObject("/probe");
    assert(ob != nullptr);

    amlp::Value globResult = harness.vm.callFunction(ob, "glob", {});
    auto* globArr = std::get_if<std::shared_ptr<amlp::Array>>(&globResult.data);
    assert(globArr != nullptr);
    std::vector<std::string> names;
    for (auto& item : (*globArr)->items) names.push_back(std::get<std::string>(item.data));
    std::sort(names.begin(), names.end());
    assert(names.size() == 2);
    assert(names[0] == "_look.c");
    assert(names[1] == "_score.c");
    // readme.txt (no glob match) and probe.c (compiled after the glob
    // files were written, but still present on disk) must not appear.
    assert(std::find(names.begin(), names.end(), "readme.txt") == names.end());

    // Bare directory path (no wildcard) still lists everything, the
    // pre-existing behavior this fix must not regress.
    amlp::Value dirResult = harness.vm.callFunction(ob, "plain_dir", {});
    auto* dirArr = std::get_if<std::shared_ptr<amlp::Array>>(&dirResult.data);
    assert(dirArr != nullptr);
    assert((*dirArr)->items.size() >= 4); // _look.c, _score.c, readme.txt, probe.c

    std::cout << "testGetDirMatchesGlobPatternInFinalPathComponentOnly OK\n";
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

    amlp::Value isInt = harness.vm.callFunction(ob, "probe_int", {});
    assert(std::get<int64_t>(isInt.data) == 1);

    amlp::Value isStr = harness.vm.callFunction(ob, "probe_string", {});
    assert(std::get<int64_t>(isStr.data) == 0);

    // An object variable with no explicit initializer defaults to a
    // real int64_t 0 (see STATUS.md's "Root-causing the __HistorySize
    // report"), which IS a real int -- intp() on it must be true.
    amlp::Value unsetVar = harness.vm.callFunction(ob, "probe_unset_var", {});
    assert(std::get<int64_t>(unsetVar.data) == 1);

    std::cout << "testIntpTrueOnlyForIntNotStringObjectOrUnsetVariable OK\n";
}

// floatp(mixed) -- func_spec.c: "int floatp(mixed);". Confirmed against
// fluffos-2.9-ds2.08/efuns_main.c's own f_floatp(): a plain T_REAL
// type-tag check, same shape as intp()/stringp() above. Picked from the
// efun-coverage audit's Tier 1 quick-win list.
static void testFloatpTrueOnlyForFloatNotIntOrString() {
    ObjectVarHarness harness;
    harness.writeFile("/floatp_probe.c",
        "int probe_float() { return floatp(1.5); }\n"
        "int probe_int() { return floatp(5); }\n"
        "int probe_string() { return floatp(\"5.0\"); }\n");
    auto ob = harness.objects.cloneObject("/floatp_probe");
    assert(ob != nullptr);

    amlp::Value isFloat = harness.vm.callFunction(ob, "probe_float", {});
    assert(std::get<int64_t>(isFloat.data) == 1);

    amlp::Value isInt = harness.vm.callFunction(ob, "probe_int", {});
    assert(std::get<int64_t>(isInt.data) == 0);

    amlp::Value isStr = harness.vm.callFunction(ob, "probe_string", {});
    assert(std::get<int64_t>(isStr.data) == 0);

    std::cout << "testFloatpTrueOnlyForFloatNotIntOrString OK\n";
}

// Phase 0 row 0.12 audit: arrayp/functionp/mapp/objectp/pointerp were all
// registered but never called by name anywhere in this suite (stringp,
// intp, floatp, undefinedp, nullp, classp already had real coverage).
// arrayp and pointerp are real aliases of the same lambda (EfunTable.cpp's
// own "int pointerp(mixed) / int arrayp(mixed)" comment) -- both names
// exercised here, not just one.
static void testArrayFunctionMapObjectPointerPredicatesEachTrueOnlyForOwnKind() {
    ObjectVarHarness harness;
    harness.writeFile("/predicate_probe.c",
        "mixed *arr; mapping m; function fn;\n"
        "void create() { arr = ({1,2}); m = ([\"a\":1]); fn = (: lower_case, \"X\" :); }\n"
        "int is_arr(mixed v)  { return arrayp(v); }\n"
        "int is_ptr(mixed v)  { return pointerp(v); }\n"
        "int is_fn(mixed v)   { return functionp(v); }\n"
        "int is_map(mixed v)  { return mapp(v); }\n"
        "int is_obj(mixed v)  { return objectp(v); }\n"
        "mixed get_arr() { return arr; }\n"
        "mixed get_fn()  { return fn; }\n"
        "mixed get_map() { return m; }\n"
        "object get_self() { return this_object(); }\n");
    auto ob = harness.objects.cloneObject("/predicate_probe");
    assert(ob != nullptr);
    auto& vm = harness.vm;

    amlp::Value arr = vm.callFunction(ob, "get_arr", {});
    amlp::Value fn = vm.callFunction(ob, "get_fn", {});
    amlp::Value map = vm.callFunction(ob, "get_map", {});
    amlp::Value self = vm.callFunction(ob, "get_self", {});
    amlp::Value number(static_cast<int64_t>(5));

    // arrayp/pointerp: true for the array, false for everything else.
    assert(std::get<int64_t>(vm.callFunction(ob, "is_arr", {arr}).data) == 1);
    assert(std::get<int64_t>(vm.callFunction(ob, "is_ptr", {arr}).data) == 1);
    assert(std::get<int64_t>(vm.callFunction(ob, "is_arr", {number}).data) == 0);
    assert(std::get<int64_t>(vm.callFunction(ob, "is_ptr", {map}).data) == 0);

    // functionp: true only for the closure.
    assert(std::get<int64_t>(vm.callFunction(ob, "is_fn", {fn}).data) == 1);
    assert(std::get<int64_t>(vm.callFunction(ob, "is_fn", {arr}).data) == 0);

    // mapp: true only for the mapping.
    assert(std::get<int64_t>(vm.callFunction(ob, "is_map", {map}).data) == 1);
    assert(std::get<int64_t>(vm.callFunction(ob, "is_map", {arr}).data) == 0);

    // objectp: true only for the real object reference.
    assert(std::get<int64_t>(vm.callFunction(ob, "is_obj", {self}).data) == 1);
    assert(std::get<int64_t>(vm.callFunction(ob, "is_obj", {number}).data) == 0);

    std::cout << "testArrayFunctionMapObjectPointerPredicatesEachTrueOnlyForOwnKind OK\n";
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

    amlp::Value three = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("ab")), amlp::Value(int64_t{3})});
    assert(std::get<std::string>(three.data) == "ababab");

    amlp::Value zero = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("ab")), amlp::Value(int64_t{0})});
    assert(std::get<std::string>(zero.data) == "");

    amlp::Value negative = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("ab")), amlp::Value(int64_t{-2})});
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

    harness.vm.callFunction(sword, "go", {amlp::Value(room)});
    harness.vm.callFunction(plain, "go", {amlp::Value(room)});

    amlp::Value foundExplicit = harness.vm.callFunction(room, "check",
        {amlp::Value(std::string("sword"))});
    assert(std::get<int64_t>(foundExplicit.data) == 1);

    amlp::Value foundDefault = harness.vm.callFunction(room, "check_default",
        {amlp::Value(std::string("sword"))});
    assert(std::get<int64_t>(foundDefault.data) == 1);

    // An object with no id() at all never matches (VM::callFunction()'s
    // missing-function return is a falsy monostate, not a match).
    amlp::Value notFound = harness.vm.callFunction(room, "check",
        {amlp::Value(std::string("shield"))});
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

    amlp::Value before = harness.vm.callFunction(probe, "before_enable", {});
    assert(std::get<int64_t>(before.data) == 0);

    amlp::Value after = harness.vm.callFunction(probe, "after_enable", {});
    assert(std::get<int64_t>(after.data) == 1);

    // The default (no-argument) form means this_object(), matching
    // func_spec.c's "object default: F__THIS_OBJECT" -- probe's own flag
    // is on from after_enable() above, so this must read 1 too.
    amlp::Value defaultArg = harness.vm.callFunction(probe, "default_arg_after", {});
    assert(std::get<int64_t>(defaultArg.data) == 1);

    // A second, unrelated object never had enable_commands() called on
    // it, so living() on it must read 0 even while probe's own flag is
    // still on -- the flag is per-object, not global.
    amlp::Value bystanderLiving =
        harness.vm.callFunction(probe, "living_of", {amlp::Value(bystander)});
    assert(std::get<int64_t>(bystanderLiving.data) == 0);

    amlp::Value disabled = harness.vm.callFunction(probe, "after_disable", {});
    assert(std::get<int64_t>(disabled.data) == 0);

    std::cout << "testLivingReflectsEnableCommandsStateAndDefaultsToCurrentObject OK\n";
}

// set_hide(int) -- func_spec.c: "void set_hide(int);". Confirmed against
// fluffos-2.9-ds2.08/efuns_main.c's own f_set_hide(): gated on
// master()->valid_hide(current_object) returning truthy before touching
// the flag at all. Picked from the efun-coverage audit's Tier 1
// quick-win list.
static void testSetHideTogglesHiddenFlagWhenMasterValidHidePermits() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_hide(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/sethide_probe1.c",
        "void go_hidden() { set_hide(1); }\n"
        "void go_unhidden() { set_hide(0); }\n");
    auto ob = harness.objects.cloneObject("/sethide_probe1");
    assert(ob != nullptr);

    assert(!ob->isHidden());
    harness.vm.callFunction(ob, "go_hidden", {});
    assert(ob->isHidden());

    // Real f_set_hide() reads the argument fresh each call, not just
    // "set once" -- calling it again with a falsy argument must clear
    // the flag back off.
    harness.vm.callFunction(ob, "go_unhidden", {});
    assert(!ob->isHidden());

    std::cout << "testSetHideTogglesHiddenFlagWhenMasterValidHidePermits OK\n";
}

// Real f_set_hide()'s own "if (!valid_hide(current_object)) { sp--;
// return; }" -- a rejecting master must leave the flag untouched, not
// just skip the driver-side bookkeeping. Also covers the no-master-
// loaded case (VM::masterObject() returning null), which must decline
// the same way, not throw -- confirmed by using the same
// ObjectVarHarness default (master_file: /unused, loadMasterObject()
// never called) every other harness-only test in this file already
// relies on.
static void testSetHideDeclinesWhenValidHideRejectsOrNoMasterLoaded() {
    ObjectVarHarness rejecting;
    rejecting.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_hide(object ob) { return 0; }\n");
    assert(rejecting.objects.loadMasterObject());
    rejecting.writeFile("/sethide_probe2.c", "void go_hidden() { set_hide(1); }\n");
    auto rejectedOb = rejecting.objects.cloneObject("/sethide_probe2");
    assert(rejectedOb != nullptr);
    rejecting.vm.callFunction(rejectedOb, "go_hidden", {});
    assert(!rejectedOb->isHidden());

    ObjectVarHarness noMaster;
    noMaster.writeFile("/sethide_probe3.c", "void go_hidden() { set_hide(1); }\n");
    auto noMasterOb = noMaster.objects.cloneObject("/sethide_probe3");
    assert(noMasterOb != nullptr);
    noMaster.vm.callFunction(noMasterOb, "go_hidden", {}); // must not throw
    assert(!noMasterOb->isHidden());

    std::cout << "testSetHideDeclinesWhenValidHideRejectsOrNoMasterLoaded OK\n";
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
    harness.vm.callFunction(mover, "go", {amlp::Value(room)});

    assert(harness.vm.dispatchCommand(mover, "look at sign") == true);
    amlp::Value arg = harness.vm.callFunction(room, "query_last_arg", {});
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

    amlp::Value verb = harness.vm.callFunction(mover, "query_seen_verb", {});
    assert(std::holds_alternative<std::string>(verb.data));
    assert(std::get<std::string>(verb.data) == "smile");

    amlp::Value arg = harness.vm.callFunction(mover, "query_seen_arg", {});
    assert(std::holds_alternative<std::string>(arg.data));
    assert(std::get<std::string>(arg.data) == "warmly");

    std::cout << "testAddActionCatchAllShortFlagReceivesRemainderAndQueryVerbReturnsFullTypedWord OK\n";
}

// Regression test for a genuine, deep, live-caught bug: a bare
// single-word command (nothing after the verb) must reach its handler
// with an *undefined* argument (this driver's Value{} monostate), never
// an empty string. Confirmed directly against fluffos-2.9-ds2.08's own
// user_parser() (add_action.c): "if (s->flags & V_NOSPACE) {
// copy_and_push_string(...); } else if (buff[length] == ' ') {
// copy_and_push_string(...); } else { push_undefined(); }" -- the
// undefined branch fires whenever there is genuinely nothing after the
// matched word, for both the plain exact-match and V_SHORT cases (only
// V_NOSPACE reslices differently). This was NOT hypothetical: it is
// exactly why a real room's own "look" command produced zero output the
// first time this project's chargen live-testing ever reached an actual
// room. cmds/mortal/_look.c's own cmd_look(str) checks
// "if(stringp(str))" before falling back to "this_player()->
// describe_current_room(1)" -- an empty string passes stringp() (real
// LPC: stringp() checks the type, not truthiness), silently routing a
// bare "look" into examine_object("") instead, which itself declines on
// "if(!str) return 0;", so cmd_look() (and therefore cmd_hook(),
// therefore the whole dispatch) returned falsy with no error, no
// exception, and no dropped connection anywhere in the chain to explain
// it -- confirmed live only by direct C++-level instrumentation of
// dispatchCommand() itself, not from any mudlib-visible symptom.
static void testDispatchCommandPassesUndefinedNotEmptyStringForBareVerbWithNoArgument() {
    ObjectVarHarness harness;
    harness.writeFile("/bv_room.c",
        "int saw_stringp;\n"
        "int saw_call;\n"
        "void init() { add_action(\"cmd_look\", \"look\"); }\n"
        // Mirrors cmds/mortal/_look.c's own cmd_look(str) shape exactly:
        // stringp(str) must be false for a bare "look".
        "int cmd_look(mixed str) {\n"
        "    saw_call = 1;\n"
        "    if(stringp(str)) { saw_stringp = 1; return 0; }\n"
        "    return 1;\n"
        "}\n"
        "int query_saw_stringp() { return saw_stringp; }\n"
        "int query_saw_call() { return saw_call; }\n");
    harness.writeFile("/bv_mover.c", "void go(object dest) { enable_commands(); move_object(dest); }\n");

    auto room = harness.objects.cloneObject("/bv_room");
    auto mover = harness.objects.cloneObject("/bv_mover");
    assert(room != nullptr && mover != nullptr);
    harness.vm.callFunction(mover, "go", {amlp::Value(room)});

    // Bare verb, nothing after it: must NOT look like a string argument.
    assert(harness.vm.dispatchCommand(mover, "look") == true);
    amlp::Value sawCall = harness.vm.callFunction(room, "query_saw_call", {});
    assert(std::get<int64_t>(sawCall.data) == 1);
    amlp::Value sawStringp = harness.vm.callFunction(room, "query_saw_stringp", {});
    assert(std::get<int64_t>(sawStringp.data) == 0);

    // Compound form still gets a real string, unaffected by this fix.
    harness.writeFile("/bv_room2.c",
        "void init() { add_action(\"cmd_look\", \"look\"); }\n"
        "mixed last;\n"
        "int cmd_look(mixed str) { last = str; return 1; }\n"
        "mixed query_last() { return last; }\n");
    auto room2 = harness.objects.cloneObject("/bv_room2");
    auto mover2 = harness.objects.cloneObject("/bv_mover");
    assert(room2 != nullptr && mover2 != nullptr);
    harness.vm.callFunction(mover2, "go", {amlp::Value(room2)});
    assert(harness.vm.dispatchCommand(mover2, "look at sign") == true);
    amlp::Value last = harness.vm.callFunction(room2, "query_last", {});
    assert(std::holds_alternative<std::string>(last.data));
    assert(std::get<std::string>(last.data) == "at sign");

    std::cout << "testDispatchCommandPassesUndefinedNotEmptyStringForBareVerbWithNoArgument OK\n";
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
    harness.vm.callFunction(mover, "go", {amlp::Value(room)});

    // cmd_decline is checked first (most recently added), returns 0;
    // the search must fall through to cmd_accept, not stop there.
    assert(harness.vm.dispatchCommand(mover, "go north") == true);

    std::cout << "testDispatchCommandTriesNextMatchWhenFirstHandlerReturnsFalsy OK\n";
}

// ---------------------------------------------------------------------
// notify_fail(string | function): real add_action.c's own
// f_notify_fail()/notify_no_command(). Sets a pending message on
// command_giver's own connection that is only ever shown if the whole
// rest of dispatch for that input line ends with nothing claiming the
// command -- see Connection.hpp's own pendingNotifyFail_ comment and
// Server::dispatchLine()'s own wiring for the exact real mechanism this
// mirrors. These tests exercise Server::dispatchLine() directly (not
// just VM::dispatchCommand()), since the actual "was the message shown"
// behavior lives at that layer, matching the connect/input-protocol
// tests' own AF_UNIX socketpair pattern just above.

static void testNotifyFailMessageShownWhenNoHandlerClaimsCommand() {
    ObjectVarHarness harness;
    harness.writeFile("/nf_player1.c",
        "void setup() {\n"
        "    enable_commands();\n"
        "    add_action(\"cmd_go\", \"go\");\n"
        "}\n"
        "int cmd_go(string arg) {\n"
        "    notify_fail(\"You can't go that way.\\n\");\n"
        "    return 0;\n"
        "}\n");
    auto player = harness.objects.cloneObject("/nf_player1");
    assert(player != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(player);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(player, "setup", {});
    amlp::Server::dispatchLine(harness.vm, conn, "go north");
    amlp::OutputContext::set(nullptr);

    char buf[256];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    std::string received(buf, static_cast<size_t>(n));
    assert(received == "You can't go that way.\n");

    ::close(fds[1]);
    std::cout << "testNotifyFailMessageShownWhenNoHandlerClaimsCommand OK\n";
}

static void testNotifyFailMessageSuppressedWhenLaterHandlerClaimsCommand() {
    // The exact real add_action fallthrough shape: two handlers
    // registered for the same verb. add_action() always prepends (real
    // add_action.c: "adding to the top of the list doesn't harm
    // anything"), so registering cmd_accept first and cmd_decline
    // second puts cmd_decline at the front, checked first -- it calls
    // notify_fail() and declines, dispatch falls through to cmd_accept,
    // which claims the command without ever touching notify_fail()
    // itself. Real user_parser(): a truthy return is an immediate
    // "return 1", notify_no_command() is never reached, so whatever
    // cmd_decline set must never surface.
    ObjectVarHarness harness;
    harness.writeFile("/nf_player2.c",
        "void setup() {\n"
        "    enable_commands();\n"
        "    add_action(\"cmd_accept\", \"go\");\n"
        "    add_action(\"cmd_decline\", \"go\");\n"
        "}\n"
        "int cmd_decline(string arg) {\n"
        "    notify_fail(\"You can't go that way.\\n\");\n"
        "    return 0;\n"
        "}\n"
        "int cmd_accept(string arg) {\n"
        "    write(\"You go \" + arg + \".\\n\");\n"
        "    return 1;\n"
        "}\n");
    auto player = harness.objects.cloneObject("/nf_player2");
    assert(player != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(player);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(player, "setup", {});
    amlp::Server::dispatchLine(harness.vm, conn, "go north");
    amlp::OutputContext::set(nullptr);

    char buf[256];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    std::string received(buf, static_cast<size_t>(n));
    // Only cmd_accept's own write() must appear -- cmd_decline's
    // notify_fail() message must never have been sent.
    assert(received == "You go north.\n");

    ::close(fds[1]);
    std::cout << "testNotifyFailMessageSuppressedWhenLaterHandlerClaimsCommand OK\n";
}

static void testNotifyFailDoesNotLeakIntoALaterUnrelatedFailedDispatch() {
    // Real clear_notify(): runs unconditionally at the start of every
    // new input line's own dispatch. A message set (and shown) for one
    // line must not reappear for a later, unrelated line that never
    // calls notify_fail() itself -- and since this driver deliberately
    // does not inject real notify_no_command()'s own hardcoded "What?\n"
    // default (see Server::dispatchLine()'s own comment), the second
    // dispatch must produce no output at all.
    ObjectVarHarness harness;
    harness.writeFile("/nf_player3.c",
        "void setup() {\n"
        "    enable_commands();\n"
        "    add_action(\"cmd_go\", \"go\");\n"
        "}\n"
        "int cmd_go(string arg) {\n"
        "    notify_fail(\"You can't go that way.\\n\");\n"
        "    return 0;\n"
        "}\n");
    auto player = harness.objects.cloneObject("/nf_player3");
    assert(player != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(player);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(player, "setup", {});
    amlp::Server::dispatchLine(harness.vm, conn, "go north");

    char buf[256];
    ssize_t n1 = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n1 > 0);
    assert(std::string(buf, static_cast<size_t>(n1)) == "You can't go that way.\n");

    // A completely different, unrecognized verb: no action matches at
    // all, and cmd_go (the only thing that ever calls notify_fail()) is
    // never reached this time.
    amlp::Server::dispatchLine(harness.vm, conn, "xyzzy");
    amlp::OutputContext::set(nullptr);

    ssize_t n2 = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n2 <= 0); // nothing sent: no stale leak, no hardcoded default either

    ::close(fds[1]);
    std::cout << "testNotifyFailDoesNotLeakIntoALaterUnrelatedFailedDispatch OK\n";
}

static void testNotifyFailFunctionFormShowsOnlyAStringReturn() {
    // Real spec: "void notify_fail(string | function);" -- confirmed
    // against func_spec.c directly, not assumed to be string-only. Real
    // notify_no_command(): calls the function with no arguments and
    // shows its return value only if that return is itself a string; a
    // non-string return (e.g. plain 0) shows nothing at all.
    ObjectVarHarness harness;
    harness.writeFile("/nf_player4.c",
        "int mode;\n"
        "void setup() {\n"
        "    enable_commands();\n"
        "    add_action(\"cmd_go\", \"go\");\n"
        "}\n"
        "void set_mode(int m) { mode = m; }\n"
        "mixed fail_message() { return mode ? \"Dynamic failure.\\n\" : 0; }\n"
        "int cmd_go(string arg) {\n"
        "    notify_fail((: fail_message :));\n"
        "    return 0;\n"
        "}\n");
    auto player = harness.objects.cloneObject("/nf_player4");
    assert(player != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(player);

    // mode=0: fail_message() returns plain 0, nothing should be shown.
    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(player, "setup", {});
    amlp::Server::dispatchLine(harness.vm, conn, "go north");
    amlp::OutputContext::set(nullptr);

    char buf[256];
    ssize_t n1 = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n1 <= 0);

    // mode=1: fail_message() returns a real string, must be shown.
    harness.vm.callFunction(player, "set_mode", {amlp::Value(static_cast<int64_t>(1))});
    amlp::OutputContext::set(&conn);
    amlp::Server::dispatchLine(harness.vm, conn, "go north");
    amlp::OutputContext::set(nullptr);

    ssize_t n2 = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n2 > 0);
    assert(std::string(buf, static_cast<size_t>(n2)) == "Dynamic failure.\n");

    ::close(fds[1]);
    std::cout << "testNotifyFailFunctionFormShowsOnlyAStringReturn OK\n";
}

static void testNotifyFailThrowsOnNonStringNonFunctionArgument() {
    ObjectVarHarness harness;
    harness.writeFile("/nf_probe5.c",
        "void probe() { notify_fail(42); }\n");
    auto probe = harness.objects.cloneObject("/nf_probe5");
    assert(probe != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(probe, "probe", {});
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("notify_fail") != std::string::npos);
    }
    assert(threw);

    std::cout << "testNotifyFailThrowsOnNonStringNonFunctionArgument OK\n";
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
    harness.vm.callFunction(mover, "go", {amlp::Value(room)});

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
    harness.vm.callFunction(mover2, "go", {amlp::Value(room2)});

    assert(harness.vm.dispatchCommand(mover2, "whoami") == true);
    amlp::Value caller = harness.vm.callFunction(room2, "query_last_caller", {});
    auto* callerPtr = std::get_if<std::shared_ptr<amlp::LpcObject>>(&caller.data);
    assert(callerPtr != nullptr && *callerPtr == mover2);

    std::cout << "testThisPlayerReturnsCommandGiverDuringDispatch OK\n";
}

static void testQueryVerbReturnsZeroOutsideOfDispatch() {
    ObjectVarHarness harness;
    harness.writeFile("/qv_lone.c", "mixed probe() { return query_verb(); }\n");
    auto ob = harness.objects.cloneObject("/qv_lone");
    assert(ob != nullptr);
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
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
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
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

    harness.vm.callFunction(ob, "set_it", {amlp::Value(std::string("wizards:thurtea"))});
    amlp::Value set = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::string>(set.data));
    assert(std::get<std::string>(set.data) == "wizards:thurtea");

    harness.vm.callFunction(ob, "clear_it", {});
    amlp::Value cleared = harness.vm.callFunction(ob, "probe", {});
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

    amlp::Value x = harness.vm.callFunction(ob, "query_x", {});
    assert(std::holds_alternative<int64_t>(x.data));
    assert(std::get<int64_t>(x.data) == 5);

    // create() read the already-initialized value, not 0/void -- proves
    // ordering, not just that the initializer eventually ran at all.
    amlp::Value seen = harness.vm.callFunction(ob, "query_seen_at_create", {});
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
    amlp::Value count = harness.vm.callFunction(ob, "query_count", {});
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

    amlp::Value tag = harness.vm.callFunction(ob, "query_tag", {});
    assert(std::holds_alternative<std::string>(tag.data));
    assert(std::get<std::string>(tag.data) == "parent");

    // child_tag's own initializer read the parent's already-initialized
    // "tag" -- proves the parent's own "$objvarinit" ran first.
    amlp::Value childTag = harness.vm.callFunction(ob, "query_child_tag", {});
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
    amlp::Value result = runProbe(
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

    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
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
    amlp::Value result = harness.vm.callFunction(obj, "probe", {});
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe(
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
    amlp::Value result = runProbe("return \"count:\" + 42;\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "count:42");
    std::cout << "testStringPlusIntAppendsDecimalDigits OK\n";
}

static void testIntPlusStringPrependsDecimalDigits() {
    amlp::Value result = runProbe("return 42 + \":count\";\n");
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
    amlp::Value result = runProbe(
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
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "/obj_add_test::tail");
    std::cout << "testObjectPlusStringPrependsItsFilename OK\n";
}

// int random(int) -- confirmed against real efuns_main.c's f_random():
// n <= 0 always yields plain 0 (not an error), and the result is always
// in [0, n). Surfaced live: domains/Praxis/setter.c's own roll_d6()
// (Palladium 3d6 attribute rolling).
static void testRandomOfNonPositiveArgumentIsZero() {
    amlp::Value result = runProbe("return random(0) + random(-5);\n");
    assert(std::holds_alternative<int64_t>(result.data));
    assert(std::get<int64_t>(result.data) == 0);
    std::cout << "testRandomOfNonPositiveArgumentIsZero OK\n";
}

static void testRandomStaysWithinZeroToNExclusiveAcrossManyDraws() {
    amlp::Value result = runProbe(
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

    amlp::Value before = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(before.data));
    assert(std::get<int64_t>(before.data) == 0);

    harness.vm.callFunction(ob, "enable_it", {});
    amlp::Value after = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(after.data));
    assert(std::get<int64_t>(after.data) == 1);

    harness.vm.callFunction(ob, "disable_it", {});
    amlp::Value cleared = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<int64_t>(cleared.data));
    assert(std::get<int64_t>(cleared.data) == 0);

    std::cout << "testSetHeartBeatThenQueryHeartBeatRoundTrips OK\n";
}

// ---------------------------------------------------------------------
// Real Scheduler: call_out registration/firing/removal, and heart_beat
// enable/disable with correct per-interval cadence. Design grounded
// directly in fluffos-2.9-ds2.08's own call_out.c/backend.c -- see
// Scheduler.hpp/.cpp's own comments for the citations. dueAt is an
// absolute steady_clock time_point (see CallOutEntry), so a "simulated
// due time" test just schedules with a due time already in the past
// (or a handful of milliseconds out) rather than actually sleeping for
// whole real seconds.
// ---------------------------------------------------------------------

static void testCallOutFiresOnceDueTimeArrivesWithExtraArgsInOrder() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/co_fire.c",
        "int fired;\n"
        "int a; int b;\n"
        "void tick(int x, int y) { fired = fired + 1; a = x; b = y; }\n"
        "int query_fired() { return fired; }\n");
    auto obj = harness.objects.cloneObject("/co_fire");
    assert(obj != nullptr);

    amlp::CallOutEntry entry;
    entry.target = obj;
    entry.function = "tick";
    entry.args = {amlp::Value(int64_t{7}), amlp::Value(int64_t{9})};
    entry.dueAt = std::chrono::steady_clock::now() - std::chrono::seconds(1); // already due
    scheduler.addCallOut(std::move(entry));

    // Not due yet -> tickCallOuts() must not fire it early. Re-check with
    // a still-pending, not-yet-due entry for the negative case.
    amlp::CallOutEntry notDue;
    notDue.target = obj;
    notDue.function = "tick";
    notDue.args = {amlp::Value(int64_t{0}), amlp::Value(int64_t{0})};
    notDue.dueAt = std::chrono::steady_clock::now() + std::chrono::hours(1);
    scheduler.addCallOut(std::move(notDue));

    scheduler.tickCallOuts();

    amlp::Value fired = harness.vm.callFunction(obj, "query_fired", {});
    assert(std::get<int64_t>(fired.data) == 1); // only the due one fired

    amlp::Value aVal = harness.vm.callFunction(obj, "query_fired", {}); // sanity: object still alive
    (void)aVal;

    std::cout << "testCallOutFiresOnceDueTimeArrivesWithExtraArgsInOrder OK\n";
}

static void testCallOutSelfReschedulingSurvivesTickIteration() {
    // Real call_out.c's own main loop moves a due entry out of the
    // pending list *before* invoking it -- otherwise a call_out whose
    // body reschedules itself (the dominant repeating-timer idiom this
    // mudlib uses, e.g. std/user.c's own rifts_regen_tick()) would
    // corrupt whatever is being iterated. This exercises exactly that
    // shape end to end through the real call_out() efun.
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/co_resched.c",
        "int ticks;\n"
        "void tick() { ticks = ticks + 1; call_out(\"tick\", 0); }\n"
        "int query_ticks() { return ticks; }\n");
    auto obj = harness.objects.cloneObject("/co_resched");
    assert(obj != nullptr);

    harness.vm.callFunction(obj, "tick", {}); // seed: fires once directly, schedules the next
    scheduler.tickCallOuts(); // fires the rescheduled one, which reschedules again
    scheduler.tickCallOuts();

    amlp::Value ticks = harness.vm.callFunction(obj, "query_ticks", {});
    assert(std::get<int64_t>(ticks.data) == 3);

    std::cout << "testCallOutSelfReschedulingSurvivesTickIteration OK\n";
}

static void testCallOutClosureFormFiresViaCallClosureNotCallFunction() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    // daemon/services.c's own "call_out((: eventCompactUcache :), 3600)"
    // shape: a bare-name closure, no bound args.
    harness.writeFile("/co_closure.c",
        "int fired;\n"
        "void mark() { fired = 1; }\n"
        "void go() { call_out((: mark :), 0); }\n"
        "int query_fired() { return fired; }\n");
    auto obj = harness.objects.cloneObject("/co_closure");
    assert(obj != nullptr);

    harness.vm.callFunction(obj, "go", {});
    scheduler.tickCallOuts();

    amlp::Value fired = harness.vm.callFunction(obj, "query_fired", {});
    assert(std::get<int64_t>(fired.data) == 1);

    std::cout << "testCallOutClosureFormFiresViaCallClosureNotCallFunction OK\n";
}

static void testRemoveCallOutByHandlePreventsFiringAndReturnsRemainingSeconds() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/co_rm_handle.c",
        "int fired;\n"
        "void tick() { fired = 1; }\n"
        "int schedule() { return call_out(\"tick\", 60); }\n"
        "int query_fired() { return fired; }\n");
    auto obj = harness.objects.cloneObject("/co_rm_handle");
    assert(obj != nullptr);

    amlp::Value handle = harness.vm.callFunction(obj, "schedule", {});
    int64_t h = std::get<int64_t>(handle.data);

    int64_t remaining = scheduler.removeCallOutByHandle(h);
    assert(remaining >= 0); // a real ~60s-out entry, not "not found"
    assert(scheduler.removeCallOutByHandle(h) == -1); // already gone, second removal finds nothing

    scheduler.tickCallOuts(); // nothing due, and the removed one can't fire regardless
    amlp::Value fired = harness.vm.callFunction(obj, "query_fired", {});
    assert(std::get<int64_t>(fired.data) == 0);

    std::cout << "testRemoveCallOutByHandlePreventsFiringAndReturnsRemainingSeconds OK\n";
}

static void testRemoveCallOutByNameIsScopedToCallingObjectAndSkipsClosures() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    // Two distinct objects each with their own pending "tick", plus a
    // closure-bound entry under the same function name on the first
    // object -- real remove_call_out(object_t*, const char*) only ever
    // matches entries with a real (owner, name) pair; a closure-bound
    // entry's own "ob" is never set for the string form, so it can never
    // match a name-based removal (see Scheduler::removeCallOutByName()'s
    // own comment).
    harness.writeFile("/co_rm_name.c",
        "int fired;\n"
        "void tick() { fired = fired + 1; }\n"
        "void schedule() { call_out(\"tick\", 60); }\n"
        "void schedule_closure() { call_out((: tick :), 60); }\n"
        "int remove_it() { return remove_call_out(\"tick\"); }\n"
        "int query_fired() { return fired; }\n");
    auto a = harness.objects.cloneObject("/co_rm_name");
    auto b = harness.objects.cloneObject("/co_rm_name");
    assert(a != nullptr && b != nullptr && a != b);

    harness.vm.callFunction(a, "schedule", {});
    harness.vm.callFunction(a, "schedule_closure", {});
    harness.vm.callFunction(b, "schedule", {});

    // Removing "tick" from a only removes a's own string-form entry --
    // not b's, and not a's own closure-bound entry either.
    amlp::Value removed = harness.vm.callFunction(a, "remove_it", {});
    assert(std::get<int64_t>(removed.data) != -1);
    // A second removal on a finds nothing left under the string form.
    amlp::Value removedAgain = harness.vm.callFunction(a, "remove_it", {});
    assert(std::get<int64_t>(removedAgain.data) == -1);
    // b's own entry is untouched.
    assert(scheduler.findCallOutByName(b, "tick") != -1);

    std::cout << "testRemoveCallOutByNameIsScopedToCallingObjectAndSkipsClosures OK\n";
}

static void testFindCallOutReturnsRemainingSecondsOrMinusOneWithoutRemoving() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/co_find.c",
        "void tick() {}\n"
        "int schedule() { return call_out(\"tick\", 45); }\n"
        "int check() { return find_call_out(\"tick\"); }\n");
    auto obj = harness.objects.cloneObject("/co_find");
    assert(obj != nullptr);

    amlp::Value beforeSchedule = harness.vm.callFunction(obj, "check", {});
    assert(std::get<int64_t>(beforeSchedule.data) == -1);

    harness.vm.callFunction(obj, "schedule", {});
    amlp::Value afterSchedule = harness.vm.callFunction(obj, "check", {});
    assert(std::get<int64_t>(afterSchedule.data) != -1);

    // find does not remove -- checking again still finds it.
    amlp::Value stillThere = harness.vm.callFunction(obj, "check", {});
    assert(std::get<int64_t>(stillThere.data) != -1);

    std::cout << "testFindCallOutReturnsRemainingSecondsOrMinusOneWithoutRemoving OK\n";
}

static void testCallOutRuntimeErrorIsIsolatedFromOtherPendingCallOuts() {
    // Matches real SETJMP/restore_context recovery in call_out.c's own
    // main loop, and this driver's own already-established per-object
    // error-isolation convention: one call_out throwing must not stop
    // the rest of that same tick's due entries from firing.
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/co_throw.c",
        "void boom() { destructed_only_function_that_does_not_exist(); }\n");
    harness.writeFile("/co_ok.c",
        "int fired;\n"
        "void tick() { fired = 1; }\n"
        "int query_fired() { return fired; }\n");
    auto thrower = harness.objects.cloneObject("/co_throw");
    auto ok = harness.objects.cloneObject("/co_ok");
    assert(thrower != nullptr && ok != nullptr);

    auto due = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    amlp::CallOutEntry boom;
    boom.target = thrower;
    boom.function = "boom";
    boom.dueAt = due;
    scheduler.addCallOut(std::move(boom));

    amlp::CallOutEntry good;
    good.target = ok;
    good.function = "tick";
    good.dueAt = due;
    scheduler.addCallOut(std::move(good));

    scheduler.tickCallOuts(); // must not throw out of this call

    amlp::Value fired = harness.vm.callFunction(ok, "query_fired", {});
    assert(std::get<int64_t>(fired.data) == 1);

    std::cout << "testCallOutRuntimeErrorIsIsolatedFromOtherPendingCallOuts OK\n";
}

static void testCallOutSkipsDestructedTargetSilently() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/co_destruct_target.c", "void tick() {}\n");
    auto obj = harness.objects.cloneObject("/co_destruct_target");
    assert(obj != nullptr);

    amlp::CallOutEntry entry;
    entry.target = obj; // weak_ptr
    entry.function = "tick";
    entry.dueAt = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    scheduler.addCallOut(std::move(entry));

    harness.vm.destructObject(obj);
    obj.reset(); // drop the last owning reference -- weak_ptr now expired

    scheduler.tickCallOuts(); // must not throw or crash on the dangling entry

    std::cout << "testCallOutSkipsDestructedTargetSilently OK\n";
}

static void testSetHeartBeatIntervalFiresOnceEveryNCyclesNotEveryCycle() {
    // Real backend.c: set_heart_beat(ob, to) with to > 1 means "fire once
    // every to heartbeat cycles", not every cycle -- confirmed live-
    // needed by std/germ.c's own set_heart_beat(5). tickHeartbeats() is
    // called directly here (bypassing the real-time gate that lives in
    // Scheduler::run(), see its own comment) so each call deterministically
    // represents exactly one cycle.
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/hb_interval.c",
        "int beats;\n"
        "void enable() { set_heart_beat(3); }\n"
        "void heart_beat() { beats = beats + 1; }\n"
        "int query_beats() { return beats; }\n"
        "int query_interval() { return query_heart_beat(this_object()); }\n");
    auto obj = harness.objects.cloneObject("/hb_interval");
    assert(obj != nullptr);

    harness.vm.callFunction(obj, "enable", {});
    amlp::Value interval = harness.vm.callFunction(obj, "query_interval", {});
    assert(std::get<int64_t>(interval.data) == 3); // the real configured value, not a bare 1

    scheduler.tickHeartbeats(); // cycle 1 of 3
    scheduler.tickHeartbeats(); // cycle 2 of 3
    amlp::Value beforeThird = harness.vm.callFunction(obj, "query_beats", {});
    assert(std::get<int64_t>(beforeThird.data) == 0); // not yet -- only 2 of 3 cycles elapsed

    scheduler.tickHeartbeats(); // cycle 3 of 3 -- fires
    amlp::Value afterThird = harness.vm.callFunction(obj, "query_beats", {});
    assert(std::get<int64_t>(afterThird.data) == 1);

    scheduler.tickHeartbeats(); // cycle 1 of the next period
    scheduler.tickHeartbeats(); // cycle 2
    amlp::Value stillOne = harness.vm.callFunction(obj, "query_beats", {});
    assert(std::get<int64_t>(stillOne.data) == 1); // countdown correctly reset to 3, not left at 0

    std::cout << "testSetHeartBeatIntervalFiresOnceEveryNCyclesNotEveryCycle OK\n";
}

static void testSetHeartBeatZeroDisablesAndStopsFutureFiring() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/hb_disable.c",
        "int beats;\n"
        "void enable() { set_heart_beat(1); }\n"
        "void disable() { set_heart_beat(0); }\n"
        "void heart_beat() { beats = beats + 1; }\n"
        "int query_beats() { return beats; }\n");
    auto obj = harness.objects.cloneObject("/hb_disable");
    assert(obj != nullptr);

    harness.vm.callFunction(obj, "enable", {});
    scheduler.tickHeartbeats();
    amlp::Value once = harness.vm.callFunction(obj, "query_beats", {});
    assert(std::get<int64_t>(once.data) == 1);

    harness.vm.callFunction(obj, "disable", {});
    scheduler.tickHeartbeats();
    scheduler.tickHeartbeats();
    amlp::Value stillOnce = harness.vm.callFunction(obj, "query_beats", {});
    assert(std::get<int64_t>(stillOnce.data) == 1); // disabled -- no further firing

    std::cout << "testSetHeartBeatZeroDisablesAndStopsFutureFiring OK\n";
}

static void testHeartbeatRuntimeErrorIsolatedFromOtherHeartbeatEnabledObjects() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/hb_throw.c",
        "void enable() { set_heart_beat(1); }\n"
        "void heart_beat() { undefined_function_boom(); }\n");
    harness.writeFile("/hb_ok.c",
        "int beats;\n"
        "void enable() { set_heart_beat(1); }\n"
        "void heart_beat() { beats = beats + 1; }\n"
        "int query_beats() { return beats; }\n");
    auto thrower = harness.objects.cloneObject("/hb_throw");
    auto ok = harness.objects.cloneObject("/hb_ok");
    assert(thrower != nullptr && ok != nullptr);

    harness.vm.callFunction(thrower, "enable", {});
    harness.vm.callFunction(ok, "enable", {});

    scheduler.tickHeartbeats(); // must not throw out of this call

    amlp::Value beats = harness.vm.callFunction(ok, "query_beats", {});
    assert(std::get<int64_t>(beats.data) == 1);

    std::cout << "testHeartbeatRuntimeErrorIsolatedFromOtherHeartbeatEnabledObjects OK\n";
}

static void testHeartbeatPrunesDestructedObjectSilently() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/hb_destruct.c",
        "void enable() { set_heart_beat(1); }\n"
        "void heart_beat() {}\n");
    auto obj = harness.objects.cloneObject("/hb_destruct");
    assert(obj != nullptr);

    harness.vm.callFunction(obj, "enable", {});
    harness.vm.destructObject(obj);
    obj.reset();

    scheduler.tickHeartbeats(); // must not throw or crash on the dangling entry
    scheduler.tickHeartbeats();

    std::cout << "testHeartbeatPrunesDestructedObjectSilently OK\n";
}

// Regression test for a genuine, live-caught crash: heart_beat() calling
// set_heart_beat() on itself (real std/user.c's own "if(!interactive(
// this_object())) { set_heart_beat(0); return; }") re-enters
// Scheduler::setHeartbeatInterval(), which mutates heartbeats_ via
// erase()/find_if(). An earlier version of tickHeartbeats() held a live
// iterator into heartbeats_ across the heart_beat() call itself and
// segfaulted in vector::erase() the moment a real character reached a
// room live (caught during this slice's own Step 4 live-verification
// pass, not written speculatively). Covers both directions: an object
// disabling itself, and one heart_beat() disabling a *different* still-
// pending object earlier in the same cycle's snapshot.
static void testHeartbeatCallingSetHeartBeatOnItselfDoesNotCorruptIteration() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/hb_reentrant.c",
        "int beats;\n"
        "void enable() { set_heart_beat(1); }\n"
        "void heart_beat() { beats = beats + 1; set_heart_beat(0); }\n"
        "int query_beats() { return beats; }\n");
    harness.writeFile("/hb_bystander.c",
        "int beats;\n"
        "void enable() { set_heart_beat(1); }\n"
        "void heart_beat() { beats = beats + 1; }\n"
        "int query_beats() { return beats; }\n");
    auto reentrant = harness.objects.cloneObject("/hb_reentrant");
    auto bystander = harness.objects.cloneObject("/hb_bystander");
    assert(reentrant != nullptr && bystander != nullptr);

    harness.vm.callFunction(reentrant, "enable", {});
    harness.vm.callFunction(bystander, "enable", {});

    scheduler.tickHeartbeats(); // must not crash: reentrant's own heart_beat() disables itself mid-cycle

    amlp::Value reentrantBeats = harness.vm.callFunction(reentrant, "query_beats", {});
    assert(std::get<int64_t>(reentrantBeats.data) == 1);
    amlp::Value bystanderBeats = harness.vm.callFunction(bystander, "query_beats", {});
    assert(std::get<int64_t>(bystanderBeats.data) == 1); // unaffected by reentrant's own disable

    // reentrant is now disabled; a further cycle only fires bystander.
    scheduler.tickHeartbeats();
    amlp::Value reentrantAfter = harness.vm.callFunction(reentrant, "query_beats", {});
    assert(std::get<int64_t>(reentrantAfter.data) == 1); // unchanged
    amlp::Value bystanderAfter = harness.vm.callFunction(bystander, "query_beats", {});
    assert(std::get<int64_t>(bystanderAfter.data) == 2);

    std::cout << "testHeartbeatCallingSetHeartBeatOnItselfDoesNotCorruptIteration OK\n";
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(target)});
    auto* arrPtr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(target)});
    auto* arrPtr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
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

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(target)});
    auto* arrPtr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
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
    amlp::Value result = runProbe(
        "string *words;\n"
        "words = ({ \"go\", \"north\" });\n"
        "return implode(words, \" \");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "go north");
    std::cout << "testImplodeJoinsStringArrayWithSeparator OK\n";
}

static void testImplodeOnEmptyArrayReturnsEmptyString() {
    amlp::Value result = runProbe("return implode(({}), \" \");\n");
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
    amlp::Value result = runProbe("return sprintf(\"%c[31m\", 27);\n");
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
    } catch (const amlp::LpcRuntimeError&) {
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
    amlp::Value result = runProbe("return sprintf(\"[%-3d]\", 7);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[7  ]");
    std::cout << "testSprintfLeftJustifiedFieldWidthPadsWithSpaces OK\n";
}

static void testSprintfRightJustifiedFieldWidthPadsWithSpaces() {
    amlp::Value result = runProbe("return sprintf(\"[%3d]\", 7);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[  7]");
    std::cout << "testSprintfRightJustifiedFieldWidthPadsWithSpaces OK\n";
}

static void testSprintfZeroPaddedFieldWidthPadsWithZeros() {
    amlp::Value result = runProbe("return sprintf(\"[%03d]\", 7);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[007]");
    std::cout << "testSprintfZeroPaddedFieldWidthPadsWithZeros OK\n";
}

static void testSprintfFieldWidthDoesNotTruncateAWiderValue() {
    amlp::Value result = runProbe("return sprintf(\"[%-3d]\", 12345);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[12345]");
    std::cout << "testSprintfFieldWidthDoesNotTruncateAWiderValue OK\n";
}

static void testSprintfCentreJustifiedFieldWidthSplitsPaddingEvenly() {
    // real sprintf.c's own add_justified(): "abc" in a field of 9 pads
    // 6 total, 3 on each side when the padding divides evenly -- the
    // exact shape secure/SimulEfun/misc.c's own dump_socket_status()
    // uses ("%|9s").
    amlp::Value result = runProbe("return sprintf(\"[%|9s]\", \"abc\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[   abc   ]");
    std::cout << "testSprintfCentreJustifiedFieldWidthSplitsPaddingEvenly OK\n";
}

static void testSprintfCentreJustifiedFieldWidthPutsExtraPadOnTheLeft() {
    // real add_justified(): "i = fs / 2 + fs % 2" -- when the padding is
    // odd, the extra character goes on the leading side, not trailing.
    amlp::Value result = runProbe("return sprintf(\"[%|5s]\", \"ab\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[  ab ]");
    std::cout << "testSprintfCentreJustifiedFieldWidthPutsExtraPadOnTheLeft OK\n";
}

static void testSprintfStringFieldWidthLeftJustifies() {
    amlp::Value result = runProbe("return sprintf(\"[%-5s]\", \"ab\");\n");
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
    amlp::Value result = runProbe("return sprintf(\"100%%\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "100%");
    std::cout << "testSprintfDoublePercentEmitsLiteralPercentAndConsumesNoArgument OK\n";
}

static void testSprintfColonFieldWidthPadsAShorterStringLeftJustified() {
    amlp::Value result = runProbe("return sprintf(\"[%:-5s]\", \"ab\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[ab   ]");
    std::cout << "testSprintfColonFieldWidthPadsAShorterStringLeftJustified OK\n";
}

static void testSprintfColonFieldWidthTruncatesALongerString() {
    amlp::Value result = runProbe("return sprintf(\"[%:-5s]\", \"abcdefgh\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[abcde]");
    std::cout << "testSprintfColonFieldWidthTruncatesALongerString OK\n";
}

// The exact live shape: build "%:-Ns" via a first sprintf("%%:-%ds", x)
// call, then use the result as a real format string in a second call.
static void testSprintfBuildingAndThenUsingADynamicColonFormatString() {
    amlp::Value result = runProbe(
        "string fmt;\n"
        "fmt = sprintf(\"%%:-%ds\", 6);\n"
        "return sprintf(fmt, \"hi\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "hi    ");
    std::cout << "testSprintfBuildingAndThenUsingADynamicColonFormatString OK\n";
}

// "%o"/"%x", "."-precision, and "*" dynamic width/precision -- general
// LPC-compliance additions confirmed against fluffos-2.9-ds2.08/sprintf.c
// directly (its own doc comment and field-size/precision parsing loop),
// not driven by a new real call site on this mudlib's own boot path.

static void testSprintfPercentXEmitsLowercaseHex() {
    amlp::Value result = runProbe("return sprintf(\"%x\", 255);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "ff");
    std::cout << "testSprintfPercentXEmitsLowercaseHex OK\n";
}

static void testSprintfPercentXThrowsOnNonIntArgument() {
    bool threw = false;
    try {
        runProbe("return sprintf(\"%x\", \"z\");\n");
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testSprintfPercentXThrowsOnNonIntArgument OK\n";
}

static void testSprintfPercentOEmitsOctal() {
    amlp::Value result = runProbe("return sprintf(\"%o\", 8);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "10");
    std::cout << "testSprintfPercentOEmitsOctal OK\n";
}

// sprintf()/printf()'s "%O" specifier -- LPC's generic value-dump format.
// Every case here is checked against real fluffos-2.9-ds2.08/sprintf.c's
// own svalue_to_string() directly (see EfunTable.cpp's own
// valueToDebugString() comment for the full citation trail), not
// guessed from the specifier's general reputation. Found live blocking
// Lil's own "eval" command (mudlib/command/eval.c's own "printf(
// \"Result = %O\n\", ...)"), the last real gap from that session's own
// end-to-end login work.

static void testSprintfPercentODumpsIntFloatAndString() {
    amlp::Value intResult = runProbe("return sprintf(\"%O\", 42);\n");
    assert(std::get<std::string>(intResult.data) == "42");

    amlp::Value negResult = runProbe("return sprintf(\"%O\", -7);\n");
    assert(std::get<std::string>(negResult.data) == "-7");

    // Real T_REAL: plain C "%f", six decimal places.
    amlp::Value floatResult = runProbe("return sprintf(\"%O\", 3.5);\n");
    assert(std::get<std::string>(floatResult.data) == "3.500000");

    // Real T_STRING: wrapped in literal double quotes, no escaping.
    amlp::Value strResult = runProbe("return sprintf(\"%O\", \"hi\");\n");
    assert(std::get<std::string>(strResult.data) == "\"hi\"");

    std::cout << "testSprintfPercentODumpsIntFloatAndString OK\n";
}

static void testSprintfPercentODumpsEmptyAndNonEmptyArrayWithNesting() {
    amlp::Value emptyResult = runProbe("return sprintf(\"%O\", ({}));\n");
    assert(std::get<std::string>(emptyResult.data) == "({ })");

    amlp::Value flatResult = runProbe("return sprintf(\"%O\", ({1, 2, 3}));\n");
    assert(std::get<std::string>(flatResult.data) ==
           "({ /* sizeof() == 3 */\n  1,\n  2,\n  3\n})");

    // Nested array: inner elements indent two spaces deeper than their
    // own containing array, real svalue_to_string()'s own recursive
    // "indent + 2" -- confirmed here, not just at the top level.
    amlp::Value nestedResult = runProbe("return sprintf(\"%O\", ({1, ({2, 3})}));\n");
    assert(std::get<std::string>(nestedResult.data) ==
           "({ /* sizeof() == 2 */\n"
           "  1,\n"
           "  ({ /* sizeof() == 2 */\n"
           "    2,\n"
           "    3\n"
           "  })\n"
           "})");

    std::cout << "testSprintfPercentODumpsEmptyAndNonEmptyArrayWithNesting OK\n";
}

static void testSprintfPercentODumpsEmptyAndNonEmptyMapping() {
    amlp::Value emptyResult = runProbe("return sprintf(\"%O\", ([]));\n");
    assert(std::get<std::string>(emptyResult.data) == "([ ])");

    // Real T_MAPPING: every entry (not just non-last ones) gets its own
    // trailing ",\n" -- confirmed directly, a real asymmetry versus
    // T_ARRAY's own "last element has no trailing comma" rule.
    amlp::Value oneResult = runProbe("return sprintf(\"%O\", ([\"a\": 1]));\n");
    assert(std::get<std::string>(oneResult.data) ==
           "([ /* sizeof() == 1 */\n  \"a\" : 1,\n])");

    std::cout << "testSprintfPercentODumpsEmptyAndNonEmptyMapping OK\n";
}

static void testSprintfPercentODumpsObjectAndDestructedObjectAsZero() {
    ObjectVarHarness harness;
    harness.writeFile("/pctO_target.c", "void create() {}\n");
    harness.writeFile("/pctO_probe.c",
        "string probe(object ob) { return sprintf(\"%O\", ob); }\n");
    auto target = harness.objects.cloneObject("/pctO_target");
    auto probe = harness.objects.cloneObject("/pctO_probe");
    assert(target != nullptr && probe != nullptr);

    amlp::Value liveResult = harness.vm.callFunction(probe, "probe", {amlp::Value(target)});
    assert(std::get<std::string>(liveResult.data) == "/" + target->filename());

    harness.objects.destructObject(target);
    amlp::Value destructedResult = harness.vm.callFunction(probe, "probe", {amlp::Value(target)});
    assert(std::get<std::string>(destructedResult.data) == "0");

    std::cout << "testSprintfPercentODumpsObjectAndDestructedObjectAsZero OK\n";
}

static void testSprintfPercentODumpsClosureWithBoundArgs() {
    amlp::Value noArgsResult = runProbe("return sprintf(\"%O\", (: lower_case :));\n");
    assert(std::get<std::string>(noArgsResult.data) == "(: lower_case :)");

    amlp::Value boundResult = runProbe("return sprintf(\"%O\", (: lower_case, \"HI\" :));\n");
    assert(std::get<std::string>(boundResult.data) == "(: lower_case, \"HI\" :)");

    std::cout << "testSprintfPercentODumpsClosureWithBoundArgs OK\n";
}

static void testSprintfPercentOFieldWidthAndPrecisionApplyLikePercentS() {
    // Real sprintf.c converts %O's own dump into an ordinary %s-typed
    // string immediately after building it -- field width/left-justify/
    // precision-truncation all apply exactly like %s, confirmed here.
    amlp::Value widthResult = runProbe("return sprintf(\"[%-6O]\", 42);\n");
    assert(std::get<std::string>(widthResult.data) == "[42    ]");

    amlp::Value precisionResult = runProbe("return sprintf(\"%.3O\", \"hello\");\n");
    // The dumped form of "hello" is the 7-character string "\"hello\"";
    // ".3" truncates that dumped form itself to its first 3 characters.
    assert(std::get<std::string>(precisionResult.data) == "\"he");

    std::cout << "testSprintfPercentOFieldWidthAndPrecisionApplyLikePercentS OK\n";
}

static void testPrintfLilEvalShapeMatchesRealResultPrefix() {
    // The exact real call shape from mudlib/command/eval.c:
    // "printf(\"Result = %O\n\", ...)" -- the specific gap this row
    // closes, not just the general specifier.
    ObjectVarHarness harness;
    harness.writeFile("/pctO_eval_probe.c",
        "string probe() { return sprintf(\"Result = %O\\n\", 5 + 5); }\n");
    auto ob = harness.objects.cloneObject("/pctO_eval_probe");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::get<std::string>(result.data) == "Result = 10\n");

    std::cout << "testPrintfLilEvalShapeMatchesRealResultPrefix OK\n";
}

static void testSprintfDotPrecisionTruncatesLongerString() {
    amlp::Value result = runProbe("return sprintf(\"[%.3s]\", \"hello\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[hel]");
    std::cout << "testSprintfDotPrecisionTruncatesLongerString OK\n";
}

// Real sprintf.c's own doc: "if precision is greater than field size,
// then field size = precision" -- field size 3, precision 5, a 2-char
// value: not truncated (shorter than the precision), but padded out to
// the *precision*, not the smaller explicit field size.
static void testSprintfDotPrecisionWidensFieldWhenGreaterThanExplicitWidth() {
    amlp::Value result = runProbe("return sprintf(\"[%3.5s]\", \"ab\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[   ab]");
    std::cout << "testSprintfDotPrecisionWidensFieldWhenGreaterThanExplicitWidth OK\n";
}

static void testSprintfStarFieldWidthPullsWidthFromLeadingArgument() {
    amlp::Value result = runProbe("return sprintf(\"[%*d]\", 5, 7);\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[    7]");
    std::cout << "testSprintfStarFieldWidthPullsWidthFromLeadingArgument OK\n";
}

static void testSprintfStarPrecisionPullsPrecisionFromLeadingArgument() {
    amlp::Value result = runProbe("return sprintf(\"[%.*s]\", 3, \"hello\");\n");
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "[hel]");
    std::cout << "testSprintfStarPrecisionPullsPrecisionFromLeadingArgument OK\n";
}

static void testSprintfZeroPaddedStarFieldWidthThrows() {
    // Deliberately not implemented (see this efun's own comment) --
    // confirms it fails loudly rather than silently misparsing the '*'
    // as a stray, unsupported type specifier.
    bool threw = false;
    try {
        runProbe("return sprintf(\"%0*d\", 5, 7);\n");
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);
    std::cout << "testSprintfZeroPaddedStarFieldWidthThrows OK\n";
}

// ---------------------------------------------------------------------
// printf(string, ...): real efuns_main.c's own f_printf() -- formats
// through the exact same machinery as sprintf() (confirmed against
// string_print_formatted(), not a separate format engine) and writes
// the result to command_giver. This driver reuses write()'s own already-
// proven target resolution (OutputContext::current()) rather than a
// second, separately-approximated one -- see EfunTable.cpp's own
// comment on the "printf" registration.

static void testPrintfWritesSprintfFormattedResultToCurrentConnection() {
    // "%|9s" (centre-justify) rather than a bare "%s" deliberately --
    // proves printf() really goes through the same sprintf() machinery
    // (including this slice's own "|" work), not a separate, simpler
    // format path that only happens to handle plain strings.
    ObjectVarHarness harness;
    harness.writeFile("/printf_probe1.c",
        "void probe() { printf(\"[%|9s]\", \"abc\"); }\n");
    auto probe = harness.objects.cloneObject("/printf_probe1");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(probe, "probe", {});
    amlp::OutputContext::set(nullptr);

    char buf[256];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    std::string received(buf, static_cast<size_t>(n));
    assert(received == "[   abc   ]");

    ::close(fds[1]);
    std::cout << "testPrintfWritesSprintfFormattedResultToCurrentConnection OK\n";
}

static void testPrintfThrowsOnNonStringFormatArgument() {
    // Real f_printf() shares sprintf()'s own format-string validation
    // (both go through string_print_formatted()) -- confirmed here by
    // reusing sprintfImpl directly rather than duplicating its checks.
    ObjectVarHarness harness;
    harness.writeFile("/printf_probe2.c", "void probe() { printf(42); }\n");
    auto probe = harness.objects.cloneObject("/printf_probe2");
    assert(probe != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(probe, "probe", {});
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("sprintf") != std::string::npos);
    }
    assert(threw);

    std::cout << "testPrintfThrowsOnNonStringFormatArgument OK\n";
}

// function_exists(string fun, void|object ob) -- real interpret.c's own
// function_exists(): returns the defining program's own filename (as a
// bare "/path/without/dotc" string) when fun is defined anywhere in
// ob's local/inherited chain, 0 otherwise. Confirmed against
// efuns_main.c's f_function_exists(): ob defaults to current_object
// when omitted.
static void testFunctionExistsReturnsTruthyStringForALocallyDefinedFunction() {
    ObjectVarHarness harness;
    harness.writeFile("/fe_probe1.c",
        "int target() { return 1; }\n"
        "mixed probe() { return function_exists(\"target\", this_object()); }\n");
    auto ob = harness.objects.cloneObject("/fe_probe1");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* strPtr = std::get_if<std::string>(&result.data);
    assert(strPtr != nullptr);
    assert(!strPtr->empty());

    std::cout << "testFunctionExistsReturnsTruthyStringForALocallyDefinedFunction OK\n";
}

static void testFunctionExistsReturnsZeroForAnUndefinedFunction() {
    ObjectVarHarness harness;
    harness.writeFile("/fe_probe2.c",
        "mixed probe() { return function_exists(\"no_such_function\", this_object()); }\n");
    auto ob = harness.objects.cloneObject("/fe_probe2");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* intPtr = std::get_if<int64_t>(&result.data);
    assert(intPtr != nullptr);
    assert(*intPtr == 0);

    std::cout << "testFunctionExistsReturnsZeroForAnUndefinedFunction OK\n";
}

// The real 34-call-site pattern in this mudlib never passes an explicit
// second argument, relying on the default-to-current_object() behavior
// (efuns_main.c: ob defaults to current_object when only 1 arg passed).
static void testFunctionExistsDefaultsToCurrentObjectWhenObjectArgumentOmitted() {
    ObjectVarHarness harness;
    harness.writeFile("/fe_probe3.c",
        "int target() { return 1; }\n"
        "mixed probe() { return function_exists(\"target\"); }\n"
        "mixed probe_missing() { return function_exists(\"nope\"); }\n");
    auto ob = harness.objects.cloneObject("/fe_probe3");
    assert(ob != nullptr);

    amlp::Value found = harness.vm.callFunction(ob, "probe", {});
    auto* strPtr = std::get_if<std::string>(&found.data);
    assert(strPtr != nullptr);
    assert(!strPtr->empty());

    amlp::Value missing = harness.vm.callFunction(ob, "probe_missing", {});
    auto* intPtr = std::get_if<int64_t>(&missing.data);
    assert(intPtr != nullptr);
    assert(*intPtr == 0);

    std::cout << "testFunctionExistsDefaultsToCurrentObjectWhenObjectArgumentOmitted OK\n";
}

// A function defined only in a parent (never overridden by the child)
// must still be found via the same local/inherited chain walk
// callFunction() itself uses -- real function_exists() finds inherited
// functions too, not just locally-declared ones.
static void testFunctionExistsFindsAnInheritedFunctionNotJustLocalOnes() {
    ObjectVarHarness harness;
    harness.writeFile("/fe_parent.c",
        "int parent_fn() { return 1; }\n");
    harness.writeFile("/fe_child.c",
        "inherit \"/fe_parent\";\n"
        "mixed probe() { return function_exists(\"parent_fn\", this_object()); }\n");
    auto ob = harness.objects.cloneObject("/fe_child");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* strPtr = std::get_if<std::string>(&result.data);
    assert(strPtr != nullptr);
    assert(!strPtr->empty());

    std::cout << "testFunctionExistsFindsAnInheritedFunctionNotJustLocalOnes OK\n";
}

static void testFunctionExistsThrowsOnNonStringFunctionNameArgument() {
    ObjectVarHarness harness;
    harness.writeFile("/fe_probe4.c", "mixed probe() { return function_exists(42); }\n");
    auto ob = harness.objects.cloneObject("/fe_probe4");
    assert(ob != nullptr);

    bool threw = false;
    try {
        harness.vm.callFunction(ob, "probe", {});
    } catch (const amlp::LpcRuntimeError& e) {
        threw = true;
        std::string msg = e.what();
        assert(msg.find("function_exists") != std::string::npos);
    }
    assert(threw);

    std::cout << "testFunctionExistsThrowsOnNonStringFunctionNameArgument OK\n";
}

// ---------------------------------------------------------------------
// Rifts combat math efuns (phase 1 of the game-logic-mechanics move,
// 2026-08-08): each test below compiles the ORIGINAL LPC function body,
// transcribed verbatim from daemon/rifts_combat.c as it stood before the
// move, and compares its output against the new C++ efun of the same
// name across a range of real inputs, before that LPC copy was removed
// from the mudlib. See STATUS.md for the full writeup.
// ---------------------------------------------------------------------

static void testPpCombatBonusEfunMatchesLpcAcrossBoundaries() {
    std::string src =
        "int pp_combat_bonus(int pp) {\n"
        "    if(pp >= 26) return 6;\n"
        "    if(pp >= 21) return 5;\n"
        "    if(pp >= 19) return 4;\n"
        "    if(pp >= 18) return 3;\n"
        "    if(pp >= 16) return 2;\n"
        "    if(pp >= 13) return 1;\n"
        "    return 0;\n"
        "}\n";
    auto obj = compileProgramObject(src);
    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    int64_t values[] = {-5, 0, 1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 25, 26, 27, 40};
    for (int64_t pp : values) {
        amlp::Value lpcResult = vm.callFunction(obj, "pp_combat_bonus", {amlp::Value(pp)});
        std::vector<amlp::Value> efunArgs{ amlp::Value(pp) };
        amlp::Value efunResult = amlp::EfunTable::instance().call("pp_combat_bonus", vm, efunArgs);
        assert(std::holds_alternative<int64_t>(lpcResult.data));
        assert(std::holds_alternative<int64_t>(efunResult.data));
        assert(std::get<int64_t>(lpcResult.data) == std::get<int64_t>(efunResult.data));
    }

    std::cout << "testPpCombatBonusEfunMatchesLpcAcrossBoundaries OK\n";
}

static void testPsDamageBonusEfunMatchesLpcAcrossBoundariesAndSupernatural() {
    std::string src =
        "int ps_damage_bonus(int ps, int supernatural) {\n"
        "    int bonus;\n"
        "    if(ps >= 31) bonus = 7;\n"
        "    else if(ps >= 30) bonus = 6;\n"
        "    else if(ps >= 26) bonus = 5;\n"
        "    else if(ps >= 21) bonus = 4;\n"
        "    else if(ps >= 18) bonus = 3;\n"
        "    else if(ps >= 16) bonus = 2;\n"
        "    else bonus = 0;\n"
        "    if(supernatural) return bonus * 2;\n"
        "    return bonus;\n"
        "}\n";
    auto obj = compileProgramObject(src);
    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    int64_t values[] = {0, 15, 16, 17, 18, 20, 21, 25, 26, 29, 30, 31, 35};
    for (int64_t ps : values) {
        for (int64_t supernatural : {int64_t{0}, int64_t{1}}) {
            amlp::Value lpcResult = vm.callFunction(obj, "ps_damage_bonus",
                {amlp::Value(ps), amlp::Value(supernatural)});
            std::vector<amlp::Value> efunArgs{ amlp::Value(ps), amlp::Value(supernatural) };
            amlp::Value efunResult = amlp::EfunTable::instance().call("ps_damage_bonus", vm, efunArgs);
            assert(std::holds_alternative<int64_t>(lpcResult.data));
            assert(std::holds_alternative<int64_t>(efunResult.data));
            assert(std::get<int64_t>(lpcResult.data) == std::get<int64_t>(efunResult.data));
        }
    }

    std::cout << "testPsDamageBonusEfunMatchesLpcAcrossBoundariesAndSupernatural OK\n";
}

static void testOccBaseApmEfunMatchesLpcAcrossAllCategoriesAndEdgeCases() {
    // Transcribed verbatim from daemon/rifts_combat.c's own private
    // occ_base_apm(), full case list included (not a shortened subset)
    // so this is a real behavioral comparison, not a reimplementation.
    std::string src =
        "int occ_base_apm(string occ) {\n"
        "    if(!occ || occ == \"\") return 2;\n"
        "    switch(occ) {\n"
        "    case \"cyber-knight\":\n"
        "    case \"crazy\":\n"
        "    case \"juicer\":\n"
        "    case \"ninja juicer\":\n"
        "    case \"delphi juicer\":\n"
        "    case \"hyperion juicer\":\n"
        "    case \"tattooed man\":\n"
        "    case \"tattoo warrior\":\n"
        "        return 6;\n"
        "    case \"master assassin\":\n"
        "    case \"city rat\":\n"
        "    case \"forger\":\n"
        "    case \"freelance spy\":\n"
        "    case \"professional thief\":\n"
        "    case \"smuggler\":\n"
        "    case \"iss peacekeeper\":\n"
        "    case \"iss specter\":\n"
        "        return 5;\n"
        "    case \"headhunter\":\n"
        "    case \"bounty hunter\":\n"
        "    case \"cs grunt\":\n"
        "    case \"cs dead boy\":\n"
        "    case \"cs ranger\":\n"
        "    case \"cs military specialist\":\n"
        "    case \"cs samas rpa pilot\":\n"
        "    case \"cs technical officer\":\n"
        "    case \"merc soldier\":\n"
        "    case \"special forces (merc)\":\n"
        "    case \"tribal warrior\":\n"
        "    case \"wilderness scout\":\n"
        "    case \"borg\":\n"
        "    case \"glitter boy pilot\":\n"
        "    case \"robot pilot\":\n"
        "    case \"ntset protector\":\n"
        "    case \"knight (europe)\":\n"
        "    case \"royal knight\":\n"
        "    case \"pirate (s.a.)\":\n"
        "    case \"sailor (s.a.)\":\n"
        "        return 4;\n"
        "    case \"ley line walker\":\n"
        "    case \"mystic\":\n"
        "    case \"shifter\":\n"
        "    case \"shaman\":\n"
        "    case \"techno-wizard\":\n"
        "    case \"ley line rifter\":\n"
        "    case \"air warlock\":\n"
        "    case \"nega-psychic\":\n"
        "        return 3;\n"
        "    default:\n"
        "        return 2;\n"
        "    }\n"
        "}\n";
    auto obj = compileProgramObject(src);
    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    // One representative from each bonus tier, plus edge cases: empty
    // string, an unrecognized occ (default), and a case-mismatched
    // variant of a real key (must NOT match -- exact-string semantics).
    const char* occs[] = {
        "cyber-knight", "juicer", "tattoo warrior",
        "master assassin", "iss specter",
        "headhunter", "sailor (s.a.)", "cs dead boy",
        "ley line walker", "nega-psychic",
        "", "civilian", "Cyber-Knight",
    };
    for (const char* occ : occs) {
        amlp::Value lpcResult = vm.callFunction(obj, "occ_base_apm",
            {amlp::Value(std::string(occ))});
        std::vector<amlp::Value> efunArgs{ amlp::Value(std::string(occ)) };
        amlp::Value efunResult = amlp::EfunTable::instance().call("occ_base_apm", vm, efunArgs);
        assert(std::holds_alternative<int64_t>(lpcResult.data));
        assert(std::holds_alternative<int64_t>(efunResult.data));
        assert(std::get<int64_t>(lpcResult.data) == std::get<int64_t>(efunResult.data));
    }

    // The "!occ" branch (undefined, not merely empty): the LPC side
    // takes an actual undefined argument; the efun side omits the
    // argument entirely, matching how the driver treats a missing
    // string arg elsewhere in this file (see occ_base_apm's own
    // registration comment).
    amlp::Value lpcUndefResult = vm.callFunction(obj, "occ_base_apm", {amlp::Value{}});
    std::vector<amlp::Value> efunUndefArgs{};
    amlp::Value efunUndefResult = amlp::EfunTable::instance().call("occ_base_apm", vm, efunUndefArgs);
    assert(std::get<int64_t>(lpcUndefResult.data) == 2);
    assert(std::get<int64_t>(efunUndefResult.data) == 2);

    std::cout << "testOccBaseApmEfunMatchesLpcAcrossAllCategoriesAndEdgeCases OK\n";
}

// roll_weapon_damage_dice() is genuinely random (shares the real
// random() efun), so there is no single "the" LPC output to diff
// against draw-for-draw. Instead this locks in the formula the LPC
// original's inline dice-rolling block actually computed (num dice of
// size sides, summed, plus bonus, floored at 1) via bounds that must
// hold on every draw, checked across many iterations per case.
static void testRollWeaponDamageDiceStaysWithinFormulaDerivedBoundsAcrossManyDraws() {
    amlp::Config config;
    amlp::ObjectManager objects(config);
    amlp::VM vm(objects, config);

    struct Case { int64_t num, sides, bonus, lo, hi; };
    Case cases[] = {
        {3, 6, 0, 3, 18},        // 3d6, no bonus
        {1, 4, -10, 1, 1},       // floored at 1: 1d4-10 is always <= 0
        {2, 4, 100, 102, 108},   // large positive bonus dominates
        {0, 6, 5, 5, 5},         // num == 0: zero dice, just the bonus
        {0, 6, -5, 1, 1},        // num == 0, non-positive total: floor
    };
    for (const auto& c : cases) {
        bool sawMin = false, sawMax = (c.num == 0);
        // 3d6's exact-18 draw is 1/216 per trial (all three dice max);
        // 500 trials left roughly a 1-in-10 chance of never seeing it,
        // a real flake seen live. 3000 trials brings that under 1e-6.
        for (int trial = 0; trial < 3000; ++trial) {
            std::vector<amlp::Value> args{
                amlp::Value(c.num), amlp::Value(c.sides), amlp::Value(c.bonus) };
            amlp::Value result = amlp::EfunTable::instance().call("roll_weapon_damage_dice", vm, args);
            assert(std::holds_alternative<int64_t>(result.data));
            int64_t damage = std::get<int64_t>(result.data);
            assert(damage >= c.lo && damage <= c.hi);
            if (damage == c.lo) sawMin = true;
            if (damage == c.hi) sawMax = true;
        }
        // For genuinely random cases (num > 0, real spread), 500 draws
        // should hit both ends of the range at least once; a bug that
        // narrowed the distribution (off-by-one on sides, wrong
        // random() convention) would show up as a range never reached.
        if (c.num > 0 && c.lo != c.hi) {
            assert(sawMin);
            assert(sawMax);
        }
    }

    std::cout << "testRollWeaponDamageDiceStaysWithinFormulaDerivedBoundsAcrossManyDraws OK\n";
}

// Shared harness for query_strike_bonus/query_parry_bonus/
// query_dodge_bonus: these call back into LPC for player stats/env/
// property reads and into a mock ADDICTION_D daemon, so they need a
// real scratch mudlib_root (ObjectVarHarness), not just an in-memory
// compiled program.
static void writeRiftsCombatBonusFixtures(ObjectVarHarness& harness) {
    // Mock player: exposes exactly the methods query_strike_bonus/
    // query_parry_bonus actually call (query_stats, query_level,
    // getenv, query_property), settable per test case.
    harness.writeFile("/player.c",
        "int stat_pp, stat_level;\n"
        "string occ_val, stance_val;\n"
        "\n"
        "void set_test_data(int pp, int level, string occ, string stance) {\n"
        "    stat_pp = pp;\n"
        "    stat_level = level;\n"
        "    occ_val = occ;\n"
        "    stance_val = stance;\n"
        "}\n"
        "\n"
        "int query_stats(string which) {\n"
        "    if(which == \"PP\") return stat_pp;\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "int query_level() { return stat_level; }\n"
        "\n"
        "string getenv(string key) {\n"
        "    if(key == \"rifts_occ\") return occ_val;\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "string query_property(string key) {\n"
        "    if(key == \"combat_stance\") return stance_val;\n"
        "    return 0;\n"
        "}\n");

    // Mock ADDICTION_D at the real path (/daemon/addiction_d) the efun
    // resolves via vm.findObject(), matching daemons.h's own
    // "#define ADDICTION_D (DIR_DAEMONS+\"/addiction_d\")". writeFile()
    // does not create parent directories, unlike every other fixture
    // path used elsewhere in this file (all single-level so far).
    mkdir((harness.tempDir + "/daemon").c_str(), 0755);
    harness.writeFile("/daemon/addiction_d.c",
        "mapping query_combat_modifiers(object who) {\n"
        "    return ([ \"strike\": 3, \"parry\": -1 ]);\n"
        "}\n");

    // The ORIGINAL LPC implementation, transcribed verbatim from
    // daemon/rifts_combat.c as it stood before the move, with the
    // ADDICTION_D macro substituted for its own literal expansion
    // (this scratch harness has no secure/include/daemons.h to
    // #include).
    harness.writeFile("/rifts_combat_orig.c",
        "private int pp_combat_bonus(int pp) {\n"
        "    if(pp >= 26) return 6;\n"
        "    if(pp >= 21) return 5;\n"
        "    if(pp >= 19) return 4;\n"
        "    if(pp >= 18) return 3;\n"
        "    if(pp >= 16) return 2;\n"
        "    if(pp >= 13) return 1;\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "private int occ_base_apm(string occ) {\n"
        "    if(!occ || occ == \"\") return 2;\n"
        "    switch(occ) {\n"
        "    case \"cyber-knight\": case \"crazy\": case \"juicer\":\n"
        "        return 6;\n"
        "    case \"master assassin\": case \"city rat\":\n"
        "        return 5;\n"
        "    case \"headhunter\": case \"bounty hunter\":\n"
        "        return 4;\n"
        "    case \"ley line walker\": case \"mystic\":\n"
        "        return 3;\n"
        "    default:\n"
        "        return 2;\n"
        "    }\n"
        "}\n"
        "\n"
        "private int position_strike_mod(object player) {\n"
        "    string pos;\n"
        "    pos = (string)player->query_property(\"combat_stance\");\n"
        "    if(!pos) return 0;\n"
        "    if(pos == \"offensive\") return 2;\n"
        "    if(pos == \"defensive\") return -2;\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "private int position_defense_mod(object player) {\n"
        "    string pos;\n"
        "    pos = (string)player->query_property(\"combat_stance\");\n"
        "    if(!pos) return 0;\n"
        "    if(pos == \"defensive\") return 2;\n"
        "    if(pos == \"offensive\") return -2;\n"
        "    return 0;\n"
        "}\n"
        "\n"
        "int query_strike_bonus(object player) {\n"
        "    int pp, level, bonus;\n"
        "    string occ;\n"
        "    mapping addmods;\n"
        "    if(!player) return 0;\n"
        "    pp    = (int)player->query_stats(\"PP\");\n"
        "    level = (int)player->query_level();\n"
        "    occ   = (string)player->getenv(\"rifts_occ\");\n"
        "    bonus = pp_combat_bonus(pp);\n"
        "    switch(occ_base_apm(occ)) {\n"
        "    case 6: bonus += level / 2; break;\n"
        "    case 5: bonus += level / 3; break;\n"
        "    case 4: bonus += level / 3; break;\n"
        "    case 3: bonus += level / 4; break;\n"
        "    default: bonus += level / 5; break;\n"
        "    }\n"
        "    bonus  += position_strike_mod(player);\n"
        "    addmods = (mapping)(\"/daemon/addiction_d\")->query_combat_modifiers(player);\n"
        "    bonus  += (int)addmods[\"strike\"];\n"
        "    return bonus;\n"
        "}\n"
        "\n"
        "int query_parry_bonus(object player) {\n"
        "    int pp, level, bonus;\n"
        "    string occ;\n"
        "    mapping addmods;\n"
        "    if(!player) return 0;\n"
        "    pp    = (int)player->query_stats(\"PP\");\n"
        "    level = (int)player->query_level();\n"
        "    occ   = (string)player->getenv(\"rifts_occ\");\n"
        "    bonus = pp_combat_bonus(pp);\n"
        "    switch(occ_base_apm(occ)) {\n"
        "    case 6: bonus += level / 2; break;\n"
        "    case 5: bonus += level / 3; break;\n"
        "    case 4: bonus += level / 3; break;\n"
        "    case 3: bonus += level / 4; break;\n"
        "    default: bonus += level / 5; break;\n"
        "    }\n"
        "    bonus  += position_defense_mod(player);\n"
        "    addmods = (mapping)(\"/daemon/addiction_d\")->query_combat_modifiers(player);\n"
        "    bonus  += (int)addmods[\"parry\"];\n"
        "    return bonus;\n"
        "}\n"
        "\n"
        "int query_dodge_bonus(object player) {\n"
        "    return query_parry_bonus(player);\n"
        "}\n");
}

static void testQueryStrikeBonusEfunMatchesLpcAcrossPlayerStates() {
    ObjectVarHarness harness;
    writeRiftsCombatBonusFixtures(harness);

    auto orig = harness.vm.findObject("/rifts_combat_orig");
    auto player = harness.vm.findObject("/player");
    assert(orig);
    assert(player);

    struct Case { int64_t pp, level; const char* occ; const char* stance; };
    Case cases[] = {
        {10, 1, "civilian", ""},
        {18, 5, "headhunter", "offensive"},
        {26, 12, "cyber-knight", "defensive"},
        {13, 8, "mystic", ""},
        {8, 0, "", ""},
    };
    for (const auto& c : cases) {
        harness.vm.callFunction(player, "set_test_data",
            { amlp::Value(c.pp), amlp::Value(c.level),
              amlp::Value(std::string(c.occ)), amlp::Value(std::string(c.stance)) });

        amlp::Value lpcResult = harness.vm.callFunction(orig, "query_strike_bonus", { amlp::Value(player) });
        std::vector<amlp::Value> efunArgs{ amlp::Value(player) };
        amlp::Value efunResult = amlp::EfunTable::instance().call("query_strike_bonus", harness.vm, efunArgs);

        assert(std::holds_alternative<int64_t>(lpcResult.data));
        assert(std::holds_alternative<int64_t>(efunResult.data));
        assert(std::get<int64_t>(lpcResult.data) == std::get<int64_t>(efunResult.data));
    }

    std::cout << "testQueryStrikeBonusEfunMatchesLpcAcrossPlayerStates OK\n";
}

static void testQueryParryBonusEfunMatchesLpcAcrossPlayerStates() {
    ObjectVarHarness harness;
    writeRiftsCombatBonusFixtures(harness);

    auto orig = harness.vm.findObject("/rifts_combat_orig");
    auto player = harness.vm.findObject("/player");
    assert(orig);
    assert(player);

    struct Case { int64_t pp, level; const char* occ; const char* stance; };
    Case cases[] = {
        {10, 1, "civilian", ""},
        {18, 5, "headhunter", "offensive"},
        {26, 12, "cyber-knight", "defensive"},
        {13, 8, "mystic", ""},
        {8, 0, "", ""},
    };
    for (const auto& c : cases) {
        harness.vm.callFunction(player, "set_test_data",
            { amlp::Value(c.pp), amlp::Value(c.level),
              amlp::Value(std::string(c.occ)), amlp::Value(std::string(c.stance)) });

        amlp::Value lpcResult = harness.vm.callFunction(orig, "query_parry_bonus", { amlp::Value(player) });
        std::vector<amlp::Value> efunArgs{ amlp::Value(player) };
        amlp::Value efunResult = amlp::EfunTable::instance().call("query_parry_bonus", harness.vm, efunArgs);

        assert(std::holds_alternative<int64_t>(lpcResult.data));
        assert(std::holds_alternative<int64_t>(efunResult.data));
        assert(std::get<int64_t>(lpcResult.data) == std::get<int64_t>(efunResult.data));
    }

    std::cout << "testQueryParryBonusEfunMatchesLpcAcrossPlayerStates OK\n";
}

static void testQueryDodgeBonusEfunMatchesLpcAliasOfParryBonus() {
    ObjectVarHarness harness;
    writeRiftsCombatBonusFixtures(harness);

    auto orig = harness.vm.findObject("/rifts_combat_orig");
    auto player = harness.vm.findObject("/player");
    assert(orig);
    assert(player);

    harness.vm.callFunction(player, "set_test_data",
        { amlp::Value(int64_t{18}), amlp::Value(int64_t{5}),
          amlp::Value(std::string("headhunter")), amlp::Value(std::string("defensive")) });

    amlp::Value lpcResult = harness.vm.callFunction(orig, "query_dodge_bonus", { amlp::Value(player) });
    std::vector<amlp::Value> efunArgs{ amlp::Value(player) };
    amlp::Value efunResult = amlp::EfunTable::instance().call("query_dodge_bonus", harness.vm, efunArgs);

    assert(std::holds_alternative<int64_t>(lpcResult.data));
    assert(std::holds_alternative<int64_t>(efunResult.data));
    assert(std::get<int64_t>(lpcResult.data) == std::get<int64_t>(efunResult.data));
    // Also confirm it actually matches query_parry_bonus's own result
    // for the same state, locking in the alias relationship itself.
    std::vector<amlp::Value> parryArgs{ amlp::Value(player) };
    amlp::Value parryResult = amlp::EfunTable::instance().call("query_parry_bonus", harness.vm, parryArgs);
    assert(std::get<int64_t>(efunResult.data) == std::get<int64_t>(parryResult.data));

    std::cout << "testQueryDodgeBonusEfunMatchesLpcAliasOfParryBonus OK\n";
}

// -----------------------------------------------------------------------
// Phase 0.13 efun growth batch (post-restructure): test_bit/set_bit/
// clear_bit, crc32, cp, inherits, get_config, query_load_average, say,
// save_variable/restore_variable. See EfunTable.cpp's own registration
// comments for each efun's real-source citations.
// -----------------------------------------------------------------------

static void testSetBitAndTestBitRoundTripSingleBit() {
    ObjectVarHarness harness;
    harness.writeFile("/bit_probe1.c",
        "mixed probe() {\n"
        "    string str = set_bit(\"\", 100);\n"
        "    int hit = test_bit(str, 100);\n"
        "    int miss50 = test_bit(str, 50);\n"
        "    int miss150 = test_bit(str, 150);\n"
        "    return ({ str, hit, miss50, miss150 });\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/bit_probe1");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && *arr);
    assert((*arr)->items.size() == 4);
    assert(std::get<int64_t>((*arr)->items[1].data) == 1);
    assert(std::get<int64_t>((*arr)->items[2].data) == 0);
    assert(std::get<int64_t>((*arr)->items[3].data) == 0);

    std::cout << "testSetBitAndTestBitRoundTripSingleBit OK\n";
}

static void testSetBitThrowsOnOutOfRangeOrNegativeBitIndex() {
    ObjectVarHarness harness;
    harness.writeFile("/bit_probe2.c",
        "mixed probe_huge() { return catch(set_bit(\"\", 10000000000000)); }\n"
        "mixed probe_neg() { return catch(set_bit(\"\", -2)); }\n");
    auto ob = harness.objects.cloneObject("/bit_probe2");
    assert(ob != nullptr);

    amlp::Value huge = harness.vm.callFunction(ob, "probe_huge", {});
    assert(std::holds_alternative<std::string>(huge.data));
    amlp::Value neg = harness.vm.callFunction(ob, "probe_neg", {});
    assert(std::holds_alternative<std::string>(neg.data));

    std::cout << "testSetBitThrowsOnOutOfRangeOrNegativeBitIndex OK\n";
}

// Real f_clear_bit(): an index past the string's current length returns
// the first argument completely unmodified (no resize, no error) --
// clearing a bit that was never set is a real no-op, not a growth point
// the way set_bit()'s own out-of-range handling is.
static void testClearBitIsNoOpPastStringLengthAndClearsWithinRange() {
    ObjectVarHarness harness;
    harness.writeFile("/bit_probe3.c",
        "mixed probe() {\n"
        "    string str = \"11111\";\n"
        "    string cleared = clear_bit(str, 7);\n"
        "    int stillMiss = test_bit(cleared, 7);\n"
        "    string setThenClear = clear_bit(set_bit(cleared, 12), 12);\n"
        "    string noop = clear_bit(setThenClear, 40);\n"
        "    return ({ stillMiss, noop == setThenClear });\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/bit_probe3");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && *arr);
    assert(std::get<int64_t>((*arr)->items[0].data) == 0);
    assert(std::get<int64_t>((*arr)->items[1].data) == 1);

    std::cout << "testClearBitIsNoOpPastStringLengthAndClearsWithinRange OK\n";
}

// Independently computed (standard reflected CRC-32, poly 0xedb88320,
// seeded 0xFFFFFFFF, no final complement -- matching real compute_crc32()
// exactly) rather than pulled from this driver's own implementation, so
// this test can actually catch a wrong polynomial or a stray final XOR.
static void testCrc32ReturnsKnownValueForHelloAndSeedValueForEmptyString() {
    ObjectVarHarness harness;
    std::vector<amlp::Value> helloArgs{ amlp::Value(std::string("hello")) };
    amlp::Value hello = amlp::EfunTable::instance().call("crc32", harness.vm, helloArgs);
    assert(std::get<int64_t>(hello.data) == 3387906425LL);

    std::vector<amlp::Value> emptyArgs{ amlp::Value(std::string("")) };
    amlp::Value empty = amlp::EfunTable::instance().call("crc32", harness.vm, emptyArgs);
    assert(std::get<int64_t>(empty.data) == 4294967295LL);

    std::cout << "testCrc32ReturnsKnownValueForHelloAndSeedValueForEmptyString OK\n";
}

static void testCrc32ThrowsOnNonStringArgument() {
    ObjectVarHarness harness;
    std::vector<amlp::Value> args{ amlp::Value(int64_t{0}) };
    bool threw = false;
    try {
        amlp::EfunTable::instance().call("crc32", harness.vm, args);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testCrc32ThrowsOnNonStringArgument OK\n";
}

static void testCpCopiesFileContentAndReturnsTruthy() {
    ObjectVarHarness harness;
    harness.writeFile("/cp_source.c", "void create() {}\n");
    harness.writeFile("/cp_probe.c",
        "mixed probe() {\n"
        "    rm(\"/cp_dest.c\");\n"
        "    int before = file_size(\"/cp_dest.c\");\n"
        "    int ok = cp(\"/cp_source.c\", \"/cp_dest.c\");\n"
        "    int after = file_size(\"/cp_dest.c\");\n"
        "    return ({ before, ok, after, read_file(\"/cp_source.c\") == read_file(\"/cp_dest.c\") });\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/cp_probe");
    assert(ob != nullptr);

    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && *arr);
    assert(std::get<int64_t>((*arr)->items[0].data) == -1);
    assert(std::get<int64_t>((*arr)->items[1].data) != 0);
    assert(std::get<int64_t>((*arr)->items[2].data) > 0);
    assert(std::get<int64_t>((*arr)->items[3].data) != 0);

    std::cout << "testCpCopiesFileContentAndReturnsTruthy OK\n";
}

// Mirrors this repo's own bundled Lil starter mudlib's real conformance
// test for this efun exactly (mudlib/single/tests/efuns/inherits.c,
// inh0/inh1/inh2.c): inh2 inherits inh1 inherits inh0 -- a chain, not
// three independent files.
static void testInheritsMatchesTransitiveChainInBothDirections() {
    ObjectVarHarness harness;
    harness.writeFile("/inh_base.c", "int marker() { return 1; }\n");
    harness.writeFile("/inh_mid.c", "inherit \"/inh_base\";\n");
    harness.writeFile("/inh_leaf.c", "inherit \"/inh_mid\";\n");

    auto base = harness.objects.cloneObject("/inh_base");
    auto mid = harness.objects.cloneObject("/inh_mid");
    auto leaf = harness.objects.cloneObject("/inh_leaf");
    assert(base && mid && leaf);

    std::vector<amlp::Value> midInLeaf{ amlp::Value(std::string("/inh_mid.c")), amlp::Value(leaf) };
    assert(std::get<int64_t>(amlp::EfunTable::instance().call("inherits", harness.vm, midInLeaf).data) == 1);

    std::vector<amlp::Value> baseInLeaf{ amlp::Value(std::string("/inh_base.c")), amlp::Value(leaf) };
    assert(std::get<int64_t>(amlp::EfunTable::instance().call("inherits", harness.vm, baseInLeaf).data) == 1);

    std::vector<amlp::Value> baseInMid{ amlp::Value(std::string("/inh_base.c")), amlp::Value(mid) };
    assert(std::get<int64_t>(amlp::EfunTable::instance().call("inherits", harness.vm, baseInMid).data) == 1);

    // Wrong direction: a base does not inherit its own descendant.
    std::vector<amlp::Value> leafInMid{ amlp::Value(std::string("/inh_leaf.c")), amlp::Value(mid) };
    assert(std::get<int64_t>(amlp::EfunTable::instance().call("inherits", harness.vm, leafInMid).data) == 0);

    std::vector<amlp::Value> leafInBase{ amlp::Value(std::string("/inh_leaf.c")), amlp::Value(base) };
    assert(std::get<int64_t>(amlp::EfunTable::instance().call("inherits", harness.vm, leafInBase).data) == 0);

    std::vector<amlp::Value> midInBase{ amlp::Value(std::string("/inh_mid.c")), amlp::Value(base) };
    assert(std::get<int64_t>(amlp::EfunTable::instance().call("inherits", harness.vm, midInBase).data) == 0);

    std::vector<amlp::Value> unknownInBase{ amlp::Value(std::string("foo")), amlp::Value(base) };
    assert(std::get<int64_t>(amlp::EfunTable::instance().call("inherits", harness.vm, unknownInBase).data) == 0);

    std::cout << "testInheritsMatchesTransitiveChainInBothDirections OK\n";
}

// functions()/variables()/fetch_variable()/store_variable(): real
// packages/contrib.c introspection efuns, confirmed to have genuine C
// bodies in this exact vendored build (PACKAGE_CONTRIB active in
// options.h) despite an earlier session's own accounting mistakenly
// filing all four under "no implementation anywhere in this project's
// own reference source" -- corrected in this session's own STATUS.md
// entry, not silently carried forward.

static void testFunctionsListsOwnAndInheritedNamesWithOverridePrecedence() {
    ObjectVarHarness harness;
    harness.writeFile("/fn_base.c",
        "int a() { return 1; }\n"
        "int shared() { return 1; }\n");
    harness.writeFile("/fn_leaf.c",
        "inherit \"/fn_base\";\n"
        "int x;\n"
        "int b() { return 2; }\n"
        "int shared() { return 2; }\n");
    auto leaf = harness.objects.cloneObject("/fn_leaf");
    assert(leaf != nullptr);

    // Default flag (all names, inherited included): a, shared, b, all
    // present exactly once, leaf's own "shared" override winning over
    // base's shadowed one (both named the same), and the synthesized
    // "$objvarinit" CodeGen.cpp adds for leaf's own "int x;" never
    // appearing at all.
    std::vector<amlp::Value> allArgs{ amlp::Value(leaf) };
    amlp::Value all = amlp::EfunTable::instance().call("functions", harness.vm, allArgs);
    auto allArr = std::get<std::shared_ptr<amlp::Array>>(all.data);
    std::vector<std::string> allNames;
    for (auto& v : allArr->items) allNames.push_back(std::get<std::string>(v.data));
    assert(allNames.size() == 3);
    assert(std::count(allNames.begin(), allNames.end(), "a") == 1);
    assert(std::count(allNames.begin(), allNames.end(), "b") == 1);
    assert(std::count(allNames.begin(), allNames.end(), "shared") == 1);
    assert(std::find(allNames.begin(), allNames.end(), "$objvarinit") == allNames.end());

    // flag&2: leaf's own directly-defined functions only, "a" (inherited
    // only, never redefined by leaf) excluded.
    std::vector<amlp::Value> ownArgs{ amlp::Value(leaf), amlp::Value(int64_t{2}) };
    amlp::Value own = amlp::EfunTable::instance().call("functions", harness.vm, ownArgs);
    auto ownArr = std::get<std::shared_ptr<amlp::Array>>(own.data);
    std::vector<std::string> ownNames;
    for (auto& v : ownArr->items) ownNames.push_back(std::get<std::string>(v.data));
    assert(ownNames.size() == 2);
    assert(std::count(ownNames.begin(), ownNames.end(), "b") == 1);
    assert(std::count(ownNames.begin(), ownNames.end(), "shared") == 1);
    assert(std::find(ownNames.begin(), ownNames.end(), "a") == ownNames.end());

    std::cout << "testFunctionsListsOwnAndInheritedNamesWithOverridePrecedence OK\n";
}

static void testFunctionsDetailedFormIncludesNumArgsAndMixedTypePlaceholders() {
    ObjectVarHarness harness;
    harness.writeFile("/fn_detail.c", "int add(int x, int y) { return x + y; }\n");
    auto ob = harness.objects.cloneObject("/fn_detail");
    assert(ob != nullptr);

    std::vector<amlp::Value> detailArgs{ amlp::Value(ob), amlp::Value(int64_t{1}) };
    amlp::Value detail = amlp::EfunTable::instance().call("functions", harness.vm, detailArgs);
    auto arr = std::get<std::shared_ptr<amlp::Array>>(detail.data);
    assert(arr->items.size() == 1);
    auto sub = std::get<std::shared_ptr<amlp::Array>>(arr->items[0].data);
    // [name, num_args, return_type, arg_type...] -- real f_functions()'s
    // own subvec layout; no declared-type metadata exists in this
    // driver's own CompiledProgram, so every type slot is the fixed
    // "mixed" placeholder documented in EfunTable.cpp's own comment.
    assert(sub->items.size() == 5);
    assert(std::get<std::string>(sub->items[0].data) == "add");
    assert(std::get<int64_t>(sub->items[1].data) == 2);
    assert(std::get<std::string>(sub->items[2].data) == "mixed");
    assert(std::get<std::string>(sub->items[3].data) == "mixed");
    assert(std::get<std::string>(sub->items[4].data) == "mixed");

    std::cout << "testFunctionsDetailedFormIncludesNumArgsAndMixedTypePlaceholders OK\n";
}

static void testVariablesListsFlattenedNamesInInheritedThenOwnOrder() {
    ObjectVarHarness harness;
    harness.writeFile("/var_base.c", "int x;\n");
    harness.writeFile("/var_leaf.c", "inherit \"/var_base\";\nint y;\n");
    auto leaf = harness.objects.cloneObject("/var_leaf");
    assert(leaf != nullptr);

    std::vector<amlp::Value> bareArgs{ amlp::Value(leaf) };
    amlp::Value bare = amlp::EfunTable::instance().call("variables", harness.vm, bareArgs);
    auto bareArr = std::get<std::shared_ptr<amlp::Array>>(bare.data);
    assert(bareArr->items.size() == 2);
    assert(std::get<std::string>(bareArr->items[0].data) == "x");
    assert(std::get<std::string>(bareArr->items[1].data) == "y");

    std::vector<amlp::Value> pairArgs{ amlp::Value(leaf), amlp::Value(int64_t{1}) };
    amlp::Value pair = amlp::EfunTable::instance().call("variables", harness.vm, pairArgs);
    auto pairArr = std::get<std::shared_ptr<amlp::Array>>(pair.data);
    assert(pairArr->items.size() == 2);
    auto first = std::get<std::shared_ptr<amlp::Array>>(pairArr->items[0].data);
    assert(std::get<std::string>(first->items[0].data) == "x");
    assert(std::get<std::string>(first->items[1].data) == "mixed");

    std::cout << "testVariablesListsFlattenedNamesInInheritedThenOwnOrder OK\n";
}

static void testFetchAndStoreVariableRoundTripByNameAndThrowOnUnknownName() {
    ObjectVarHarness harness;
    harness.writeFile("/fv_probe.c",
        "int x = 5;\n"
        "mixed do_fetch(string n) { return fetch_variable(n); }\n"
        "void do_store(string n, mixed v) { store_variable(n, v); }\n");
    auto ob = harness.objects.cloneObject("/fv_probe");
    assert(ob != nullptr);

    amlp::Value fetched = harness.vm.callFunction(ob, "do_fetch",
        { amlp::Value(std::string("x")) });
    assert(std::get<int64_t>(fetched.data) == 5);

    harness.vm.callFunction(ob, "do_store",
        { amlp::Value(std::string("x")), amlp::Value(int64_t{42}) });
    amlp::Value refetched = harness.vm.callFunction(ob, "do_fetch",
        { amlp::Value(std::string("x")) });
    assert(std::get<int64_t>(refetched.data) == 42);

    bool fetchThrew = false;
    try {
        harness.vm.callFunction(ob, "do_fetch", { amlp::Value(std::string("nope")) });
    } catch (const amlp::LpcRuntimeError&) {
        fetchThrew = true;
    }
    assert(fetchThrew);

    bool storeThrew = false;
    try {
        harness.vm.callFunction(ob, "do_store",
            { amlp::Value(std::string("nope")), amlp::Value(int64_t{1}) });
    } catch (const amlp::LpcRuntimeError&) {
        storeThrew = true;
    }
    assert(storeThrew);

    std::cout << "testFetchAndStoreVariableRoundTripByNameAndThrowOnUnknownName OK\n";
}

static void testGetConfigReturnsMudNameForIndexZeroAndThrowsForNegative() {
    ObjectVarHarness harness;
    std::vector<amlp::Value> zeroArgs{ amlp::Value(int64_t{0}) };
    amlp::Value name = amlp::EfunTable::instance().call("get_config", harness.vm, zeroArgs);
    assert(std::get<std::string>(name.data) == "AMLP");

    std::vector<amlp::Value> negArgs{ amlp::Value(int64_t{-1}) };
    bool threw = false;
    try {
        amlp::EfunTable::instance().call("get_config", harness.vm, negArgs);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testGetConfigReturnsMudNameForIndexZeroAndThrowsForNegative OK\n";
}

static void testQueryLoadAverageReturnsAStringInRealFormat() {
    ObjectVarHarness harness;
    std::vector<amlp::Value> noArgs;
    amlp::Value result = amlp::EfunTable::instance().call("query_load_average", harness.vm, noArgs);
    auto* s = std::get_if<std::string>(&result.data);
    assert(s != nullptr);
    assert(s->find("cmds/s") != std::string::npos);
    assert(s->find("comp lines/s") != std::string::npos);

    std::cout << "testQueryLoadAverageReturnsAStringInRealFormat OK\n";
}

// Real send_say(): the surrounding object and everyone else in it (except
// origin itself) are eligible targets; origin never receives its own
// message even when it has its own live connection.
static void testSayBroadcastsToRoomSiblingsButNotOriginItself() {
    ObjectVarHarness harness;
    harness.writeFile("/say_room.c", "void create() {}\n");
    harness.writeFile("/say_mover.c", "void go(object dest) { move_object(dest); }\n");
    harness.writeFile("/say_actor.c",
        "void go(object dest) { move_object(dest); }\n"
        "void speak(string s) { say(s); }\n");

    auto room = harness.objects.cloneObject("/say_room");
    auto actor = harness.objects.cloneObject("/say_actor");
    auto listener = harness.objects.cloneObject("/say_mover");
    assert(room && actor && listener);

    harness.vm.callFunction(actor, "go", {amlp::Value(room)});
    harness.vm.callFunction(listener, "go", {amlp::Value(room)});

    int fdsActor[2];
    int fdsListener[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsActor) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsListener) == 0);
    amlp::Connection connActor(fdsActor[0]);
    amlp::Connection connListener(fdsListener[0]);
    connActor.attach(actor);
    connListener.attach(listener);

    harness.vm.callFunction(actor, "speak", {amlp::Value(std::string("hi there\n"))});

    char buf[256];
    ssize_t n = ::recv(fdsListener[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    assert(std::string(buf, static_cast<size_t>(n)) == "hi there\n");

    // The speaker's own peer socket must receive nothing at all.
    ssize_t nActor = ::recv(fdsActor[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(nActor < 0);

    ::close(fdsActor[1]);
    ::close(fdsListener[1]);
    std::cout << "testSayBroadcastsToRoomSiblingsButNotOriginItself OK\n";
}

static void testSayAvoidArgumentExcludesSpecifiedTarget() {
    ObjectVarHarness harness;
    harness.writeFile("/say_room2.c", "void create() {}\n");
    harness.writeFile("/say_mover2.c", "void go(object dest) { move_object(dest); }\n");
    harness.writeFile("/say_actor2.c",
        "void go(object dest) { move_object(dest); }\n"
        "void speak(string s, object avoid) { say(s, avoid); }\n");

    auto room = harness.objects.cloneObject("/say_room2");
    auto actor = harness.objects.cloneObject("/say_actor2");
    auto keep = harness.objects.cloneObject("/say_mover2");
    auto skip = harness.objects.cloneObject("/say_mover2");
    assert(room && actor && keep && skip);

    harness.vm.callFunction(actor, "go", {amlp::Value(room)});
    harness.vm.callFunction(keep, "go", {amlp::Value(room)});
    harness.vm.callFunction(skip, "go", {amlp::Value(room)});

    int fdsKeep[2];
    int fdsSkip[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsKeep) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsSkip) == 0);
    amlp::Connection connKeep(fdsKeep[0]);
    amlp::Connection connSkip(fdsSkip[0]);
    connKeep.attach(keep);
    connSkip.attach(skip);

    harness.vm.callFunction(actor, "speak", {amlp::Value(std::string("secret\n")), amlp::Value(skip)});

    char buf[256];
    ssize_t nKeep = ::recv(fdsKeep[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(nKeep > 0);
    assert(std::string(buf, static_cast<size_t>(nKeep)) == "secret\n");

    ssize_t nSkip = ::recv(fdsSkip[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(nSkip < 0);

    ::close(fdsKeep[1]);
    ::close(fdsSkip[1]);
    std::cout << "testSayAvoidArgumentExcludesSpecifiedTarget OK\n";
}

// Matches this repo's own bundled Lil starter mudlib's real conformance
// test for this efun exactly (mudlib/single/tests/efuns/save_variable.c):
// an embedded '\n' saves as a literal '\r' byte, '"'/'\\' are backslash-
// escaped, and arrays/mappings always carry a trailing comma before the
// closing delimiter, even after the last element.
static void testSaveVariableMatchesRealFormatForStringsNumbersArraysAndMappings() {
    ObjectVarHarness harness;

    auto save = [&](amlp::Value v) -> std::string {
        std::vector<amlp::Value> args{ std::move(v) };
        return std::get<std::string>(amlp::EfunTable::instance().call("save_variable", harness.vm, args).data);
    };

    assert(save(amlp::Value(std::string("\n"))) == std::string("\"\r\""));
    assert(save(amlp::Value(std::string("\""))) == "\"\\\"\"");
    assert(save(amlp::Value(int64_t{-1})) == "-1");
    assert(save(amlp::Value(int64_t{22})) == "22");
    assert(save(amlp::Value(1.2)) == "1.200000");

    auto emptyArr = std::make_shared<amlp::Array>();
    assert(save(amlp::Value(emptyArr)) == "({})");

    auto oneArr = std::make_shared<amlp::Array>();
    oneArr->items.push_back(amlp::Value(int64_t{0}));
    assert(save(amlp::Value(oneArr)) == "({0,})");

    auto oneMap = std::make_shared<amlp::Mapping>();
    oneMap->entries.emplace_back(amlp::Value(int64_t{1}), amlp::Value(int64_t{22}));
    assert(save(amlp::Value(oneMap)) == "([1:22,])");

    std::cout << "testSaveVariableMatchesRealFormatForStringsNumbersArraysAndMappings OK\n";
}

// Matches this repo's own bundled Lil starter mudlib's real conformance
// test for this efun exactly (mudlib/single/tests/efuns/
// restore_variable.c): round-trips save_variable()'s own output, and
// rejects the same five malformed shapes that real restore_svalue()
// (object.c) rejects -- trailing garbage after a top-level quoted
// string (both the plain and escaped exit paths) and a lone '-' with no
// digit after it.
static void testRestoreVariableRoundTripsSaveVariableOutputAndRejectsMalformedInput() {
    ObjectVarHarness harness;

    auto restore = [&](const std::string& s) -> amlp::Value {
        std::vector<amlp::Value> args{ amlp::Value(s) };
        return amlp::EfunTable::instance().call("restore_variable", harness.vm, args);
    };
    auto throws = [&](const std::string& s) -> bool {
        try {
            restore(s);
        } catch (const amlp::LpcRuntimeError&) {
            return true;
        }
        return false;
    };

    assert(std::get<std::string>(restore("\"\r\"").data) == "\n");
    assert(std::get<std::string>(restore("\"\\\"\"").data) == "\"");
    assert(std::get<int64_t>(restore("-1").data) == -1);
    assert(std::get<int64_t>(restore("22").data) == 22);
    assert(std::get<double>(restore("1.200000").data) == 1.2);

    auto arr = std::get<std::shared_ptr<amlp::Array>>(restore("({0,})").data);
    assert(arr && arr->items.size() == 1);
    assert(std::get<int64_t>(arr->items[0].data) == 0);

    auto map = std::get<std::shared_ptr<amlp::Mapping>>(restore("([1:22,])").data);
    assert(map && map->entries.size() == 1);
    assert(std::get<int64_t>(map->entries[0].first.data) == 1);
    assert(std::get<int64_t>(map->entries[0].second.data) == 22);

    // The five real value_errs shapes from Lil's own test content.
    assert(throws("\"\"x"));    // trailing garbage after an empty plain string
    assert(throws("\"\\"));     // unterminated escape
    assert(throws("\"\\x\\")); // unterminated escape after a real char
    assert(throws("\"\\x\"x")); // trailing garbage after an escaped string
    assert(throws("-x"));       // '-' with no digit after it

    std::cout << "testRestoreVariableRoundTripsSaveVariableOutputAndRejectsMalformedInput OK\n";
}

// -----------------------------------------------------------------------
// Phase 0.13 efun growth batch, continued: children, set_light,
// set_debug_level, bind.
// -----------------------------------------------------------------------

// Mirrors this repo's own bundled Lil starter mudlib's real conformance
// test for this efun (mudlib/single/tests/efuns/children.c): every
// currently-loaded clone of the same file, plus the file's own first
// loaded instance, all match a plain prefix lookup by filename.
static void testChildrenReturnsEveryLiveObjectMatchingFilenamePrefix() {
    ObjectVarHarness harness;
    harness.writeFile("/children_probe.c", "void create() {}\n");
    harness.writeFile("/children_other.c", "void create() {}\n");

    // LiveObjectRegistry entries are weak_ptr (see its own header
    // comment) -- a clone this driver's own C++ reference-counting model
    // has no other owner for is not enumerable here, unlike real
    // FluffOS's own persistent, refcount-independent object table.
    // Every clone this test wants children() to see is kept alive in
    // this vector for exactly that reason, deliberately, not an
    // oversight -- matching how a real clone stays alive in practice via
    // an environment/inventory slot or some other genuine reference.
    std::vector<std::shared_ptr<amlp::LpcObject>> liveClones;
    liveClones.push_back(harness.objects.cloneObject("/children_probe"));
    assert(liveClones.back() != nullptr);
    for (int i = 0; i < 4; ++i) {
        liveClones.push_back(harness.objects.cloneObject("/children_probe"));
        assert(liveClones.back() != nullptr);
    }
    auto unrelated = harness.objects.cloneObject("/children_other");
    assert(unrelated != nullptr);

    // Real __FILE__ always carries ".c" -- confirmed the driver's own
    // extension-less LpcObject::filename() storage still matches it.
    std::vector<amlp::Value> args{ amlp::Value(std::string("/children_probe.c")) };
    amlp::Value result = amlp::EfunTable::instance().call("children", harness.vm, args);
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && *arr);
    assert((*arr)->items.size() == 5);
    for (auto& item : (*arr)->items) {
        auto* ob = std::get_if<std::shared_ptr<amlp::LpcObject>>(&item.data);
        assert(ob != nullptr && *ob);
        assert((*ob)->filename() == "/children_probe");
    }

    std::cout << "testChildrenReturnsEveryLiveObjectMatchingFilenamePrefix OK\n";
}

// Real add_light(): the delta propagates up through every ancestor
// environment, and set_light() itself returns the *topmost* ancestor's
// own resulting total, not the calling object's own total.
static void testSetLightAccumulatesUpThroughEnvironmentChainAndReturnsRootTotal() {
    ObjectVarHarness harness;
    harness.writeFile("/light_root.c", "void create() {}\n");
    harness.writeFile("/light_mid.c", "void go(object dest) { move_object(dest); }\n");
    harness.writeFile("/light_leaf.c",
        "void go(object dest) { move_object(dest); }\n"
        "int shine(int n) { return set_light(n); }\n");

    auto root = harness.objects.cloneObject("/light_root");
    auto mid = harness.objects.cloneObject("/light_mid");
    auto leaf = harness.objects.cloneObject("/light_leaf");
    assert(root && mid && leaf);

    harness.vm.callFunction(mid, "go", {amlp::Value(root)});
    harness.vm.callFunction(leaf, "go", {amlp::Value(mid)});

    amlp::Value result = harness.vm.callFunction(leaf, "shine", {amlp::Value(int64_t{3})});
    assert(std::get<int64_t>(result.data) == 3);
    assert(leaf->totalLight() == 3);
    assert(mid->totalLight() == 3);
    assert(root->totalLight() == 3);

    // A second call accumulates rather than replacing.
    amlp::Value second = harness.vm.callFunction(leaf, "shine", {amlp::Value(int64_t{2})});
    assert(std::get<int64_t>(second.data) == 5);
    assert(root->totalLight() == 5);

    std::cout << "testSetLightAccumulatesUpThroughEnvironmentChainAndReturnsRootTotal OK\n";
}

static void testSetDebugLevelAcceptsIntOrStringWithoutThrowing() {
    ObjectVarHarness harness;
    std::vector<amlp::Value> intArgs{ amlp::Value(int64_t{10}) };
    amlp::EfunTable::instance().call("set_debug_level", harness.vm, intArgs);
    std::vector<amlp::Value> stringArgs{ amlp::Value(std::string("connections")) };
    amlp::EfunTable::instance().call("set_debug_level", harness.vm, stringArgs);

    std::cout << "testSetDebugLevelAcceptsIntOrStringWithoutThrowing OK\n";
}

// All bind tests use a master object defining a real (permissive)
// valid_bind(), matching the established set_hide()/shadow()-style
// harness pattern -- real bind() always denies without one.
static void testBindRebindsClosureOwnerAndChangesResolution() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_bind(object binder, object old_owner, object new_owner) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/bind_owner_a.c",
        "int greet() { return 111; }\n"
        "mixed make() { return (: greet :); }\n");
    harness.writeFile("/bind_owner_b.c", "int greet() { return 222; }\n");

    auto ownerA = harness.objects.cloneObject("/bind_owner_a");
    auto ownerB = harness.objects.cloneObject("/bind_owner_b");
    assert(ownerA && ownerB);

    amlp::Value closureVal = harness.vm.callFunction(ownerA, "make", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::Closure>>(closureVal.data));

    amlp::Value beforeCall =
        harness.vm.callClosure(std::get<std::shared_ptr<amlp::Closure>>(closureVal.data), {});
    assert(std::get<int64_t>(beforeCall.data) == 111);

    std::vector<amlp::Value> bindArgs{ closureVal, amlp::Value(ownerB) };
    amlp::Value rebound = amlp::EfunTable::instance().call("bind", harness.vm, bindArgs);
    assert(std::holds_alternative<std::shared_ptr<amlp::Closure>>(rebound.data));

    amlp::Value afterCall =
        harness.vm.callClosure(std::get<std::shared_ptr<amlp::Closure>>(rebound.data), {});
    assert(std::get<int64_t>(afterCall.data) == 222);

    std::cout << "testBindRebindsClosureOwnerAndChangesResolution OK\n";
}

static void testBindIsNoOpWhenNewOwnerMatchesCurrentOwner() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_bind(object binder, object old_owner, object new_owner) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/bind_self.c",
        "int greet() { return 42; }\n"
        "mixed make() { return (: greet :); }\n");
    auto ob = harness.objects.cloneObject("/bind_self");
    assert(ob != nullptr);

    amlp::Value closureVal = harness.vm.callFunction(ob, "make", {});
    std::vector<amlp::Value> bindArgs{ closureVal, amlp::Value(ob) };
    amlp::Value result = amlp::EfunTable::instance().call("bind", harness.vm, bindArgs);
    assert(std::get<std::shared_ptr<amlp::Closure>>(result.data) ==
           std::get<std::shared_ptr<amlp::Closure>>(closureVal.data));

    std::cout << "testBindIsNoOpWhenNewOwnerMatchesCurrentOwner OK\n";
}

static void testBindThrowsWhenNoMasterIsLoaded() {
    ObjectVarHarness harness; // no master loaded at all
    harness.writeFile("/bind_owner_c.c",
        "int greet() { return 1; }\n"
        "mixed make() { return (: greet :); }\n");
    harness.writeFile("/bind_owner_d.c", "int greet() { return 2; }\n");
    auto ownerC = harness.objects.cloneObject("/bind_owner_c");
    auto ownerD = harness.objects.cloneObject("/bind_owner_d");
    assert(ownerC && ownerD);

    amlp::Value closureVal = harness.vm.callFunction(ownerC, "make", {});
    std::vector<amlp::Value> bindArgs{ closureVal, amlp::Value(ownerD) };
    bool threw = false;
    try {
        amlp::EfunTable::instance().call("bind", harness.vm, bindArgs);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testBindThrowsWhenNoMasterIsLoaded OK\n";
}

// --- tell_object / tell_room / shout / this_interactive / this_user /
// map_mapping / filter_mapping (Phase 0.13, batch continued 2026-08-22) --
// Confirmed directly against fluffos-2.9-ds2.08's own object.c
// (tell_object), simulate.c (tell_room, shout_string), efun_defs.c
// (this_interactive/this_user as F_THIS_PLAYER|F_ALIAS_FLAG), and
// mapping.c (map_mapping, filter_mapping) before writing any of this --
// see EfunTable.cpp's own registrations for the full real-semantics
// writeup, including shout()'s own documented, deliberate departure from
// literal (but provably dead-in-this-build) O_LISTENER semantics.

static void testTellObjectWritesToConnectionOrCallsCatchTellWhenNotInteractive() {
    ObjectVarHarness harness;
    harness.writeFile("/to1_interactive.c", "void create() {}\n");
    harness.writeFile("/to1_npc.c",
        "string got = \"\";\n"
        "void catch_tell(string s) { got += s; }\n"
        "string get_got() { return got; }\n");
    harness.writeFile("/to1_caller.c",
        "void ping(object target, string s) { tell_object(target, s); }\n");
    auto interactiveOb = harness.objects.cloneObject("/to1_interactive");
    auto npc = harness.objects.cloneObject("/to1_npc");
    auto caller = harness.objects.cloneObject("/to1_caller");
    assert(interactiveOb != nullptr && npc != nullptr && caller != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(interactiveOb);

    harness.vm.callFunction(caller, "ping",
        {amlp::Value(interactiveOb), amlp::Value(std::string("hi there\n"))});
    char buf[64];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    assert(std::string(buf, static_cast<size_t>(n)) == "hi there\n");

    harness.vm.callFunction(caller, "ping", {amlp::Value(npc), amlp::Value(std::string("npc msg\n"))});
    amlp::Value got = harness.vm.callFunction(npc, "get_got", {});
    assert(std::holds_alternative<std::string>(got.data));
    assert(std::get<std::string>(got.data) == "npc msg\n");

    ::close(fds[1]);
    std::cout << "testTellObjectWritesToConnectionOrCallsCatchTellWhenNotInteractive OK\n";
}

static void testTellRoomBroadcastsToDirectInventoryExcludingAvoid() {
    ObjectVarHarness harness;
    harness.writeFile("/tr1_room.c", "void create() {}\n");
    harness.writeFile("/tr1_a.c", "void create() {}\n");
    harness.writeFile("/tr1_b.c", "void create() {}\n");
    harness.writeFile("/tr1_c.c", "void create() {}\n");
    harness.writeFile("/tr1_caller.c",
        "void announce(object room, object avoidOb) { tell_room(room, \"hey\\n\", avoidOb); }\n");
    auto room = harness.objects.cloneObject("/tr1_room");
    auto a = harness.objects.cloneObject("/tr1_a");
    auto b = harness.objects.cloneObject("/tr1_b");
    auto c = harness.objects.cloneObject("/tr1_c");
    auto caller = harness.objects.cloneObject("/tr1_caller");
    assert(room && a && b && c && caller);

    harness.vm.moveObject(a, room);
    harness.vm.moveObject(b, room);
    harness.vm.moveObject(c, room);

    int fdsA[2], fdsB[2], fdsC[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsA) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsB) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsC) == 0);
    amlp::Connection connA(fdsA[0]);
    amlp::Connection connB(fdsB[0]);
    amlp::Connection connC(fdsC[0]);
    connA.attach(a);
    connB.attach(b);
    connC.attach(c);

    harness.vm.callFunction(caller, "announce", {amlp::Value(room), amlp::Value(b)});

    char buf[64];
    ssize_t n = ::recv(fdsA[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    assert(std::string(buf, static_cast<size_t>(n)) == "hey\n");

    n = ::recv(fdsB[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n < 0); // avoided -- nothing received

    n = ::recv(fdsC[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    assert(std::string(buf, static_cast<size_t>(n)) == "hey\n");

    ::close(fdsA[1]);
    ::close(fdsB[1]);
    ::close(fdsC[1]);
    std::cout << "testTellRoomBroadcastsToDirectInventoryExcludingAvoid OK\n";
}

static void testShoutBroadcastsToEveryoneExceptCommandGiver() {
    ObjectVarHarness harness;
    harness.writeFile("/sh_a.c", "void create() {}\n");
    harness.writeFile("/sh_b.c", "void create() {}\n");
    harness.writeFile("/sh_caller.c", "void yell(string s) { shout(s); }\n");
    auto a = harness.objects.cloneObject("/sh_a");
    auto b = harness.objects.cloneObject("/sh_b");
    auto caller = harness.objects.cloneObject("/sh_caller");
    assert(a && b && caller);

    int fdsA[2], fdsB[2], fdsCaller[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsA) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsB) == 0);
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fdsCaller) == 0);
    amlp::Connection connA(fdsA[0]);
    amlp::Connection connB(fdsB[0]);
    amlp::Connection connCaller(fdsCaller[0]);
    connA.attach(a);
    connB.attach(b);
    connCaller.attach(caller);

    harness.vm.pushCommandGiver(caller);
    harness.vm.callFunction(caller, "yell", {amlp::Value(std::string("hear ye\n"))});
    harness.vm.popCommandGiver();

    char buf[64];
    ssize_t n = ::recv(fdsA[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    assert(std::string(buf, static_cast<size_t>(n)) == "hear ye\n");

    n = ::recv(fdsB[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    assert(std::string(buf, static_cast<size_t>(n)) == "hear ye\n");

    n = ::recv(fdsCaller[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n < 0); // command_giver excluded

    ::close(fdsA[1]);
    ::close(fdsB[1]);
    ::close(fdsCaller[1]);
    std::cout << "testShoutBroadcastsToEveryoneExceptCommandGiver OK\n";
}

static void testThisInteractiveAndThisUserReturnConnectionBoundObjectNotCommandGiver() {
    ObjectVarHarness harness;
    harness.writeFile("/ti1_conn.c",
        "mixed probe_player() { return this_player(); }\n"
        "mixed probe_interactive() { return this_interactive(); }\n"
        "mixed probe_user() { return this_user(); }\n");
    harness.writeFile("/ti1_other.c", "void create() {}\n");
    auto connOb = harness.objects.cloneObject("/ti1_conn");
    auto otherOb = harness.objects.cloneObject("/ti1_other");
    assert(connOb != nullptr && otherOb != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(connOb);

    amlp::OutputContext::set(&conn);
    // command_giver reassigned away from the connection's own bound
    // object -- this_player() must follow the reassignment,
    // this_interactive()/this_user() must not.
    harness.vm.pushCommandGiver(otherOb);

    amlp::Value playerResult = harness.vm.callFunction(connOb, "probe_player", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(playerResult.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(playerResult.data) == otherOb);

    amlp::Value interactiveResult = harness.vm.callFunction(connOb, "probe_interactive", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(interactiveResult.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(interactiveResult.data) == connOb);

    amlp::Value userResult = harness.vm.callFunction(connOb, "probe_user", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::LpcObject>>(userResult.data));
    assert(std::get<std::shared_ptr<amlp::LpcObject>>(userResult.data) == connOb);

    harness.vm.popCommandGiver();
    amlp::OutputContext::set(nullptr);
    ::close(fds[1]);
    std::cout << "testThisInteractiveAndThisUserReturnConnectionBoundObjectNotCommandGiver OK\n";
}

static void testMapMappingReplacesValuesKeepingKeysViaStringFunctionName() {
    ObjectVarHarness harness;
    harness.writeFile("/mm_target.c",
        "int times_ten(string key, int value) { return value * 10; }\n");
    harness.writeFile("/mm_caller.c",
        "mixed probe(object target) {\n"
        "    mapping m = ([\"a\": 1, \"b\": 2]);\n"
        "    return map_mapping(m, \"times_ten\", target);\n"
        "}\n");
    auto target = harness.objects.cloneObject("/mm_target");
    auto caller = harness.objects.cloneObject("/mm_caller");
    assert(target != nullptr && caller != nullptr);

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(target)});
    auto* mapPtr = std::get_if<std::shared_ptr<amlp::Mapping>>(&result.data);
    assert(mapPtr != nullptr && *mapPtr != nullptr);
    assert((*mapPtr)->entries.size() == 2);
    for (auto& entry : (*mapPtr)->entries) {
        const std::string& key = std::get<std::string>(entry.first.data);
        int64_t value = std::get<int64_t>(entry.second.data);
        if (key == "a") assert(value == 10);
        else if (key == "b") assert(value == 20);
        else assert(false);
    }
    std::cout << "testMapMappingReplacesValuesKeepingKeysViaStringFunctionName OK\n";
}

static void testFilterMappingKeepsOnlyEntriesWhereCallbackIsTruthy() {
    ObjectVarHarness harness;
    harness.writeFile("/fm_target.c",
        "int value_over_one(string key, int value) { return value > 1; }\n");
    harness.writeFile("/fm_caller.c",
        "mixed probe(object target) {\n"
        "    mapping m = ([\"a\": 1, \"b\": 2, \"c\": 3]);\n"
        "    return filter_mapping(m, \"value_over_one\", target);\n"
        "}\n");
    auto target = harness.objects.cloneObject("/fm_target");
    auto caller = harness.objects.cloneObject("/fm_caller");
    assert(target != nullptr && caller != nullptr);

    amlp::Value result = harness.vm.callFunction(caller, "probe", {amlp::Value(target)});
    auto* mapPtr = std::get_if<std::shared_ptr<amlp::Mapping>>(&result.data);
    assert(mapPtr != nullptr && *mapPtr != nullptr);
    assert((*mapPtr)->entries.size() == 2);
    for (auto& entry : (*mapPtr)->entries) {
        const std::string& key = std::get<std::string>(entry.first.data);
        assert(key == "b" || key == "c");
    }
    std::cout << "testFilterMappingKeepsOnlyEntriesWhereCallbackIsTruthy OK\n";
}

// --- strwidth / reset_eval_cost / remove_shadow / oldcrypt / next_bit /
// element_of / shuffle (Phase 0.13, batch continued 2026-08-22) --
// Confirmed directly against fluffos-2.9-ds2.08's own efun_defs.c
// (strwidth/reset_eval_cost aliases), packages/contrib.c
// (remove_shadow/oldcrypt/element_of/shuffle), and efuns_main.c
// (next_bit) before writing any of this -- see EfunTable.cpp's own
// registrations for the full real-semantics writeup.

static void testStrwidthReturnsSameLengthAsSizeof() {
    ObjectVarHarness harness;
    harness.writeFile("/sw_probe.c", "int probe(string s) { return strwidth(s); }\n");
    auto ob = harness.objects.cloneObject("/sw_probe");
    assert(ob != nullptr);
    amlp::Value result = harness.vm.callFunction(ob, "probe", {amlp::Value(std::string("hello"))});
    assert(std::get<int64_t>(result.data) == 5);
    std::cout << "testStrwidthReturnsSameLengthAsSizeof OK\n";
}

// set_eval_limit()/reset_eval_cost()/eval_cost()/max_eval_cost(): all
// four real names share one dispatch keyed on the argument value itself
// (efuns_main.c's own f_set_eval_limit(), confirmed directly before
// writing any of this -- an earlier version of this test encoded a wrong,
// unverified assumption that reset_eval_cost()'s own default meant
// "lower the ceiling to zero", which real code does not do at all; see
// EfunTable.cpp's own corrected registration comment for the full
// citation). Called directly through EfunTable rather than an LPC
// wrapper for the parts that probe eval-cost state around the calls: a
// bytecode-executed wrapper function would consume its own ticks against
// whatever ceiling is active, the same self-tripping concern already
// documented for set_eval_limit above.

static void testResetEvalCostZeroesUsedCostLeavingCeilingUnchanged() {
    ObjectVarHarness harness;
    harness.writeFile("/rec_probe.c",
        "int burn() { int i; for (i = 0; i < 20; i++) {} return i; }\n");
    auto ob = harness.objects.cloneObject("/rec_probe");
    assert(ob != nullptr);

    // Measures the real per-call instruction cost first rather than
    // guessing a hardcoded ceiling low enough to trip on a second,
    // un-reset call but not a first -- this driver's own exact
    // instruction-cost accounting is an implementation detail this test
    // should not need to hardcode a guess against.
    harness.vm.setMaxEvalCost(1000000);
    harness.vm.resetEvalCost();
    harness.vm.callFunction(ob, "burn", {});
    int64_t costPerCall = harness.vm.evalCost();
    assert(costPerCall > 0);

    int64_t ceiling = costPerCall + costPerCall / 2;
    harness.vm.setMaxEvalCost(ceiling);
    harness.vm.resetEvalCost();
    amlp::Value first = harness.vm.callFunction(ob, "burn", {});
    assert(std::get<int64_t>(first.data) == 20);

    // No reset in between: cost accumulates across calls (real cross-
    // dispatch accumulation, matching VM.hpp's own documented reset
    // points), so repeating the same burn on top of what is already used
    // trips the still-unchanged ceiling.
    bool threw = false;
    try {
        harness.vm.callFunction(ob, "burn", {});
    } catch (const amlp::EvalCostError&) {
        threw = true;
    }
    assert(threw);

    // reset_eval_cost() with no arguments (real default 0): zeroes the
    // *used* cost back to zero while leaving the ceiling itself
    // completely untouched, and returns that unchanged ceiling.
    std::vector<amlp::Value> noArgs;
    amlp::Value resetResult = amlp::EfunTable::instance().call("reset_eval_cost", harness.vm, noArgs);
    assert(std::get<int64_t>(resetResult.data) == ceiling);
    assert(harness.vm.evalCost() == 0);
    assert(harness.vm.maxEvalCost() == ceiling);

    amlp::Value second = harness.vm.callFunction(ob, "burn", {});
    assert(std::get<int64_t>(second.data) == 20);

    std::cout << "testResetEvalCostZeroesUsedCostLeavingCeilingUnchanged OK\n";
}

static void testEvalCostAndMaxEvalCostQueryWithoutMutatingStateThenExplicitArgumentSetsCeiling() {
    ObjectVarHarness harness;
    harness.writeFile("/ec_probe.c",
        "int burn() { int i; for (i = 0; i < 10; i++) {} return i; }\n");
    auto ob = harness.objects.cloneObject("/ec_probe");
    assert(ob != nullptr);

    harness.vm.setMaxEvalCost(1000000);
    harness.vm.resetEvalCost();
    harness.vm.callFunction(ob, "burn", {});
    int64_t usedAfterBurn = harness.vm.evalCost();
    assert(usedAfterBurn > 0 && usedAfterBurn < 1000000);

    std::vector<amlp::Value> noArgs;
    // max_eval_cost(): pure query of the ceiling, no mutation.
    amlp::Value ceiling = amlp::EfunTable::instance().call("max_eval_cost", harness.vm, noArgs);
    assert(std::get<int64_t>(ceiling.data) == 1000000);
    assert(harness.vm.maxEvalCost() == 1000000);
    assert(harness.vm.evalCost() == usedAfterBurn);

    // eval_cost(): pure query of the *remaining* budget, no mutation.
    amlp::Value remaining = amlp::EfunTable::instance().call("eval_cost", harness.vm, noArgs);
    assert(std::get<int64_t>(remaining.data) == 1000000 - usedAfterBurn);
    assert(harness.vm.maxEvalCost() == 1000000);
    assert(harness.vm.evalCost() == usedAfterBurn);

    // An explicit argument to any of the four names sets the ceiling
    // directly, same as set_eval_limit(x) -- all four share one real
    // dispatch keyed on the argument value, not the name used to call it.
    std::vector<amlp::Value> explicitArgs{amlp::Value(int64_t{500})};
    amlp::Value setResult = amlp::EfunTable::instance().call("eval_cost", harness.vm, explicitArgs);
    assert(std::get<int64_t>(setResult.data) == 500);
    assert(harness.vm.maxEvalCost() == 500);

    std::cout << "testEvalCostAndMaxEvalCostQueryWithoutMutatingStateThenExplicitArgumentSetsCeiling OK\n";
}

static void testRemoveShadowSplicesOutOfChainAndReturnsZeroWhenNotShadowing() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());

    harness.writeFile("/rs1_victim.c", "void create() {}\n");
    harness.writeFile("/rs1_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n"
        "int unshadow() { return remove_shadow(this_object()); }\n");
    auto victim = harness.objects.cloneObject("/rs1_victim");
    auto sh = harness.objects.cloneObject("/rs1_shadow");
    assert(victim != nullptr && sh != nullptr);

    harness.vm.callFunction(sh, "attach", {amlp::Value(victim)});
    assert(victim->shadowedBy().lock() == sh);
    assert(sh->shadowing().lock() == victim);

    amlp::Value result = harness.vm.callFunction(sh, "unshadow", {});
    assert(std::get<int64_t>(result.data) == 1);
    assert(!victim->shadowedBy().lock());
    assert(!sh->shadowing().lock());

    // No longer part of any shadow relationship -- returns 0, not an error.
    amlp::Value again = harness.vm.callFunction(sh, "unshadow", {});
    assert(std::get<int64_t>(again.data) == 0);

    std::cout << "testRemoveShadowSplicesOutOfChainAndReturnsZeroWhenNotShadowing OK\n";
}

static void testOldcryptTruncatesSaltToFirstTwoCharactersUnlikeCrypt() {
    ObjectVarHarness harness;
    harness.writeFile("/oldcrypt_probe.c",
        "string probe(string pw, string salt) { return oldcrypt(pw, salt); }\n");
    auto ob = harness.objects.cloneObject("/oldcrypt_probe");
    assert(ob != nullptr);

    amlp::Value r1 = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("hunter2")), amlp::Value(std::string("ab"))});
    amlp::Value r2 = harness.vm.callFunction(ob, "probe",
        {amlp::Value(std::string("hunter2")), amlp::Value(std::string("abXXXXXX"))});
    const std::string& hash1 = std::get<std::string>(r1.data);
    const std::string& hash2 = std::get<std::string>(r2.data);
    // Both salts share the same first two characters ("ab"); oldcrypt()
    // only ever reads those, so the results are identical even though
    // the second call's own salt argument is much longer -- unlike
    // crypt(), which would use the whole thing.
    assert(hash1 == hash2);
    assert(hash1.rfind("ab", 0) == 0);

    std::cout << "testOldcryptTruncatesSaltToFirstTwoCharactersUnlikeCrypt OK\n";
}

static void testNextBitFindsFollowingSetBitWithRealBoundaryAsymmetry() {
    ObjectVarHarness harness;
    harness.writeFile("/nb_probe.c",
        "string make() {\n"
        "    string s = \"\";\n"
        "    s = set_bit(s, 0);\n"
        "    s = set_bit(s, 3);\n"
        "    s = set_bit(s, 10);\n"
        "    return s;\n"
        "}\n"
        "int probe(string s, int start) { return next_bit(s, start); }\n");
    auto ob = harness.objects.cloneObject("/nb_probe");
    assert(ob != nullptr);

    amlp::Value made = harness.vm.callFunction(ob, "make", {});
    assert(std::holds_alternative<std::string>(made.data));

    // start == 0 is inclusive: bit 0 is set, so it is returned itself.
    amlp::Value fromZero = harness.vm.callFunction(ob, "probe", {made, amlp::Value(int64_t{0})});
    assert(std::get<int64_t>(fromZero.data) == 0);

    // start > 0 is exclusive: next_bit(s, 3) must skip bit 3 itself even
    // though it is set, landing on bit 10 instead.
    amlp::Value fromThree = harness.vm.callFunction(ob, "probe", {made, amlp::Value(int64_t{3})});
    assert(std::get<int64_t>(fromThree.data) == 10);

    // Nothing left after the last set bit.
    amlp::Value fromTen = harness.vm.callFunction(ob, "probe", {made, amlp::Value(int64_t{10})});
    assert(std::get<int64_t>(fromTen.data) == -1);

    std::cout << "testNextBitFindsFollowingSetBitWithRealBoundaryAsymmetry OK\n";
}

static void testElementOfReturnsAMemberOfTheArrayAndThrowsWhenEmpty() {
    ObjectVarHarness harness;
    harness.writeFile("/eo_probe.c", "mixed probe(mixed *arr) { return element_of(arr); }\n");
    auto ob = harness.objects.cloneObject("/eo_probe");
    assert(ob != nullptr);

    auto arr = std::make_shared<amlp::Array>();
    arr->items.push_back(amlp::Value(int64_t{7}));
    arr->items.push_back(amlp::Value(int64_t{8}));
    arr->items.push_back(amlp::Value(int64_t{9}));
    for (int i = 0; i < 20; ++i) {
        amlp::Value result = harness.vm.callFunction(ob, "probe", {amlp::Value(arr)});
        int64_t v = std::get<int64_t>(result.data);
        assert(v == 7 || v == 8 || v == 9);
    }

    auto empty = std::make_shared<amlp::Array>();
    bool threw = false;
    try {
        harness.vm.callFunction(ob, "probe", {amlp::Value(empty)});
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testElementOfReturnsAMemberOfTheArrayAndThrowsWhenEmpty OK\n";
}

static void testShuffleReordersInPlaceAndKeepsSameElementsAndIdentity() {
    ObjectVarHarness harness;
    harness.writeFile("/sf_probe.c", "mixed *probe(mixed *arr) { return shuffle(arr); }\n");
    auto ob = harness.objects.cloneObject("/sf_probe");
    assert(ob != nullptr);

    auto arr = std::make_shared<amlp::Array>();
    for (int64_t i = 0; i < 20; ++i) arr->items.push_back(amlp::Value(i));

    amlp::Value result = harness.vm.callFunction(ob, "probe", {amlp::Value(arr)});
    auto* resultArr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(resultArr != nullptr && *resultArr == arr); // same identity, mutated in place

    assert(arr->items.size() == 20);
    std::vector<int64_t> seen;
    for (auto& v : arr->items) seen.push_back(std::get<int64_t>(v.data));
    std::sort(seen.begin(), seen.end());
    for (int64_t i = 0; i < 20; ++i) assert(seen[static_cast<size_t>(i)] == i);

    std::cout << "testShuffleReordersInPlaceAndKeepsSameElementsAndIdentity OK\n";
}

// --- real_time / remove_interactive / file_length / refs / heart_beats /
// query_ip_port (Phase 0.13, batch continued 2026-08-22) -- confirmed
// directly against fluffos-2.9-ds2.08's own packages/contrib.c
// (real_time, remove_interactive, file_length, query_ip_port) and
// packages/develop.c (refs) before writing any of this -- see
// EfunTable.cpp's own registrations for the full real-semantics writeup.

static void testRealTimeReturnsCurrentUnixTime() {
    ObjectVarHarness harness;
    harness.writeFile("/rt_probe.c", "int probe() { return real_time(); }\n");
    auto ob = harness.objects.cloneObject("/rt_probe");
    assert(ob != nullptr);

    int64_t before = static_cast<int64_t>(std::time(nullptr));
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    int64_t after = static_cast<int64_t>(std::time(nullptr));
    int64_t got = std::get<int64_t>(result.data);
    assert(got >= before && got <= after);

    std::cout << "testRealTimeReturnsCurrentUnixTime OK\n";
}

static void testRemoveInteractiveClosesConnectionWithoutDestructingAndReturnsZeroWhenNotInteractive() {
    ObjectVarHarness harness;
    harness.writeFile("/ri_probe.c", "void create() {}\n");
    auto ob = harness.objects.cloneObject("/ri_probe");
    assert(ob != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(ob);

    harness.writeFile("/ri_caller.c",
        "int disconnect(object target) { return remove_interactive(target); }\n");
    auto caller = harness.objects.cloneObject("/ri_caller");
    assert(caller != nullptr);

    amlp::Value result = harness.vm.callFunction(caller, "disconnect", {amlp::Value(ob)});
    assert(std::get<int64_t>(result.data) == 1);
    assert(!conn.isOpen());
    assert(!ob->isDestructed());
    assert(amlp::InteractiveRegistry::find(ob) == nullptr);

    // No longer interactive -- a second call is a no-op returning 0.
    amlp::Value again = harness.vm.callFunction(caller, "disconnect", {amlp::Value(ob)});
    assert(std::get<int64_t>(again.data) == 0);

    ::close(fds[1]);
    std::cout << "testRemoveInteractiveClosesConnectionWithoutDestructingAndReturnsZeroWhenNotInteractive OK\n";
}

static void testFileLengthCountsNewlinesAndReturnsNegativeForMissingOrDirectory() {
    ObjectVarHarness harness;
    harness.writeFile("/fl_three_lines.txt", "one\ntwo\nthree\n");
    harness.writeFile("/fl_probe.c", "int probe(string path) { return file_length(path); }\n");
    auto ob = harness.objects.cloneObject("/fl_probe");
    assert(ob != nullptr);

    amlp::Value three = harness.vm.callFunction(ob, "probe", {amlp::Value(std::string("/fl_three_lines.txt"))});
    assert(std::get<int64_t>(three.data) == 3);

    amlp::Value missing = harness.vm.callFunction(ob, "probe", {amlp::Value(std::string("/fl_does_not_exist.txt"))});
    assert(std::get<int64_t>(missing.data) == -1);

    amlp::Value dir = harness.vm.callFunction(ob, "probe", {amlp::Value(std::string("/"))});
    assert(std::get<int64_t>(dir.data) == -2);

    std::cout << "testFileLengthCountsNewlinesAndReturnsNegativeForMissingOrDirectory OK\n";
}

static void testRefsReflectsSharedReferenceCountMinusOne() {
    ObjectVarHarness harness;
    harness.writeFile("/refs_probe.c",
        "mixed *shared;\n"
        "void create() { shared = ({ 1, 2, 3 }); }\n"
        "int probe_shared() { mixed *alias = shared; return refs(alias); }\n"
        "int probe_fresh() { mixed *fresh = ({ 9 }); return refs(fresh); }\n"
        "int probe_string() { string s = \"hi\"; return refs(s); }\n");
    auto ob = harness.objects.cloneObject("/refs_probe");
    assert(ob != nullptr);

    // "alias" and the object's own "shared" variable both hold the same
    // underlying array -- at least one extra real reference beyond the
    // fresh local a truly-unshared array would have.
    amlp::Value sharedResult = harness.vm.callFunction(ob, "probe_shared", {});
    amlp::Value freshResult = harness.vm.callFunction(ob, "probe_fresh", {});
    assert(std::get<int64_t>(sharedResult.data) > std::get<int64_t>(freshResult.data));

    // This driver has no interned/shared string concept -- always 0.
    amlp::Value stringResult = harness.vm.callFunction(ob, "probe_string", {});
    assert(std::get<int64_t>(stringResult.data) == 0);

    std::cout << "testRefsReflectsSharedReferenceCountMinusOne OK\n";
}

static void testHeartBeatsListsEveryObjectWithHeartbeatEnabledSkippingDestructed() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/hb_a.c", "void create() {} void heart_beat() {}\n");
    harness.writeFile("/hb_b.c", "void create() {} void heart_beat() {}\n");
    auto a = harness.objects.cloneObject("/hb_a");
    auto b = harness.objects.cloneObject("/hb_b");
    assert(a != nullptr && b != nullptr);

    scheduler.setHeartbeatInterval(a, 1);
    scheduler.setHeartbeatInterval(b, 1);

    harness.writeFile("/hb_caller.c", "mixed *probe() { return heart_beats(); }\n");
    auto caller = harness.objects.cloneObject("/hb_caller");
    assert(caller != nullptr);

    amlp::Value result = harness.vm.callFunction(caller, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr && (*arr)->items.size() == 2);
    bool sawA = false, sawB = false;
    for (auto& item : (*arr)->items) {
        auto ob = std::get<std::shared_ptr<amlp::LpcObject>>(item.data);
        if (ob == a) sawA = true;
        if (ob == b) sawB = true;
    }
    assert(sawA && sawB);

    std::cout << "testHeartBeatsListsEveryObjectWithHeartbeatEnabledSkippingDestructed OK\n";
}

static void testQueryIpPortReturnsConfiguredPortForInteractiveElseZero() {
    ObjectVarHarness harness;
    harness.writeFile("/qip_probe.c", "int probe(object ob) { return query_ip_port(ob); }\n");
    harness.writeFile("/qip_plain.c", "void create() {}\n");
    auto probe = harness.objects.cloneObject("/qip_probe");
    auto interactive = harness.objects.cloneObject("/qip_plain");
    auto plain = harness.objects.cloneObject("/qip_plain");
    assert(probe != nullptr && interactive != nullptr && plain != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(interactive);

    amlp::Value connected = harness.vm.callFunction(probe, "probe", {amlp::Value(interactive)});
    assert(std::get<int64_t>(connected.data) == harness.config.port());

    amlp::Value notConnected = harness.vm.callFunction(probe, "probe", {amlp::Value(plain)});
    assert(std::get<int64_t>(notConnected.data) == 0);

    ::close(fds[1]);
    std::cout << "testQueryIpPortReturnsConfiguredPortForInteractiveElseZero OK\n";
}

// named_livings(): only objects with a real set_living_name() entry AND
// O_ENABLE_COMMANDS, matching real packages/contrib.c's own hashed_living[]
// walk -- distinct from livings() (every O_ENABLE_COMMANDS object, no
// living-name requirement at all). Also applies the same O_HIDDEN/
// valid_hide() gate first_inventory()/next_inventory() already exercise
// above, reusing the identical denying-master harness pattern.
static void testNamedLivingsListsOnlyLivingNamedObjectsWithCommandsEnabledRespectingHidden() {
    ObjectVarHarness denying;
    denying.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_hide(object ob) { return query_privs(ob) == \"wiz\"; }\n");
    assert(denying.objects.loadMasterObject());

    // named: real living name + enable_commands -- must appear.
    denying.writeFile("/nl_named.c",
        "void setup() { enable_commands(); set_living_name(\"alice\"); }\n");
    // no_commands: living name set, but enable_commands() never called --
    // real f_named_livings()'s own O_ENABLE_COMMANDS check must exclude it.
    denying.writeFile("/nl_no_commands.c",
        "void setup() { set_living_name(\"hermit\"); }\n");
    // unnamed: enable_commands() called, but no living name at all -- would
    // appear in livings() but never in named_livings().
    denying.writeFile("/nl_unnamed.c",
        "void setup() { enable_commands(); }\n");
    // hidden: real living name + enable_commands + set_hide(1) -- must be
    // skipped by an unprivileged observer, same as first_inventory()'s own
    // skip loop above.
    denying.writeFile("/nl_hidden.c",
        "void setup() { enable_commands(); set_living_name(\"shade\"); }\n"
        "void grant_wiz() { set_privs(this_object(), \"wiz\"); }\n"
        "void hide() { set_hide(1); }\n");
    denying.writeFile("/nl_probe.c", "object *probe() { return named_livings(); }\n");

    auto named = denying.objects.cloneObject("/nl_named");
    auto noCommands = denying.objects.cloneObject("/nl_no_commands");
    auto unnamed = denying.objects.cloneObject("/nl_unnamed");
    auto hidden = denying.objects.cloneObject("/nl_hidden");
    auto probe = denying.objects.cloneObject("/nl_probe");
    assert(named && noCommands && unnamed && hidden && probe);

    denying.vm.callFunction(named, "setup", {});
    denying.vm.callFunction(noCommands, "setup", {});
    denying.vm.callFunction(unnamed, "setup", {});
    denying.vm.callFunction(hidden, "setup", {});
    denying.vm.callFunction(hidden, "grant_wiz", {});
    denying.vm.callFunction(hidden, "hide", {});
    assert(hidden->isHidden());

    amlp::Value result = denying.vm.callFunction(probe, "probe", {});
    auto* arr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(arr != nullptr);
    bool sawNamed = false, sawNoCommands = false, sawUnnamed = false, sawHidden = false;
    for (auto& item : (*arr)->items) {
        auto* ob = std::get_if<std::shared_ptr<amlp::LpcObject>>(&item.data);
        assert(ob != nullptr);
        if (*ob == named) sawNamed = true;
        if (*ob == noCommands) sawNoCommands = true;
        if (*ob == unnamed) sawUnnamed = true;
        if (*ob == hidden) sawHidden = true;
    }
    assert(sawNamed);
    assert(!sawNoCommands);
    assert(!sawUnnamed);
    assert(!sawHidden);

    std::cout << "testNamedLivingsListsOnlyLivingNamedObjectsWithCommandsEnabledRespectingHidden OK\n";
}

// query_notify_fail(): a non-consuming peek at whatever notify_fail() last
// set, distinct from the one-shot takePendingNotifyFail() notify_no_command()
// itself uses (Server::dispatchLine()) -- checked here by peeking mid-dispatch
// and then confirming the eventual notify_no_command() message still arrives
// unaffected, proving the peek genuinely did not consume it.
static void testQueryNotifyFailPeeksPendingMessageWithoutConsumingIt() {
    ObjectVarHarness harness;
    harness.writeFile("/qnf_player.c",
        "mixed before;\n"
        "mixed after;\n"
        "void setup() {\n"
        "    enable_commands();\n"
        "    add_action(\"cmd_go\", \"go\");\n"
        "}\n"
        "int cmd_go(string arg) {\n"
        "    before = query_notify_fail();\n"
        "    notify_fail(\"You can't go that way.\\n\");\n"
        "    after = query_notify_fail();\n"
        "    return 0;\n"
        "}\n"
        "mixed get_before() { return before; }\n"
        "mixed get_after() { return after; }\n");
    auto player = harness.objects.cloneObject("/qnf_player");
    assert(player != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(player);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(player, "setup", {});
    amlp::Server::dispatchLine(harness.vm, conn, "go north");
    amlp::OutputContext::set(nullptr);

    amlp::Value before = harness.vm.callFunction(player, "get_before", {});
    assert(std::holds_alternative<int64_t>(before.data));
    assert(std::get<int64_t>(before.data) == 0);

    amlp::Value after = harness.vm.callFunction(player, "get_after", {});
    assert(std::holds_alternative<std::string>(after.data));
    assert(std::get<std::string>(after.data) == "You can't go that way.\n");

    // The peek must not have consumed the message -- notify_no_command()'s
    // own real take, fired by dispatchLine() itself once cmd_go returned 0,
    // still delivers it to the connection.
    char buf[256];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n > 0);
    std::string received(buf, static_cast<size_t>(n));
    assert(received == "You can't go that way.\n");

    ::close(fds[1]);
    std::cout << "testQueryNotifyFailPeeksPendingMessageWithoutConsumingIt OK\n";
}

// request_term_size(): a bare IAC DO NAWS with no interactive
// command_giver being a silent no-op, matching real f_request_term_size()'s
// implicit command_giver->interactive write target.
static void testRequestTermSizeSendsIacDoNawsAndIsNoOpWithoutInteractiveCommandGiver() {
    ObjectVarHarness harness;
    harness.writeFile("/rts_probe.c", "void probe() { request_term_size(); }\n");
    auto probe = harness.objects.cloneObject("/rts_probe");
    assert(probe != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(probe);

    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(probe, "probe", {});
    amlp::OutputContext::set(nullptr);

    unsigned char expected[] = {255, 253, 31}; // IAC DO NAWS
    char buf[16];
    ssize_t n = ::recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT);
    assert(n == static_cast<ssize_t>(sizeof(expected)));
    assert(std::memcmp(buf, expected, sizeof(expected)) == 0);
    ::close(fds[1]);

    // No interactive command_giver at all (OutputContext left unset) --
    // silent no-op, not a throw.
    auto plain = harness.objects.cloneObject("/rts_probe");
    assert(plain != nullptr);
    harness.vm.callFunction(plain, "probe", {});

    std::cout << "testRequestTermSizeSendsIacDoNawsAndIsNoOpWithoutInteractiveCommandGiver OK\n";
}

// pluralize(): a mechanical port of packages/contrib.c's real ~440-line
// C body, verified against a representative slice covering every stage
// of the real algorithm (default rule, PLURAL_SAME, several chop/suffix
// exception-table entries, exception-table-over-general-rule precedence,
// each general suffix-rule letter this sample can reach, the "a "/"an "
// determiner strip, and the "X of Y" clause) rather than every one of
// its ~50 exception-table rows.
static void testPluralizeMatchesRealExceptionTableGeneralRulesAndOfClauseAcrossVariousInputs() {
    ObjectVarHarness harness;
    harness.writeFile("/pl_probe.c",
        "mixed p(string s) { return pluralize(s); }\n");
    auto probe = harness.objects.cloneObject("/pl_probe");
    assert(probe != nullptr);

    auto p = [&](const std::string& in) -> amlp::Value {
        return harness.vm.callFunction(probe, "p", {amlp::Value(in)});
    };
    auto str = [](const amlp::Value& v) -> std::string {
        assert(std::holds_alternative<std::string>(v.data));
        return std::get<std::string>(v.data);
    };

    // Default rule: plain "+s", nothing else matches.
    assert(str(p("cat")) == "cats");
    // Exception table: PLURAL_CHOP + 4, suffix "eese".
    assert(str(p("goose")) == "geese");
    // Exception table: PLURAL_SAME -- unchanged.
    assert(str(p("sheep")) == "sheep");
    // Exception table wins over the general "*ff -> chop 2, +ves" rule
    // that would otherwise apply to a word ending "ff" -- real
    // case 'C': "liff" sets suffix "s" with no chop before the general
    // rules ever run.
    assert(str(p("cliff")) == "cliffs");
    // General rule: last letter 'x' -> "+es".
    assert(str(p("fox")) == "foxes");
    // General rule: 'y' preceded by a consonant -> chop 1, "+ies".
    assert(str(p("baby")) == "babies");
    // General rule: 'y' preceded by a vowel -> default "+s", no chop.
    assert(str(p("day")) == "days");
    // General rule: 'f' preceded by neither 'e' nor 'f' -> chop 1, "+ves".
    assert(str(p("loaf")) == "loaves");
    // General rule: 'f' preceded by 'e' -> real explicit no-op break,
    // falls through to the plain default "+s" ("*ef -> *efs").
    assert(str(p("chef")) == "chefs");
    // General rule: 'f' preceded by 'f' -> chop 2 (doubled), "+ves".
    assert(str(p("half")) == "halves");
    // General rule: "*man" -> chop 3, "+men".
    assert(str(p("foreman")) == "foremen");
    // "a "/"an " determiner strip.
    assert(str(p("a sword")) == "swords");
    assert(str(p("an apple")) == "apples");
    // "X of Y" -- only X is pluralized, " of Y" rides along verbatim.
    assert(str(p("loaf of bread")) == "loaves of bread");
    // Empty input: real int 0, not an empty string.
    amlp::Value empty = p("");
    assert(std::holds_alternative<int64_t>(empty.data) && std::get<int64_t>(empty.data) == 0);

    std::cout << "testPluralizeMatchesRealExceptionTableGeneralRulesAndOfClauseAcrossVariousInputs OK\n";
}

// unique_mapping(): groups an array's elements by a callback result into
// a mapping, both call shapes (closure, and string function name with an
// explicit object target -- this driver's own established simplification
// of real process_efun_callback()'s "defaults to current_object"
// convenience form, matching filter_array's own scoping), extra
// trailing arguments passed through to the callback, and first-
// appearance order for both the mapping's own keys and each group's
// elements (this implementation's own documented, deliberate choice for
// the real hash-bucket order ambiguity, see its own registration
// comment).
static void testUniqueMappingGroupsByCallbackResultInFirstAppearanceOrderForClosureAndStringForms() {
    ObjectVarHarness harness;
    harness.writeFile("/um_probe.c",
        "int classify(int n, int mod) { return n % mod; }\n"
        "mixed probe_closure(mixed *arr) {\n"
        "    return unique_mapping(arr, (: $1 % 2 :));\n"
        "}\n"
        "mixed probe_string(mixed *arr) {\n"
        "    return unique_mapping(arr, \"classify\", this_object(), 3);\n"
        "}\n"
        "mixed probe_empty() { return unique_mapping(({}), (: $1 :)); }\n");
    auto probe = harness.objects.cloneObject("/um_probe");
    assert(probe != nullptr);

    auto makeIntArray = [](std::vector<int64_t> nums) {
        auto arr = std::make_shared<amlp::Array>();
        for (auto n : nums) arr->items.emplace_back(n);
        return arr;
    };

    // Closure form: group by parity. First appearance order: 3 (odd)
    // seen before 4 (even).
    amlp::Value closureResult = harness.vm.callFunction(
        probe, "probe_closure", {amlp::Value(makeIntArray({3, 4, 5, 6, 7}))});
    auto* closureMap = std::get_if<std::shared_ptr<amlp::Mapping>>(&closureResult.data);
    assert(closureMap != nullptr && *closureMap != nullptr);
    assert((*closureMap)->entries.size() == 2);
    assert(std::get<int64_t>((*closureMap)->entries[0].first.data) == 1); // odd first
    {
        auto* oddGroup = std::get_if<std::shared_ptr<amlp::Array>>(&(*closureMap)->entries[0].second.data);
        assert(oddGroup != nullptr && (*oddGroup)->items.size() == 3);
        assert(std::get<int64_t>((*oddGroup)->items[0].data) == 3);
        assert(std::get<int64_t>((*oddGroup)->items[1].data) == 5);
        assert(std::get<int64_t>((*oddGroup)->items[2].data) == 7);
    }
    assert(std::get<int64_t>((*closureMap)->entries[1].first.data) == 0); // even second
    {
        auto* evenGroup = std::get_if<std::shared_ptr<amlp::Array>>(&(*closureMap)->entries[1].second.data);
        assert(evenGroup != nullptr && (*evenGroup)->items.size() == 2);
        assert(std::get<int64_t>((*evenGroup)->items[0].data) == 4);
        assert(std::get<int64_t>((*evenGroup)->items[1].data) == 6);
    }

    // String function name + explicit target object form, extra arg
    // (mod = 3) passed through after the element.
    amlp::Value stringResult = harness.vm.callFunction(
        probe, "probe_string", {amlp::Value(makeIntArray({1, 2, 3, 4, 5}))});
    auto* stringMap = std::get_if<std::shared_ptr<amlp::Mapping>>(&stringResult.data);
    assert(stringMap != nullptr && *stringMap != nullptr);
    assert((*stringMap)->entries.size() == 3); // 1%3, 2%3, 0%3
    assert(std::get<int64_t>((*stringMap)->entries[0].first.data) == 1);
    assert(std::get<int64_t>((*stringMap)->entries[1].first.data) == 2);
    assert(std::get<int64_t>((*stringMap)->entries[2].first.data) == 0);

    // Empty array -> empty mapping.
    amlp::Value emptyResult = harness.vm.callFunction(probe, "probe_empty", {});
    auto* emptyMap = std::get_if<std::shared_ptr<amlp::Mapping>>(&emptyResult.data);
    assert(emptyMap != nullptr && *emptyMap != nullptr && (*emptyMap)->entries.empty());

    std::cout << "testUniqueMappingGroupsByCallbackResultInFirstAppearanceOrderForClosureAndStringForms OK\n";
}

// reclaim_objects(): eagerly sweeps every live object's own variables,
// recursing into arrays and mappings, rewriting a stale (destructed but
// still referenced) object reference to int 0 and returning how many
// were found. The mapping-key case is checked specifically: unlike an
// ordinary object-variable or array-element reference (which this
// driver's own lazy coerceIfDestructed() already self-heals the moment
// anything reads it via LPC, making a before/after comparison through
// LPC alone circular), a destructed mapping *key* is never coerced
// anywhere else in this driver (VM.cpp's own Index-mapping branch only
// ever coerces the value half of an entry, confirmed directly) --
// giving reclaim_objects() a real, LPC-observable, non-circular effect:
// the whole entry is erased (real map_delete() semantics), shrinking
// sizeof(m) by one, not left as a still-present 0-keyed entry.
static void testReclaimObjectsCoercesStaleReferencesAndErasesDestructedMappingKeysReturningCount() {
    ObjectVarHarness harness;
    harness.writeFile("/rc_target.c", "void create() {}\n");
    harness.writeFile("/rc_holder.c",
        "object stale_ob;\n"
        "mixed *stale_arr;\n"
        "mapping stale_key_map;\n"
        "void setup(object ob) {\n"
        "    stale_ob = ob;\n"
        "    stale_arr = ({ ob, 42 });\n"
        "    stale_key_map = ([ ob: \"value\" ]);\n"
        "}\n"
        "int map_size() { return sizeof(stale_key_map); }\n");
    harness.writeFile("/rc_probe.c", "int probe() { return reclaim_objects(); }\n");

    auto target = harness.objects.cloneObject("/rc_target");
    auto holder = harness.objects.cloneObject("/rc_holder");
    auto probe = harness.objects.cloneObject("/rc_probe");
    assert(target != nullptr && holder != nullptr && probe != nullptr);

    harness.vm.callFunction(holder, "setup", {amlp::Value(target)});
    amlp::Value sizeBefore = harness.vm.callFunction(holder, "map_size", {});
    assert(std::get<int64_t>(sizeBefore.data) == 1);

    harness.vm.destructObject(target); // target stays alive via this local

    amlp::Value cleaned = harness.vm.callFunction(probe, "probe", {});
    assert(std::holds_alternative<int64_t>(cleaned.data));
    // stale_ob (1) + stale_arr's element 0 (1) + the destructed-keyed
    // mapping entry (1, real map_delete(), see the function comment).
    assert(std::get<int64_t>(cleaned.data) == 3);

    // The mapping entry was erased outright, not rewritten to a 0 key --
    // sizeof(m) genuinely shrank, an effect no lazy read anywhere else
    // in this driver ever produces for a mapping key.
    amlp::Value sizeAfter = harness.vm.callFunction(holder, "map_size", {});
    assert(std::get<int64_t>(sizeAfter.data) == 0);

    std::cout << "testReclaimObjectsCoercesStaleReferencesAndErasesDestructedMappingKeysReturningCount OK\n";
}

// replace_program(): swaps an object's own program to one of its
// inherited ancestors' -- deferred until VM::processPendingReplacePrograms()
// runs (matching real replace_programs()'s own once-per-driver-tick
// timing, not applied inline), then drops the object's own
// (non-ancestor) functions while keeping the ancestor's own variables'
// *values* intact across the swap (the real var_offset shuffle).
static void testReplaceProgramDeferredSwapPreservesInheritedVariablesAndDropsOwnFunctions() {
    ObjectVarHarness harness;
    harness.writeFile("/rp_parent.c",
        "int shared_var;\n"
        "void create() {}\n"
        "string desc() { return \"a room\"; }\n"
        "int get_shared() { return shared_var; }\n");
    harness.writeFile("/rp_child.c",
        "inherit \"/rp_parent\";\n"
        "int child_var;\n"
        "void create() {}\n"
        "string special() { return \"special\"; }\n"
        "void set_vars(int s, int c) { shared_var = s; child_var = c; }\n"
        "void do_replace() { replace_program(\"/rp_parent\"); }\n");

    auto ob = harness.objects.cloneObject("/rp_child");
    assert(ob != nullptr);

    // Before any replace_program() call: everything resolves normally,
    // both the child's own function and the inherited one.
    assert(std::get<std::string>(harness.vm.callFunction(ob, "desc", {}).data) == "a room");
    assert(std::get<std::string>(harness.vm.callFunction(ob, "special", {}).data) == "special");
    harness.vm.callFunction(ob, "set_vars", {amlp::Value(int64_t{42}), amlp::Value(int64_t{99})});
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_shared", {}).data) == 42);
    assert(ob->variables().size() == 2); // shared_var (inherited) + child_var (own)

    // Stages the swap -- nothing about the object changes yet.
    harness.vm.callFunction(ob, "do_replace", {});
    assert(std::get<std::string>(harness.vm.callFunction(ob, "special", {}).data) == "special");
    assert(ob->variables().size() == 2);

    // Simulates one driver tick passing.
    harness.vm.processPendingReplacePrograms();

    // Now applied: the child's own function is gone (the object's whole
    // program is literally /rp_parent's now), the inherited one still
    // resolves (it always was part of /rp_parent), shared_var's own
    // *value* survived the swap (42, not reset or garbled), and the
    // object's own variable count shrank to /rp_parent's own (1).
    // VM::callFunction()'s own documented convention for an undefined
    // function is a silent void return, not a throw (that is
    // OpCode::Call's own behavior for a bare in-LPC call, a different
    // entry point) -- checked accordingly, not against an exception.
    amlp::Value afterSwap = harness.vm.callFunction(ob, "special", {});
    assert(afterSwap.isVoid());
    assert(std::get<std::string>(harness.vm.callFunction(ob, "desc", {}).data) == "a room");
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_shared", {}).data) == 42);
    assert(ob->variables().size() == 1);

    std::cout << "testReplaceProgramDeferredSwapPreservesInheritedVariablesAndDropsOwnFunctions OK\n";
}

// query_replaced_program(): 0 (not an empty string) before any
// replace_program() call, still 0 while a swap is only *staged* (real
// replaced_program is written by replace_programs() itself, never by
// f_replace_program() staging the request), and the real leading-slash-
// prefixed name only once the deferred swap has actually applied.
static void testQueryReplacedProgramReflectsOnlyAnActuallyAppliedSwap() {
    ObjectVarHarness harness;
    harness.writeFile("/qrp_parent.c", "void create() {}\n");
    harness.writeFile("/qrp_child.c",
        "inherit \"/qrp_parent\";\n"
        "void create() {}\n"
        "void do_replace() { replace_program(\"/qrp_parent\"); }\n");
    harness.writeFile("/qrp_probe.c",
        "mixed probe(object ob) { return query_replaced_program(ob); }\n");

    auto ob = harness.objects.cloneObject("/qrp_child");
    auto probe = harness.objects.cloneObject("/qrp_probe");
    assert(ob != nullptr && probe != nullptr);

    amlp::Value before = harness.vm.callFunction(probe, "probe", {amlp::Value(ob)});
    assert(std::holds_alternative<int64_t>(before.data) && std::get<int64_t>(before.data) == 0);

    harness.vm.callFunction(ob, "do_replace", {});
    amlp::Value staged = harness.vm.callFunction(probe, "probe", {amlp::Value(ob)});
    assert(std::holds_alternative<int64_t>(staged.data) && std::get<int64_t>(staged.data) == 0);

    harness.vm.processPendingReplacePrograms();
    amlp::Value applied = harness.vm.callFunction(probe, "probe", {amlp::Value(ob)});
    assert(std::holds_alternative<std::string>(applied.data));
    assert(std::get<std::string>(applied.data) == "/qrp_parent");

    std::cout << "testQueryReplacedProgramReflectsOnlyAnActuallyAppliedSwap OK\n";
}

static void testReplaceProgramThrowsWhenTargetNotInheritedOrCalledOnSimulEfunObject() {
    ObjectVarHarness harness;
    harness.writeFile("/rp2_unrelated.c", "void create() {}\n");
    harness.writeFile("/rp2_child.c",
        "void create() {}\n"
        "void try_replace(string name) { replace_program(name); }\n");
    auto ob = harness.objects.cloneObject("/rp2_child");
    assert(ob != nullptr);

    // Real "program to replace the current with has to be inherited" --
    // /rp2_unrelated is a real, loadable file, just not in ob's own
    // inherit chain.
    bool threwNotInherited = false;
    try {
        harness.vm.callFunction(ob, "try_replace", {amlp::Value(std::string("/rp2_unrelated"))});
    } catch (const amlp::LpcRuntimeError&) {
        threwNotInherited = true;
    }
    assert(threwNotInherited);

    // Real "replace_program on simul_efun object".
    harness.writeFile("/simul_efun.c",
        "void create() {}\n"
        "void try_replace_self() { replace_program(\"/simul_efun\"); }\n");
    assert(harness.objects.loadSimulEfunObject());
    bool threwSimulEfun = false;
    try {
        harness.vm.callFunction(harness.objects.simulEfunObject(), "try_replace_self", {});
    } catch (const amlp::LpcRuntimeError&) {
        threwSimulEfun = true;
    }
    assert(threwSimulEfun);

    std::cout << "testReplaceProgramThrowsWhenTargetNotInheritedOrCalledOnSimulEfunObject OK\n";
}

// function_owner(): the closure's own owner, or int 0 once that owner
// is genuinely gone (not merely destructed-but-still-referenced --
// Closure::owner is already a weak_ptr, real code applies no separate
// O_DESTRUCTED filter of its own either).
static void testFunctionOwnerReturnsClosureOwnerOrZeroOnceOwnerIsGone() {
    ObjectVarHarness harness;
    harness.writeFile("/fo_owner.c",
        "int greet() { return 111; }\n"
        "mixed make() { return (: greet :); }\n");
    auto owner = harness.objects.cloneObject("/fo_owner");
    assert(owner != nullptr);

    amlp::Value closureVal = harness.vm.callFunction(owner, "make", {});
    assert(std::holds_alternative<std::shared_ptr<amlp::Closure>>(closureVal.data));

    {
        std::vector<amlp::Value> ownerArgs{closureVal};
        amlp::Value ownerResult = amlp::EfunTable::instance().call("function_owner", harness.vm, ownerArgs);
        auto* ownerOb = std::get_if<std::shared_ptr<amlp::LpcObject>>(&ownerResult.data);
        assert(ownerOb != nullptr && *ownerOb == owner);
        // ownerResult itself holds a strong shared_ptr to the same
        // object (the efun's own real return value) -- must not outlive
        // this scope, or it would keep the object alive on its own and
        // the "owner is genuinely gone" check below would pass for the
        // wrong reason (isDestructed()-style staleness, not an actually
        // expired weak_ptr).
    }

    // Wrong argument type throws.
    bool threw = false;
    std::vector<amlp::Value> badArgs{amlp::Value(int64_t{5})};
    try {
        amlp::EfunTable::instance().call("function_owner", harness.vm, badArgs);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    // Destructed but still referenced (a live local shared_ptr kept
    // below, the same pattern the destructed-object test suite already
    // establishes): the weak_ptr still locks successfully, but real
    // put_unrefed_object()'s own explicit O_DESTRUCTED check must still
    // read this back as 0, not the object.
    harness.vm.destructObject(owner); // owner stays alive via this local
    std::vector<amlp::Value> destructedArgs{closureVal};
    amlp::Value destructedResult =
        amlp::EfunTable::instance().call("function_owner", harness.vm, destructedArgs);
    assert(std::holds_alternative<int64_t>(destructedResult.data) &&
           std::get<int64_t>(destructedResult.data) == 0);

    // Once the owner's last real reference is gone entirely, the
    // weak_ptr itself expires too -- a second, independent way to reach
    // the same real int-0 answer.
    owner.reset();
    std::vector<amlp::Value> goneArgs{closureVal};
    amlp::Value goneResult = amlp::EfunTable::instance().call("function_owner", harness.vm, goneArgs);
    assert(std::holds_alternative<int64_t>(goneResult.data) && std::get<int64_t>(goneResult.data) == 0);

    std::cout << "testFunctionOwnerReturnsClosureOwnerOrZeroOnceOwnerIsGone OK\n";
}

static void testNumClassesAlwaysReturnsZeroSinceClassDeclarationsDoNotExist() {
    ObjectVarHarness harness;
    harness.writeFile("/nc_probe.c", "void create() {}\n");
    auto ob = harness.objects.cloneObject("/nc_probe");
    assert(ob != nullptr);

    std::vector<amlp::Value> args{amlp::Value(ob)};
    amlp::Value result = amlp::EfunTable::instance().call("num_classes", harness.vm, args);
    assert(std::holds_alternative<int64_t>(result.data) && std::get<int64_t>(result.data) == 0);

    bool threw = false;
    std::vector<amlp::Value> badArgs{amlp::Value(int64_t{1})};
    try {
        amlp::EfunTable::instance().call("num_classes", harness.vm, badArgs);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testNumClassesAlwaysReturnsZeroSinceClassDeclarationsDoNotExist OK\n";
}

static void testSetAuthorAcceptsStringReturnsVoidAndThrowsOnWrongType() {
    ObjectVarHarness harness;

    std::vector<amlp::Value> args{amlp::Value(std::string("someone"))};
    amlp::Value result = amlp::EfunTable::instance().call("set_author", harness.vm, args);
    assert(result.isVoid());

    bool threw = false;
    std::vector<amlp::Value> badArgs{amlp::Value(int64_t{1})};
    try {
        amlp::EfunTable::instance().call("set_author", harness.vm, badArgs);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    std::cout << "testSetAuthorAcceptsStringReturnsVoidAndThrowsOnWrongType OK\n";
}

// replaceable(): true only when every one of ob's own *locally defined*
// functions is create(), the compiler's own synthesized "$objvarinit"
// (only present when a top-level variable actually has an initializer
// expression), or in the caller's explicit ignore list -- false the
// moment any other real function exists, and always false for the
// simul_efun object regardless.
static void testReplaceableTrueOnlyWhenEveryLocalFunctionIsIgnorable() {
    ObjectVarHarness harness;
    harness.writeFile("/repl_bare.c", "void create() {}\n");
    harness.writeFile("/repl_with_init.c", "int x = 5;\nvoid create() {}\n");
    harness.writeFile("/repl_extra.c", "void create() {}\nint extra() { return 1; }\n");

    auto bare = harness.objects.cloneObject("/repl_bare");
    auto withInit = harness.objects.cloneObject("/repl_with_init");
    auto extra = harness.objects.cloneObject("/repl_extra");
    assert(bare && withInit && extra);

    auto callReplaceable = [&](std::shared_ptr<amlp::LpcObject> ob,
                                std::shared_ptr<amlp::Array> ignore = nullptr) {
        std::vector<amlp::Value> args{amlp::Value(ob)};
        if (ignore) args.emplace_back(ignore);
        return amlp::EfunTable::instance().call("replaceable", harness.vm, args);
    };

    // Only create(): trivially replaceable.
    assert(std::get<int64_t>(callReplaceable(bare).data) == 1);

    // create() plus a synthesized "$objvarinit" from the initializer --
    // still replaceable, the synthesized name is auto-ignored.
    assert(std::get<int64_t>(callReplaceable(withInit).data) == 1);

    // create() plus a genuine extra function -- not replaceable.
    assert(std::get<int64_t>(callReplaceable(extra).data) == 0);

    // ...unless that function is explicitly ignored.
    auto ignoreExtra = std::make_shared<amlp::Array>();
    ignoreExtra->items.emplace_back(std::string("extra"));
    assert(std::get<int64_t>(callReplaceable(extra, ignoreExtra).data) == 1);

    // Wrong argument type throws.
    bool threw = false;
    std::vector<amlp::Value> badArgs{amlp::Value(int64_t{1})};
    try {
        amlp::EfunTable::instance().call("replaceable", harness.vm, badArgs);
    } catch (const amlp::LpcRuntimeError&) {
        threw = true;
    }
    assert(threw);

    // Always false for the simul_efun object itself, even with no
    // functions of its own beyond create().
    harness.writeFile("/simul_efun.c", "void create() {}\n");
    assert(harness.objects.loadSimulEfunObject());
    assert(std::get<int64_t>(callReplaceable(harness.objects.simulEfunObject()).data) == 0);

    std::cout << "testReplaceableTrueOnlyWhenEveryLocalFunctionIsIgnorable OK\n";
}

// query_num(): a mechanical port of packages/contrib.c's real
// query_num()/number_as_string() (~90 lines), verified against a
// representative slice covering every stage of the real assembly order
// (units, teens, dashed and round tens, hundreds with and without a
// trailing units group, thousands with and without a trailing hundreds
// group, the comma-vs-no-comma distinction between a hundreds group
// that followed thousands and one that didn't, the "many" ceiling at
// 99999 and at an explicit lower limit, and a negative n) rather than
// exhaustively random-number testing.
static void testQueryNumMatchesRealAssemblyOrderAcrossRepresentativeInputsAndLimits() {
    ObjectVarHarness harness;
    harness.writeFile("/qn_probe.c",
        "string p(int n) { return query_num(n); }\n"
        "string p_limit(int n, int limit) { return query_num(n, limit); }\n");
    auto probe = harness.objects.cloneObject("/qn_probe");
    assert(probe != nullptr);

    auto p = [&](int64_t n) -> std::string {
        amlp::Value r = harness.vm.callFunction(probe, "p", {amlp::Value(n)});
        assert(std::holds_alternative<std::string>(r.data));
        return std::get<std::string>(r.data);
    };
    auto pLimit = [&](int64_t n, int64_t limit) -> std::string {
        amlp::Value r = harness.vm.callFunction(probe, "p_limit",
            {amlp::Value(n), amlp::Value(limit)});
        assert(std::holds_alternative<std::string>(r.data));
        return std::get<std::string>(r.data);
    };

    assert(p(0) == "zero");
    assert(p(5) == "five");
    assert(p(15) == "fifteen");
    assert(p(20) == "twenty");           // round ten: no trailing dash
    assert(p(21) == "twenty-one");       // dashed ten
    assert(p(100) == "one hundred");     // round hundred: no trailing units
    assert(p(105) == "one hundred and five");
    assert(p(1000) == "one thousand");   // round thousand: no trailing hundreds
    // Thousands with a round-hundred remainder: real assembly still
    // inserts "and" (from the thousands-then-hundreds "changed" branch),
    // not a comma, since the hundreds group itself has no further units.
    assert(p(1500) == "one thousand and five hundred");
    // Thousands with a non-round-hundred remainder: a comma before the
    // hundreds group, then "and" before the final units group.
    assert(p(1250) == "one thousand, two hundred and fifty");
    // Upper boundary, inclusive -- exercises every assembly stage at
    // once (thousands, comma, hundreds, "and", dashed units).
    assert(p(99999) == "ninety-nine thousand, nine hundred and ninety-nine");

    // Past the real 99999 ceiling, or negative: "many" regardless of an
    // explicit limit.
    assert(p(100000) == "many");
    assert(p(-5) == "many");

    // Explicit limit: "many" once n exceeds it, a real conversion
    // otherwise -- and the default (1-arg form, limit 0) never applies
    // a ceiling below 99999 at all, already exercised by p(99999) above.
    assert(pLimit(50, 10) == "many");
    assert(pLimit(5, 10) == "five");

    std::cout << "testQueryNumMatchesRealAssemblyOrderAcrossRepresentativeInputsAndLimits OK\n";
}

// origin(): real efuns_main.c's own f_origin(), reading a single
// per-call-path-tagged scalar (VM::currentOrigin()). Each test below
// targets exactly one of the 8 real ORIGIN_* values through the one
// real call path this driver verified it against reference source
// (see EfunTable.cpp's own origin() registration comment and each real
// call site's own citation in VM.cpp/EfunTable.cpp/Server.cpp/
// Scheduler.cpp), not a couple of representative cases -- this is the
// one efun in the whole row explicitly flagged as easy to get subtly
// wrong, so every value gets its own dedicated, separately-named test.

static void testOriginReturnsLocalForABareSameObjectCall() {
    ObjectVarHarness harness;
    harness.writeFile("/origin_local.c",
        "string outer() { return probe(); }\n"
        "string probe() { return origin(); }\n");
    auto ob = harness.objects.cloneObject("/origin_local");
    assert(ob != nullptr);

    // outer() itself is reached with Origin::Driver (this call's own
    // default) -- irrelevant here, not checked. What matters is
    // probe()'s own origin, reached via a bare call *from LPC code*
    // (outer() calling probe()), real F_CALL_FUNCTION_BY_ADDRESS's own
    // "caller_type = ORIGIN_LOCAL".
    amlp::Value result = harness.vm.callFunction(ob, "outer", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "local");

    std::cout << "testOriginReturnsLocalForABareSameObjectCall OK\n";
}

static void testOriginReturnsCallOtherForACallOtherDispatch() {
    ObjectVarHarness harness;
    harness.writeFile("/origin_co_target.c", "string probe() { return origin(); }\n");
    harness.writeFile("/origin_co_caller.c",
        "string call_it(object b) { return b->probe(); }\n");
    auto target = harness.objects.cloneObject("/origin_co_target");
    auto caller = harness.objects.cloneObject("/origin_co_caller");
    assert(target != nullptr && caller != nullptr);

    // Real f__call_other()'s own "call_origin = ORIGIN_CALL_OTHER;"
    // (efuns_main.c) -- target's probe() must see "call_other", not
    // "local" (it is not target's own bare call) or "efun".
    amlp::Value result = harness.vm.callFunction(caller, "call_it", {amlp::Value(target)});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "call_other");

    std::cout << "testOriginReturnsCallOtherForACallOtherDispatch OK\n";
}

static void testOriginReturnsSimulForABareCallResolvingToSimulEfun() {
    ObjectVarHarness harness;
    harness.writeFile("/simul_efun.c", "string se_probe() { return origin(); }\n");
    assert(harness.objects.loadSimulEfunObject());
    harness.writeFile("/origin_simul_caller.c",
        // se_probe is defined only on the simul_efun object, never
        // locally or via inherit -- this bare call must fall through
        // all the way to tier 3.
        "string call_it() { return se_probe(); }\n");
    auto caller = harness.objects.cloneObject("/origin_simul_caller");
    assert(caller != nullptr);

    // Real call_simul_efun()'s own "call_direct(simul_efun_ob, ...,
    // ORIGIN_SIMUL_EFUN, ...)" (eoperators.c).
    amlp::Value result = harness.vm.callFunction(caller, "call_it", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "simul");

    std::cout << "testOriginReturnsSimulForABareCallResolvingToSimulEfun OK\n";
}

static void testOriginReturnsInternalForCallOutStringFormFiring() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/origin_internal.c",
        "string result;\n"
        "void go() { call_out(\"fire\", 0); }\n"
        "void fire() { result = origin(); }\n"
        "string query_result() { return result; }\n");
    auto ob = harness.objects.cloneObject("/origin_internal");
    assert(ob != nullptr);

    harness.vm.callFunction(ob, "go", {});
    scheduler.tickCallOuts();

    // Real call_out.c's own firing loop: "apply(cop->function.s, cop->ob,
    // extra, ORIGIN_INTERNAL)" -- not "driver" despite being a
    // Scheduler/driver-triggered fire (see Scheduler.cpp's own citation
    // for why heart_beat firing, by contrast, really is "driver").
    amlp::Value result = harness.vm.callFunction(ob, "query_result", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "internal");

    std::cout << "testOriginReturnsInternalForCallOutStringFormFiring OK\n";
}

static void testOriginReturnsEfunForAMapArrayCallback() {
    ObjectVarHarness harness;
    harness.writeFile("/origin_efun.c",
        "string tag(mixed x) { return origin(); }\n"
        "mixed *probe(mixed *arr) { return map_array(arr, \"tag\", this_object()); }\n");
    auto ob = harness.objects.cloneObject("/origin_efun");
    assert(ob != nullptr);

    auto arg = std::make_shared<amlp::Array>();
    arg->items.emplace_back(int64_t{1});
    arg->items.emplace_back(int64_t{2});

    // Real array.c's own map_array()/f_map(): "apply(func, ob,
    // 1+numex, ORIGIN_EFUN)" for its own string-target-object callback
    // shape -- a mudlib-supplied callback argument invoked from inside
    // an efun's own C body, the real, narrow meaning of ORIGIN_EFUN.
    amlp::Value result = harness.vm.callFunction(ob, "probe", {amlp::Value(arg)});
    auto* resultArr = std::get_if<std::shared_ptr<amlp::Array>>(&result.data);
    assert(resultArr != nullptr && (*resultArr)->items.size() == 2);
    for (auto& item : (*resultArr)->items) {
        assert(std::holds_alternative<std::string>(item.data));
        assert(std::get<std::string>(item.data) == "efun");
    }

    std::cout << "testOriginReturnsEfunForAMapArrayCallback OK\n";
}

static void testOriginReturnsFunctionalForAnInlineLambdaBody() {
    ObjectVarHarness harness;
    harness.writeFile("/origin_functional.c",
        "string probe() {\n"
        "    mixed f = (: origin() :);\n"
        "    return evaluate(f);\n"
        "}\n");
    auto ob = harness.objects.cloneObject("/origin_functional");
    assert(ob != nullptr);

    // Real call_function_pointer()'s own "case FP_FUNCTIONAL: ...
    // caller_type = ORIGIN_FUNCTIONAL;" (function.c) -- an anonymous
    // "(: ... :)" body, not the "local" a named function pointer's own
    // target would see (see testOriginReturnsLocalForABareSameObjectCall's
    // own sibling coverage of that distinction via
    // isSynthesizedLambdaName()).
    amlp::Value result = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::string>(result.data));
    assert(std::get<std::string>(result.data) == "functional");

    std::cout << "testOriginReturnsFunctionalForAnInlineLambdaBody OK\n";
}

static void testOriginReturnsDriverForTopLevelDispatchHeartBeatAndCommandDispatch() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/origin_driver.c",
        "string hb_result;\n"
        "string cmd_result;\n"
        "void setup() { enable_commands(); add_action(\"cmd_go\", \"go\"); }\n"
        "void enable_hb() { set_heart_beat(1); }\n"
        "string probe() { return origin(); }\n"
        "void heart_beat() { hb_result = origin(); }\n"
        "string query_hb() { return hb_result; }\n"
        "int cmd_go(string arg) { cmd_result = origin(); return 1; }\n"
        "string query_cmd() { return cmd_result; }\n");
    auto ob = harness.objects.cloneObject("/origin_driver");
    assert(ob != nullptr);

    // add_action() only registers correctly from inside a command-giver-
    // resolvable context -- resolveCommandGiver() (EfunTable.cpp) falls
    // back to OutputContext::current()'s own bound object when
    // VM::commandGiver()'s explicit stack is empty, so a bare setup()
    // call with no active Connection/OutputContext silently registers
    // nothing (real add_action()'s own documented no-op-outside-context
    // behavior). Matches the exact working pattern the
    // query_notify_fail test suite already establishes: a real
    // Connection, OutputContext::set() around setup(), and dispatch
    // through Server::dispatchLine() rather than VM::dispatchCommand()
    // directly.
    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(ob);
    amlp::OutputContext::set(&conn);
    harness.vm.callFunction(ob, "setup", {});

    // (a) VM::callFunction()'s own default -- real logon()/create()/
    // process_input()/net_dead()/window_size()/every master apply all
    // confirmed real ORIGIN_DRIVER, this is the shared entry point all
    // of them go through.
    amlp::Value direct = harness.vm.callFunction(ob, "probe", {});
    assert(std::holds_alternative<std::string>(direct.data));
    assert(std::get<std::string>(direct.data) == "driver");

    // (b) heart_beat firing -- real backend.c's own "call_direct(ob,
    // ..., ORIGIN_DRIVER, 0)". set_heart_beat() is a core efun, called
    // from within LPC (enable_hb()), the same "bare call from the
    // object's own code" shape the existing set_heart_beat test suite
    // already establishes -- not something to invoke directly through
    // VM::callFunction() by name, which would just silently no-op
    // against a nonexistent "set_heart_beat" *local* function.
    harness.vm.callFunction(ob, "enable_hb", {});
    scheduler.tickHeartbeats();
    amlp::Value hb = harness.vm.callFunction(ob, "query_hb", {});
    assert(std::holds_alternative<std::string>(hb.data));
    assert(std::get<std::string>(hb.data) == "driver");

    // (c) top-level command dispatch, the real entry point (real
    // process_input()/parse_command() from a freshly typed line, this
    // driver's own Server::dispatchLine() -> VM::dispatchCommand(),
    // reached with no LPC frame already active, current_object null) --
    // real add_action.c's own "where = (current_object ? ORIGIN_EFUN :
    // ORIGIN_DRIVER);" resolves to the driver half here.
    amlp::Server::dispatchLine(harness.vm, conn, "go");
    amlp::OutputContext::set(nullptr);
    amlp::Value cmd = harness.vm.callFunction(ob, "query_cmd", {});
    assert(std::holds_alternative<std::string>(cmd.data));
    assert(std::get<std::string>(cmd.data) == "driver");
    ::close(fds[1]);

    std::cout << "testOriginReturnsDriverForTopLevelDispatchHeartBeatAndCommandDispatch OK\n";
}

static void testOriginNameCoversAllEightRealValuesIncludingTheUnreachableFunctionPointer() {
    // Direct C++-level check of the string table itself (real
    // origin_name()'s own "driver"/"local"/"call_other"/"simul"/
    // "internal"/"efun"/"function pointer"/"functional"), including
    // Origin::FunctionPointer -- verified unreachable from any genuine
    // LPC call path in this driver (see Origin's own VM.hpp comment for
    // the full real-semantics citation for why), so this is the only
    // way to confirm origin_name() would still report it correctly if
    // something unexpected ever did reach it.
    assert(std::string(amlp::originName(amlp::Origin::Driver)) == "driver");
    assert(std::string(amlp::originName(amlp::Origin::Local)) == "local");
    assert(std::string(amlp::originName(amlp::Origin::CallOther)) == "call_other");
    assert(std::string(amlp::originName(amlp::Origin::SimulEfun)) == "simul");
    assert(std::string(amlp::originName(amlp::Origin::Internal)) == "internal");
    assert(std::string(amlp::originName(amlp::Origin::Efun)) == "efun");
    assert(std::string(amlp::originName(amlp::Origin::FunctionPointer)) == "function pointer");
    assert(std::string(amlp::originName(amlp::Origin::Functional)) == "functional");

    std::cout << "testOriginNameCoversAllEightRealValuesIncludingTheUnreachableFunctionPointer OK\n";
}

// reload_object(): real object.c's own reload_object(), resetting an
// object back to a freshly-cloned-looking state in place. Verified
// against reference source in full before writing anything (see
// EfunTable.cpp's own registration comment for the complete citation);
// zero real call sites across all six of this row's mudlib corpora,
// re-confirmed fresh rather than assumed unchanged. Each test below
// targets one real, distinct piece of its behavior rather than one
// broad smoke test.

static void testReloadObjectResetsVariablesReinitializesAndCallsCreateAgain() {
    ObjectVarHarness harness;
    harness.writeFile("/reload_core.c",
        "int plain;\n"
        "int initialized = 5;\n"
        "int create_count;\n"
        "void create() { create_count = create_count + 1; }\n"
        "void set_plain(int v) { plain = v; }\n"
        "void set_initialized(int v) { initialized = v; }\n"
        "int get_plain() { return plain; }\n"
        "int get_initialized() { return initialized; }\n"
        "int get_create_count() { return create_count; }\n"
        "void set_create_count(int v) { create_count = v; }\n");
    auto ob = harness.objects.cloneObject("/reload_core");
    assert(ob != nullptr);

    // Sanity: the fresh clone's own create() already ran once, and the
    // real initializer already applied once.
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_create_count", {}).data) == 1);
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_initialized", {}).data) == 5);

    harness.vm.callFunction(ob, "set_plain", {amlp::Value(int64_t{42})});
    harness.vm.callFunction(ob, "set_initialized", {amlp::Value(int64_t{99})});
    // create_count is itself an ordinary object variable, so it is
    // subject to the exact same zero-then-reinit step everything else
    // is -- it cannot be used to count *across* a reload by simply
    // expecting it to keep incrementing (a real reload_object() zeroes
    // it right along with everything else, the same fresh-slate
    // guarantee that makes this whole efun meaningful in the first
    // place). Corrupted to an arbitrary value here instead, specifically
    // so "did create() genuinely run again" can still be told apart from
    // "create() was skipped": skipped would leave it at 0 (the zero
    // step's own result), genuinely re-run leaves it at 1.
    harness.vm.callFunction(ob, "set_create_count", {amlp::Value(int64_t{777})});
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_plain", {}).data) == 42);
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_initialized", {}).data) == 99);

    std::vector<amlp::Value> reloadArgs{amlp::Value(ob)};
    amlp::EfunTable::instance().call("reload_object", harness.vm, reloadArgs);

    // plain has no initializer -- zeroed, and create() does not set it,
    // so it stays 0. initialized has a real initializer -- zeroed, then
    // the real re-run "$objvarinit" puts it straight back to 5, not 0
    // and not the 99 it was overwritten to (real call_create()'s own
    // "call___INIT(ob); ...; apply(APPLY_CREATE, ...)" order, confirmed
    // directly, not just create() alone). create_count reads back as 1,
    // not the corrupted 777 and not the bare-zeroed 0 -- create()
    // genuinely ran again, from the freshly-zeroed baseline, not skipped.
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_plain", {}).data) == 0);
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_initialized", {}).data) == 5);
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_create_count", {}).data) == 1);

    std::cout << "testReloadObjectResetsVariablesReinitializesAndCallsCreateAgain OK\n";
}

static void testReloadObjectClosesOwnedSocketsWithNoCallbackFiring() {
    ObjectVarHarness harness;
    harness.writeFile("/reload_socket.c",
        "int fd;\n"
        "int close_fired;\n"
        "int make() { fd = socket_create(2, \"on_read\", \"on_close\"); return fd; }\n"
        "void on_read(int f, string m, string a) {}\n"
        "void on_close(int f) { close_fired = 1; }\n"
        "int get_close_fired() { return close_fired; }\n");
    auto ob = harness.objects.cloneObject("/reload_socket");
    assert(ob != nullptr);

    amlp::Value madeFd = harness.vm.callFunction(ob, "make", {});
    assert(std::get<int64_t>(madeFd.data) >= 0);
    // Captured *before* reload zeroes ob's own "fd" variable -- reading
    // socket_status() through ob's own (post-reload) fd afterward would
    // check whatever that now-zeroed slot happens to read as instead
    // (0, a handle some *other* still-open socket in this same global
    // registry could easily own), not this test's own actual socket.
    int handle = static_cast<int>(std::get<int64_t>(madeFd.data));

    std::vector<amlp::Value> statusArgsBefore{amlp::Value(int64_t{handle})};
    amlp::Value before = amlp::EfunTable::instance().call("socket_status", harness.vm, statusArgsBefore);
    auto* beforeArr = std::get_if<std::shared_ptr<amlp::Array>>(&before.data);
    assert(beforeArr != nullptr && !(*beforeArr)->items.empty());

    std::vector<amlp::Value> reloadArgs{amlp::Value(ob)};
    amlp::EfunTable::instance().call("reload_object", harness.vm, reloadArgs);

    // Real close_referencing_sockets()'s own "socket_close(i, SC_FORCE)"
    // -- SC_FORCE alone, without SC_DO_CALLBACK, so the socket is gone
    // but on_close() never runs.
    std::vector<amlp::Value> statusArgs{amlp::Value(int64_t{handle})};
    amlp::Value after = amlp::EfunTable::instance().call("socket_status", harness.vm, statusArgs);
    auto* afterArr = std::get_if<std::shared_ptr<amlp::Array>>(&after.data);
    assert(afterArr != nullptr && (*afterArr)->items.empty());
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_close_fired", {}).data) == 0);

    std::cout << "testReloadObjectClosesOwnedSocketsWithNoCallbackFiring OK\n";
}

static void testReloadObjectRemovesPendingCallOutsAndDisablesHeartbeat() {
    ObjectVarHarness harness;
    amlp::Scheduler scheduler(harness.vm);
    harness.vm.setScheduler(&scheduler);
    harness.writeFile("/reload_timers.c",
        "int co_fired;\n"
        "int hb_fired;\n"
        "void go() { call_out(\"fire_co\", 0); set_heart_beat(1); }\n"
        "void fire_co() { co_fired = 1; }\n"
        "void heart_beat() { hb_fired = hb_fired + 1; }\n"
        "int get_co_fired() { return co_fired; }\n"
        "int get_hb_fired() { return hb_fired; }\n");
    auto ob = harness.objects.cloneObject("/reload_timers");
    assert(ob != nullptr);

    harness.vm.callFunction(ob, "go", {});

    std::vector<amlp::Value> reloadArgs{amlp::Value(ob)};
    amlp::EfunTable::instance().call("reload_object", harness.vm, reloadArgs);

    // Real "set_heart_beat(obj, 0); remove_all_call_out(obj);" -- neither
    // the pending call_out nor the heartbeat should ever fire once
    // ticked, proving both were genuinely removed, not merely masked.
    scheduler.tickCallOuts();
    scheduler.tickHeartbeats();
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_co_fired", {}).data) == 0);
    assert(std::get<int64_t>(harness.vm.callFunction(ob, "get_hb_fired", {}).data) == 0);

    std::cout << "testReloadObjectRemovesPendingCallOutsAndDisablesHeartbeat OK\n";
}

static void testReloadObjectCascadeDestructsObjectsThatWereShadowingIt() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());
    harness.writeFile("/reload_sh_victim.c", "void create() {}\n");
    harness.writeFile("/reload_sh_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n");
    auto victim = harness.objects.cloneObject("/reload_sh_victim");
    auto shadowOb = harness.objects.cloneObject("/reload_sh_shadow");
    assert(victim != nullptr && shadowOb != nullptr);

    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});
    assert(victim->shadowedBy().lock() == shadowOb);

    // victim is the base of the chain (shadowed by shadowOb, shadowing
    // nothing itself) -- real reload_object()'s own cascade destructs
    // every object shadowing it, identical to destruct()'s own real
    // cascade, but victim itself survives (it is being reloaded, not
    // destructed).
    std::vector<amlp::Value> reloadArgs{amlp::Value(victim)};
    amlp::EfunTable::instance().call("reload_object", harness.vm, reloadArgs);

    assert(shadowOb->isDestructed());
    assert(!victim->isDestructed());
    assert(!victim->shadowedBy().lock());

    std::cout << "testReloadObjectCascadeDestructsObjectsThatWereShadowingIt OK\n";
}

static void testReloadObjectSplicesOutWithoutDestructingAnythingWhenItIsItselfTheShadow() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());
    harness.writeFile("/reload_sp_victim.c", "void create() {}\n");
    harness.writeFile("/reload_sp_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n");
    auto victim = harness.objects.cloneObject("/reload_sp_victim");
    auto shadowOb = harness.objects.cloneObject("/reload_sp_shadow");
    assert(victim != nullptr && shadowOb != nullptr);

    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});
    assert(shadowOb->shadowing().lock() == victim);

    // shadowOb is itself the shadow (shadowing victim, shadowed by
    // nothing) -- real reload_object()'s own cascade condition
    // ("obj->shadowed && !obj->shadowing") is false here, so this takes
    // the splice branch instead: shadowOb's own shadowing relationship
    // to victim is severed, and *neither* object gets destructed.
    std::vector<amlp::Value> reloadArgs{amlp::Value(shadowOb)};
    amlp::EfunTable::instance().call("reload_object", harness.vm, reloadArgs);

    assert(!shadowOb->isDestructed());
    assert(!victim->isDestructed());
    assert(!shadowOb->shadowing().lock());
    assert(!victim->shadowedBy().lock());

    std::cout << "testReloadObjectSplicesOutWithoutDestructingAnythingWhenItIsItselfTheShadow OK\n";
}

static void testReloadObjectLeavesAnActiveSnoopRelationshipUntouched() {
    // Real reload_object() (object.c) has no snoop-related line anywhere
    // in its own body -- confirmed directly, unlike destruct_object()'s
    // own explicit snoop unlinking (simulate.c). A snoop relationship
    // involving the reloaded object must survive completely intact.
    ObjectVarHarness harness;
    harness.writeFile("/reload_sn_victim.c", "void create() {}\n");
    harness.writeFile("/reload_sn_snooper.c",
        "object start(object victim) { return snoop(this_object(), victim); }\n");
    auto victim = harness.objects.cloneObject("/reload_sn_victim");
    auto snooper = harness.objects.cloneObject("/reload_sn_snooper");
    assert(victim != nullptr && snooper != nullptr);

    int fds[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    amlp::Connection conn(fds[0]);
    conn.attach(victim);

    harness.vm.callFunction(snooper, "start", {amlp::Value(victim)});
    assert(victim->snoopedBy().lock() == snooper);
    assert(snooper->snooping().lock() == victim);

    std::vector<amlp::Value> reloadArgs{amlp::Value(victim)};
    amlp::EfunTable::instance().call("reload_object", harness.vm, reloadArgs);

    assert(victim->snoopedBy().lock() == snooper);
    assert(snooper->snooping().lock() == victim);

    ::close(fds[1]);
    std::cout << "testReloadObjectLeavesAnActiveSnoopRelationshipUntouched OK\n";
}

// destruct(): real destruct_object()'s own "if (ob->flags &
// O_EFUN_SOCKET) close_referencing_sockets(ob);" (simulate.c), the real
// gap found and precisely located while implementing reload_object()'s
// own identical need last session, ported here. Both real semantics
// checked: closing the destructed object's own socket directly, and
// closing a *cascaded* shadow object's own socket too (real
// destruct_object() unconditionally closes referencing sockets for
// whatever it actually destructs, including every object the real
// shadow-chain cascade destructs along with the one named explicitly).

static void testDestructClosesOwnedSocketsWithNoCallbackFiring() {
    ObjectVarHarness harness;
    harness.writeFile("/destruct_socket.c",
        "int close_fired;\n"
        "int make() { return socket_create(2, \"on_read\", \"on_close\"); }\n"
        "void on_read(int f, string m, string a) {}\n"
        "void on_close(int f) { close_fired = 1; }\n"
        "int get_close_fired() { return close_fired; }\n");
    auto ob = harness.objects.cloneObject("/destruct_socket");
    assert(ob != nullptr);

    amlp::Value madeFd = harness.vm.callFunction(ob, "make", {});
    assert(std::get<int64_t>(madeFd.data) >= 0);
    // Captured before destruct(), the same reasoning
    // testReloadObjectClosesOwnedSocketsWithNoCallbackFiring's own
    // comment already establishes -- ob's own variables are not zeroed
    // by destruct() the way reload_object() does, but ob is destructed,
    // and this driver's own callFunction() silently no-ops against a
    // destructed target either way, so reading the handle back through
    // ob afterward would not work regardless.
    int handle = static_cast<int>(std::get<int64_t>(madeFd.data));

    std::vector<amlp::Value> statusArgsBefore{amlp::Value(int64_t{handle})};
    amlp::Value before = amlp::EfunTable::instance().call("socket_status", harness.vm, statusArgsBefore);
    auto* beforeArr = std::get_if<std::shared_ptr<amlp::Array>>(&before.data);
    assert(beforeArr != nullptr && !(*beforeArr)->items.empty());

    std::vector<amlp::Value> destructArgs{amlp::Value(ob)};
    amlp::EfunTable::instance().call("destruct", harness.vm, destructArgs);

    // Real "socket_close(i, SC_FORCE)" -- SC_FORCE alone, without
    // SC_DO_CALLBACK, so the socket is gone but on_close() never runs.
    // ob itself is destructed (not reloaded), so get_close_fired() is
    // read through the still-live local shared_ptr, matching this
    // driver's own established "destructed but still referenced" test
    // pattern rather than through a fresh lookup.
    std::vector<amlp::Value> statusArgsAfter{amlp::Value(int64_t{handle})};
    amlp::Value after = amlp::EfunTable::instance().call("socket_status", harness.vm, statusArgsAfter);
    auto* afterArr = std::get_if<std::shared_ptr<amlp::Array>>(&after.data);
    assert(afterArr != nullptr && (*afterArr)->items.empty());
    assert(ob->isDestructed());

    std::cout << "testDestructClosesOwnedSocketsWithNoCallbackFiring OK\n";
}

static void testDestructShadowCascadeAlsoClosesTheCascadedObjectsOwnSockets() {
    ObjectVarHarness harness;
    harness.writeFile("/unused.c",
        "void create() {}\n"
        "int valid_shadow(object ob) { return 1; }\n");
    assert(harness.objects.loadMasterObject());
    harness.writeFile("/destruct_sh_victim.c", "void create() {}\n");
    harness.writeFile("/destruct_sh_shadow.c",
        "object attach(object victim) { return shadow(victim, 1); }\n"
        "int make() { return socket_create(2, \"on_read\", 0); }\n"
        "void on_read(int f, string m, string a) {}\n");
    auto victim = harness.objects.cloneObject("/destruct_sh_victim");
    auto shadowOb = harness.objects.cloneObject("/destruct_sh_shadow");
    assert(victim != nullptr && shadowOb != nullptr);

    harness.vm.callFunction(shadowOb, "attach", {amlp::Value(victim)});
    assert(victim->shadowedBy().lock() == shadowOb);

    // shadowOb owns a socket of its own -- not victim, the object
    // actually named in the destruct() call below.
    amlp::Value madeFd = harness.vm.callFunction(shadowOb, "make", {});
    assert(std::get<int64_t>(madeFd.data) >= 0);
    int handle = static_cast<int>(std::get<int64_t>(madeFd.data));

    // victim is the base of the chain -- destructing it cascades,
    // destructing shadowOb along with it (the exact same real cascade
    // reload_object()'s own equivalent test already exercises).
    std::vector<amlp::Value> destructArgs{amlp::Value(victim)};
    amlp::EfunTable::instance().call("destruct", harness.vm, destructArgs);
    assert(shadowOb->isDestructed());

    // shadowOb's own socket must be closed too -- proof the callback
    // genuinely threads through the recursive cascade, not just the
    // one object named directly in the destruct() call.
    std::vector<amlp::Value> statusArgs{amlp::Value(int64_t{handle})};
    amlp::Value status = amlp::EfunTable::instance().call("socket_status", harness.vm, statusArgs);
    auto* statusArr = std::get_if<std::shared_ptr<amlp::Array>>(&status.data);
    assert(statusArr != nullptr && (*statusArr)->items.empty());

    std::cout << "testDestructShadowCascadeAlsoClosesTheCascadedObjectsOwnSockets OK\n";
}

int main() {
    // Real efuns (sizeof, write, etc.) are only registered here, in this
    // test binary, so the VM-level tests below can call them. Names like
    // "nonexistent_marker_efun" used by the short-circuit tests are not
    // among them, and stay correctly undefined.
    amlp::registerCoreEfuns();

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
    testDoWhileParsesToDoWhileStmt();
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
    testSscanfVmMatchesHexSpecifier();
    testSscanfVmHexSpecifierAcceptsLeading0xPrefix();
    testSscanfVmMatchesFloatSpecifier();
    testSscanfVmAdjacentSThenDWithNoLiteralBetween();
    testSscanfVmAdjacentSThenXWithNoLiteralBetween();
    testSscanfVmAdjacentSThenLiteralPercentWithNoLiteralBetween();
    testSscanfVmTwoAdjacentSSpecifiersThrows();
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
    testThrowIntIsCaughtVerbatimByCatch();
    testThrowStringIsCaughtVerbatimByCatch();
    testThrowArrayValueIsCaughtVerbatimByCatch();
    testThrowWithWrongArgCountThrowsLpcRuntimeError();
    testThrowInsideCalledFunctionWithNoCatchOfItsOwnReachesCallersCatchIntact();
    testAdjacentStringLiteralsParseAsSingleConcatenatedLiteral();
    testAdjacentStringLiteralsVmExecution();
    testBreakAndContinueParseToDedicatedStmtNodes();
    testBreakOutsideLoopThrowsAtCodegen();
    testBreakStopsForLoopEarlyVmExecution();
    testContinueSkipsRestOfForLoopBodyVmExecution();
    testContinueInWhileLoopSkipsToConditionRecheckVmExecution();
    testDoWhileExecutesBodyAtLeastOnceEvenWhenConditionFalseVmExecution();
    testDoWhileLoopsWhileConditionTrueVmExecution();
    testContinueInDoWhileLoopSkipsToConditionRecheckVmExecution();
    testBreakStopsDoWhileLoopEarlyVmExecution();
    testBreakInInnerLoopDoesNotAffectOuterLoopVmExecution();
    testNullStatementAsLoopBodyParsesAndExecutesAsNoOp();
    testReadFileReturnsFileContentAndFalsyForMissingFile();
    testWriteFileThenReadFileRoundTrips();
    testCreateRuntimeErrorFailsLoadInsteadOfCrashing();
    testAbsoluteIncludePathResolvesAgainstMudlibRoot();
    testCppWarningsDoNotFailPreprocessing();
    testIncludeDirConfigSupportsColonSeparatedListLikeRealMudosCfg();
    testIncludeDirSingleEntryWithNoColonStillWorks();
    testFileDunderPredefineResolvesToRealLpcPathNotHostFilesystemPath();
    testDirDunderPredefineTruncatesAfterLastSlashWithMultipleSegments();
    testDirDunderAdjacentToStringLiteralMatchesRealShadowTestShape();
    testUnnamedFunctionParameterParsesAndDoesNotBreakOtherLocals();
    testMultipleUnnamedParametersInOneFunctionDoNotCollide();
    testUnnamedParameterMixedWithNamedOnesStaysPositionallyCorrect();
    testToFloatIntArgConvertsToDouble();
    testToFloatStringArgParsesLeadingFloat();
    testToFloatFloatArgPassesThrough();
    testTypeofReturnsCorrectTypeStringForEachKind();
    testRenameFileAndVerifyViaReadFile();
    testRmdirRemovesEmptyDirectoryAndFailsOnNonEmpty();
    testAbsReturnsPositiveForNegativeIntAndFloat();
    testMaxAndMinReturnCorrectElementFromIntArray();
    testMathEfunsSqrtFloorCeilCosExpLog();
    testTrigAndLog10EfunsMatchKnownExactValues();
    testAsinAcosThrowOutsideDomainButAtanDoesNot();
    testRegexpAssocAliasProducesSameResultAsRegAssoc();
    testRemoveActionRemovesPreviouslyAddedActionAndReturnsZeroWhenNothingToRemove();
    testRmDeletesFileAndReturnsZeroForMissingPath();
    testSetEvalLimitActuallyChangesTheEnforcedCeiling();
    testMapAliasCallsMethodOnTargetForEachElementSameAsMapArray();
    testQueryOnceInteractiveAliasMatchesUserp();
    testObjectsReturnsEveryLiveObjectAndOmitsDestructedOnes();
    testObjectsWithStringFilterExcludesFalsyResultsAndAbortsOnMissingFunction();
    testLivingsReturnsOnlyObjectsWithCommandsEnabled();
    testShallowAndDeepInheritListWalkARealThreeLevelChain();
    testClonepTrueForCloneFalseForBlueprintAndNonObject();
    testVirtualpTrueOnlyForACompileObjectResultAndDefaultsToThisObject();
    testCallStackMode1ReturnsObjectsCurrentFirstWalkingOutward();
    testCallStackModes2And3ThrowNotImplementedButModesOutOfRangeAlsoThrow();
    testCommandsReturnsRegisteredActionsOnTheCommandGiverItself();
    testSocketAddressForInteractiveObjectReturnsPeerAddrAndPortOrZero();
    testSocketAddressForHandleDistinguishesLocalFromRemote();
    testQueryHostNameMatchesRealGethostname();
    testFlushMessagesIsANoOpThatNeverThrowsForEitherObjectKind();
    testGlobalIncludeFileMacroResolvesWhenConfigured();
    testGlobalIncludeFileMacroUnresolvedWhenNotConfigured();
    testGlobalIncludeFileIsANoOpForMudlibsThatNeverSetIt();
    testSqrtNegativeArgThrows();
    testRegexpBasicMatchReturnsOneAndNoMatchReturnsZero();
    testRegexpThirdArgIllegalForStringFormThrows();
    testRegexpArrayFormSelectsMatchingLinesWithIndexAndInvertFlags();
    testRegexpBadPatternThrows();
    testRegexplodeSplitsStringOnPatternMatches();
    testRegexplodeWithCaptureGroupPatternUsesFullMatchNotGroupText();
    testRegAssocMatchesRealDocCommentExample();
    testRegAssocZeroPatternsReturnsWholeStringWithDefaultToken();
    testMapDeleteAndMDeleteAliasBothRemoveTheKey();
    testAllocateAllocateMappingCopyAndValues();
    testClasspAlwaysReturnsFalseSinceNoClassTypeExists();
    testAllPreviousObjectsReturnsSameArrayAsPreviousObjectMinusOne();
    testLocaltimeReturnsElevenElementArrayMatchingKnownEpochInstant();
    testTimeReturnsPlausibleCurrentEpochAndCtimeFormatsAKnownInstant();
    testStatOnRegularFileReturnsSizeAndMtimeArray();
    testReadBytesReadsSubrangeAndHandlesNegativeStartAndMissingFile();
    testWriteBytesOverwritesAtOffsetThenReadBytesConfirmsIt();
    testLinkCreatesASecondNameForTheSameFileContent();
    testUniqueArrayGroupsElementsByClosureResultAndExcludesSkipValue();
    testBaseNameReturnsSameAsFileNameSinceNoCloneSuffixExistsHere();
    testDebugMessageAcceptsAStringArgumentAndDoesNotThrow();
    testUptimeIsNonNegativeAndNonDecreasingAcrossTwoCalls();
    testRusageReturnsMappingWithExpectedKeysAndNonNegativeValues();
    testCommandDispatchesToCurrentObjectsOwnActionTableAndReturnsTruthy();
    testShutdownSetsSchedulerRequestFlag();
    testInEditAlwaysReturnsFalseSinceEdIsNotImplemented();
    testInInputReflectsPendingInputToStateOnAConnectedObject();
    testMatchPathReturnsDeepestMatchingPrefix();
    testCallOutInfoListsPendingEntryWithOwnerFunctionAndDelay();
    testShadowAttachInterceptsCallOtherWhenShadowDefinesFunction();
    testShadowFallsThroughToVictimWhenFunctionUndefinedOnShadowRegardlessOfReturnValue();
    testShadowDefinedFunctionReturningFalsyIsStillFinalNotAFallThroughTrigger();
    testShadowGuardsAgainstReenteringItselfViaCurrentObjectCheck();
    testShadowDestructingVictimCascadesEntireChain();
    testShadowDestructingShadowSplicesItOutLeavingVictimIntact();
    testShadowDeniedWhenMasterHasNoValidShadowApproval();
    testShadowQueryFormAndQueryShadowingReturnBothDirectionsOrZero();
    testShadowRejectsSelfShadowAlreadyShadowingAndAlreadyShadowed();
    testSnoopStartLinksBothDirectionsAndQueryReflectsThem();
    testSnoopOutputDuplicationCallsReceiveSnoopOnSnooperWithMatchingText();
    testSnoopDeniesNotInteractiveThrowsAndLoopReturnsFalsy();
    testSnoopChainCycleDeniedByAntiLoopWalk();
    testSnoopStopFormUnlinksAndReturnsByItself();
    testSnoopVictimDisconnectClearsBothSidesOfTheRelationship();
    testSnoopSnooperDestructedClearsVictimsSnoopedBy();
    testIacSequencesAreStrippedAndNeverReachDispatchedLines();
    testIacIacIsAnEscapedLiteral0xffDataByteNotACommand();
    testTelnetWillEchoAndNawsAreSilentlyAcceptedOtherOptionsRefused();
    testTelnetDoEchoIsSilentlyAcceptedOtherOptionsRefusedWithWont();
    testNawsSubnegotiationUpdatesTerminalWidthAndHeight();
    testNawsSubnegotiationSplitAcrossTwoReadsStillParsesCorrectly();
    testQueryScreenWidthAndHeightReturnNegotiatedValues();
    testInputToNoEchoFlagSendsIacWillEchoImmediately();
    testEchoReenabledWithIacWontEchoWhenAwaitedLineArrives();
    testWindowSizeUpdateFlagSetOnNawsAndConsumedOnce();
    testWindowSizeUpdateFlagNotSetByPlainDataLines();
    testTerminalColourSubstitutesRecognizedTokensWithMaxColorsOn();
    testTerminalColourStripsRecognizedTokensWithMaxColorsOff();
    testTerminalColourLeavesUnrecognizedTokensAndPlainTextAsIs();
    testTerminalColourWithNoMarkupReturnsStringUnchanged();
    testTerminalColourMultipleRealCodesInOneString();
    testQueryIpNumberAndQueryIpNameReturnLoopbackAddressForCurrentConnection();
    testQueryIpNumberReturnsZeroWithNoCurrentConnection();
    testSocketStatusReturnsRealShapeArrayForKnownFdAndIncludesItInTheAllForm();
    testSocketCreateRejectsUnsupportedModesAndReturnsIncreasingHandles();
    testSocketWriteOnUnknownHandleReturnsFdRangeAndErrorTextMatchesReal();
    testSocketStreamCreateBindListenAcceptConnectWriteReadCloseRoundTrip();
    testSocketDatagramWriteAndReadCallbackCarriesSenderAddress();
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
    testFireNetDeadIfLinkDeadCallsApplyWhenPeerClosesConnection();
    testFireNetDeadIfLinkDeadIsNoOpWhileConnectionStillOpen();
    testFireNetDeadIfLinkDeadSkipsAfterExplicitConnectionClose();
    testDestructEfunClosesTargetObjectsOwnConnectionNotCallersConnection();
    testDestructEfunStillClosesOwnConnectionWhenSelfDestructing();
    testDestructEfunOnNonInteractiveObjectDoesNotTouchAnyConnection();
    testUserpAndInteractiveBothTrueWhileConnectionIsLive();
    testUserpStaysTrueAfterDisconnectWhileInteractiveGoesFalse();
    testUserpReturnsFalseForObjectNeverBoundToAnyConnection();
    testQueryIdleIsZeroImmediatelyAfterConnectionEstablished();
    testQueryIdleReflectsMostRecentDispatchedLineNotJustConnectionTime();
    testQueryIdleThrowsForObjectNeverBoundToAnyConnection();
    testFindPlayerFindsCurrentlyConnectedObjectByLivingName();
    testFindPlayerStillFindsObjectAfterDisconnectViaOnceInteractive();
    testFindPlayerDoesNotMatchAnObjectThatWasNeverInteractive();
    testFindLivingMatchesAnNpcThatWasNeverInteractive();
    testFindLivingReturnsNullWithoutEnableCommands();
    testFindLivingReturnsNullForUnknownName();
    testFindLivingDoesNotMatchADestructedObjectsFormerLivingName();
    testMessageRoutesToTargetObjectsOwnConnectionNotCurrentOne();
    testCallOutAcceptsRealArgumentShapeAndReturnsHandle();
    testRemoveCallOutReturnsMinusOneWhenNothingPendingUnderThatName();
    testCallOtherWithStringTargetResolvesAlreadyLoadedObject();
    testCallOtherWithStringTargetAutoCompilesAndLoadsOnFirstUse();
    testCallOtherWithStringTargetToNonexistentFileThrows();
    testConvertNameMudlibFunctionWorksWithNewLowerCaseAndReplaceStringEfuns();
    testUpperCaseFoldsLowercaseLettersAndLeavesEverythingElseUnchanged();
    testUpperCaseMatchesRealGuildTagUppercasingShape();
    testUpperCaseThrowsOnNonStringArgument();
    testCapitalizeUppercasesOnlyALowercaseFirstCharacter();
    testStrlenAndStrstrAliasesWorkByTheirOwnNames();
    testCryptWithExplicitSaltIsDeterministicAndSaltIsThePrefix();
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
    testDestructedObjectIsUnlinkedFromEnvironmentInventory();
    testCallOtherOnDestructedObjectIsSilentNoOp();
    testCallClosureThrowsForDestructedOwnerEvenWhenStillReferenced();
    testDispatchCommandSkipsActionFromDestructedOwnerEvenWhenStillReferenced();
    testCallOutSkipsDestructedTargetEvenWhenStillReferenced();
    testDestructedObjectInLocalVariableReadsBackAsIntZero();
    testDestructedObjectInObjectVariableReadsBackAsIntZero();
    testDestructedObjectInArrayElementReadsBackAsIntZeroWhenIndexed();
    testDestructedObjectInMappingValueReadsBackAsIntZeroWhenIndexed();
    testNonDestructedObjectInVariableAndArrayStillReadsBackAsRealObject();
    testCallOutAcceptsClosureAsFirstArgument();
    testPreviousObjectReturnsCallerAcrossCallOther();
    testPreviousObjectDoesNotChangeAcrossSameObjectLocalCall();
    testPreviousObjectMinusOneReturnsFullChain();
    testUnguardedClosureRoundTripsThroughSecurityAndMasterShape();
    testSaveObjectRestoreObjectRoundTripsNestedMappingsAndArrays();
    testRestoreObjectParsesRealFluffosOnDiskFormatScalarsAndNesting();
    testRestoreObjectSkipsRealFormatCommentHeaderLineAndParsesEmptyContainers();
    testRestoreObjectRealFormatStringEscapesAndEmbeddedNewline();
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
    testDollarLambdaParamBindsClosuresOwnFirstCallTimeArgument();
    testDollarLambdaParamMultipleParametersBindPositionally();
    testDollarLambdaParamFilterMatchesRealEventsDShape();
    testDollarParamOutsideLambdaBodyThrowsParseError();
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
    testFirstInventoryAndNextInventoryWalkChildrenInMoveOrder();
    testFirstInventoryStringArgumentResolvesOrThrows();
    testFirstInventoryAndNextInventorySkipHiddenSiblingsUnlessPermitted();
    testStrcmpMatchesRealCComparisonSemantics();
    testMapDeleteRemovesKeyInPlaceAndLeavesOthersIntact();
    testCloneObjectAcceptsPathWithTrailingDotCWithoutDoublingExtension();
    testGetDirMatchesGlobPatternInFinalPathComponentOnly();
    testIntpTrueOnlyForIntNotStringObjectOrUnsetVariable();
    testFloatpTrueOnlyForFloatNotIntOrString();
    testArrayFunctionMapObjectPointerPredicatesEachTrueOnlyForOwnKind();
    testRepeatStringConcatenatesNTimesAndEmptyForZeroOrNegative();
    testPresentFindsInventoryItemByIdApplyNotByOtherFunctions();
    testLivingReflectsEnableCommandsStateAndDefaultsToCurrentObject();
    testSetHideTogglesHiddenFlagWhenMasterValidHidePermits();
    testSetHideDeclinesWhenValidHideRejectsOrNoMasterLoaded();
    testAddActionExactVerbMatchDispatchesWithRemainderAsArgumentAndDeclinesUnknownVerbs();
    testAddActionCatchAllShortFlagReceivesRemainderAndQueryVerbReturnsFullTypedWord();
    testDispatchCommandPassesUndefinedNotEmptyStringForBareVerbWithNoArgument();
    testDispatchCommandTriesNextMatchWhenFirstHandlerReturnsFalsy();
    testNotifyFailMessageShownWhenNoHandlerClaimsCommand();
    testNotifyFailMessageSuppressedWhenLaterHandlerClaimsCommand();
    testNotifyFailDoesNotLeakIntoALaterUnrelatedFailedDispatch();
    testNotifyFailFunctionFormShowsOnlyAStringReturn();
    testNotifyFailThrowsOnNonStringNonFunctionArgument();
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
    testCallOutFiresOnceDueTimeArrivesWithExtraArgsInOrder();
    testCallOutSelfReschedulingSurvivesTickIteration();
    testCallOutClosureFormFiresViaCallClosureNotCallFunction();
    testRemoveCallOutByHandlePreventsFiringAndReturnsRemainingSeconds();
    testRemoveCallOutByNameIsScopedToCallingObjectAndSkipsClosures();
    testFindCallOutReturnsRemainingSecondsOrMinusOneWithoutRemoving();
    testCallOutRuntimeErrorIsIsolatedFromOtherPendingCallOuts();
    testCallOutSkipsDestructedTargetSilently();
    testSetHeartBeatIntervalFiresOnceEveryNCyclesNotEveryCycle();
    testSetHeartBeatZeroDisablesAndStopsFutureFiring();
    testHeartbeatRuntimeErrorIsolatedFromOtherHeartbeatEnabledObjects();
    testHeartbeatPrunesDestructedObjectSilently();
    testHeartbeatCallingSetHeartBeatOnItselfDoesNotCorruptIteration();
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
    testSprintfCentreJustifiedFieldWidthSplitsPaddingEvenly();
    testSprintfCentreJustifiedFieldWidthPutsExtraPadOnTheLeft();
    testSprintfStringFieldWidthLeftJustifies();
    testSprintfDoublePercentEmitsLiteralPercentAndConsumesNoArgument();
    testSprintfColonFieldWidthPadsAShorterStringLeftJustified();
    testSprintfColonFieldWidthTruncatesALongerString();
    testSprintfBuildingAndThenUsingADynamicColonFormatString();
    testSprintfPercentXEmitsLowercaseHex();
    testSprintfPercentXThrowsOnNonIntArgument();
    testSprintfPercentOEmitsOctal();
    testSprintfPercentODumpsIntFloatAndString();
    testSprintfPercentODumpsEmptyAndNonEmptyArrayWithNesting();
    testSprintfPercentODumpsEmptyAndNonEmptyMapping();
    testSprintfPercentODumpsObjectAndDestructedObjectAsZero();
    testSprintfPercentODumpsClosureWithBoundArgs();
    testSprintfPercentOFieldWidthAndPrecisionApplyLikePercentS();
    testPrintfLilEvalShapeMatchesRealResultPrefix();
    testSprintfDotPrecisionTruncatesLongerString();
    testSprintfDotPrecisionWidensFieldWhenGreaterThanExplicitWidth();
    testSprintfStarFieldWidthPullsWidthFromLeadingArgument();
    testSprintfStarPrecisionPullsPrecisionFromLeadingArgument();
    testSprintfZeroPaddedStarFieldWidthThrows();
    testPrintfWritesSprintfFormattedResultToCurrentConnection();
    testPrintfThrowsOnNonStringFormatArgument();
    testFunctionExistsReturnsTruthyStringForALocallyDefinedFunction();
    testFunctionExistsReturnsZeroForAnUndefinedFunction();
    testFunctionExistsDefaultsToCurrentObjectWhenObjectArgumentOmitted();
    testFunctionExistsFindsAnInheritedFunctionNotJustLocalOnes();
    testFunctionExistsThrowsOnNonStringFunctionNameArgument();
    testPpCombatBonusEfunMatchesLpcAcrossBoundaries();
    testPsDamageBonusEfunMatchesLpcAcrossBoundariesAndSupernatural();
    testOccBaseApmEfunMatchesLpcAcrossAllCategoriesAndEdgeCases();
    testRollWeaponDamageDiceStaysWithinFormulaDerivedBoundsAcrossManyDraws();
    testQueryStrikeBonusEfunMatchesLpcAcrossPlayerStates();
    testQueryParryBonusEfunMatchesLpcAcrossPlayerStates();
    testQueryDodgeBonusEfunMatchesLpcAliasOfParryBonus();
    testSetBitAndTestBitRoundTripSingleBit();
    testSetBitThrowsOnOutOfRangeOrNegativeBitIndex();
    testClearBitIsNoOpPastStringLengthAndClearsWithinRange();
    testCrc32ReturnsKnownValueForHelloAndSeedValueForEmptyString();
    testCrc32ThrowsOnNonStringArgument();
    testCpCopiesFileContentAndReturnsTruthy();
    testInheritsMatchesTransitiveChainInBothDirections();
    testGetConfigReturnsMudNameForIndexZeroAndThrowsForNegative();
    testQueryLoadAverageReturnsAStringInRealFormat();
    testSayBroadcastsToRoomSiblingsButNotOriginItself();
    testSayAvoidArgumentExcludesSpecifiedTarget();
    testSaveVariableMatchesRealFormatForStringsNumbersArraysAndMappings();
    testRestoreVariableRoundTripsSaveVariableOutputAndRejectsMalformedInput();
    testChildrenReturnsEveryLiveObjectMatchingFilenamePrefix();
    testSetLightAccumulatesUpThroughEnvironmentChainAndReturnsRootTotal();
    testSetDebugLevelAcceptsIntOrStringWithoutThrowing();
    testBindRebindsClosureOwnerAndChangesResolution();
    testBindIsNoOpWhenNewOwnerMatchesCurrentOwner();
    testBindThrowsWhenNoMasterIsLoaded();
    testTellObjectWritesToConnectionOrCallsCatchTellWhenNotInteractive();
    testTellRoomBroadcastsToDirectInventoryExcludingAvoid();
    testShoutBroadcastsToEveryoneExceptCommandGiver();
    testThisInteractiveAndThisUserReturnConnectionBoundObjectNotCommandGiver();
    testMapMappingReplacesValuesKeepingKeysViaStringFunctionName();
    testFilterMappingKeepsOnlyEntriesWhereCallbackIsTruthy();
    testStrwidthReturnsSameLengthAsSizeof();
    testResetEvalCostZeroesUsedCostLeavingCeilingUnchanged();
    testEvalCostAndMaxEvalCostQueryWithoutMutatingStateThenExplicitArgumentSetsCeiling();
    testRemoveShadowSplicesOutOfChainAndReturnsZeroWhenNotShadowing();
    testOldcryptTruncatesSaltToFirstTwoCharactersUnlikeCrypt();
    testNextBitFindsFollowingSetBitWithRealBoundaryAsymmetry();
    testElementOfReturnsAMemberOfTheArrayAndThrowsWhenEmpty();
    testShuffleReordersInPlaceAndKeepsSameElementsAndIdentity();
    testRealTimeReturnsCurrentUnixTime();
    testRemoveInteractiveClosesConnectionWithoutDestructingAndReturnsZeroWhenNotInteractive();
    testFileLengthCountsNewlinesAndReturnsNegativeForMissingOrDirectory();
    testRefsReflectsSharedReferenceCountMinusOne();
    testHeartBeatsListsEveryObjectWithHeartbeatEnabledSkippingDestructed();
    testQueryIpPortReturnsConfiguredPortForInteractiveElseZero();
    testNamedLivingsListsOnlyLivingNamedObjectsWithCommandsEnabledRespectingHidden();
    testQueryNotifyFailPeeksPendingMessageWithoutConsumingIt();
    testRequestTermSizeSendsIacDoNawsAndIsNoOpWithoutInteractiveCommandGiver();
    testPluralizeMatchesRealExceptionTableGeneralRulesAndOfClauseAcrossVariousInputs();
    testUniqueMappingGroupsByCallbackResultInFirstAppearanceOrderForClosureAndStringForms();
    testReclaimObjectsCoercesStaleReferencesAndErasesDestructedMappingKeysReturningCount();
    testReplaceProgramDeferredSwapPreservesInheritedVariablesAndDropsOwnFunctions();
    testQueryReplacedProgramReflectsOnlyAnActuallyAppliedSwap();
    testReplaceProgramThrowsWhenTargetNotInheritedOrCalledOnSimulEfunObject();
    testFunctionOwnerReturnsClosureOwnerOrZeroOnceOwnerIsGone();
    testNumClassesAlwaysReturnsZeroSinceClassDeclarationsDoNotExist();
    testSetAuthorAcceptsStringReturnsVoidAndThrowsOnWrongType();
    testReplaceableTrueOnlyWhenEveryLocalFunctionIsIgnorable();
    testQueryNumMatchesRealAssemblyOrderAcrossRepresentativeInputsAndLimits();
    testOriginReturnsLocalForABareSameObjectCall();
    testOriginReturnsCallOtherForACallOtherDispatch();
    testOriginReturnsSimulForABareCallResolvingToSimulEfun();
    testOriginReturnsInternalForCallOutStringFormFiring();
    testOriginReturnsEfunForAMapArrayCallback();
    testOriginReturnsFunctionalForAnInlineLambdaBody();
    testOriginReturnsDriverForTopLevelDispatchHeartBeatAndCommandDispatch();
    testOriginNameCoversAllEightRealValuesIncludingTheUnreachableFunctionPointer();
    testReloadObjectResetsVariablesReinitializesAndCallsCreateAgain();
    testReloadObjectClosesOwnedSocketsWithNoCallbackFiring();
    testReloadObjectRemovesPendingCallOutsAndDisablesHeartbeat();
    testReloadObjectCascadeDestructsObjectsThatWereShadowingIt();
    testReloadObjectSplicesOutWithoutDestructingAnythingWhenItIsItselfTheShadow();
    testReloadObjectLeavesAnActiveSnoopRelationshipUntouched();
    testDestructClosesOwnedSocketsWithNoCallbackFiring();
    testDestructShadowCascadeAlsoClosesTheCascadedObjectsOwnSockets();
    testFunctionsListsOwnAndInheritedNamesWithOverridePrecedence();
    testFunctionsDetailedFormIncludesNumArgsAndMixedTypePlaceholders();
    testVariablesListsFlattenedNamesInInheritedThenOwnOrder();
    testFetchAndStoreVariableRoundTripByNameAndThrowOnUnknownName();
    testSocketReleaseAndAcquireHandOffOwnershipAndCallbacks();
    testSocketReleaseRevertsWhenNeverAcquiredAndRejectsTheWrongCaller();
    std::cout << "all tests passed\n";
    return 0;
}
