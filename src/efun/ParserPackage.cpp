#include "amlp/efun/ParserPackage.hpp"
#include "amlp/core/Errors.hpp"
#include "amlp/object/LpcObject.hpp"
#include "amlp/vm/VM.hpp"
#include <algorithm>
#include <cctype>
#include <optional>

namespace amlp {

namespace {

struct TokenDef {
    const char* name;
    int token;
    bool modLegal;
};

// real "token_def_t tokens[]" (packages/parser.c). mod_legal matches
// real code exactly: only the four object-family tokens accept an
// "OBJ:lvpc"-style modifier suffix, STR/WRD never do.
constexpr TokenDef kTokenDefs[] = {
    {"OBJ", ParserToken::ObjA, true}, {"STR", ParserToken::Str, false}, {"WRD", ParserToken::Wrd, false},
    {"LIV", ParserToken::LivA, true}, {"OBS", ParserToken::Obs, true},  {"LVS", ParserToken::Lvs, true},
};

// real tokenize() (packages/parser.c). Returns std::nullopt at the end
// of the rule string (real tokenize()'s own "return 0" for "at the
// end"), advancing `pos` past whatever token was read. Throws for
// everything real tokenize() itself calls error() for.
std::optional<int> tokenizeOne(const std::string& rule, size_t& pos, const std::vector<std::string>& literals,
                                int& weight) {
    while (pos < rule.size() && rule[pos] == ' ') pos++;
    if (pos >= rule.size()) return std::nullopt;

    size_t start = pos;
    size_t sp = rule.find(' ', pos);
    size_t wordEnd = (sp == std::string::npos) ? rule.size() : sp;
    size_t wlen = wordEnd - start;
    pos = wordEnd;

    // real "if (n == 3 || (n > 4 && start[3] == ':'))" -- a bare 3-letter
    // token name, or a 3-letter name followed by ':' and at least one
    // modifier letter.
    if (wlen == 3 || (wlen > 4 && rule[start + 3] == ':')) {
        for (const auto& td : kTokenDefs) {
            if (rule.compare(start, 3, td.name) != 0) continue;
            int i = td.token;
            if (wlen != 3) {
                if (!td.modLegal) {
                    throw LpcRuntimeError(std::string("parser rule: illegal to have modifiers to '") + td.name +
                                           "'");
                }
                for (size_t k = start + 4; k < wordEnd; k++) {
                    switch (rule[k]) {
                        case 'l':
                            i |= ParserToken::LivModifier;
                            break;
                        case 'v':
                            i |= ParserToken::VisOnlyModifier;
                            break;
                        case 'p':
                            i |= ParserToken::PluralModifier;
                            break;
                        case 'c':
                            i |= ParserToken::ChooseModifier;
                            break;
                        default:
                            throw LpcRuntimeError(std::string("parser rule: unknown modifier '") + rule[k] + "'");
                    }
                }
            }
            // real weight bump: "switch(i) { default: /* some kind of
            // object */ weight += 2; ...; case STR_TOKEN: case
            // WRD_TOKEN: weight++; }" -- STR/WRD carry no modifiers, so
            // checking the two fixed values directly reproduces the
            // same switch exactly.
            if (i == ParserToken::Str || i == ParserToken::Wrd) {
                weight += 1;
            } else {
                weight += 2;
                if (i & ParserToken::PluralModifier) weight -= 1;
                if (i & ParserToken::LivModifier) weight += 1;
                if (!(i & ParserToken::VisOnlyModifier)) weight += 1;
            }
            return i;
        }
    }

    // real "(*weightp)++; /* must be a literal */" plus the literals[]
    // linear scan.
    weight += 1;
    std::string word = rule.substr(start, wlen);
    for (size_t li = 0; li < literals.size(); li++) {
        if (literals[li].size() == wlen && literals[li].compare(0, wlen, word) == 0) {
            return -(static_cast<int>(li) + 1);
        }
    }
    std::string shown = word.size() > 50 ? word.substr(0, 50) + "..." : word;
    throw LpcRuntimeError("parser rule: unknown token '" + shown + "'");
}

// Real parser.c's own process-wide `literals[]` global, populated by
// interrogate_master() (via f_parse_add_rule()) and read back later by
// rule_string() (via f_parse_dump()) with no re-fetch in between.
// Mirrored here the same way -- see ParserPackage::addRule()'s own
// comment for what this driver's own slice deliberately does not port
// from real interrogate_master() (the USERS/specials halves, both
// purely about sentence matching, not rule registration).
std::vector<std::string>& cachedLiterals() {
    static std::vector<std::string> literals;
    return literals;
}

} // namespace

std::vector<int> ParserPackage::tokenizeRule(const std::string& rule, const std::vector<std::string>& literals,
                                              int& weightOut) {
    std::vector<int> result;
    weightOut = 1;
    size_t pos = 0;
    int hasObj = 0;
    bool hasPlural = false;

    // real make_rule()'s own "while (idx < MAX_MATCHES) { ...; idx++; }
    // error(...)" loop, including its real off-by-one: a rule that uses
    // exactly all 10 slots always falls through to the "only 10 tokens
    // permitted" error below, even when the 10th token genuinely was
    // the last one in the string -- real code's loop-exit condition is
    // checked BEFORE it ever gets a chance to notice the input ran out
    // on that final token (the "no more input" check only happens on
    // the NEXT attempted token, a call this loop shape never reaches
    // once the count hits 10). Confirmed directly from source, not
    // silently "fixed": real mudlib rules are always far shorter than
    // 10 tokens (this driver's own corpus survey, STATUS.md's
    // 2026-08-18 entry, found nothing longer than 3), so this is
    // dead-letter in practice, faithfully kept rather than quietly
    // behaving differently than the real driver does.
    int idx = 0;
    while (idx < 10) {
        auto tok = tokenizeOne(rule, pos, literals, weightOut);
        if (!tok) return result;
        if (*tok >= ParserToken::ObjA) {
            if (++hasObj == 3) {
                throw LpcRuntimeError("parser rule: only two object tokens allowed per rule");
            }
            if (*tok & ParserToken::PluralModifier) {
                if (hasPlural) throw LpcRuntimeError("parser rule: only one plural token allowed per rule");
                hasPlural = true;
            }
        }
        result.push_back(*tok);
        idx++;
    }
    throw LpcRuntimeError("parser rule: only 10 tokens permitted per rule");
}

std::string ParserPackage::ruleString(const std::vector<int>& tokens, const std::vector<std::string>& literals) {
    std::string out;
    for (int rawTok : tokens) {
        int tok = rawTok & ~ParserToken::ChooseModifier;
        switch (tok) {
            case ParserToken::ObjA:
            case ParserToken::Obj:
                out += "OBJ ";
                break;
            case ParserToken::LivA:
            case ParserToken::Liv:
                out += "LIV ";
                break;
            case ParserToken::Obs:
            case ParserToken::Obs | ParserToken::VisOnlyModifier:
                out += "OBS ";
                break;
            case ParserToken::Lvs:
            case ParserToken::Lvs | ParserToken::VisOnlyModifier:
                out += "LVS ";
                break;
            case ParserToken::Str:
                out += "STR ";
                break;
            case ParserToken::Wrd:
                out += "WRD ";
                break;
            default:
                // real code: "default: p = strput(p, end,
                // literals[-(tok + 1)]);" -- `tok` there is the switch
                // expression's own side-effecting assignment ("switch
                // ((tok = vn->token[index++]) & ~CHOOSE_MODIFIER)"),
                // which still holds the RAW, unmasked token value in
                // C's own comma/assignment semantics; only the switch's
                // own selector went through the mask. Using the masked
                // `tok` here instead of `rawTok` would corrupt every
                // negative (literal) value: -1 & ~CHOOSE_MODIFIER is
                // -65, not -1, since a negative int's high bits are
                // already all 1s in two's complement. rawTok is the
                // correct real equivalent.
                if (rawTok <= 0) {
                    size_t li = static_cast<size_t>(-(rawTok + 1));
                    if (li < literals.size()) out += literals[li];
                    out += " ";
                }
                break;
        }
    }
    if (!out.empty()) out.pop_back(); // real rule_string(): "*(p-1) = 0; /* nuke last space */"
    return out;
}

std::unordered_map<std::string, std::vector<VerbEntry>>& ParserPackage::verbs() {
    static std::unordered_map<std::string, std::vector<VerbEntry>> table;
    return table;
}

namespace {
// real "if (verb_entry->match_name == verb && verb_entry->real_name ==
// verb && !(verb_entry->flags & VB_IS_SYN)) break;" -- the one shape
// every real plain (non-synonym) verb entry has: real_name==match_name
// ==name, isSynonym==false. Used by addRule() (find-or-create) and by
// addSynonym()'s own "oldVerb must already be a real verb" lookup
// (real code's separate `vb` search uses the same condition minus the
// name argument being the same on both sides, which is automatically
// true for a plain entry).
VerbEntry* findPlainEntry(std::vector<VerbEntry>& entries, const std::string& name) {
    for (auto& e : entries) {
        if (e.realName == name && e.matchName == name && !e.isSynonym) return &e;
    }
    return nullptr;
}

void fillLitFromTokens(VerbRuleNode& node) {
    // real "for (i = 0, j = 0; tokens[i]; i++) if (tokens[i] <= 0 && j <
    // 2) lit[j++] = -(tokens[i]+1);" -- first two literal-token indices,
    // scanning every token, not stopping the scan once both are found.
    for (int t : node.tokens) {
        if (t > 0) continue;
        int litIndex = -(t + 1);
        if (node.lit[0] == -1) {
            node.lit[0] = litIndex;
        } else if (node.lit[1] == -1) {
            node.lit[1] = litIndex;
        }
    }
}

// See VerbRuleNode::hasObjectToken's own comment: real VB_HAS_OBJ,
// computed per-node here instead of per-verb.
bool computeHasObjectToken(const std::vector<int>& tokens) {
    for (int t : tokens) {
        if (t >= ParserToken::ObjA) return true;
    }
    return false;
}
} // namespace

void ParserPackage::addRule(const std::string& verb, const std::string& rule,
                             const std::shared_ptr<LpcObject>& handler, const std::vector<std::string>& literals) {
    cachedLiterals() = literals;

    int weight = 0;
    std::vector<int> tokens = tokenizeRule(rule, cachedLiterals(), weight);

    auto& entries = verbs()[verb];
    VerbEntry* target = findPlainEntry(entries, verb);
    if (!target) {
        VerbEntry entry;
        entry.realName = verb;
        entry.matchName = verb;
        entries.push_back(std::move(entry));
        target = &entries.back();
    }

    VerbRuleNode node;
    node.tokens = tokens;
    node.weight = weight;
    node.handler = handler;
    node.hasObjectToken = computeHasObjectToken(tokens);
    fillLitFromTokens(node);

    // real "verb_node->next = verb_entry->node; verb_entry->node =
    // verb_node;" -- prepend, newest first.
    target->nodes.insert(target->nodes.begin(), std::move(node));
}

void ParserPackage::addSynonym(const std::string& newVerb, const std::string& oldVerb, const std::string& rule,
                                const std::shared_ptr<LpcObject>& caller, const std::vector<std::string>& literals) {
    // real "if (old_verb == new_verb) error(\"Verb cannot be a synonym
    // for itself.\\n\");" -- a plain string-content comparison here;
    // real code's own version is a shared-string *pointer* comparison,
    // but shared strings guarantee unique interning, so the observable
    // condition (same text) is identical either way.
    if (newVerb == oldVerb) {
        throw LpcRuntimeError("parse_add_synonym: a verb cannot be a synonym for itself");
    }

    // real code's own `old_verb = SHARED_STRING(sp-1)` can come back
    // null purely from FluffOS's own global shared-string table never
    // having interned that exact text before (a real, but driver-
    // internal, quirk this driver has no equivalent of -- Value::string
    // is a plain std::string, no interning table at all), which real
    // code checks separately ("if (!old_verb) error(...)") before even
    // trying the verb lookup below. That separate check is redundant
    // with the lookup's own failure case for every real, observable
    // purpose (an unregistered name fails the same "is not a verb!"
    // error whichever check catches it first), so this driver only
    // needs the one lookup-failure check that actually matters here.
    // find(), not operator[]: a lookup miss must not leave a spurious
    // empty entry behind in the registry (operator[] would silently
    // default-construct one).
    VerbEntry* vb = nullptr;
    if (auto oldIt = verbs().find(oldVerb); oldIt != verbs().end()) {
        vb = findPlainEntry(oldIt->second, oldVerb);
    }
    if (!vb) {
        throw LpcRuntimeError("parse_add_synonym: '" + oldVerb + "' is not a verb");
    }

    auto& newEntries = verbs()[newVerb];
    bool wantSynonym = rule.empty();
    // real "if (verb_entry->real_name == new_verb && verb_entry->
    // match_name == old_verb) { if (rule) { if (!(flags & VB_IS_SYN))
    // break; } else { if (flags & VB_IS_SYN) break; } }" -- reuse an
    // existing entry only if it already has the right (rule-copy vs
    // alias) shape; otherwise a fresh one is created alongside it.
    VerbEntry* target = nullptr;
    for (auto& e : newEntries) {
        if (e.realName == newVerb && e.matchName == oldVerb && e.isSynonym == wantSynonym) {
            target = &e;
            break;
        }
    }
    if (!target) {
        VerbEntry entry;
        entry.realName = newVerb;
        // real code sets match_name to old_verb here unconditionally,
        // for BOTH forms -- even a freshly-created 3-arg rule-copy
        // entry's own match_name is old_verb, not new_verb, which is
        // why parse_dump() can print "Verb new_verb (old_verb):" for a
        // rule-copied verb, matching a real, if slightly surprising,
        // behavior (see VerbEntry's own class comment) rather than the
        // "real_name==match_name" shape a plain parse_add_rule()-created
        // entry always has.
        entry.matchName = oldVerb;
        newEntries.push_back(std::move(entry));
        target = &newEntries.back();
    }

    if (!rule.empty()) {
        cachedLiterals() = literals;
        int weight = 0;
        std::vector<int> tokens = tokenizeRule(rule, cachedLiterals(), weight);

        // real "for (vn = vb->node; vn; vn = vn->next) { for (i = 0;
        // tokens[i]; i++) if (vn->token[i] != tokens[i]) break; if
        // (!tokens[i] && !vn->token[i]) break; }" -- find the exact
        // rule (by full token-sequence equality) already registered
        // under oldVerb.
        VerbRuleNode* found = nullptr;
        for (auto& n : vb->nodes) {
            if (n.tokens == tokens) {
                found = &n;
                break;
            }
        }
        if (!found) {
            throw LpcRuntimeError("parse_add_synonym: no such rule defined under '" + oldVerb + "'");
        }
        auto foundHandler = found->handler.lock();
        if (foundHandler != caller) {
            throw LpcRuntimeError("parse_add_synonym: rule owned by a different object");
        }

        // real "memcpy(verb_node, vn, ...)" -- a full copy of the
        // matched node (same tokens/lit/weight/handler), prepended onto
        // the target entry, which stays non-synonym.
        VerbRuleNode copy = *found;
        target->nodes.insert(target->nodes.begin(), std::move(copy));
    } else {
        // real "syn->flags = VB_IS_SYN | ...; syn->real = vb;" -- pure
        // aliasing, no rule nodes of its own.
        target->isSynonym = true;
        target->synonymOf = vb->realName;
    }
}

void ParserPackage::removeRules(const std::string& verb, const std::shared_ptr<LpcObject>& handler) {
    auto it = verbs().find(verb);
    if (it == verbs().end()) return;
    // real f_parse_remove()'s own bucket walk only ever does real work
    // for entries where match_name==verb -- which, restricted to this
    // name's own entries, means exactly the non-synonym one(s) (a
    // synonym entry keyed by this name has match_name pointing at its
    // *target* verb instead, and has no `node` list of its own to walk
    // -- see this method's own header comment on real code's
    // accidental UB there, deliberately not replicated).
    for (auto& entry : it->second) {
        if (entry.isSynonym) continue;
        auto& nodes = entry.nodes;
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                                    [&](const VerbRuleNode& n) { return n.handler.lock() == handler; }),
                    nodes.end());
    }
}

std::string ParserPackage::dump() {
    auto& table = verbs();
    std::vector<std::string> names;
    names.reserve(table.size());
    for (const auto& [name, entries] : table) names.push_back(name);
    std::sort(names.begin(), names.end());

    std::string out;
    for (const auto& name : names) {
        for (const VerbEntry& v : table.at(name)) {
            if (v.realName == v.matchName) {
                out += "Verb " + v.realName + ":\n";
            } else {
                out += "Verb " + v.realName + " (" + v.matchName + "):\n";
            }
            if (v.isSynonym) {
                out += "  Synonym for: " + v.synonymOf + "\n";
                continue;
            }
            for (const auto& node : v.nodes) {
                auto handler = node.handler.lock();
                // This driver's own destruct() does not drop every
                // shared_ptr to a destructed object the moment it is
                // destructed (see LpcObject::isDestructed()'s own
                // comment: it keeps working as a plain C++ object until
                // the last reference actually goes away) -- so a
                // weak_ptr can still .lock() successfully here for an
                // object that is, for every real-LPC-visible purpose,
                // already gone. Treated the same as an expired
                // weak_ptr, matching the same "destructed reads back as
                // gone" convention this driver's own %O sprintf
                // formatter already applies to any object value
                // (EfunTable.cpp's own "if (!*ov ||
                // (*ov)->isDestructed()) return \"0\";").
                if (handler && handler->isDestructed()) handler.reset();
                // real "(/%s)" (f_parse_dump(): "vn->handler->obname"),
                // but this driver's own LpcObject::filename() already
                // stores the full leading-slash path for a genuinely
                // loaded/cloned object (confirmed against
                // ObjectManager::compile()'s own "config_.mudlibRoot()
                // + filename + \".c\"" concatenation, which requires
                // filename to already start with '/') -- not the bare,
                // slash-free obname real FluffOS keeps separately.
                // Prepending another '/' here would double it up, so
                // this uses filename() as-is.
                std::string handlerText = handler ? handler->filename() : "destructed";
                out += "  (" + handlerText + ") " + ruleString(node.tokens, cachedLiterals()) + "\n";
            }
        }
    }
    return out;
}

void ParserPackage::onObjectDestroyed(const std::shared_ptr<LpcObject>& destructed) {
    if (!destructed) return;
    // real "if (pinfo->flags & PI_VERB_HANDLER) { ... }" -- an object
    // that never successfully registered a rule (parse_add_rule()/
    // parse_add_synonym() always set this flag on success, see their
    // own EfunTable.cpp registrations) has nothing to unlink; skip the
    // full registry walk entirely, matching real code's own guard.
    if (!destructed->hasParseInfo() || !(destructed->parseInfoFlags() & ParserInfoFlag::VerbHandler)) {
        destructed->setHasParseInfo(false);
        destructed->setParseInfoFlags(0);
        return;
    }

    // real "for (i = 0; i < VERB_HASH_SIZE; i++) { verb_t *v = verbs[i];
    // while (v) { ... unlink every node whose handler == pinfo->ob ...
    // v = v->next; } }" -- every verb entry, every name, not scoped to
    // one verb the way removeRules() (parse_remove()'s own real
    // equivalent) is. Synonym entries have no nodes of their own in
    // this driver's representation (see VerbRuleNode's own comment), so
    // iterating them here is harmless, matching real code's own
    // accidental-but-inert behavior on a verb_syn_t there.
    for (auto& [name, entries] : verbs()) {
        for (auto& entry : entries) {
            auto& nodes = entry.nodes;
            nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                                        [&](const VerbRuleNode& n) { return n.handler.lock() == destructed; }),
                        nodes.end());
        }
    }

    // real remove_ids(pinfo) is a no-op here (nothing in this slice
    // populates the noun/adj/plural id cache it would free -- see
    // LpcObject::parseInfoFlags()'s own comment); real "FREE(pinfo);"
    // is this.
    destructed->setHasParseInfo(false);
    destructed->setParseInfoFlags(0);
}

const std::vector<std::string>& ParserPackage::currentLiterals() { return cachedLiterals(); }

// ============================================================================
// Noun-phrase-to-object resolution engine, pieces 1-4 (ROADMAP.md row
// 0.13a item 8): the real per-object noun/adjective/plural cache
// (interrogate_object()), the rest of interrogate_master() (the USERS/
// SPECIALS halves), environment-based object collection
// (rec_add_object()/find_uninited_objects()/add_objects_from_array()/
// get_objects_from_array()), and the word -> object-index hash table
// (add_hash_entry()/add_to_hash_table()) -- together, real load_objects()
// in full. Not yet called by parseSentence() itself: item 8 piece 5
// (parse_obj(), the real word-matching logic that would actually consume
// LoadedObjectSet) is the next slice. See LoadedObjectSet's own header
// comment for why this is a real, complete, independently testable
// pipeline on its own rather than a partial stand-in.
// ============================================================================

namespace {

constexpr size_t kMaxNumObjects = 1024; // real MAX_NUM_OBJECTS (packages/parser.h)
constexpr int kRaoInReach = 1;          // real RAO_INREACH
constexpr int kRaoMy = 2;               // real RAO_MY

// real parse_copy_array(): copies only the T_STRING elements of `v` (if
// it is even an array at all), silently skipping and compacting past
// any non-string element -- not every noun()/plural()/adjective() lfun
// in a real mudlib is guaranteed well-behaved.
std::vector<std::string> arrayOfStringsFrom(const Value& v) {
    std::vector<std::string> out;
    auto* arrPtr = std::get_if<std::shared_ptr<Array>>(&v.data);
    if (!arrPtr || !*arrPtr) return out;
    for (const Value& item : (*arrPtr)->items) {
        if (auto* s = std::get_if<std::string>(&item.data)) out.push_back(*s);
    }
    return out;
}

// real NEED_REFRESH(ob): "(ob->pinfo && ((ob->pinfo->flags &
// (PI_SETUP|PI_REFRESH)) != PI_SETUP))" -- note the real short-circuit:
// an object that never called parse_init() at all (no pinfo) is never
// "in need of refresh" by this macro's own definition, which is exactly
// why real rec_add_object()/add_objects_from_array() also gate on
// ob->pinfo separately before ever adding an object to the numbered
// universe at all -- an object invisible to the parser stays invisible
// to it, it is never silently auto-registered.
bool needRefresh(const std::shared_ptr<LpcObject>& ob) {
    if (!ob->hasParseInfo()) return false;
    int flags = ob->parseInfoFlags();
    return (flags & (ParserInfoFlag::Setup | ParserInfoFlag::Refresh)) != ParserInfoFlag::Setup;
}

// real find_uninited_objects() (packages/parser.c): the default (no
// explicit parse_env) discovery-pass environment walk, item 8 piece 3.
void findUninitedObjects(std::vector<std::shared_ptr<LpcObject>>& discovered, const std::shared_ptr<LpcObject>& ob) {
    if (!ob || ob->isDestructed()) return;
    if (needRefresh(ob)) {
        if (discovered.size() >= kMaxNumObjects) return;
        discovered.push_back(ob);
    }
    for (auto& child : ob->inventory()) findUninitedObjects(discovered, child);
}

// real get_objects_from_array() (packages/parser.c): the explicit-
// parse_env discovery-pass equivalent of findUninitedObjects() above,
// item 8 piece 3.
void getObjectsFromArray(std::vector<std::shared_ptr<LpcObject>>& discovered, const std::vector<Value>& arr) {
    for (const Value& item : arr) {
        if (auto* nestedPtr = std::get_if<std::shared_ptr<Array>>(&item.data); nestedPtr && *nestedPtr) {
            getObjectsFromArray(discovered, (*nestedPtr)->items);
        }
        auto* obPtr = std::get_if<std::shared_ptr<LpcObject>>(&item.data);
        if (!obPtr || !*obPtr || (*obPtr)->isDestructed()) continue;
        if (needRefresh(*obPtr)) {
            if (discovered.size() >= kMaxNumObjects) return;
            discovered.push_back(*obPtr);
        }
    }
}

// real init_users() (packages/parser.c): the discovery-pass half of
// interrogate_master()'s own USERS handling, item 8 piece 2 -- any
// master-user-list object that already has pinfo and needs refreshing.
void initUsersDiscovery(std::vector<std::shared_ptr<LpcObject>>& discovered, const std::vector<Value>& users) {
    for (const Value& item : users) {
        auto* obPtr = std::get_if<std::shared_ptr<LpcObject>>(&item.data);
        if (!obPtr || !*obPtr || !(*obPtr)->hasParseInfo()) continue;
        if (needRefresh(*obPtr)) {
            if (discovered.size() >= kMaxNumObjects) return;
            discovered.push_back(*obPtr);
        }
    }
}

// real rec_add_object() (packages/parser.c): the default (no explicit
// parse_env) object-index-assignment pass, item 8 piece 3. Walks
// `ob->inventory()` directly rather than reimplementing real code's own
// first_inv()/next_inv() pointer-chase -- this driver's own inventory_
// vector already preserves the same real traversal order, the same
// intrusive-list-to-container simplification already used elsewhere in
// this class (e.g. the verb registry's own hash-bucket-to-map collapse).
void recAddObject(LoadedObjectSet& result, std::vector<bool>& myObjects, const std::shared_ptr<LpcObject>& parseUser,
                   const std::shared_ptr<LpcObject>& ob, int flags) {
    if (!ob || ob->isDestructed()) return;
    if (ob->hasParseInfo()) {
        if (result.objects.size() >= kMaxNumObjects) return;
        size_t index = result.objects.size();
        if (flags & kRaoMy) {
            if (myObjects.size() <= index) myObjects.resize(index + 1);
            myObjects[index] = true;
        }
        if (ob == parseUser) {
            result.meObject = static_cast<int>(index);
            flags |= kRaoMy;
        }
        result.inReach.push_back((flags & kRaoInReach) != 0);
        result.objects.push_back(ob);
        int obFlags = ob->parseInfoFlags();
        if (!(obFlags & ParserInfoFlag::InvVisible)) return;
        if (!(obFlags & ParserInfoFlag::InvAccessible)) flags &= ~kRaoInReach;
    }
    for (auto& child : ob->inventory()) recAddObject(result, myObjects, parseUser, child, flags);
}

// real add_objects_from_array() (packages/parser.c): the explicit-
// parse_env object-index-assignment pass, item 8 piece 3. Unlike
// recAddObject() above, a nested array is only descended into when the
// immediately preceding (non-array) element's own PI_INV_VISIBLE flag
// was set -- real code's own caller-supplied "object, then its own
// contents as a nested array" convention -- and reaching parseUser here
// does not propagate RAO_MY onto that same element's own recorded
// flags, only onto a nested array immediately following it (real
// last_was_me, confirmed a genuine, deliberate asymmetry with
// rec_add_object() above, not a porting slip).
void addObjectsFromArray(LoadedObjectSet& result, std::vector<bool>& myObjects,
                          const std::shared_ptr<LpcObject>& parseUser, const std::vector<Value>& arr, int flags) {
    int lastFlags = 0;
    bool lastWasMe = false;
    for (const Value& item : arr) {
        if (auto* nestedPtr = std::get_if<std::shared_ptr<Array>>(&item.data); nestedPtr && *nestedPtr) {
            if (lastFlags & ParserInfoFlag::InvVisible) {
                int f = flags;
                if (!(lastFlags & ParserInfoFlag::InvAccessible)) f &= ~kRaoInReach;
                if (lastWasMe) f |= kRaoMy;
                addObjectsFromArray(result, myObjects, parseUser, (*nestedPtr)->items, f);
            }
        }
        lastFlags = 0;
        lastWasMe = false;
        auto* obPtr = std::get_if<std::shared_ptr<LpcObject>>(&item.data);
        if (!obPtr || !*obPtr || (*obPtr)->isDestructed()) continue;
        const auto& ob = *obPtr;
        if (!ob->hasParseInfo()) continue;
        if (result.objects.size() >= kMaxNumObjects) return;
        size_t index = result.objects.size();
        if (flags & kRaoMy) {
            if (myObjects.size() <= index) myObjects.resize(index + 1);
            myObjects[index] = true;
        }
        if (ob == parseUser) {
            result.meObject = static_cast<int>(index);
            lastWasMe = true;
        }
        result.inReach.push_back((flags & kRaoInReach) != 0);
        result.objects.push_back(ob);
        lastFlags = ob->parseInfoFlags();
    }
}

// real add_hash_entry() (packages/parser.c), item 8 piece 4: find or
// create `word`'s own HashEntry, its three bool vectors pre-sized to
// `objectCount` -- safe because, exactly like real code's own
// add_to_hash_table() (the only real caller of add_hash_entry() outside
// mark_hash_entry(), which this driver does not port -- see
// LoadedObjectSet::hashTable's own header comment on real add_nicknames()
// not being ported either), this only ever runs once the numbered
// object universe has already stopped growing.
HashEntry& addHashEntry(LoadedObjectSet& result, const std::string& word, size_t objectCount) {
    auto [it, inserted] = result.hashTable.try_emplace(word);
    if (inserted) {
        it->second.nounObjs.assign(objectCount, false);
        it->second.pluralObjs.assign(objectCount, false);
        it->second.adjObjs.assign(objectCount, false);
    }
    return it->second;
}

// real add_to_hash_table() (packages/parser.c), item 8 piece 4: folds
// object index `index`'s own cached noun/plural/adjective ids (piece 1)
// into the shared word hash table.
void addToHashTable(LoadedObjectSet& result, const std::shared_ptr<LpcObject>& ob, size_t index) {
    if (!ob->hasParseInfo()) return; // real "if (!pi) return;"
    for (const auto& id : ob->parseNounIds()) {
        HashEntry& he = addHashEntry(result, id, result.objects.size());
        he.isNoun = true;
        he.nounObjs[index] = true;
    }
    for (const auto& pl : ob->parsePluralIds()) {
        HashEntry& he = addHashEntry(result, pl, result.objects.size());
        he.isPlural = true;
        he.pluralObjs[index] = true;
    }
    for (const auto& adj : ob->parseAdjIds()) {
        HashEntry& he = addHashEntry(result, adj, result.objects.size());
        he.isAdj = true;
        he.adjObjs[index] = true;
    }
}

// real master_user_list (packages/parser.c) -- cached the same real way
// literals[] already is (cachedLiterals(), above). See
// ParserPackage::invalidateMasterUsersCache()'s own header comment
// (ParserPackage.hpp) for why this stays real process-wide state,
// invalidated only by an explicit parse_refresh() on master, rather than
// folding into LoadedObjectSet the way SentenceSession folded
// parse_sentence()'s own globals.
struct MasterUsersCache {
    bool valid = false;
    std::vector<Value> users; // real master_user_list's own item array, unfiltered
};
MasterUsersCache& masterUsersCache() {
    static MasterUsersCache cache;
    return cache;
}

// real interrogate_master()'s MS_HAS_USERS branch (packages/parser.c),
// item 8 piece 2.
const std::vector<Value>& fetchMasterUsers(VM& vm) {
    auto& cache = masterUsersCache();
    if (!cache.valid) {
        cache.users.clear();
        if (auto master = vm.masterObject()) {
            Value ret = vm.callFunction(master, "parse_command_users", {});
            if (auto* arrPtr = std::get_if<std::shared_ptr<Array>>(&ret.data); arrPtr && *arrPtr) {
                cache.users = (*arrPtr)->items;
            }
        }
        cache.valid = true;
    }
    return cache.users;
}

} // namespace

SpecialWordResult ParserPackage::checkSpecialWord(const std::string& word) {
    // real special_table[] (packages/parser.c's MS_HAS_SPECIALS branch)
    // -- see this method's own header comment (ParserPackage.hpp) for
    // why this is a plain static table rather than a cached/invalidated
    // one the way literals[]/master_user_list are.
    static const std::unordered_map<std::string, SpecialWordResult> kTable = {
        {"the", {SpecialWordKind::Article, 0}},
        {"me", {SpecialWordKind::Self, 0}},
        {"myself", {SpecialWordKind::Self, 0}},
        {"all", {SpecialWordKind::All, 0}},
        {"of", {SpecialWordKind::Of, 0}},
        {"and", {SpecialWordKind::And, 0}},
        {"a", {SpecialWordKind::Ordinal, 1}},
        {"an", {SpecialWordKind::Ordinal, 1}},
        {"any", {SpecialWordKind::Ordinal, 1}},
        {"first", {SpecialWordKind::Ordinal, 1}},
        {"second", {SpecialWordKind::Ordinal, 2}},
        {"other", {SpecialWordKind::Ordinal, 2}},
        {"third", {SpecialWordKind::Ordinal, 3}},
        {"fourth", {SpecialWordKind::Ordinal, 4}},
        {"fifth", {SpecialWordKind::Ordinal, 5}},
        {"sixth", {SpecialWordKind::Ordinal, 6}},
        {"seventh", {SpecialWordKind::Ordinal, 7}},
        {"eighth", {SpecialWordKind::Ordinal, 8}},
        {"ninth", {SpecialWordKind::Ordinal, 9}},
    };
    if (auto it = kTable.find(word); it != kTable.end()) return it->second;

    // real check_special_word()'s own numeric-ordinal fallback ("3rd",
    // "21st", ...): digits followed by exactly the right suffix for
    // that number, with real code's own "a teen is always 'th'" rule --
    // if the digit two before the end is '1', the suffix is always
    // "th" regardless of the last digit.
    if (!word.empty() && std::isdigit(static_cast<unsigned char>(word[0]))) {
        size_t digits = 0;
        while (digits < word.size() && std::isdigit(static_cast<unsigned char>(word[digits]))) digits++;
        if (digits > 0 && digits < word.size()) {
            long n = std::stol(word.substr(0, digits));
            std::string suffix = word.substr(digits);
            std::string ending = "th";
            if (!(digits >= 2 && word[digits - 2] == '1')) {
                switch (word[digits - 1]) {
                    case '1':
                        ending = "st";
                        break;
                    case '2':
                        ending = "nd";
                        break;
                    case '3':
                        ending = "rd";
                        break;
                }
            }
            if (suffix == ending) return SpecialWordResult{SpecialWordKind::Ordinal, n};
        }
    }
    return SpecialWordResult{};
}

void ParserPackage::interrogateObject(VM& vm, const std::shared_ptr<LpcObject>& ob) {
    // real "if (ob->pinfo->flags & PI_REFRESH) remove_ids(ob->pinfo);" --
    // a deliberate no-op here: C++ vectors self-manage the memory real
    // remove_ids() exists to free, and real remove_ids()'s own guard
    // ("if (pinfo->flags & PI_SETUP)") is provably always false at this
    // exact point in real code anyway -- real f_parse_refresh() clears
    // PI_SETUP in the same bitwise AND that sets PI_REFRESH ("pi->flags
    // &= PI_VERB_HANDLER; pi->flags |= PI_REFRESH;"), so by the time
    // interrogate_object() ever observes PI_REFRESH set, PI_SETUP is
    // already unset -- meaning real remove_ids() never actually frees
    // anything here either, confirmed directly from source, not assumed.
    int flags = ob->parseInfoFlags();
    if ((flags & ParserInfoFlag::Setup) && !(flags & ParserInfoFlag::Refresh)) {
        return; // real cache hit: "if (pinfo->flags & PI_SETUP && !(pinfo->flags & PI_REFRESH)) return;"
    }

    ob->setParseNounIds(arrayOfStringsFrom(vm.callFunction(ob, "parse_command_id_list", {})));
    if (ob->isDestructed()) return;

    // real "/* in case of an error */ ob->pinfo->flags |= PI_SETUP;
    // ob->pinfo->flags &= ~(PI_LIVING|PI_INV_ACCESSIBLE|PI_INV_VISIBLE);"
    // -- set right after the first apply succeeds, not at the end of
    // this function, so an object destructed by a later apply below
    // still ends up marked SETUP (avoiding it looking permanently
    // "not yet interrogated" to NEED_REFRESH() from here on).
    flags = ob->parseInfoFlags() | ParserInfoFlag::Setup;
    flags &= ~(ParserInfoFlag::Living | ParserInfoFlag::InvAccessible | ParserInfoFlag::InvVisible);
    ob->setParseInfoFlags(flags);

    ob->setParsePluralIds(arrayOfStringsFrom(vm.callFunction(ob, "parse_command_plural_id_list", {})));
    if (ob->isDestructed()) return;

    ob->setParseAdjIds(arrayOfStringsFrom(vm.callFunction(ob, "parse_command_adjectiv_id_list", {})));
    if (ob->isDestructed()) return;

    if (isTruthy(vm.callFunction(ob, "is_living", {}))) {
        ob->setParseInfoFlags(ob->parseInfoFlags() | ParserInfoFlag::Living);
    }
    if (ob->isDestructed()) return;

    if (isTruthy(vm.callFunction(ob, "inventory_accessible", {}))) {
        ob->setParseInfoFlags(ob->parseInfoFlags() | ParserInfoFlag::InvAccessible);
    }
    if (ob->isDestructed()) return;

    if (isTruthy(vm.callFunction(ob, "inventory_visible", {}))) {
        ob->setParseInfoFlags(ob->parseInfoFlags() | ParserInfoFlag::InvVisible);
    }
}

void ParserPackage::invalidateMasterUsersCache() { masterUsersCache().valid = false; }

LoadedObjectSet ParserPackage::loadObjects(VM& vm, const std::shared_ptr<LpcObject>& parseUser,
                                            const Value* envArray) {
    LoadedObjectSet result;

    // Step 1 (real load_objects()'s own comment: LPC code run during
    // interrogation can move objects, so the environment must not be
    // walked live while walking it for object-index-assignment
    // purposes -- discover first, interrogate, only then assign
    // indices in a second, separate pass below).
    std::vector<std::shared_ptr<LpcObject>> discovered;
    const std::vector<Value>* envItems = nullptr;
    if (envArray) {
        if (auto* arrPtr = std::get_if<std::shared_ptr<Array>>(&envArray->data); arrPtr && *arrPtr) {
            envItems = &(*arrPtr)->items;
        }
        if (envItems) getObjectsFromArray(discovered, *envItems);
    } else {
        if (!parseUser || parseUser->isDestructed()) {
            throw LpcRuntimeError("parse_sentence: no this_player() to parse from");
        }
        findUninitedObjects(discovered, parseUser->environment().lock());
    }

    const std::vector<Value>& masterUsers = fetchMasterUsers(vm);
    initUsersDiscovery(discovered, masterUsers);

    // Step 2: interrogate every discovered object (piece 1).
    for (auto& ob : discovered) interrogateObject(vm, ob);

    // Step 3: the real object-index-assignment pass.
    std::vector<bool> myObjects;
    if (envItems) {
        addObjectsFromArray(result, myObjects, parseUser, *envItems, kRaoInReach);
    } else {
        recAddObject(result, myObjects, parseUser, parseUser->environment().lock(), kRaoInReach);
    }

    // real "he = add_hash_entry(my_string); he->flags |= HV_ADJ;
    // bitvec_copy(&he->pv.adj, &my_objects);" -- the fixed "my"
    // adjective, covering exactly the objects the RAO_MY walk above
    // marked. Real add_nicknames(parse_nicks) is not ported here --
    // nothing in this driver's own parseSentence() signature consumes a
    // `nicks` argument yet either (see that method's own comment), so
    // there is nothing real for it to be exercised with.
    {
        HashEntry& myEntry = addHashEntry(result, "my", result.objects.size());
        myEntry.isAdj = true;
        for (size_t i = 0; i < myObjects.size() && i < result.objects.size(); i++) {
            if (myObjects[i]) myEntry.adjObjs[i] = true;
        }
    }

    // real load_objects()'s own final "num_people" loop, unconditional
    // regardless of whether an explicit parse_env was given (confirmed
    // directly -- real code has no "if (!parse_env)" guard around it,
    // and parse_user is always current_object for parse_sentence(),
    // never influenced by parse_env either -- see f_parse_sentence(),
    // packages/parser.c:3070, "parse_user = current_object;"
    // unconditionally): any master-user-list object not already
    // reachable within parseUser's own environment tree, provided their
    // own environment chain never crosses an inventory_visible()==false
    // link before (if ever) reaching parseUser's own environment.
    if (parseUser && !parseUser->isDestructed()) {
        auto parseUserEnv = parseUser->environment().lock();
        for (const Value& item : masterUsers) {
            auto* obPtr = std::get_if<std::shared_ptr<LpcObject>>(&item.data);
            if (!obPtr || !*obPtr || !(*obPtr)->hasParseInfo()) continue;
            const auto& ob = *obPtr;

            std::shared_ptr<LpcObject> env = ob;
            bool reachedParseUserEnv = false;
            while (env) {
                if (env == parseUserEnv) {
                    reachedParseUserEnv = true;
                    break;
                }
                env = env->environment().lock();
                if (env && env->hasParseInfo() && !(env->parseInfoFlags() & ParserInfoFlag::InvVisible)) {
                    env = nullptr;
                }
            }
            if (reachedParseUserEnv) continue;
            if (result.objects.size() >= kMaxNumObjects) break;
            result.inReach.push_back(true); // real "object_flags[num_objects + num_people] = 1;"
            result.objects.push_back(ob);
        }
    }

    // Step 4 (piece 4): cur_livings/cur_accessible plus the word hash
    // table, all built in the same final index-ordered pass real
    // load_objects()'s own "for (i...) add_to_hash_table(...)" loop is.
    result.isLiving.assign(result.objects.size(), false);
    result.isAccessible.assign(result.objects.size(), false);
    for (size_t i = 0; i < result.objects.size(); i++) {
        addToHashTable(result, result.objects[i], i);
        if (result.objects[i]->parseInfoFlags() & ParserInfoFlag::Living) result.isLiving[i] = true;
        if (result.inReach[i]) result.isAccessible[i] = true;
    }

    return result;
}

// ============================================================================
// Sentence matching (real f_parse_sentence() and everything it calls:
// parse_sentence()/parse_recurse() (the sentence tokenizer, ROADMAP.md
// row 0.13a item 6), parse_rules()/parse_rule()/we_are_finished() (the
// recursive-descent matcher, item 9), check_functions()/process_answer()/
// make_function()/push_real_names()/do_the_call() (the can_/direct_/
// indirect_/do_ callback machinery, also item 9), and
// make_error_message()/get_the_error() (error reporting, item 10)).
//
// Restricted to STR/WRD/literal-only rules -- no OBJ/LIV/OBS/LVS support
// yet. Confirmed directly from source before writing any of this that
// this really is a separable, real, complete slice: interrogate_object()
// (the function that actually populates a live object's noun/adjective/
// plural id cache) has exactly one real call site, load_objects(),
// itself only reachable from parse_obj(), itself only reached by
// parse_rule()'s own OBJ-family token case -- a rule using only STR/WRD/
// literal tokens never touches any of that machinery at all, real code
// included, so this is a genuine subset of real behavior, not a
// simplification of it. VerbRuleNode::hasObjectToken marks the rules
// this slice cannot yet attempt; parseRulesFor() below skips them
// entirely rather than attempting and silently mismatching them.
// ============================================================================

namespace {

// real word_t (packages/parser.h). See the header's own note on why
// this stays private to this file. `rawStart`/`rawEnd` are byte offsets
// into the original (pre-lowercasing) input, spanning exactly the kept
// characters this word is made of -- provably equivalent to real code's
// own less precise `start`/`end` pointers once real strput_words()'s
// own leading/trailing-whitespace trim is accounted for (real code
// trims down to the same boundary this driver computes directly).
struct SentenceWord {
    std::string text;
    size_t rawStart = 0;
    size_t rawEnd = 0;
};

// real match_t (packages/parser.h). The object-token payload (val.obs/
// val.number/ordinal) is not included -- nothing in this slice ever
// produces an OBJ/LIV/OBS/LVS match (VerbRuleNode::hasObjectToken's own
// comment). Extend, don't replace, when object matching lands.
struct SentenceMatch {
    int token = 0;
    int first = 0;
    int last = 0;
};

// real parser_error_t, reduced to the one payload shape (a plain
// string) this slice's own reachable error kind (Allocated) carries.
struct ParserErrorInfo {
    int errorType = ParserErrorType::None;
    std::string allocatedMessage;
};

// real parse_result_t -- one already-resolved winning match, cached by
// we_are_finished() and invoked later by do_the_call(). See
// ParserPackage.hpp's own removed comment (now here, since this type
// moved): all four res[i] use the real "do_" prefix; the index selects
// one of four real naming *strategies* for the same call (real
// make_function()'s own "try" parameter).
struct SentenceMatchResult {
    std::weak_ptr<LpcObject> handler;
    struct FunctionCall {
        std::string functionName;
        std::vector<Value> args;
    };
    FunctionCall res[4];
};

// real parse_state_t (packages/parser.h): the per-recursion-branch state
// real parse_rule() threads through as a pointer, copied by value
// ("local_state = *state;") at every point real code starts a genuinely
// new branch. Passed by reference here for the same "mutate in place,
// tail-recurse" steps real code's own pointer aliasing achieves; a local
// `MatchState` copy is made explicitly, matching real code exactly,
// wherever real code copies `*state` into a `local_state`.
struct MatchState {
    int tokIndex = 0;
    int wordIndex = 0;
    int numMatches = 0;
    int numErrors = 0;
};

// real parser.c's own single shared set of "current parse in progress"
// globals (num_words/words[]/matches[]/parse_vn/parse_verb_entry/
// best_match/best_error_match/best_num_errors/found_level/
// current_error_info/best_error_info/best_result/parse_user), collected
// into one object constructed fresh per parseSentence() call and
// threaded through by reference instead. A deliberate, real behavior
// improvement over real code's own global mutable statics -- real
// parse_sentence()'s own recursion guard is explicitly disabled ("may
// not be done in case of an error, or in case of tail recursion", real
// code's own comment), meaning a genuinely reentrant call in real
// FluffOS can silently corrupt an outer parse still in progress; this
// shape cannot do that, since each call gets its own session with no
// shared mutable global at all. Not a fidelity loss: nothing any real
// caller depends on requires the corruption itself, only each call's
// own correct result, and single-call behavior is identical either way.
struct SentenceSession {
    std::string rawInput;
    std::shared_ptr<LpcObject> caller; // real parse_user

    std::vector<SentenceWord> words;
    std::vector<SentenceMatch> matches; // real matches[], reused/overwritten via the same add_match() high-water-mark technique

    const VerbEntry* matchedVerb = nullptr; // real parse_verb_entry
    const VerbRuleNode* currentNode = nullptr; // real parse_vn

    int bestMatchWeight = 0;   // real best_match
    int bestErrorWeight = 0;   // real best_error_match
    int bestNumErrors = 5732;  // real best_num_errors -- "Yes. Exactly 5,732 errors. Don't ask." (reset_error()'s own real comment, kept verbatim)
    int foundLevel = 0;        // real found_level

    ParserErrorInfo currentError; // real current_error_info
    ParserErrorInfo bestError;    // real best_error_info

    std::optional<SentenceMatchResult> bestResult; // real best_result
};

// real isignore(x) = (!uisprint(x) || x == '\'').
bool isIgnorableChar(unsigned char c) { return !std::isprint(c) || c == '\''; }
// real iskeep(x) = uisalnum(x) || x == '*'.
bool isKeepChar(unsigned char c) { return std::isalnum(c) || c == '*'; }

// real parse_sentence()'s own word-splitting front end (packages/
// parser.c) -- item 6, the sentence tokenizer. Ported to byte offsets
// into a std::string rather than raw char* pointer arithmetic; see
// SentenceWord's own comment for why the resulting rawStart/rawEnd are
// provably equivalent to real code's own less-precise pointers. Real
// MAX_WORD_LENGTH/MAX_WORDS_PER_LINE truncation (pure C buffer-safety
// limits) is not ported -- no realistic input reaches them, and this
// driver's own containers have no equivalent fixed-size hazard to guard
// against.
std::vector<SentenceWord> splitSentenceWords(const std::string& input) {
    std::vector<SentenceWord> words;
    size_t n = input.size();
    size_t i = 0;

    std::string current;
    size_t wordRawStart = 0;
    size_t wordRawEnd = 0;
    bool haveWord = false;

    auto finishWord = [&]() {
        if (haveWord) {
            words.push_back(SentenceWord{current, wordRawStart, wordRawEnd});
            current.clear();
            haveWord = false;
        }
    };

    while (i < n) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (isIgnorableChar(c)) {
            i++;
            continue;
        }
        if (isKeepChar(c)) {
            unsigned char lower = std::isupper(c) ? static_cast<unsigned char>(std::tolower(c)) : c;
            if (!haveWord) {
                haveWord = true;
                wordRawStart = i;
            }
            current += static_cast<char>(lower);
            wordRawEnd = i;
            i++;
            continue;
        }
        // separator: whitespace, or a run of other punctuation -- real
        // code's own "!iskeep(*inp) && !uisspace(*inp)" skip-run does
        // not special-case isignore() chars within it either, so
        // neither does this.
        finishWord();
        if (std::isspace(c)) {
            i++;
            while (i < n && std::isspace(static_cast<unsigned char>(input[i]))) i++;
        } else {
            while (i < n) {
                unsigned char cc = static_cast<unsigned char>(input[i]);
                if (isKeepChar(cc) || std::isspace(cc)) break;
                i++;
            }
        }
    }
    finishWord();
    return words;
}

// real strput_words() (packages/parser.c): the ORIGINAL-cased,
// original-punctuated text spanning word[first..last] inclusive,
// trimmed of leading/trailing whitespace -- used for a STR/WRD token's
// own matched text, deliberately distinct from the lowercased/stripped
// form used for grammar matching itself. Everything *between*
// session.words[first] and session.words[last] (including any
// punctuation the grammar-matching pass silently ignored) is preserved
// verbatim, since this operates on `session.rawInput` directly rather
// than reassembling from the individual (already-normalized) word
// strings.
std::string extractWordRange(const SentenceSession& session, int first, int last) {
    if (first < 0 || last < first || last >= static_cast<int>(session.words.size())) return "";
    size_t start = session.words[first].rawStart;
    size_t end = session.words[last].rawEnd;
    const std::string& raw = session.rawInput;
    while (start <= end && start < raw.size() && std::isspace(static_cast<unsigned char>(raw[start]))) start++;
    while (end > start && end < raw.size() && std::isspace(static_cast<unsigned char>(raw[end]))) end--;
    if (start > end || start >= raw.size()) return "";
    return raw.substr(start, end - start + 1);
}

int tokenAt(const VerbRuleNode& node, int index) {
    return index >= 0 && index < static_cast<int>(node.tokens.size()) ? node.tokens[index] : 0;
}

// real add_match() (packages/parser.c): appends to (or, past the first
// pass through this depth, overwrites) session.matches at
// state.numMatches, the same shared-array-reused-across-sibling-
// branches technique real code's own fixed `matches[]` array uses, via
// a vector instead. Real code's own "if (token == ERROR_TOKEN)
// state->num_errors++;" is folded in here too.
void addMatch(SentenceSession& session, MatchState& state, int token, int first, int last) {
    if (state.numMatches < static_cast<int>(session.matches.size())) {
        session.matches[state.numMatches] = SentenceMatch{token, first, last};
    } else {
        session.matches.push_back(SentenceMatch{token, first, last});
    }
    if (token == ParserToken::Error) state.numErrors++;
    state.numMatches++;
}

// real check_literal() (packages/parser.c): real parse_rules()'s own
// fast prefilter, confirming literal `literals[litIndex]` appears
// somewhere at or after word index `start`. Returns the word index
// right after the match (so a second prefilter literal must appear
// strictly later), or 0 if not found.
int checkLiteral(const SentenceSession& session, int litIndex, int start) {
    const auto& literals = ParserPackage::currentLiterals();
    if (litIndex < 0 || static_cast<size_t>(litIndex) >= literals.size()) return 0;
    const std::string& want = literals[litIndex];
    for (int i = start; i < static_cast<int>(session.words.size()); i++) {
        if (session.words[i].text == want) return i + 1;
    }
    return 0;
}

void weAreFinished(VM& vm, SentenceSession& session, MatchState& state);

// real parse_rule() (packages/parser.c), restricted to the token kinds
// this slice actually supports -- STR_TOKEN, WRD_TOKEN, literal words,
// and the rule terminator. real code's own OBJ/LIV/OBS/LVS case is not
// reached: parseRulesFor() below never attempts a node with
// hasObjectToken set, so `session.currentNode->tokens` never actually
// contains one here. Kept as an explicit, documented dead branch rather
// than silently absent, so the noun-phrase-resolution slice has an
// obvious, exact insertion point.
void parseRule(VM& vm, SentenceSession& session, MatchState& state) {
    const VerbRuleNode& node = *session.currentNode;
    const int numWords = static_cast<int>(session.words.size());

    while (true) {
        int tok = tokenAt(node, state.tokIndex++);
        if (state.wordIndex == numWords && tok) {
            return; // real "Ran out of words to parse."
        }
        int masked = tok & ~ParserToken::ChooseModifier;

        if (masked == 0) {
            if (state.wordIndex == numWords) weAreFinished(vm, session, state);
            return;
        }

        if (masked >= ParserToken::ObjA) {
            // Unreachable -- see this function's own header comment.
            return;
        }

        if (masked == ParserToken::Str) {
            if (tokenAt(node, state.tokIndex) == 0) {
                // real "At end; match must be the whole thing."
                int start = state.wordIndex;
                state.wordIndex = numWords;
                addMatch(session, state, ParserToken::Str, start, state.wordIndex - 1);
                parseRule(vm, session, state);
            } else {
                int start = state.wordIndex++;
                while (state.wordIndex <= numWords) {
                    MatchState local = state;
                    addMatch(session, local, ParserToken::Str, start, state.wordIndex - 1);
                    parseRule(vm, session, local);
                    state.wordIndex++;
                }
            }
            return;
        }

        if (masked == ParserToken::Wrd) {
            addMatch(session, state, ParserToken::Wrd, state.wordIndex, state.wordIndex);
            state.wordIndex++;
            parseRule(vm, session, state);
            return;
        }

        // literal (tok <= 0)
        int litIndex = -(tok + 1);
        const auto& literals = ParserPackage::currentLiterals();
        bool matched = litIndex >= 0 && static_cast<size_t>(litIndex) < literals.size() &&
                       state.wordIndex < numWords && session.words[state.wordIndex].text == literals[litIndex];
        if (matched) {
            state.wordIndex++;
            continue;
        }
        // Mismatch. Real code's own recovery (a forward search that
        // marks the *previous* match as an error) only ever fires when
        // the immediately preceding token was itself an object-family
        // one -- unreachable here for the same reason the OBJ branch
        // above is. A preceding STR token, a preceding WRD/literal
        // token, or this being the rule's very first token (real
        // "state->tok_index == 1") all just return without recording
        // an error either way in real code, which is exactly what
        // falling out of this loop here does.
        return;
    }
}

// real make_error_message() (packages/parser.c), restricted to the
// STR/WRD/literal branches this slice reaches -- the object-token
// branch (`query_the_short()`) is unreachable, same reasoning as
// parseRule()'s own OBJ case.
void makeErrorMessage(SentenceSession& session, int which, ParserErrorInfo& err) {
    std::string buf = "You can't ";
    buf += session.words[0].text;
    buf += ' ';

    int cnt = 0;
    int match = 0;
    for (int tok : session.currentNode->tokens) {
        if (tok == ParserToken::Str || tok == ParserToken::Wrd) {
            if (tok == ParserToken::Str && cnt == which - 1) {
                buf += "that ";
                cnt++;
                continue;
            }
            buf += extractWordRange(session, session.matches[match].first, session.matches[match].last);
            buf += ' ';
            cnt++;
            match++;
        } else if (tok <= 0) {
            const auto& literals = ParserPackage::currentLiterals();
            int litIndex = -(tok + 1);
            if (litIndex >= 0 && static_cast<size_t>(litIndex) < literals.size()) buf += literals[litIndex];
            buf += ' ';
        }
        // tok >= ObjA (object-family): unreachable, see this
        // function's own header comment.
    }
    if (!buf.empty()) buf.pop_back(); // real "p--;" -- nuke the trailing space
    buf += ".\n";

    err.errorType = ParserErrorType::Allocated;
    err.allocatedMessage = buf;
}

// real process_answer() (packages/parser.c). `wasDefined` replaces real
// code's own "!sv" (real apply() returns a null svalue_t* specifically
// for an undefined function; this driver instead checks
// VM::functionExists() up front at the call site, and passes the
// result here explicitly, rather than trying to infer it from the
// returned Value -- see the monostate handling below for why that
// distinction has to be made before the call, not after).
// Returns: 1 accept, 0 undefined/wrong-type (try next), -2 generated
// error (an explicit falsy int), -1 generated error (an explicit
// string), -3 abort (already have an equally good candidate).
int processAnswer(SentenceSession& session, MatchState& state, bool wasDefined, const Value& result, int which) {
    if (!wasDefined) return 0;
    if (auto* s = std::get_if<std::string>(&result.data)) {
        if (state.numErrors == session.bestNumErrors) return -3;
        if (state.numErrors++ == 0) {
            session.currentError.errorType = ParserErrorType::Allocated;
            session.currentError.allocatedMessage = *s;
        }
        return -1;
    }
    // real "if (sv->type == T_NUMBER) { if (sv->u.number) return 1;
    // ... }" -- this driver's own monostate ("void") is folded into the
    // same falsy-zero branch here: a genuine LPC function with no
    // explicit `return` statement implicitly returns int 0 in real
    // semantics, but this driver's own Return opcode represents that
    // exact case as monostate instead of a real int64_t 0 (VM.cpp's own
    // "if (localStack.empty()) return Value{};") -- a pre-existing,
    // driver-wide representation choice unrelated to this efun, not
    // something to work around by treating a no-return can_/direct_/
    // do_ function as if it were undefined (which `wasDefined` above
    // already correctly rules out for a genuinely undefined name).
    // Every other real "not a number, not a string" result (an object/
    // array/mapping/closure) still falls through to the final "return
    // 0" below, matching real code's own "if (sv->type != T_STRING)
    // return 0;" exactly.
    if (std::holds_alternative<std::monostate>(result.data)) {
        if (state.numErrors == session.bestNumErrors) return -3;
        if (state.numErrors++ == 0) makeErrorMessage(session, which, session.currentError);
        return -2;
    }
    if (auto* n = std::get_if<int64_t>(&result.data)) {
        if (*n != 0) return 1;
        if (state.numErrors == session.bestNumErrors) return -3;
        if (state.numErrors++ == 0) makeErrorMessage(session, which, session.currentError);
        return -2;
    }
    return 0;
}

// real prefixes[] (packages/parser.c).
const char* const kPrefixes[] = {"can_", "direct_", "indirect_", "do_", "direct_", "indirect_"};

struct BuiltFunction {
    std::string functionName;
    std::vector<Value> args;
};

// real make_function() (packages/parser.c), restricted to the STR/WRD/
// literal branches this slice reaches -- the OBJ-family branch
// (push_object()/push_bitvec_as_array()/direct_object/indirect_object)
// is unreachable, same reasoning as parseRule()'s own OBJ case. Real
// code's own `state`/`target` parameters are unused inside the real
// function body itself (confirmed by inspection, not an oversight
// here) except for `target`'s one real OBJ-branch use, so neither is
// threaded through this port at all.
BuiltFunction makeFunction(SentenceSession& session, int which, int tryIdx) {
    BuiltFunction result;
    std::string name = kPrefixes[which];

    if (tryIdx < 2) {
        name += session.matchedVerb->matchName;
    } else {
        name += "verb";
        result.args.push_back(Value(session.matchedVerb->matchName));
    }

    // real "if (try == 3) { buf = strput(buf, end, \"_rule\"); buf++;
    // ... }" -- the name is truncated to exactly "<prefix>verb_rule"
    // for tryIdx == 3; real code's own token loop keeps running after
    // this (still pushing arguments), it just writes past the
    // truncating null, which no caller ever reads back. This port
    // reproduces the observable effect directly: stop appending to
    // `name` from here on, but keep processing every token's own
    // argument-pushing side effect exactly as real code does.
    bool nameTruncated = (tryIdx == 3);
    if (tryIdx == 3) {
        name += "_rule";
        result.args.push_back(Value(ParserPackage::ruleString(session.currentNode->tokens, ParserPackage::currentLiterals())));
    }

    int match = 0;
    const auto& literals = ParserPackage::currentLiterals();
    for (int tok : session.currentNode->tokens) {
        if (!nameTruncated) name += '_';

        int masked = tok & ~ParserToken::ChooseModifier;
        if (masked >= ParserToken::ObjA) {
            // Unreachable -- see this function's own header comment.
            match++;
            continue;
        }
        if (masked == ParserToken::Str) {
            if (!nameTruncated) name += "str";
            result.args.push_back(Value(extractWordRange(session, session.matches[match].first, session.matches[match].last)));
            match++;
            continue;
        }
        if (masked == ParserToken::Wrd) {
            if (!nameTruncated) name += "wrd";
            result.args.push_back(Value(extractWordRange(session, session.matches[match].first, session.matches[match].last)));
            match++;
            continue;
        }
        // literal
        int litIndex = -(tok + 1);
        std::string litText = (litIndex >= 0 && static_cast<size_t>(litIndex) < literals.size()) ? literals[litIndex] : "";
        if (tryIdx == 0) {
            if (!nameTruncated) name += litText;
        } else if (tryIdx < 3) {
            if (!nameTruncated) name += "word";
            result.args.push_back(Value(litText));
        }
        // tryIdx == 3: real code contributes neither a name segment
        // nor an argument for a literal token.
    }

    result.functionName = name;
    return result;
}

// real push_real_names() (packages/parser.c).
std::vector<Value> pushRealNames(SentenceSession& session, int tryIdx) {
    std::vector<Value> args;
    if (tryIdx >= 2) {
        args.push_back(Value(extractWordRange(session, 0, 0)));
    }
    int match = 0;
    for (int tok : session.currentNode->tokens) {
        if (tok > 0) {
            args.push_back(Value(extractWordRange(session, session.matches[match].first, session.matches[match].last)));
            match++;
        }
    }
    return args;
}

// real check_functions() (packages/parser.c): probes can_ (real
// which == 0) under all four real naming strategies, first against
// `player`, then (tryIdx 4-7, i.e. tryIdx % 4 again) against the rule's
// own registering object (session.currentNode->handler) if the first
// four attempts found nothing.
bool checkFunctions(VM& vm, SentenceSession& session, MatchState& state, const std::shared_ptr<LpcObject>& player) {
    std::shared_ptr<LpcObject> ob = player;
    if (!ob || ob->isDestructed()) return false;

    int ret = 0;
    for (int tryIdx = 0; !ret && tryIdx < 8; tryIdx++) {
        if (tryIdx == 4) {
            ob = session.currentNode->handler.lock();
            if (!ob || ob->isDestructed()) return false;
        }
        BuiltFunction built = makeFunction(session, 0, tryIdx % 4);
        std::vector<Value> extra = pushRealNames(session, tryIdx % 4);
        built.args.insert(built.args.end(), extra.begin(), extra.end());

        bool exists = vm.functionExists(ob, built.functionName);
        Value result = exists ? vm.callFunction(ob, built.functionName, built.args) : Value{};
        ret = processAnswer(session, state, exists, result, 0);
        if (ob->isDestructed()) return false;
        if (ret == -3) return false;
    }
    if (!ret) {
        if (state.numErrors == session.bestNumErrors) return false;
        if (state.numErrors++ == 0) makeErrorMessage(session, 0, session.currentError);
    }
    return true;
}

// real we_are_finished() (packages/parser.c), minus the per-match
// object-token loop and check_object_relations() -- both real no-ops
// for this slice, since no match here ever carries an OBJ-family token
// and state.numObjs (real parse_state_t::num_objs) accordingly never
// leaves 0. Not modeled as a field on MatchState at all, since nothing
// in this slice could ever set it to anything else.
void weAreFinished(VM& vm, SentenceSession& session, MatchState& state) {
    if (session.foundLevel < 2) session.foundLevel = 2;
    if (session.bestMatchWeight >= session.currentNode->weight) return;
    if (state.numErrors) {
        if (state.numErrors > session.bestNumErrors) return;
        if (state.numErrors == session.bestNumErrors && session.currentNode->weight < session.bestErrorWeight) return;
    }

    if (!checkFunctions(vm, session, state, session.caller)) return;

    if (state.numErrors) {
        int weight = session.currentNode->weight;
        // real ERR_THERE_IS_NO reweighting is unreachable here (that
        // error kind is exclusively produced by object-token matching).
        if (state.numErrors == session.bestNumErrors && weight <= session.bestErrorWeight) return;
        session.bestError = session.currentError;
        session.currentError = ParserErrorInfo{};
        session.bestNumErrors = state.numErrors;
        session.bestErrorWeight = weight;
        return;
    }

    session.bestMatchWeight = session.currentNode->weight;
    auto handler = session.currentNode->handler.lock();
    if (!handler || handler->isDestructed()) return;

    SentenceMatchResult result;
    result.handler = handler;
    for (int tryIdx = 0; tryIdx < 4; tryIdx++) {
        BuiltFunction built = makeFunction(session, 3, tryIdx);
        std::vector<Value> extra = pushRealNames(session, tryIdx);
        built.args.insert(built.args.end(), extra.begin(), extra.end());
        result.res[tryIdx].functionName = built.functionName;
        result.res[tryIdx].args = std::move(built.args);
    }
    session.bestResult = std::move(result);
}

// real parse_rules() (packages/parser.c): tries every rule node
// registered under the matched verb, skipping any node this slice
// cannot attempt (hasObjectToken -- see VerbRuleNode's own comment),
// any node whose weight cannot beat the current best match, and any
// node whose own lit[0]/lit[1] prefilter fails.
void parseRulesFor(VM& vm, SentenceSession& session) {
    for (const VerbRuleNode& node : session.matchedVerb->nodes) {
        if (node.hasObjectToken) continue;
        if (session.bestMatchWeight > node.weight) continue;

        int pos = 0;
        if (node.lit[0] != -1) {
            pos = checkLiteral(session, node.lit[0], 1);
            if (pos == 0) continue;
        }
        if (node.lit[1] != -1) {
            if (checkLiteral(session, node.lit[1], pos) == 0) continue;
        }

        session.currentNode = &node;
        MatchState state;
        state.tokIndex = 0;
        state.wordIndex = 1;
        state.numMatches = 0;
        state.numErrors = 0;
        parseRule(vm, session, state);
    }
}

// real do_the_call() (packages/parser.c): invokes the first of the four
// pre-built do_ variants that is actually defined on the handler.
void doTheCall(VM& vm, SentenceSession& session) {
    auto ob = session.bestResult->handler.lock();
    if (!ob || ob->isDestructed()) return;
    for (int i = 0; i < 4; i++) {
        if (ob->isDestructed()) return;
        const auto& call = session.bestResult->res[i];
        if (vm.functionExists(ob, call.functionName)) {
            vm.callFunction(ob, call.functionName, call.args);
            return;
        }
    }
    throw LpcRuntimeError("parse_sentence: parse accepted, but no do_* function found on /" + ob->filename());
}

// real get_the_error() (packages/parser.c), with `obj` fixed at real
// code's own -1 -- confirmed directly that every real call site
// (f_parse_sentence(), f_parse_my_rules()) passes -1, making the
// "push_object(loaded_objects[obj])" branch genuinely dead code in real
// usage; only the "push_undefined()" (real const0u, a plain int 0)
// branch is ever reached, so this driver only implements that one.
Value getTheError(VM& vm, SentenceSession& session) {
    if (session.bestError.errorType == ParserErrorType::Allocated) {
        if (!vm.masterObject()) return Value(static_cast<int64_t>(0));
        std::vector<Value> args = {Value(static_cast<int64_t>(ParserErrorType::Allocated)),
                                    Value(static_cast<int64_t>(0)), Value(session.bestError.allocatedMessage)};
        Value ret = vm.applyMaster("parser_error_message", std::move(args));
        if (std::holds_alternative<std::monostate>(ret.data)) return Value(static_cast<int64_t>(0));
        return ret;
    }
    // real "default: ...; hack.u.number = -found_level; return &hack;"
    return Value(static_cast<int64_t>(-session.foundLevel));
}

} // namespace

Value ParserPackage::parseSentence(VM& vm, const std::shared_ptr<LpcObject>& caller, const std::string& sentence,
                                     bool debugFlag) {
    // real code's own check order: the "not known by the parser" guard
    // runs before anything else, including the debug-flag check below.
    if (!caller || !caller->hasParseInfo()) {
        throw LpcRuntimeError("parse_sentence: object is not known by the parser (call parse_init() first)");
    }
    // real f_parse_sentence()'s own `#else error("Parser debugging not
    // enabled. (compile with -DDEBUG or -DPARSE_DEBUG).\n");` -- this
    // driver has no such tracing at all, so that branch is always the
    // real one taken for a truthy debug argument.
    if (debugFlag) {
        throw LpcRuntimeError("parse_sentence: parser debugging not enabled (compile with -DDEBUG or -DPARSE_DEBUG)");
    }

    SentenceSession session;
    session.rawInput = sentence;
    session.caller = caller;

    std::vector<SentenceWord> rawWords = splitSentenceWords(sentence);

    // real parse_sentence()'s own "find an interpretation, first word
    // must be shared (verb)" loop -- tries progressively longer leading
    // word-phrases as the verb candidate (real code's own findstring()-
    // based multi-word-verb mechanism, replicated here via a plain
    // registry lookup instead of a global string-interning table, which
    // this driver has no equivalent of -- see this file's own earlier
    // comments on the same point for addRule()/addSynonym()).
    for (size_t i = 1; i <= rawWords.size(); i++) {
        std::string candidate = rawWords[0].text;
        for (size_t k = 1; k < i; k++) {
            candidate += ' ';
            candidate += rawWords[k].text;
        }

        auto it = verbs().find(candidate);
        if (it == verbs().end()) continue;

        for (VerbEntry& ve : it->second) {
            const VerbEntry* target = &ve;
            if (ve.isSynonym) {
                auto targetIt = verbs().find(ve.synonymOf);
                target = (targetIt != verbs().end()) ? findPlainEntry(targetIt->second, ve.synonymOf) : nullptr;
                if (!target) continue;
            }

            if (session.foundLevel < 1) session.foundLevel = 1;

            session.words.clear();
            SentenceWord verbWord;
            verbWord.text = candidate;
            verbWord.rawStart = rawWords[0].rawStart;
            verbWord.rawEnd = rawWords[i - 1].rawEnd;
            session.words.push_back(verbWord);
            for (size_t k = i; k < rawWords.size(); k++) session.words.push_back(rawWords[k]);

            session.matchedVerb = target;
            parseRulesFor(vm, session);
        }
    }

    if (session.bestMatchWeight != 0) {
        doTheCall(vm, session);
        return Value(static_cast<int64_t>(1));
    }
    return getTheError(vm, session);
}

} // namespace amlp
