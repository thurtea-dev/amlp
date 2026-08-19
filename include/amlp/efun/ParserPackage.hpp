#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "amlp/vm/Value.hpp"

namespace amlp {

class LpcObject;
class VM;

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

// real parse_info_t's own PI_* flag bits (packages/parser.h). Stored
// directly on LpcObject (LpcObject::parseInfoFlags(), valid only while
// LpcObject::hasParseInfo() is true -- see that method's own comment)
// rather than in a separate struct real parse_info_t would be, since
// this driver's object model already has one canonical per-object state
// holder and nothing here needs pinfo's own separate allocation
// lifetime. PI_LIVING/PI_INV_ACCESSIBLE/PI_INV_VISIBLE are cached
// results of the is_living()/inventory_accessible()/inventory_visible()
// applies (real interrogate_object(), packages/parser.c) -- not
// produced or read by anything in this slice, listed here only so the
// noun-phrase-resolution slice that does need them has the real bit
// values on hand already, not rediscovered from parser.h again.
namespace ParserInfoFlag {
constexpr int Setup = 1;         // real PI_SETUP
constexpr int Living = 2;        // real PI_LIVING (not yet produced/read -- see comment above)
constexpr int VerbHandler = 4;   // real PI_VERB_HANDLER
constexpr int RemoteLivings = 8; // real PI_REMOTE_LIVINGS
constexpr int InvAccessible = 16; // real PI_INV_ACCESSIBLE (not yet produced/read)
constexpr int InvVisible = 32;    // real PI_INV_VISIBLE (not yet produced/read)
constexpr int Refresh = 64;      // real PI_REFRESH
} // namespace ParserInfoFlag

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

    // real VB_HAS_OBJ, computed per-rule rather than per-verb (real
    // code ORs it onto the *verb's* own flags, used only to gate
    // load_objects() -- packages/parser.c's own f_parse_add_rule()).
    // Stored here instead because this driver's own sentence-matching
    // slice needs a per-*node* decision, not per-verb: used by
    // ParserPackage::parseSentence()'s own lazy load_objects() trigger
    // -- true if this node has ANY object-family token, 1 or 2.
    bool hasObjectToken = false;

    // How many OBJ/LIV/OBS/LVS tokens this rule has (0, 1, or 2 --
    // tokenizeRule() already rejects 3 or more). ROADMAP.md row 0.13a
    // item 8 piece 5 (parse_obj(), ParserPackage.cpp) implements real
    // single-object resolution (objectTokenCount <= 1) in full: the
    // noun-phrase word-matching itself (articles/all/my/ordinals/
    // adjective chains/singular-plural) plus the per-candidate can_/
    // direct_/indirect_/do_ disambiguation family for ONE object
    // (singular_check_functions()/plural_check_functions()). The real
    // TWO-object ambiguity family (dependent_check_functions()/
    // check_object_relations(), packages/parser.c:2184-2493 -- deciding
    // which direct/indirect object PAIR is jointly valid when a rule
    // like "give OBJ to LIV" has two independent candidate sets) is a
    // genuinely separate, larger piece of real code needing its own
    // slice, not attempted here -- ParserPackage::parseRulesFor() skips
    // any node with objectTokenCount >= 2 entirely, the same honest
    // "not yet supported, not silently wrong" stance already used
    // elsewhere in this row (m_values()'s own non-zero-column
    // rejection).
    int objectTokenCount = 0;
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

// real parser_error_t's own error_type values (include/parser_error.h,
// confirmed against the real vendored header directly). Since item 8
// piece 5 (parse_obj(), single-object resolution) landed, every kind
// except ManyPaths is reachable: IsNot/NotLiving/NotAccessible/Ambig/
// Ordinal/BadMultiple/ThereIsNo all come from parse_obj() itself or the
// per-candidate can_/direct_/indirect_/do_ disambiguation family
// (singular_check_functions()/plural_check_functions()); Allocated is a
// can_/direct_/... callback's own explicit string/falsy-int rejection.
// ManyPaths is exclusively produced by check_object_relations() (the
// two-object ambiguity family, VerbRuleNode::objectTokenCount's own
// comment) -- still unreachable, since a node with two object tokens
// never reaches sentence matching at all.
namespace ParserErrorType {
constexpr int None = 0;
constexpr int IsNot = 1;        // ERR_IS_NOT
constexpr int NotLiving = 2;    // ERR_NOT_LIVING
constexpr int NotAccessible = 3; // ERR_NOT_ACCESSIBLE
constexpr int Ambig = 4;        // ERR_AMBIG
constexpr int Ordinal = 5;      // ERR_ORDINAL
constexpr int Allocated = 6;    // ERR_ALLOCATED -- a can_/direct_/... callback's own explicit string return
constexpr int ThereIsNo = 7;    // ERR_THERE_IS_NO
constexpr int BadMultiple = 8;  // ERR_BAD_MULTIPLE
constexpr int ManyPaths = 9;    // ERR_MANY_PATHS (not yet reachable -- two-object rules only)
} // namespace ParserErrorType

// real parser_error_t (packages/parser.h): a tagged union in real code
// (parser_error_u), the one payload shape this slice's own reachable
// error kind (Allocated) actually carries is a plain string. The
// per-call scratch types this needs (SentenceWord/SentenceMatch/
// ParserErrorInfo/SentenceMatchResult, real word_t/match_t/
// parser_error_t/parse_result_t) live entirely inside ParserPackage.cpp
// -- unlike VerbEntry/VerbRuleNode above, nothing outside one
// parseSentence() call ever needs to see them, so there is no reason
// for them to be part of this class's own public shape.

// real special_word_t's own SW_* kinds (packages/parser.h) -- the fixed
// article/self/all/of/and/ordinal word table real interrogate_master()'s
// MS_HAS_SPECIALS branch populates once and check_special_word() reads
// (ROADMAP.md row 0.13a item 8, piece 2). Real code guards this behind a
// master_state cache-once bit even though the table is hardcoded and
// never invalidated by anything (not even parse_refresh()) -- this port
// skips modeling that flag entirely (ParserPackage::checkSpecialWord()
// is a pure function over a static local table), a legitimate
// simplification with no observable difference, the same category as
// the verb registry's own hash-bucket-to-map collapse.
enum class SpecialWordKind { None, Article, Self, All, Of, And, Ordinal };
struct SpecialWordResult {
    SpecialWordKind kind = SpecialWordKind::None;
    long arg = 0; // real *arg: the ordinal number for SW_ORDINAL, unused otherwise
};

// real hash_entry_t (packages/parser.h): one word's own noun/plural/
// adjective membership across the current parse's numbered object
// universe. Real code's own fixed-size bitvec_t (a MAX_NUM_OBJECTS==1024
// -bit C array) is a pure sizing/perf detail with no LPC-visible
// contract -- represented here as a plain std::vector<bool> per real
// bitvec_t field, sized to LoadedObjectSet::objects, the same
// hash-bucket-chaining-to-map simplification already used for the verb
// registry (ParserPackage's own class comment).
struct HashEntry {
    bool isNoun = false;     // real HV_NOUN
    bool isPlural = false;   // real HV_PLURAL
    bool isAdj = false;      // real HV_ADJ
    bool isNickname = false; // real HV_NICKNAME (not yet set by anything -- see loadObjects()'s own comment)
    std::vector<bool> nounObjs;   // real pv.noun
    std::vector<bool> pluralObjs; // real pv.plural
    std::vector<bool> adjObjs;    // real pv.adj
};

// real loaded_objects[]/object_flags[]/cur_livings/cur_accessible/
// me_object/hash_table[] (packages/parser.c) -- the whole numbered
// object universe one load_objects() call builds, collected into one
// object constructed fresh per call and returned by value instead of
// real code's own process-wide statics. The same deliberate
// architecture improvement already made for parse_sentence()'s own
// SentenceSession (see that struct's own comment for why this is not a
// fidelity loss -- nothing here is reentrant-sensitive either).
struct LoadedObjectSet {
    std::vector<std::shared_ptr<LpcObject>> objects; // real loaded_objects[]
    std::vector<bool> inReach;      // real object_flags[i] & RAO_INREACH
    std::vector<bool> isLiving;     // real cur_livings (from each object's own PI_LIVING)
    std::vector<bool> isAccessible; // real cur_accessible (copied from inReach at add_to_hash_table() time -- see .cpp)
    int meObject = -1;              // real me_object: index of parseUser itself, or -1
    std::unordered_map<std::string, HashEntry> hashTable; // real hash_table[HASH_SIZE], keyed by word text

    // real num_people: how many of the trailing entries in `objects`
    // were appended by loadObjects()'s own final "num_people" fallback
    // loop (a connected user visible-but-not-reachable through
    // parseUser's own environment tree). Needed by ParserPackage's own
    // real all_objects() (item 8 piece 5, ParserPackage.cpp): those
    // trailing entries are excluded from a rule's default OBJ/LIV/OBS/
    // LVS candidate universe unless the rule's own registering object
    // has PI_REMOTE_LIVINGS set (real "all_objects(&objects,
    // parse_vn->handler->pinfo->flags & PI_REMOTE_LIVINGS)").
    size_t numPeople = 0;
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
    // A handler weak_ptr that fails to .lock() at all -- the object was
    // reference-counted away without ever going through
    // destructObject()/reloadObject() (nothing else calls
    // onObjectDestroyed() below, real code's own free_object() has no
    // equivalent path this driver lacks the same trigger for) -- still
    // prints "(destructed)" as a graceful fallback rather than silently
    // dropping the line or dereferencing a dangling pointer. Genuinely
    // real FluffOS objects cannot reach this state (their own
    // reference-counted free always runs parse_free() as part of the
    // same free_object() call), so this is a deliberate, narrow
    // difference from real behavior, not a gap in onObjectDestroyed()
    // itself -- see its own comment for the real, eager cleanup path
    // that keeps a properly-`destruct()`ed handler's rules from ever
    // reaching dump() at all once it runs.
    static std::string dump();

    // real parse_free() (packages/parser.c), called from free_object():
    // if `destructed` was ever PI_VERB_HANDLER (successfully called
    // parse_add_rule()/parse_add_synonym() at least once), unlinks every
    // rule node it owns from every verb entry in the registry, then
    // clears its own pinfo (matching real "FREE(pinfo);" -- once freed,
    // `destructed->hasParseInfo()` reads back false, the same as an
    // object real code's own free_object() actually deallocated).
    // real remove_ids(pinfo) is not ported here since nothing in this
    // slice populates the per-object noun/adj/plural id cache it frees
    // (see LpcObject::parseInfoFlags()'s own comment) -- vacuous, not
    // skipped. Wired into ObjectManager::destructObject()'s/
    // reloadObject()'s own onDestructed callback (EfunTable.cpp's
    // "destruct"/"reload_object" registrations), the exact same real
    // trigger point and the exact same "fires once per object either
    // call actually destructs, including every shadow-chain cascade
    // link" shape SocketRegistry::closeAllOwnedBy() already uses there.
    static void onObjectDestroyed(const std::shared_ptr<LpcObject>& destructed);

    // real parser.c's own process-wide `literals[]` global, as last
    // populated by addRule()/addSynonym() (see their own comments) --
    // exposed so parseSentence() below can reuse the exact same cache
    // for literal-token matching that rule registration already
    // maintains, matching real code's own single shared array.
    static const std::vector<std::string>& currentLiterals();

    // real check_special_word() (packages/parser.c): looks `word` up in
    // the fixed article/self/all/of/and/ordinal table, falling back to
    // real code's own numeric-ordinal parsing ("3rd", "21st", ... --
    // including its own "a teen is always 'th'" rule: a two-or-more
    // digit number whose second-to-last digit is '1' always takes "th",
    // regardless of its last digit) before returning SpecialWordKind::
    // None. Not yet called by anything in this driver (only the
    // noun-phrase matcher itself, item 8 piece 5, reads this) -- real
    // and independently testable now, same reasoning as tokenizeRule()'s
    // own standalone testability before parse_add_rule() existed to
    // call it.
    static SpecialWordResult checkSpecialWord(const std::string& word);

    // real interrogate_object() (packages/parser.c), ROADMAP.md row
    // 0.13a item 8 piece 1: populates `ob`'s own real
    // LpcObject::parseNounIds()/parsePluralIds()/parseAdjIds() (the
    // parse_command_id_list()/parse_command_plural_id_list()/
    // parse_command_adjectiv_id_list() applies) and the real PI_LIVING/
    // PI_INV_ACCESSIBLE/PI_INV_VISIBLE flags (is_living()/
    // inventory_accessible()/inventory_visible()), in the exact real
    // order and with the exact real early-return-on-destruct checkpoint
    // after each apply. A no-op cache hit if `ob` already has
    // ParserInfoFlag::Setup and not ParserInfoFlag::Refresh (real
    // "if (pinfo->flags & PI_SETUP && !(pinfo->flags & PI_REFRESH))
    // return;"). Throws nothing -- `ob` must already have
    // hasParseInfo() true (real code dereferences ob->pinfo
    // unconditionally; this is the caller's responsibility, matching
    // every other pinfo-gated real function in this class).
    static void interrogateObject(VM& vm, const std::shared_ptr<LpcObject>& ob);

    // real "master_state &= ~MS_HAS_USERS" (f_parse_refresh()'s own
    // master_ob special case, packages/parser.c) -- invalidates the
    // cached master()->users() result loadObjects() below otherwise
    // reuses across separate calls (real interrogate_master()'s own
    // MS_HAS_USERS caching, ROADMAP.md row 0.13a item 8 piece 2, kept
    // as real process-wide state rather than folded into the fresh-
    // per-call LoadedObjectSet: unlike SentenceSession's own state,
    // this cache's real lifetime genuinely spans multiple separate
    // parse_sentence() calls until something explicitly invalidates it,
    // a real observable contract mudlib code can depend on, not an
    // internal-only implementation detail safe to simplify away). Wired
    // into the parse_refresh efun's own master-object branch,
    // EfunTable.cpp.
    static void invalidateMasterUsersCache();

    // real load_objects() (packages/parser.c), ROADMAP.md row 0.13a
    // item 8 pieces 2 (the USERS half)+3+4: builds the full numbered
    // object universe for one parse, either from `envArray` (real
    // `parse_env`, when non-null -- add_objects_from_array()/
    // get_objects_from_array()) or from `parseUser`'s own environment
    // (the default path -- rec_add_object()/find_uninited_objects()),
    // interrogates every newly-discovered object (piece 1, above) plus
    // every stale-cached master()->users() entry (init_users()), then
    // walks the whole master user list once more to add any visible-
    // but-not-already-reachable user (real load_objects()'s own final
    // "num_people" loop), and finally builds the word -> object-index
    // hash table (piece 4, add_to_hash_table()). Real add_nicknames()
    // (a caller-supplied object -> nickname mapping) is not ported here
    // -- nothing in this driver's own parseSentence() signature
    // consumes a `nicks` argument yet either (see that method's own
    // comment), so there is nothing real for it to be exercised with;
    // HashEntry::isNickname stays false for every entry this method
    // produces. Called lazily from parseSentence() itself now (real
    // "if (!objects_loaded && (parse_verb_entry->flags & VB_HAS_OBJ))
    // load_objects();"), the first time a matched verb has at least one
    // rule with an object-family token -- real load_objects() has no
    // efun surface of its own either (it is exclusively parse_sentence()'s
    // own internal pipeline), consumed by ParserPackage::parseObj() (item
    // 8 piece 5, ROADMAP.md row 0.13a).
    static LoadedObjectSet loadObjects(VM& vm, const std::shared_ptr<LpcObject>& parseUser,
                                        const Value* envArray = nullptr);

    // real f_parse_sentence() (packages/parser.c). Real signature is
    // `mixed parse_sentence(string, void|int, void|object*,
    // void|mapping)`; this driver only implements the required first
    // argument -- `debugFlag`, if explicitly truthy, throws the exact
    // real "Parser debugging not enabled" error real code's own
    // `#else` branch throws when compiled without `-DDEBUG`/
    // `-DPARSE_DEBUG` (this driver has no such tracing at all, so that
    // branch is always the real one taken); `env`/`nicks` are accepted
    // for real signature compatibility but never consulted (both are
    // exclusively read by loadObjects(), which this driver always calls
    // with parseUser's own environment, never an explicit envArray).
    // OBJ/LIV/OBS/LVS tokens now resolve real objects (item 8 piece 5,
    // parse_obj(), plus the single-object slice of item 9's own
    // can_/direct_/indirect_/do_ disambiguation family --
    // singular_check_functions()/plural_check_functions()) for any rule
    // with at most one object-family token; a rule needing TWO
    // (VerbRuleNode::objectTokenCount's own comment -- the real
    // dependent_check_functions()/check_object_relations() ambiguity
    // family) is skipped by parseRulesFor(), the same honest "not yet
    // supported" stance already used elsewhere in this row. Returns int
    // 1 on a successful match (after invoking the winning do_* function,
    // real do_the_call()); otherwise a real, negative "how close did we
    // get" signal (`-foundLevel`, see .cpp) or, if some candidate's own
    // can_/direct_/... callback rejected explicitly (a falsy return or
    // an explicit string), whatever master()->parser_error_message()
    // returns for the resulting error (real int 0 if master does not
    // define that apply, matching real code's own "apply_master_ob()
    // returned null" -> "*sp = const0" fallback).
    static Value parseSentence(VM& vm, const std::shared_ptr<LpcObject>& caller, const std::string& sentence,
                                bool debugFlag);

private:
    static std::unordered_map<std::string, std::vector<VerbEntry>>& verbs();
};

} // namespace amlp
