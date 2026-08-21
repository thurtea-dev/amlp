// file: /single/account_d.c
//
// notes/ACCOUNT_LOGIN_PLAN.md's own first build slice, "Account
// storage" (2026-08-21): account_exists(), create_account(),
// check_password(), account_exists(), no login integration yet,
// testable in isolation via eval, matching that plan's own "Rough
// build ordering" item 1 exactly. Every efun this needs (crypt,
// save_object, restore_object, mkdir, file_size) was already confirmed
// real and working before this was built, per that plan's own "Driver
// hooks actually available" section: this is genuinely new mudlib
// code, not a driver gap.
//
// Per-account state lives in a separate /single/account_record.c
// instance, one save_object()/restore_object() file per account under
// ACCOUNTS_DIR, bucketed by the account name's own first letter (see
// that file's own header comment for why a per-account object exists
// at all rather than this daemon just calling save_object() on
// itself).

#include <globals.h>

private
string account_path(string name) {
    name = lower_case(name);
    return ACCOUNTS_DIR + "/" + name[0..0] + "/" + name;
}

private
void ensure_dirs(string name) {
    // mkdir() returns 0 when the directory already exists (real
    // file.c's own mkdir() behavior, ported here too): no need to
    // check first, a redundant call is harmless, matching the "does
    // not create missing parent directories either" contract
    // save_object()'s own registration comment documents, callers that
    // need the tree to exist make it themselves, this is that call.
    mkdir(ACCOUNTS_DIR);
    mkdir(ACCOUNTS_DIR + "/" + lower_case(name)[0..0]);
}

int
account_exists(string name) {
    if (!name || name == "") {
        return 0;
    }
    return file_size(account_path(name) + ".o") != -1;
}

int
create_account(string name, string password) {
    object rec;
    string path;

    if (!name || name == "" || !password || password == "") {
        return 0;
    }
    if (account_exists(name)) {
        return 0;
    }

    ensure_dirs(name);
    path = account_path(name);

    rec = new(ACCOUNT_RECORD);
    rec->set_name(lower_case(name));
    // Real "crypt(str2, 0)" idiom (secure/std/login.c's own
    // confirm_password(), see crypt()'s own EfunTable.cpp citation):
    // salt 0 generates a fresh random salt for a brand-new password.
    rec->set_hash(crypt(password, 0));
    rec->set_created(time());
    rec->set_characters(({}));
    rec->save_me(path);
    destruct(rec);

    return 1;
}

int
check_password(string name, string password) {
    object rec;
    string path;
    string hash;
    int ok;

    if (!account_exists(name)) {
        return 0;
    }

    path = account_path(name);
    rec = new(ACCOUNT_RECORD);
    rec->load_me(path);
    hash = rec->query_hash();
    // Real crypt() verification idiom: passing the already-stored hash
    // back in as the "salt" argument re-derives it with the same salt
    // crypt(3) itself embeds in the hash's own leading bytes, so a
    // correct password reproduces the identical hash string; confirmed
    // directly from this driver's own crypt() implementation (a
    // string salt of length >= 2 is used as-is, not regenerated).
    ok = hash && crypt(password, hash) == hash;
    destruct(rec);

    return ok;
}
