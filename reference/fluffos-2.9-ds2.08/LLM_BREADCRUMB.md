# LLM breadcrumb - fluffos-2.9-ds2.08/

1. Read `/CLAUDE.md` at the repo root first.
2. Read `README.md` in this directory (above).
3. This tree has no gameplay-mudlib sibling and no build relationship to
   the rest of the repo - it exists purely as a citable reference copy of
   real FluffOS 2.9 source, not something AMLP builds, runs, or installs.

## Gotchas specific to this directory

- This is vendored upstream C driver source, not AMLP content. Changes
  here should be rare and deliberate, and only ever made to correct the
  vendored copy itself (e.g. a citation turns out to be wrong) - AMLP's
  own driver work happens under `src/`/`include/` (C++), not here.
- No README.md/LLM_BREADCRUMB.md pairs exist for the subdirectories of
  this tree (`testsuite/`, `packages/`, `compat/`, `Win32/`, etc.) by
  design - don't add them unless that scoping decision changes.
- `options.h` and `local_options.nm3` document this vendored copy's own
  as-shipped build configuration; they describe how real FluffOS was
  once built, not anything AMLP's own build does.
- Nothing in this directory is compiled by AMLP's `CMakeLists.txt`. If you
  ever wire this tree into AMLP's own build, this breadcrumb and the
  README's "kept for citation only" framing both need a real rewrite, not
  just a note.
