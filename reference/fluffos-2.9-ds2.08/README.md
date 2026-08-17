# fluffos-2.9-ds2.08/

The FluffOS 2.9 C driver source - vendored third-party code, not
AMLP-authored. This is kept here purely as a citable reference for real
FluffOS/MudOS behavior (lexer, compiler, VM, config semantics) while
building AMLP, an independent C++ LPC driver reimplementation. It is not
compiled or built as part of AMLP's own build (see the repo root
`.gitignore`'s own note on this); nothing here is a build dependency.

For AMLP's own documentation, see the repo root `README.md`, `ROADMAP.md`,
`STATUS.md`, and each `src/<module>/instruct.md`. For FluffOS itself, see
the driver's own `README`, `ChangeLog*`, and `INSTALL` files in this
directory, and `options.h` for the compiled-in feature flags this vendored
copy ships with.

Per repo convention, only this top-level directory gets a
README.md/LLM_BREADCRUMB.md pair - its internal subdirectories
(`testsuite/`, `packages/`, `compat/`, `Win32/`, etc.) do not, since
they're vendored upstream code rather than AMLP content.

Local build config: `local_options.nm3` (this build's actual option
overrides) and `options.h` (the full option set, mostly upstream
defaults) - both preserved as shipped for reference, not applied to any
AMLP build.
