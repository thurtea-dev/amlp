// mudlib:  Lil
// file:    /clone/wand_of_creation.c
// purpose: Wand of Creation -- a single held item bundling the
//          real-efun-only subset of temp/wiz_tools/staff_of_creation.c's
//          own behavior. See /WAND_OF_CREATION_SCOPING.md at the mudlib
//          root for the full scoping trail: which of the original
//          tool's calls are real efuns here, which are std/object.c
//          conventions Lil never had, and which (review, real build via
//          _qcs, wizard-permission gating, auto-load persistence) are
//          deliberately left out of this first version.
//
// No inherits at all, matching Lil's own plain command-file style
// (command/dest.c etc) -- id()/short()/long() are written directly here
// as plain functions instead of the set_X()/query_X() accessor
// convention the original tool assumed (Lil has no std/object.c base
// providing those; this driver's own ApplyTable does recognize
// short()/long()/id() as real, driver-known apply names, so those are
// what get defined).
//
// move(): live-verified real bug, not architecture guesswork -- an
// earlier draft of this file called "ob->move_object(dest)" via "->" to
// place clones. This driver's call_other (confirmed directly:
// src/vm/VM.cpp's own callFunction(), used by both "->" and the
// call_other() efun) never falls back to the efun table -- it only
// finds a function genuinely defined in ob's own program, and silently
// returns void if there is none, exactly like real FluffOS's own
// call_other-to-undefined-function semantics. "move_object" is only a
// driver efun, not something wand_of_creation.c or a freshly cloned
// object defines, so "ob->move_object(dest)" always silently did
// nothing -- confirmed live: environment(ob) stayed 0 after the call.
// The real reference tool's own tanstaafl_base.c already works around
// this the same way its whole ecosystem does: it calls "ob->move(env)",
// relying on every object in its mudlib inheriting a base class
// (/std/object) that defines a "move(dest)" wrapper around the bare
// "move_object(dest)" efun call (current_object() correctly resolves to
// ob only from inside ob's own code). Lil's closest equivalent,
// inherit/base.c, defines the identical wrapper. This file defines its
// own copy directly (no inherit at all, matching this file's own style)
// so the wand itself can be placed by external code (there is no "get"
// command in Lil to place it any other way), and clone/cmd_create both
// call ob->move(dest) on the objects they place, exactly matching the
// original tool's real convention and its real limitation: this only
// works for a target object that itself defines "move" -- true of
// everything this file itself creates (the generated skeleton in
// cmd_create() defines one too) and of anything else in this mudlib
// that already inherits BASE (e.g. /clone/user), not guaranteed for an
// arbitrary unrelated file cloned via "clone <path>", the same real
// constraint tanstaafl_base.c's own cmd_clone_ob() already has.
int move(mixed dest) {
    return move_object(dest);
}

string *wids;

void create() {
    wids = ({ "wand", "wand of creation", "creation wand" });
}

int id(string arg) {
    return arg && member_array(arg, wids) != -1;
}

string short() {
    return "a wand of creation";
}

string long() {
    return
        "A slender wand humming with creative potential. It lets its\n"
        "wielder clone existing objects, purge unwanted ones from the\n"
        "room, and bring new ones into being. Usable only while held.\n"
        "Commands: clone <path>, purge <id>, create <name>\n";
}

static int held() {
    return environment(this_object()) == this_player();
}

// Held-only gating: staff_of_creation.c's own init() only calls
// add_action() at all when "environment(this_object()) == this_player()"
// is already true at the moment init() fires. This driver's own
// VM::moveObject() only propagates init() calls through objects with
// commandsEnabled() true (a living/player-only flag), checked on the
// *mover* and on the *destination's existing occupants* -- live-verified
// this correctly fires when the wand enters a room a connected player is
// already standing in (leg 2: the player is an existing occupant of that
// room, commandsEnabled() is true, so the wand's own init() is called
// with the player as command_giver). Registered unconditionally here
// rather than gated the way the original's init() is, since that
// colocation is the one reliable moment this driver actually calls
// init() on a plain item at all; each command below checks held() for
// itself instead -- functionally the same "only usable while held" gate,
// evaluated at command time instead of at init() time.
void init() {
    add_action("cmd_clone", "clone");
    add_action("cmd_purge", "purge");
    add_action("cmd_create", "create");
}

// clone <path> -- mirrors temp/wiz_tools/tanstaafl_base.c's own
// cmd_clone_ob() efun-for-efun, minus the absolute_path()/get_path()
// cwd resolution it uses (not a real efun here, and Lil has no cwd
// concept at all -- command/eval.c, command/dest.c, command/codefor.c
// all already just take the path exactly as typed, so this does too).
// clone_to_wizard()'s own real placement logic (living things go to the
// room, never inventory; everything else prefers inventory, falls back
// to the room) is preserved, using ob->move(dest) -- see this file's own
// move() comment above for why, and for the real limitation this
// inherits from the original tool.
int cmd_clone(string str) {
    object ob, env;
    string err;

    if (!held()) {
        write("You are not holding the wand of creation.\n");
        return 1;
    }
    if (!str || !sizeof(str)) {
        write("Clone what?\n");
        return 1;
    }
    err = catch(ob = clone_object(str));
    if (err || !ob) {
        write("Cannot clone: " + str + (err ? " " + err : "") + "\n");
        return 1;
    }

    env = environment(this_player());
    if (living(ob)) {
        if (env && !catch(ob->move(env))) {
            write("Cloned into the room: " + str + "\n");
        } else {
            write("Clone succeeded but could not be placed; destructing it.\n");
            destruct(ob);
        }
        return 1;
    }
    if (!catch(ob->move(this_player()))) {
        write("Cloned into your inventory: " + str + "\n");
    } else if (env && !catch(ob->move(env))) {
        write("Cloned into the room: " + str + "\n");
    } else {
        write("Clone succeeded but could not be placed; destructing it.\n");
        destruct(ob);
    }
    return 1;
}

// purge <id> -- mirrors cmd_purge_ob(), minus the ob->remove() call
// (not real here; destruct() alone is Lil's own existing pattern, see
// command/dest.c).
int cmd_purge(string str) {
    object ob;

    if (!held()) {
        write("You are not holding the wand of creation.\n");
        return 1;
    }
    if (!str || !sizeof(str)) {
        write("Purge what?\n");
        return 1;
    }
    ob = present(str, environment(this_player()));
    if (!ob) {
        write("Not here: " + str + "\n");
        return 1;
    }
    if (living(ob)) {
        write("Cannot purge living objects.\n");
        return 1;
    }
    write("Purged: " + str + "\n");
    destruct(ob);
    return 1;
}

// create <name> -- the real substitute for the missing _qcs quick-create
// system: the same "write raw LPC to a fresh file, then load/clone it"
// idiom already proven live in this exact driver by command/eval.c and
// command/codefor.c, applied to a real, named, persistent file instead
// of a throwaway eval scratch file. The generated skeleton defines its
// own move() wrapper (see this file's own move() comment above) so the
// freshly created object can always be placed reliably, unlike an
// arbitrary file cloned via "clone <path>".
int cmd_create(string str) {
    string fname, path, body;
    object ob, room;

    if (!held()) {
        write("You are not holding the wand of creation.\n");
        return 1;
    }
    if (!str || !sizeof(str)) {
        write("Create what? (usage: create <name>)\n");
        return 1;
    }
    fname = replace_string(str, " ", "_");
    path = "/data/created/" + fname;

    if (file_size(path + ".c") != -1) rm(path + ".c");
    if (find_object(path)) destruct(find_object(path));

    body = "// auto-generated by the wand of creation\n"
        "void create() {\n"
        "}\n"
        "int move(mixed dest) {\n"
        "    return move_object(dest);\n"
        "}\n"
        "string short() { return \"" + str + "\"; }\n"
        "string long() { return \"" + str + ", freshly created by the wand of creation.\\n\"; }\n"
        "int id(string arg) { return arg == \"" + str + "\"; }\n";
    write_file(path + ".c", body, 1);

    ob = load_object(path);
    if (!ob) {
        write("Creation failed: could not compile " + path + ".c\n");
        return 1;
    }
    ob = clone_object(path);
    if (!ob) {
        write("Creation failed: could not clone " + path + ".c\n");
        return 1;
    }

    room = environment(this_player());
    if (room && !catch(ob->move(room))) {
        write("Created and placed here: " + str + "\n");
    } else if (!catch(ob->move(this_player()))) {
        write("Created into your inventory: " + str + "\n");
    } else {
        write("Created but could not be placed; destructing it.\n");
        destruct(ob);
    }
    return 1;
}
