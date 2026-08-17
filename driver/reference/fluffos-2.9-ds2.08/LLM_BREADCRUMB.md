# LLM breadcrumb - fluffos-2.9-ds2.08/

1. Read `/CLAUDE.md` at the repo root first.
2. Read `README.md` in this directory (above).
3. Check the parent `nightmare3_fluffos_v2/LLM_BREADCRUMB.md` (and the
   repo root's) before assuming this directory's gotchas are
   self-contained.
4. Related: `../lib/` is the actual gameplay code this driver runs;
   changes here almost never need to happen for gameplay work.

## Gotchas specific to this directory

- This is vendored upstream C driver source, not AetherMUD content.
  Changes here should be rare and deliberate - almost all gameplay work
  happens in `../lib/` (LPC), not here (C).
- No README.md/LLM_BREADCRUMB.md pairs exist for the subdirectories of
  this tree (`testsuite/`, `packages/`, `compat/`, `Win32/`, etc.) by
  design - don't add them unless that scoping decision changes.
- `options.h` controls compiled-in driver features. `PRIVS` is `#undef`
  in this build, which means `query_privs()`/`master()->valid_apply()`
  can never return true for anyone - this directly affected how the
  first-admin bootstrap fix in `../lib/std/user.c` had to be designed
  (it cannot rely on the PRIVS-based privilege-bypass mechanism at all).
  If you ever flip `PRIVS` on, re-audit anything that assumed it was
  off.
- Rebuilding requires `./configure nm3 && make && make install` from
  within this directory - see `docs/INSTALL.md` for the full sequence.
