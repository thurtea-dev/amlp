#include <globals.h>

// notes/ACCOUNT_LOGIN_PLAN.md build ordering item 2 (login integration,
// 2026-08-21): a real input_to() state machine wired to ACCOUNT_D
// (/single/account_d.c, item 1, done earlier the same day), replacing
// the previous unconditional clone-and-exec() with no auth at all (this
// file's own former top-of-file comment, "needs fixed to handle
// passwords", is what this closes). Structurally modeled on
// temp/core-lib/secure/login.c's own logon() -> getUserLogin() ->
// enterPassword()/createUser() shape, already read and cited in that
// plan's own reference-corpora survey, but calling ACCOUNT_D directly
// rather than a separate authenticationService daemon (this plan's own
// deliberately smaller first slice).
//
// Character concept (that plan's own item 3): resolved 2026-08-21, same
// day as this file's own login integration -- see globals.h's own
// CHARACTERS_DIR comment for the "merge, not a separate object"
// decision, and /clone/user.c's own load_character()/save_character()
// for the actual persistence. A successful login still clones/execs
// /clone/user exactly the way the old flow did, only gated on a real
// account now (and, since item 3, also carrying real persisted state
// across a disconnect/reconnect), using the account name itself as the
// character name (the same single-character-per-account shape this
// project's own prior scratch-mudlib live verification already
// exercised for real, see STATUS-ARCHIVE.md's "Confirm <name> as your
// account and first character name?" walkthrough) -- no per-account
// character list, no character creation prompts, no y/n account-name
// confirm step either (a deliberate simplification against that same
// historical reference: this slice's own scope, per the plan's build-
// ordering item 2, is "account name -> password, no character concept
// yet", not a full port of the fuller flow; item 3 added persistence,
// still not multi-character or a character-creation flow, items 4/5).

private string account_name;
private int tries;
private string pending_password;

#ifdef __INTERACTIVE_CATCH_TELL__
void catch_tell(string str) {
    receive(str);
}
#endif

private
int
valid_account_name(string name)
{
    // Kept deliberately simple: real corpora reject far more (reserved
    // words, profanity lists, length caps) but none of that is a driver
    // gap or an account_d.c contract this slice needs to satisfy --
    // only requirement account_d.c's own account_path() actually has is
    // "no '/' in the name" (it is used unescaped as a path segment).
    if (!name || name == "") return 0;
    if (strsrch(name, "/") != -1) return 0;
    return 1;
}

private
void
enter_game()
{
    object user;

    write("\n");
#ifdef __PACKAGE_UIDS__
    seteuid(getuid(this_object()));
#endif
    user = new(USER_OB);
    user->set_name(account_name);
    // notes/ACCOUNT_LOGIN_PLAN.md build ordering item 3: restores this
    // character's own persisted state (currently just login_count) if a
    // save file already exists (a returning account), or leaves every
    // variable at its just-initialized default and starts counting from
    // this login if not (a brand-new account, right after
    // create_account() above) -- load_character() itself handles both
    // cases identically, no branch needed here.
    user->load_character(ACCOUNT_D->character_path(account_name));
    write("Welcome back! You have logged in " + user->query_login_count() +
        " time(s).\n");
    exec(user, this_object());
    user->setup();
#ifndef __NO_ENVIRONMENT__
    user->move(START_LOC);
#endif
    destruct(this_object());
}

void
login_timeout()
{
    write("\nTimed out waiting for input. Goodbye.\n");
    destruct(this_object());
}

void
logon()
{
#ifdef __NO_ADD_ACTION__
    set_this_player(this_object());
#endif
    write("Welcome to Library!\n\n");
    cat("/etc/motd");
    write("\nWhat account name do you wish? ");
#ifdef __PACKAGE_UIDS__
    seteuid(getuid(this_object()));
#endif
    tries = 0;
    call_out("login_timeout", LOGIN_TIMEOUT_SECS);
    input_to("got_account_name");
}

void
got_account_name(string str)
{
    remove_call_out("login_timeout");

    if (!valid_account_name(str)) {
        write("\nThat is not a valid account name. What account name do you wish? ");
        call_out("login_timeout", LOGIN_TIMEOUT_SECS);
        input_to("got_account_name");
        return;
    }

    account_name = str;
    call_out("login_timeout", LOGIN_TIMEOUT_SECS);
    if (ACCOUNT_D->account_exists(account_name)) {
        write("\nPassword: ");
        input_to("got_login_password", INPUT_NOECHO);
    } else {
        write("\nNo such account. Creating a new account named '" +
            account_name + "'.\n");
        write("Please choose a password of at least " + MIN_PASSWORD_LEN +
            " letters: ");
        input_to("got_new_password", INPUT_NOECHO);
    }
}

void
got_login_password(string str)
{
    remove_call_out("login_timeout");

    if (ACCOUNT_D->check_password(account_name, str)) {
        enter_game();
        return;
    }

    tries++;
    if (tries >= MAX_LOGIN_TRIES) {
        write("\nToo many failed attempts. Goodbye.\n");
        destruct(this_object());
        return;
    }

    write("\nIncorrect password. Password: ");
    call_out("login_timeout", LOGIN_TIMEOUT_SECS);
    input_to("got_login_password", INPUT_NOECHO);
}

void
got_new_password(string str)
{
    remove_call_out("login_timeout");

    if (!str || strlen(str) < MIN_PASSWORD_LEN) {
        write("\nToo short. Please choose a password of at least " +
            MIN_PASSWORD_LEN + " letters: ");
        call_out("login_timeout", LOGIN_TIMEOUT_SECS);
        input_to("got_new_password", INPUT_NOECHO);
        return;
    }

    pending_password = str;
    write("\nPlease confirm your password choice: ");
    call_out("login_timeout", LOGIN_TIMEOUT_SECS);
    input_to("got_confirm_password", INPUT_NOECHO);
}

void
got_confirm_password(string str)
{
    remove_call_out("login_timeout");

    if (str != pending_password) {
        pending_password = 0;
        write("\nPasswords did not match. Please choose a password of at least " +
            MIN_PASSWORD_LEN + " letters: ");
        call_out("login_timeout", LOGIN_TIMEOUT_SECS);
        input_to("got_new_password", INPUT_NOECHO);
        return;
    }

    if (!ACCOUNT_D->create_account(account_name, pending_password)) {
        // Only real failure path left once got_account_name() has
        // already confirmed !account_exists(): create_account()'s own
        // empty-name/empty-password guards, already ruled out above by
        // valid_account_name()/the MIN_PASSWORD_LEN check.
        write("\nCould not create that account. Goodbye.\n");
        destruct(this_object());
        return;
    }

    pending_password = 0;
    write("\nAccount created.\n");
    enter_game();
}
