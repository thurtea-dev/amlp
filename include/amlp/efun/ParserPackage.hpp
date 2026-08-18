#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace amlp {

class LpcObject;

// Real packages/parser.h's own token convention (confirmed directly
// against the vendored source, not from memory): a rule-string token is
// a single int -- positive values name one of six token *kinds* (OBJ/
// LIV/OBS/LVS/STR/WRD), optionally OR'd with modifier bits; values <= 0
// are a literal word, encoded as -(index_into_literals + 1) so literal
// index 0 (the common case) still round-trips through a nonzero int.
// Kept as named constants, not a C++ enum class, so the exact bitwise
// OR/AND real tokenize()/rule_string() do reads identically here.
namespace ParserToken {
constexpr int Error = 1;
constexpr int Str = 2;
constexpr int Wrd = 3;
constexpr int LivModifier = 8;
constexpr int VisOnlyModifier = 16;
constexpr int PluralModifier = 32;
constexpr int ChooseModifier = 64;
constexpr int ObjA = 4;                     // real OBJ_A_TOKEN
constexpr int LivA = ObjA | LivModifier;    // real LIV_A_TOKEN
constexpr int Obj = ObjA | VisOnlyModifier; // real OBJ_TOKEN
constexpr int Liv = LivA | VisOnlyModifier; // real LIV_TOKEN
constexpr int Obs = ObjA | PluralModifier;  // real OBS_TOKEN
constexpr int Lvs = LivA | PluralModifier;  // real LVS_TOKEN
} // namespace ParserToken

// real verb_node_t (packages/parser.h): one grammar rule registered
// under a verb. tokens is real verb_node_t::token[] without its real 0
// terminator (size() serves the same purpose here). lit[0]/lit[1] are
// the rule's first two literal-token indices (real verb_node_t::lit,
// -1 when absent) -- real parse_rules()'s own fast prefilter before
// attempting a full match, not consumed by anything in this slice yet,
// stored now so the sentence-matching slice needs no format change
// here. handler is weak_ptr, matching every other cross-object
// reference this driver stores outside its owner (LpcObject.hpp's own
// actions_[].owner, shadowedBy_/shadowing_, etc.) -- a rule registered
// by an object that later gets destructed simply becomes unreachable
// through it, it does not keep the object alive.
struct VerbRuleNode {
    std::vector<int> tokens;
    int lit[2] = {-1, -1};
    int weight = 0;
    std::weak_ptr<LpcObject> handler;
};

// real verb_t/verb_syn_t (packages/parser.h). A verb *name* can
// genuinely map to more than one of these at once in real code -- e.g.
// `parse_add_rule("push", "OBJ")` (a plain verb, real_name==match_name==
// "push") and, separately, `parse_add_synonym("push", "carry")` (a
// synonym entry, real_name=="push", match_name=="carry") can coexist
// under the identical name "push"; real `parse_sentence()`'s own verb
// lookup does not `break` after the first match, it tries every entry
// sharing that real_name as an independent interpretation (confirmed
// directly against parser.c's own verb-lookup loop). See ParserPackage's
// own class comment for why the registry below is keyed by name but
// holds a *list* of entries per name, not one.
struct VerbEntry {
    std::string realName;
    std::string matchName;
    bool isSynonym = false;
    std::string synonymOf;
    std::vector<VerbRuleNode> nodes; // newest-first, matching real add-at-head order
};

// The real parser package's rule-string tokenizer plus its verb/rule
// registry (packages/parser.c's static verbs[VERB_HASH_SIZE] hash
// table). Real code's own hash-bucket chaining is an implementation
// detail with no observable LPC-visible contract on its own (see .cpp
// comments for the specific real behaviors that ARE ported faithfully,
// off-by-one quirks included) -- but one real, observable consequence of
// it is preserved here: a single verb name can carry more than one
// VerbEntry at once (see VerbEntry's own comment), so this is a map from
// name to a *list* of entries, not one entry per name. Global,
// process-wide static state, the same shape this codebase's other
// efun-package registries already use for exactly this reason
// (object/LivingNameRegistry.hpp: real parser.c's own verbs[] is one
// table shared by the whole game, not per-object or per-VM state).
class ParserPackage {
public:
    // real tokenize()/make_rule() (packages/parser.c), ported together
    // since make_rule() is just tokenize() called in a loop with
    // bookkeeping around it. Throws LpcRuntimeError on a malformed rule
    // -- unknown token name, an unrecognized modifier letter, a
    // modifier on STR/WRD, more than two object tokens, more than one
    // plural token, or more than 10 tokens total (real MAX_MATCHES,
    // including its own real off-by-one -- see the .cpp definition).
    // `literals` is the master's own real parse_command_prepos_list()
    // word list (see .cpp for why real APPLY_LITERALS is not what its
    // name suggests).
    static std::vector<int> tokenizeRule(const std::string& rule, const std::vector<std::string>& literals,
                                          int& weightOut);

    // real rule_string() (packages/parser.c): the exact inverse of
    // tokenizeRule() above (modulo the real CHOOSE_MODIFIER bit, which
    // real code also masks out before printing -- it never changes
    // which of the six token names is shown).
    static std::string ruleString(const std::vector<int>& tokens, const std::vector<std::string>& literals);

    // real f_parse_add_rule() (packages/parser.c), minus the parts that
    // only matter once sentence matching exists (see .cpp comment):
    // finds or creates verb's own real_name==match_name==verb entry,
    // tokenizes rule, computes lit[0]/lit[1] the same way real code
    // does, and prepends a new VerbRuleNode. Also caches `literals`
    // for later use by dump()'s own ruleString() calls, mirroring real
    // parser.c's own process-wide `literals[]` global. Throws on a
    // malformed rule (propagated straight from tokenizeRule()).
    static void addRule(const std::string& verb, const std::string& rule,
                         const std::shared_ptr<LpcObject>& handler, const std::vector<std::string>& literals);

    // real f_parse_add_synonym() (packages/parser.c), both its real
    // forms:
    // - 2-arg (`rule` empty): pure verb aliasing. `newVerb` becomes
    //   another name that resolves to `oldVerb`'s own already-registered
    //   rule set (a fresh or reused VerbEntry with isSynonym=true,
    //   synonymOf==oldVerb) -- no new rule nodes, nothing copied.
    // - 3-arg (`rule` non-empty): copies ONE already-registered rule
    //   node from `oldVerb` (matched by exact tokenized-rule equality)
    //   onto a fresh or reused *non*-synonym VerbEntry named `newVerb`.
    //   Requires that the matched node's own handler is exactly
    //   `caller` (real "Rule owned by different object." check) --
    //   `caller` must be the same object identity `addRule()` was
    //   originally called with for that exact rule, not merely any
    //   caller with `hasParseInfo()` set.
    // Both forms require `oldVerb` to already name a real, non-synonym
    // verb (real "%s is not a verb!" -- see .cpp for why this driver
    // has no equivalent of real code's own separate, shared-string-
    // interning-specific null check for an unregistered name, only the
    // one real lookup-failure check that actually matters here) and
    // reject `newVerb == oldVerb` (real "Verb cannot be a synonym for
    // itself."). Throws LpcRuntimeError on any of these, or on a
    // malformed `rule` (propagated from tokenizeRule()).
    static void addSynonym(const std::string& newVerb, const std::string& oldVerb, const std::string& rule,
                            const std::shared_ptr<LpcObject>& caller, const std::vector<std::string>& literals);

    // real f_parse_remove() (packages/parser.c): unlinks every rule
    // node under verb whose handler is exactly `handler` (real
    // "(*vn)->handler == current_object"). Does not touch synonym
    // entries -- real code's own attempt to do so there is accidental
    // struct-reinterpretation (verb_syn_t has no `node` field at the
    // offset real code reads as one), not a real, checkable behavior
    // this driver could faithfully port; harmless to skip, since no
    // real non-buggy outcome depends on it. A verb with no matching
    // entry, or no matching handler, is a silent no-op, matching real
    // code's own behavior either way.
    static void removeRules(const std::string& verb, const std::shared_ptr<LpcObject>& handler);

    // real f_parse_dump() (packages/parser.c). Iterates in alphabetical
    // verb-name order rather than real code's own hash-bucket order --
    // a deliberate difference: real order is an unspecified artifact of
    // DO_HASH()'s own bit mixing, never documented or relied on by any
    // real call site (parse_dump() is a debug/introspection tool), so a
    // well-defined order is a strict improvement, not a fidelity loss.
    // A destructed handler prints "(destructed)" in place of real
    // code's own "(/obname)" -- real code has no equivalent case (real
    // parse_free(), called from free_object(), unlinks a destructed
    // handler's own rules immediately; this slice does not port that
    // cleanup yet, see .cpp comment), so this is this slice's own
    // honest way of surfacing a rule whose handler is gone rather than
    // silently hiding it or dereferencing a dangling pointer.
    static std::string dump();

private:
    static std::unordered_map<std::string, std::vector<VerbEntry>>& verbs();
};

} // namespace amlp
