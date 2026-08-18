// file: globals.h

#ifdef __SENSIBLE_MODIFIERS__
#define staticf protected
#define staticv nosave
#else
#define staticf static
#define staticv static
#endif

// tests.h (ASSERT/ASSERT2/SAVETP/RESTORETP) dropped along with stock
// Lil's own self-test suite (single/tests/, command/tests.c,
// inherit/tests.c) -- see LIBRARY_MUDLIB_PLAN.md. Nothing kept in this
// mudlib used any of those macros; the untouched originals are still
// available at temp/lil/include/tests.h if that suite is ever needed
// again.

#define SINGLE_DIR "/single"
#define CONFIG_DIR "/etc"
#define LOG_DIR    "/log"

#define LOGIN_OB   "/clone/login"
#define USER_OB    "/clone/user"

// Real MudOS/Lil convention was VOID_OB ("/single/void"), a bookkeeping
// placeholder with no real description, no exits -- stock Lil moves
// every fresh login there and leaves them with nothing else to do (see
// LIBRARY_MUDLIB_PLAN.md's own finding: Lil has no room concept at
// all). This mudlib has one real starting room instead, so login.c's
// own post-setup move() (and master.c's destruct_environment_of()
// safety fallback) both target this instead of a true void.
#define START_LOC  "/single/start_room"

#define ROOT_UID     "Root"
#define BACKBONE_UID "Backbone"

#define BASE            "/inherit/base"
#define OVERRIDES_FILE  "/single/simul_efun"

#define COMMAND_PREFIX "/command/"

// classes for message() efun.
#define M_STATUS "status"
#define M_SAY    "say"

#define WAND_OB "/clone/wand_of_creation"

// include/command.h's own "inherit CLEAN_UP;" -- every kept command
// file that includes command.h (who.c, say.c, quit.c, shutdown.c)
// depends on this. Confirmed live: removing this and inherit/clean_up.c
// broke all four with "expected token type in inherit statement path".
#define CLEAN_UP "/inherit/clean_up"
