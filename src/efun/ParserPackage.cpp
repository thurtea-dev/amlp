#include "amlp/efun/ParserPackage.hpp"
#include "amlp/core/Errors.hpp"
#include "amlp/object/LpcObject.hpp"
#include <algorithm>
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

} // namespace amlp
