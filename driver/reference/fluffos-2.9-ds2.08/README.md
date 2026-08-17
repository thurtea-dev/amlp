# fluffos-2.9-ds2.08/

The FluffOS 2.9 C driver source - vendored third-party code, not
AetherMUD-authored. This compiles into `../bin/driver` and
`../bin/addr_server`.

This is upstream driver code. For AetherMUD-specific documentation, see
the repo root `README.md`/`CLAUDE.md` and `docs/INSTALL.md` for how this
gets built. For FluffOS itself, see the driver's own `README`,
`ChangeLog*`, and `INSTALL` files in this directory, and `options.h` for
the compiled-in feature flags (notably: `PRIVS` is `#undef`, i.e. off, in
this build - see CLAUDE.md / the admin bootstrap notes for why that
matters to the mudlib).

Per repo convention, only this top-level directory gets a
README.md/LLM_BREADCRUMB.md pair - its internal subdirectories
(`testsuite/`, `packages/`, `compat/`, `Win32/`, etc.) do not, since
they're vendored upstream code rather than AetherMUD content.

Local build config: `local_options.nm3` (this build's actual option
overrides) and `options.h` (the full option set, mostly upstream
defaults).
