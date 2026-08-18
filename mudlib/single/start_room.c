// mudlib:  library
// file:    /single/start_room.c
// purpose: the one starting location every fresh login lands in, and
//          currently the only real room this mudlib has. Stock Lil had
//          no room concept at all -- login.c used to move a fresh
//          player straight into VOID_OB ("/single/void"), a bare
//          bookkeeping placeholder with no description and no exits
//          (see LIBRARY_MUDLIB_PLAN.md's own finding). This replaces
//          that with a real, describable location.
//
// No "look" command exists anywhere in this mudlib (stock Lil never
// shipped one either -- see WAND_OF_CREATION_SCOPING.md's own note on
// this exact point), so init() below writes the room's own description
// straight to whoever just arrived, the same moment a "look" command
// would have, rather than leaving it unreachable.

#include <globals.h>

void
create()
{
}

string
short()
{
    return "the entrance hall";
}

string
long()
{
    return
        "A plain stone entrance hall. This is the library mudlib's only\n"
        "real room right now, nothing else has been built here beyond a\n"
        "wand of creation resting on a low pedestal near the door.\n"
        "Commands: clone <path>, purge <id>, create <name> (once the\n"
        "wand is held), say <text>, who, quit.\n";
}

int
id(string arg)
{
    return arg == "room" || arg == "entrance hall" || arg == "entrance" || arg == "hall";
}

// init: fires every time a living moves into this room, not just the
// first time (a real MudOS/FluffOS apply, already recognized and fired
// by this driver -- see ApplyTable.cpp) -- so the wand-of-creation hand-
// out is guarded against handing out a second one to someone who
// already has one (walks out and back in, reconnects, etc.). The
// description write is unguarded -- showing it again on re-entry is
// exactly what a real "look" would do too.
//
// The handed-out wand is moved here in two steps, not one, matching
// wand_of_creation.c's own documented mechanism (see its own comment on
// held()/init()): this driver's VM::moveObject() only calls init() on
// the *moved* object (which is where the wand's own add_action calls
// live) when the destination's existing occupants already have
// commandsEnabled() true -- true here because the player is already a
// genuine occupant of this room by the time this function runs, not
// true of the player's own (empty) inventory. Moving the wand straight
// into the player with a single move() call would leave its add_action
// calls never registered -- confirmed live the first time this was
// tried, all three wand commands silently fell through to commandHook()
// and failed with "source file not found".
void
init()
{
    object player;
    object wand;

    player = this_player();
    if (!player) {
        return;
    }

    write(long());

    if (present("wand", player)) {
        return;
    }
    wand = clone_object(WAND_OB);
    if (wand) {
        wand->move(this_object()); // enters the room the player already occupies -- fires the wand's own init()
        wand->move(player);        // now genuinely held
    }
}
