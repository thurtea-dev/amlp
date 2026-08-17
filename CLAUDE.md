# CLAUDE.md

Instructions for Claude Code when working in this repository.

## Non-negotiable rules

These two rules have governed this project's entire development history and
must continue unchanged.

1. **Never run `git commit` or `git push`, under any circumstance, for any
   reason.** Stage with `git add` only. When work is done, report what
   changed and provide a complete, ready-to-use commit message for the human
   to run manually. Never include a `Co-Authored-By` line in any commit
   message you draft.

2. **No em dashes and no emojis anywhere** in code, comments, commit
   messages, or documentation, unless there is an actual syntax or code
   reason. Use a period, comma, or start a new sentence instead.

## Orientation

- `ROADMAP.md` is the master phase/row tracker. Its checkboxes are the
  authoritative signal for what is actually done.
- `STATUS.md` is the dated development log, most recent entries first.
  Older entries are archived in `STATUS-ARCHIVE.md`.
- `prompt.md` is a menu of ready-to-run task prompts for specific rows.
  It has a track record of describing some subsystems incorrectly, so
  verify anything it says against `reference/fluffos-2.9-ds2.08` (the
  vendored real FluffOS 2.9 source) before implementing anything it
  describes.
- Every `src/<module>/` has its own `instruct.md` describing that module's
  task backlog. These frame tasks as open regardless of actual completion
  status, so they are **not** a live status signal. Only `ROADMAP.md`'s
  checkboxes and `STATUS.md`'s dated entries are.
