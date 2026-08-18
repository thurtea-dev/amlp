# Wand of Creation -- scoping notes (2026-08-18)

Scoping pass only, no mudlib code written yet. Source: read in full,
`temp/wiz_tools/staff_of_creation.c` (41 lines) plus its real dependency
chain, cross-checked against `src/efun/EfunTable.cpp` directly (not
assumed) and against this driver's actual bundled mudlib,
`mudlib/` (Lil).

## What staff_of_creation.c actually is

It is a thin wrapper. All of its real behavior comes from two other
files:

- `tanstaafl_base.c` -- **inherited**, provides the actual `build`,
  `clone`, `purge` commands. This is the real functional dependency.
- `creation_review_menu_d.c` -- **delegated to** for the `review`
  command (the apprentice coding queue). Read in full: every branch of
  its menu calls `APPRENTICE_D->...` (`list_coding_queue`,
  `get_submission`, `format_submission`, `coding_mark_done`).
  `APPRENTICE_D` is referenced but **never defined anywhere** in the
  `temp/wiz_tools/` dump or the rest of this repo (grepped directly).
  This is a dead end regardless of efun support -- there is nothing to
  port `review` from.

Two more files in `temp/wiz_tools/` reference `staff_of_creation`/
`tanstaafl_base` by name, but as consumers, not dependencies:
- `ring_of_dominion.c` -- a bigger admin item that bundles creation +
  demotion + dominion powers into one ring, reusing
  `creation_review_menu_d.c` the same way. Not something
  `staff_of_creation.c` itself needs.
- `staff_supplies_chest.c` -- a starter-kit container that hands new
  wizards a `staff_of_creation` among other tools via `ensure_tool(...)`.
  Also not a dependency, just a consumer.

Every other file in `temp/wiz_tools/` (`staff_of_dominion.c`,
`staff_of_demotion.c`, `apprentice_kit.c`, `admin_cheat_sheet.c`,
`mailbox.c`, `mailbox_welcome.c`, `dominion_menu_d.c`,
`demotion_menu_d.c`, `rp_skill_menu_d.c`, `rp_skill_tool.c`,
`skill_slip.c`, `staff_board.c`, `tattoo_gun.c`, `tattoo_menu_d.c`,
`wiz_reference_book.c`, `card_format_d.c`) has **no reference to or
from** `staff_of_creation.c` at all -- unrelated, out of scope for this
item.

## The bigger mismatch: the reference mudlib versus Lil

`staff_of_creation.c`/`tanstaafl_base.c` assume a full mudlib this
driver does not actually have bundled:
- `/domains/adm/...` directory conventions, `/cmds/creator/_qcs`'s own
  `qcs_dispatch()` -- the entire real implementation of the `build`
  verb -- and `/cmds/creator/_qcs_room`'s `qcs_resolve_realm_file()`.
  None of this exists anywhere in this repo, not in `temp/wiz_tools/`
  and not in `mudlib/`.
- `admin_wizp()`, `coding_wizp()`, `has_wiz_tool()` -- simul_efuns from
  that same, unprovided mudlib's own simul_efun file, not real driver
  efuns.
- A `std/object.c`-style base class providing `set_name()`/`set_id()`/
  `set_short()`/`set_long()`/`set_mass()`/`set_value()`/
  `set_property()`/`query_short()`/`query_name()`, plus `move()`/
  `remove()` wrappers around the real `move_object()`/`destruct()`
  efuns.

This driver's actual bundled mudlib, `mudlib/` (Lil), is a genuinely
minimal MudOS bootstrap mudlib by design (its own `readme`: "a very
minimal mudlib... intended for use in bootstrapping a mudlib to be
built from scratch"). Its `command/` directory holds about a dozen
bare `main(string arg)`-style files (`eval.c`, `codefor.c`, `dest.c`,
`rm.c`, `ed.c`, `say.c`, `quit.c`, `shutdown.c`, `speed.c`, `tests.c`,
`update.c`, `who.c`) and nothing resembling a wizard-tool or
base-object system at all. Confirmed live: this driver's own
`ApplyTable::known()` (`src/apply/ApplyTable.cpp`) recognizes `short`/
`long`/`id` as real apply names, but grepping `VM.cpp` and `src/net/`
directly shows the C++ core never actually calls any of them -- there
is no `look` command anywhere in Lil's own `command/` directory either.
So "based on staff_of_creation.c" has to mean porting its *intent*
(one tool, held by a player, that can build/clone/purge/create) against
Lil's own real conventions and this driver's real efun set, not a
literal file port -- there is no `_qcs` to delegate `build` to even if
every other efun tanstaafl_base.c calls were real.

One thing worth noting: Lil's own `command/eval.c` and `command/
codefor.c` already demonstrate a real, working "bring a new thing into
being" idiom in this exact driver+mudlib combination -- `write_file()` a
temp `.c` skeleton, then `load_object()`/`clone_object()` it. That is
the closest real analog to what `staff_of_creation.c`'s own doc comment
promises ("bring new rooms, monsters, and objects into being") and is
worth building the wand's own `create` verb on top of.

## Efun/apply cross-check

Checked every bareword call in `staff_of_creation.c` + `tanstaafl_base.c`
+ `creation_review_menu_d.c` against `src/efun/EfunTable.cpp` directly
(`grep -oP 'registerEfun\("\K[^"]+'`), plus two special cases that are
real but not table-registered.

**Real, already supported:**
`environment`, `add_action`, `this_player`, `this_object`, `living`,
`move_object` (bare `move` is not real -- see below), `file_size`,
`clone_object`, `write`, `destruct`, `present`, `notify_fail`,
`capitalize`, `replace_string`, `to_int`, `input_to`, `base_name`,
`write_file`, `read_file`, `load_object`, `rm`, `mkdir`, `get_dir`,
`query_verb`, `command`. Plus two real-but-special-cased: `sscanf`
(compiled as `OpCode::Sscanf`, not table-registered) and `catch`
(real syntax, row 0.1).

**Missing -- used by the reference files but not real here:**
- `set_name`/`set_id`/`set_short`/`set_long`/`set_mass`/`set_value`/
  `set_property`/`query_short`/`query_name` -- `std/object.c`'s own
  accessor convention, not efuns in FluffOS or this driver at all. The
  wand's `create()` should set plain object variables and define
  `short()`/`long()`/`id()` directly -- the real apply names this
  driver's own `ApplyTable` already recognizes -- instead of calling
  nonexistent `set_X()` efuns.
- `admin_wizp`, `coding_wizp`, `has_wiz_tool` -- simul_efuns from the
  unprovided original mudlib, not present in Lil's own
  `single/simul_efun.c` either. Lil has no wizard/permission concept at
  all today.
- `absolute_path` -- not a real efun here (confirmed by direct grep).
- `remove` -- not a real efun; `std/object.c`'s own pre-destruct
  wrapper. `destruct()` alone is what Lil's own `dest.c` already calls.
- `query_auto_load` -- grepped `ObjectManager.cpp`/`VM.cpp` directly:
  never called anywhere. This driver has no reboot/auto-load
  persistence mechanism at all; including it would be inert dead code.

**Out of reach regardless of efun support** (missing supporting
infrastructure, not a driver gap): `review` (needs `APPRENTICE_D`,
never defined anywhere) and real `build` (needs `_qcs`, never provided
anywhere).

## Buildable now (first version, uses only what's real today)

1. `create()`/`short()`/`long()`/`id()` using plain object variables and
   this driver's own recognized apply names, not `set_X()` accessors.
2. `clone <path>` -- `clone_object()` + `move_object()` placement,
   `catch()` for errors, mirroring `tanstaafl_base.c`'s
   `cmd_clone_ob()` efun-for-efun once `absolute_path()`/cwd resolution
   is dropped (Lil has no cwd concept either -- accept the path as
   typed, same as `eval.c`/`dest.c`/`codefor.c` already do).
3. `purge <id>` -- `present()` + `living()` guard + `destruct()`,
   mirroring `cmd_purge_ob()` exactly, dropping the `ob->remove()` call
   (not real here -- `destruct()` alone is Lil's own existing pattern).
4. A genuinely new `create <name>` verb built on Lil's own proven
   `write_file()` + `load_object()`/`clone_object()` idiom -- a real,
   working "bring a new thing into being" capability true to the tool's
   stated intent, even without `_qcs`.
5. `add_action`-gated commands with the held-only guard
   (`environment(this_object()) == this_player()`), matching
   `staff_of_creation.c`'s own `init()` exactly.

## Blocked / deferred (needs a decision or missing infrastructure, not an efun gap)

1. `review` / apprentice queue -- no `APPRENTICE_D` anywhere to port
   from. Would need its own from-scratch design if wanted at all; out
   of scope for a first version.
2. Wizard-only permission gating -- Lil has no wizard/permission
   concept at all yet. Needs a real decision (a hardcoded name list, a
   new player-variable convention, or no gating at all for a
   single-player test build) before the wand can meaningfully restrict
   itself the way the original tool does.
3. Real `build` (room/exit/description editing UX) -- `_qcs` was never
   provided. The `create <name>` idea above is a real but much smaller
   substitute, not a room editor.
4. `query_auto_load`-based reboot persistence -- no auto-load mechanism
   exists in this driver at all; would be dead code if included.
