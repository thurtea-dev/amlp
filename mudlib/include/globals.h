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

// notes/ACCOUNT_LOGIN_PLAN.md's own first build slice: a real login/
// account system, greenlit 2026-08-19, this daemon and its per-account
// record object built 2026-08-21. ACCOUNTS_DIR is bucketed by the
// account name's own first letter (account_d.c's own account_path()),
// matching the pattern several vendored corpora use for large flat
// save trees, e.g. skylib_fluffos_v3's own save/bank_accounts/<letter>/.
#define ACCOUNT_D      "/single/account_d"
#define ACCOUNT_RECORD "/single/account_record"
#define ACCOUNTS_DIR   "/accounts"

// notes/ACCOUNT_LOGIN_PLAN.md build ordering item 2 (login integration,
// 2026-08-21): the real input_to() flag bit, matching real comm.h's own
// "#define I_NOECHO 0x1 /* input_to flag */" (confirmed already live in
// this driver's own input_to() registration comment, EfunTable.cpp) --
// this mudlib had no header-level name for it yet, only the driver-side
// citation, so /clone/login.c needed this defined before it could ask
// for it by name the way real login code does (e.g. temp/core-lib's own
// secure/login.c, "input_to(\"enterPassword\", INPUT_NOECHO | ...)").
#define INPUT_NOECHO 1

// Same session, same file: a login-attempt counter and an idle-input_to
// timeout, both named directly in ACCOUNT_LOGIN_PLAN.md's own item 2
// scope ("should be part of this from the start, not bolted on later"),
// sized against real precedent rather than picked arbitrarily --
// MAX_LOGIN_TRIES 3 is the common mudlib convention for a password
// retry limit before disconnecting; LOGIN_TIMEOUT_SECS 90 matches
// temp/core-lib's own cited "call_out(\"timeout\", 90)" idle-login
// disconnect exactly (ACCOUNT_LOGIN_PLAN.md's own reference-corpora
// survey already read and cited this). MIN_PASSWORD_LEN 5 matches this
// project's own prior live-verification session's real prompt text
// against an earlier scratch mudlib ("Please choose a password of at
// least 5 letters", STATUS-ARCHIVE.md), reused rather than invented
// fresh.
#define MAX_LOGIN_TRIES    3
#define LOGIN_TIMEOUT_SECS 90
#define MIN_PASSWORD_LEN   5

// include/command.h's own "inherit CLEAN_UP;" -- every kept command
// file that includes command.h (who.c, say.c, quit.c, shutdown.c)
// depends on this. Confirmed live: removing this and inherit/clean_up.c
// broke all four with "expected token type in inherit statement path".
#define CLEAN_UP "/inherit/clean_up"
