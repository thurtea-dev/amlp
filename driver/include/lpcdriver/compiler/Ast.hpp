#pragma once
#include <memory>
#include <string>
#include <vector>

namespace lpcdriver {

struct AstNode {
    virtual ~AstNode() = default;
};
using AstPtr = std::unique_ptr<AstNode>;

struct StringLiteral : AstNode {
    std::string value;
};

struct IntLiteral : AstNode {
    int64_t value = 0;
};

struct FloatLiteral : AstNode {
    double value = 0.0;
};

struct VarRefExpr : AstNode {
    std::string name;
};

// "And"/"Or" are the short-circuit logical && / || (see emitLogicalExpr);
// "BitAnd" is the plain "&" operator, distinct because it is neither
// short-circuiting nor purely numeric in real LPC -- on two arrays it is
// set intersection, not a bitwise op (confirmed against the FluffOS
// reference driver: eoperators.c's f_and() branches on T_ARRAY before
// falling back to integer "&"). Hit live in secure/daemon/master.c:
// "sizeof(privs & ok)".
// "BitOr"/"BitXor" are the plain "|"/"^" operators, both int-only in
// this driver (unlike BitAnd, real FluffOS's "|" is also array union --
// array.c's f_or() -- but nothing this driver runs yet needs that, only
// the plain-int flags-bitmask shape -- secure/std/login.c's own
// "input_to(\"get_password\", 1 | 2)").
enum class BinOp { Eq, Neq, Lt, Lte, Gt, Gte, Add, Or, And, Sub, Mul, Div, Mod, BitAnd, BitOr, BitXor };

struct BinaryExpr : AstNode {
    BinOp op;
    AstPtr left;
    AstPtr right;
};

enum class UnaryOp { Not, Neg };

struct UnaryExpr : AstNode {
    UnaryOp op = UnaryOp::Not;
    AstPtr operand;
};

struct TernaryExpr : AstNode {
    AstPtr condition;
    AstPtr thenBranch;
    AstPtr elseBranch;
};

struct CallExpr : AstNode {
    std::string callee;
    std::vector<AstPtr> args;
    // "efun::name(...)" (grammar.y's "efun_override: L_EFUN L_COLON_COLON
    // identifier"), real LPC's explicit escape hatch straight to the core
    // efun table, skipping local/inherited functions and the simul_efun
    // object entirely -- e.g. secure/SimulEfun/misc.c's own
    // "efun::destruct(ob)", needed there because this file defines its
    // own simul_efun "destruct(object)" wrapper and must still be able to
    // reach the real one. CodeGen::emitCallExpr() emits OpCode::CallEfun
    // instead of the usual tiered OpCode::Call when this is set.
    bool forceEfun = false;

    // "::name(...)" or "qualifier::name(...)" -- an explicit call to an
    // *inherited* definition of a function, bypassing this program's own
    // local definition of the same name even when one exists (grammar.y's
    // function_name production: "L_COLON_COLON identifier" for the bare
    // form, "identifier L_COLON_COLON identifier" for the qualified one
    // -- confirmed live: secure/daemon/account_d.c's own "::create();",
    // calling the daemon.c parent's create() in addition to this file's
    // own override, and secure/daemon/banish.c's "daemon::create();",
    // the same idea naming which ancestor explicitly). Overwhelmingly
    // common across this mudlib (800+ files) since it is the standard
    // way an overridden create()/init() still runs its parent's own
    // setup. An empty parentQualifier means the bare form: search the
    // whole inherited chain depth-first for the nearest match, the same
    // order VM.cpp's findFunctionInChain already walks for a plain call
    // -- just starting one level down, skipping this program's own
    // top-level functions. A non-empty parentQualifier (e.g. "daemon")
    // restricts the search to the inherited program whose own "inherit"
    // path's last path component matches it.
    bool parentCall = false;
    std::string parentQualifier;
};

// The function-name argument is a full expression, not necessarily a
// string literal: real LPC allows "call_other(target, name_var, ...)"
// with the name resolved at run time (confirmed live in the mudlib --
// secure/daemon/master.c's call_other(file, arg), where "arg" is a plain
// variable). The "target->name(...)" syntax is always a literal name
// syntactically (there is no "target->(expr)(...)" form in LPC), so that
// path wraps the identifier in a StringLiteral to fit this same shape.
struct CallOtherExpr : AstNode {
    AstPtr target;
    AstPtr function;
    std::vector<AstPtr> args;
};

// sscanf(target, format, ...outputVars). Real LPC gives sscanf() its own
// grammar production (grammar.y's "sscanf:" rule uses "lvalue_list", not
// a plain arg list) precisely because its trailing arguments are implicit
// lvalues, not ordinary by-value expressions -- there is no "&var"
// reference syntax in LPC's sscanf, unlike C's. Matching that, this stores
// the output arguments as plain names to resolve to local/object-var
// slots at codegen time, the same way AssignStmt already does.
struct SscanfExpr : AstNode {
    AstPtr target;
    AstPtr format;
    std::vector<std::string> varNames;
};

// "catch(expr)", real LPC's own control-flow construct for trapping a
// runtime error (grammar.y: "catch: L_CATCH expr_or_block", a dedicated
// grammar production, not a function call -- confirmed against the
// FluffOS reference driver directly, not inferred). Only the
// parenthesized-expression form is implemented ("catch(expr)"), not the
// block form ("catch { stmts }") grammar.y's own "expr_or_block: block |
// '(' comma_expr ')'" also allows -- nothing in this mudlib uses the
// block form. See CodeGen::emitCatchExpr()/VM.cpp's PushCatchFrame
// handling for the runtime semantics (guarded's own result is always
// discarded; catch(expr) evaluates to 0 on success or the error message
// string on failure, confirmed against interpret.c's F_END_CATCH/
// do_catch()).
struct CatchExpr : AstNode {
    AstPtr guarded;
};

// "(: name, bound_args... :)" -- a closure/function-pointer literal
// (see Value.hpp's Closure comment for the full citation and the real
// forms deliberately not implemented). Only the bare-identifier form:
// "name" is always a plain identifier token here, resolved lazily at
// call time rather than at parse/codegen time (see
// VM::callClosure()'s own comment) -- there is no expression form
// here, unlike CallOtherExpr's function-name argument.
struct ClosureLiteralExpr : AstNode {
    std::string functionName;
    std::vector<AstPtr> boundArgs;
};

// "(: comma_expr :)" -- the general "inline lambda" closure literal
// grammar.y keeps as a distinct production from the bare-identifier one
// above ("L_FUNCTION_OPEN comma_expr ':' ')'", confirmed by direct
// reading, not guessed: real LPC's LALR grammar has two separate rules
// here, told apart at parse time by whether the first token is a bare
// name immediately followed by "," or ":" -- see Parser.cpp's own
// comment at the "(:" recognition site for the disambiguation and the
// two confirmed real call sites this covers, std/user/editor.c lines 31
// and 64). Every expression in bodyExprs is evaluated in order for side
// effect except the last, whose value becomes the closure's return
// value when later invoked -- ordinary comma-expression semantics,
// matching grammar.y's own "comma_expr: expr0 | comma_expr ',' expr0"
// used for the body. Confirmed via grammar.y's own
// "if ($2->kind == NODE_STRING) yywarn(\"Function pointer returning
// string constant is NOT a function call\")": the reference compiler
// accepts a body ending in a bare string constant and only *warns* that
// it looks like a mistake, proving the body is real compiled code run
// at call time (not a disguised "call this method name on this
// object"), i.e. std/user/editor.c's own
// "(: previous_object(), \"abort\" :)" really does evaluate
// previous_object() for effect and then just return the string
// "abort" when invoked, never actually calling abort() on anything.
// See CodeGen.cpp's PendingLambda handling for how this compiles to
// its own anonymous FunctionEntry, and Value.hpp's Closure comment
// (now superseded for this case) for the prior bare-name-only scope
// note.
struct InlineLambdaExpr : AstNode {
    std::vector<AstPtr> bodyExprs;
};

struct ExprStmt : AstNode {
    AstPtr expr;
};

struct ReturnStmt : AstNode {
    AstPtr expr;
};

struct VarDeclStmt : AstNode {
    std::string type;
    bool isArray = false;
    std::string name;
    AstPtr initializer;
};

struct AssignStmt : AstNode {
    std::string name;
    AstPtr value;
};

// Assignment as an expression (as opposed to AssignStmt, which is
// statement-level "name = expr;"). Needed so assignment can appear inside a
// for-loop's init/update clauses ("for (i = 0; ...; i = i + 1)") and other
// expression contexts, matching real LPC where "=" is a right-associative
// expression operator (grammar.y: "%right L_ASSIGN", the lowest-precedence
// operator, looser even than "?:"). Only a bare variable name target is
// supported this slice, same scope limitation AssignStmt already has.
// isCompound covers "+=", "-=", "*=", "/=", "%=" (e.g. real usage in
// secure/daemon/master.c: "files += ({ lines[i] });"), each desugaring at
// codegen time to "name = name <op> value". Since the target here is
// always a bare variable name (never an expression with side effects),
// desugaring to a read-modify-write is behaviorally identical to real
// LPC's single-evaluation compound assignment, just simpler to generate.
struct AssignExpr : AstNode {
    std::string name;
    AstPtr value;
    bool isCompound = false;
    BinOp compoundOp = BinOp::Add;
};

enum class IncDecOp { Inc, Dec };

// "++x" / "--x" (prefix) and "x++" / "x--" (postfix). Scoped to a bare
// variable name target for now (needed by for-loop update clauses and
// standalone statements, per the plan); indexed targets like "arr[i]++"
// are not supported and throw a clear parse error rather than silently
// misparsing.
// "name" is used when indexTarget is null (the original bare-variable-
// only scope this struct started with); a non-null indexTarget/indexKey
// pair means an indexed target instead, e.g. std/living.c's own
// "healing[\"intox\"]--" (confirmed live -- the driver previously
// rejected this real mudlib line the same way IndexAssignExpr's own
// comment describes for indexed assignment, and for the same reason:
// only a bare variable name was recognized as a valid ++/-- target).
// See CodeGen::emitIncDecExpr()'s indexed branch for the codegen.
struct IncDecExpr : AstNode {
    IncDecOp op = IncDecOp::Inc;
    bool prefix = true;
    std::string name;
    AstPtr indexTarget;
    AstPtr indexKey;
};

struct ArrayLiteralExpr : AstNode {
    std::vector<AstPtr> elements;
};

struct MappingLiteralExpr : AstNode {
    std::vector<std::pair<AstPtr, AstPtr>> entries;
};

// "<N" inside an index/range bound means "N from the end", real LPC's
// own reverse-indexing syntax (grammar.y: "expr4 '[' '<' comma_expr
// ']'" and its range-form siblings) -- confirmed against eoperators.c's
// f_range()/f_extract_range(): the actual index used is "length - N"
// (so "<1" is the last element, "<2" the second-to-last, ...), computed
// against the target's own runtime length, not resolvable at parse
// time. indexFromEnd/rangeEndFromEnd record which bound(s), if any,
// used this form; see VM.cpp's OpCode::Index/RangeIndex handling for
// where the actual "length - N" conversion happens.
struct IndexExpr : AstNode {
    AstPtr target;
    AstPtr index;
    AstPtr rangeEnd = nullptr; // non-null means range index, e.g. str[start..end]
    bool indexFromEnd = false;
    bool rangeEndFromEnd = false;
};

// isCompound/compoundOp mirror AssignExpr's own fields: "target[index]
// += value" etc, e.g. std/user.c's own "player_data[\"general\"]
// [\"quest points\"] += (int)call_other(...)". target/index are
// evaluated twice by CodeGen::emitIndexAssignStmt() in this case (once
// to read the current value, once to write the new one) rather than
// duplicated on the stack -- harmless for every real target/index this
// mudlib's own compound-indexed-assignment call sites actually use
// (plain variable reads and string-literal keys, no side effects), but
// would double any side effect a more exotic target/index expression
// happened to have. Flagged here rather than silently assumed safe.
struct IndexAssignStmt : AstNode {
    AstPtr target;
    AstPtr index;
    AstPtr value;
    bool isCompound = false;
    BinOp compoundOp = BinOp::Add;
};

// The expression-producing counterpart to IndexAssignStmt above, needed
// when an indexed assignment appears as a sub-expression rather than a
// standalone statement -- e.g. std/user/more.c's own
// "if(!(__More[\"class\"] = cl)) ...", confirmed live: the driver's
// parser previously only recognized a bare variable name as an
// assignment target inside an expression (see Parser.cpp's own comment
// at the "sawAssignOp" site) and rejected this real mudlib line
// outright. Same fields as IndexAssignStmt; see
// CodeGen::emitIndexAssignExpr() for why this needs its own codegen
// rather than reusing emitIndexAssignStmt() as-is (OpCode::IndexAssign
// consumes its three operands and leaves nothing behind, matching
// statement-context needs, but an expression use needs the assigned
// value left on the stack afterward).
struct IndexAssignExpr : AstNode {
    AstPtr target;
    AstPtr index;
    AstPtr value;
    bool isCompound = false;
    BinOp compoundOp = BinOp::Add;
};

struct Block : AstNode {
    std::vector<AstPtr> statements;
};

struct IfStmt : AstNode {
    AstPtr condition;
    std::unique_ptr<Block> thenBranch;
    std::unique_ptr<Block> elseBranch;
};

struct WhileStmt : AstNode {
    AstPtr condition;
    std::unique_ptr<Block> body;
};

// "for (init; condition; update) body". init is either a VarDeclStmt (a
// single declaration, optionally with an initializer, e.g. "int i = 0") or
// a plain expression statement (e.g. "i = 0", reusing AssignExpr); any of
// the three clauses may be empty (null), matching real LPC's
// "for_expr: /* EMPTY */ | comma_expr" grammar rule -- an empty condition
// means "always true", same as C.
struct ForStmt : AstNode {
    AstPtr init;
    AstPtr condition;
    AstPtr update;
    std::unique_ptr<Block> body;
};

struct BreakStmt : AstNode {};
struct ContinueStmt : AstNode {};

// "foreach (var in collection) body" and "foreach (var, valueVar in
// collection) body". Each loop variable may be a pre-existing name
// (declareXxxVar false, resolved like any other variable reference) or
// declare a brand-new local inline ("foreach (string s in ...)",
// declareXxxVar true) -- both are real LPC (grammar.y's foreach_var:
// L_DEFINED_NAME | single_new_local_def). For a mapping, single-variable
// foreach iterates its keys (matching real LPC); two-variable foreach
// iterates (key, value) pairs. For an array, single-variable foreach
// iterates elements; two-variable foreach over a plain array is not
// meaningfully supported (real usage of the two-variable form in this
// mudlib is always over mappings) -- see CodeGen::emitForeachStmt.
// A single "case value:" or "default:" label with no attached statements
// of its own -- it is a marker interleaved into SwitchStmt::body in
// source order, exactly where it appeared, so fallthrough (no implicit
// break between cases, matching real LPC/C) falls out naturally from
// just emitting body statements in that same order. value == nullptr
// means "default:". Range case labels ("case A..B:") are not
// implemented -- not used anywhere in this mudlib -- and throw a clear
// parse error rather than silently misparsing.
struct CaseLabel : AstNode {
    AstPtr value;
};

struct SwitchStmt : AstNode {
    AstPtr subject;
    std::vector<AstPtr> body; // CaseLabel and ordinary statement nodes, interleaved
};

struct ForeachStmt : AstNode {
    std::string varName;
    bool declareVar = false;
    bool hasValueVar = false;
    std::string valueVarName;
    bool declareValueVar = false;
    AstPtr collection;
    std::unique_ptr<Block> body;
};

struct Param {
    std::string type;
    std::string name;
    bool isArray = false;
};

struct FunctionDecl : AstNode {
    std::string returnType;
    bool returnTypeIsArray = false;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Block> body;
};

struct ObjectVarDecl : AstNode {
    std::string type;
    bool isArray = false;
    std::string name;
    // See Parser.hpp's DeclPrefix::isPrivate comment. Consumed by
    // CodeGen::generate(): a private variable still occupies a real slot
    // in the flattened per-object layout (an inheriting child's own
    // code must land at the same slot offsets a parent's already-
    // compiled bytecode expects), but that slot is recorded under a
    // synthesized, non-collidable placeholder name in the
    // CompiledProgram::objectVarNames a child inherits, rather than its
    // real name -- so the real name stays reachable from this file's
    // own code (ordinary resolveVariable() lookups here still use it)
    // while staying invisible to, and non-collidable with, any child.
    bool isPrivate = false;
    // "type name = expr;" -- real, standard LPC (confirmed against the
    // FluffOS reference driver's grammar), needed live by secure/daemon/
    // wiztools.c's own "string *REISSUED_TOOLS = ({ ... });". Evaluated
    // once per object instance, before create() runs (there is no
    // dedicated grammar-level "initializer" production in real LPC;
    // real compilers thread this into the object's own implicit
    // initialization sequence the same way this driver does -- see
    // CodeGen::generate()'s own synthesized "$objvarinit" function and
    // ObjectManager::runObjectVarInitializers()). Null when this
    // variable has no initializer, the overwhelmingly common case.
    AstPtr initializer;
};

struct Program : AstNode {
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    std::vector<std::unique_ptr<ObjectVarDecl>> objectVars;
    std::vector<std::string> inherits;
};

} // namespace lpcdriver
