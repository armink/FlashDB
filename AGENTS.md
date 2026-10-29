# AGENTS.md

Instructions for AI coding agents working in this repository (Copilot coding agent,
Claude Code, Codex, ...). Review-specific checks live in `.github/copilot-instructions.md`.

## What this project is

FlashDB is an ultra-lightweight embedded database for MCUs, plain C99, relying on FAL
(Flash Abstraction Layer) to reach flash. It provides KVDB (key/value) and TSDB (time
series) on top of raw flash or a file.

## Environment and verification

This repository **cannot be built on its own**: `src/` and `inc/` are library sources and
the tests need an external BSP project.

CI (`.github/workflows/main.yml`) does the following:

1. clones `armink/FlashDBAutoTestBSP` and `armink/UtestRunner`;
2. copies `src/*`, `tests/*`, `inc/fdb_def.h`, `inc/fdb_low_lvl.h` and `inc/flashdb.h` into
   the BSP's `packages/FlashDB/`;
3. cross-compiles with `scons --global-macros="FDB_WRITE_GRAN=<n>"`;
4. runs `qemu_runner.py` under QEMU and reads `rtt_console.log` for the result.

`FDB_WRITE_GRAN` takes `1, 8, 32, 64, 128, 256`; CI covers all of them.

So:

- with network and scons available, reproduce CI locally with the steps above;
- without that environment, **do not claim tests passed**; state "not verified" and say which
  granularities still need a run;
- do not treat compiling `src/*.c` on an x86 host as verification: `fdb_cfg.h` has to be
  provided by the user, and many problems only appear at a specific granularity.

## Layout

- `inc/` public headers, `flashdb.h` is the only entry point
- `src/` implementation, `fdb_read_cache.c` is the optional read cache
- `port/fal/` vendored third-party FAL, **read-only, do not modify**
- `tests/` utest cases
- `demos/` per-platform examples
- `docs/` English and Chinese documentation, must be updated when an API changes

## Hard rules

1. **Every change ships with test closure.** Touching `src/` means touching
   `tests/fdb_kvdb_tc.c` or `tests/fdb_tsdb_tc.c` in the same commit, and registering the new
   case in the `UTEST_UNIT_RUN(...)` list of `static void testcase(void)`. For a bug fix,
   write the failing case first (its name carries the issue number, e.g.
   `test_fdb_github_issue_249`), then change the implementation. Pure comment / pure `docs/` /
   pure formatting changes are exempt, but explain why behaviour is unaffected; a pure
   refactor may skip new cases but must name the existing ones that cover it.
2. C99 only: no C++, no C11 atomics, no compiler builtins.
3. All flash access goes through `fal_flash_*`; never call the chip HAL directly.
4. Do not add heap allocation. The only allocation point is `src/fdb_read_cache.c`,
   overridable via `FDB_READ_CACHE_MALLOC` / `FDB_READ_CACHE_FREE`.
5. Feature code goes inside `#ifdef FDB_USING_<FEATURE>`; a new config macro must be added to
   `inc/fdb_cfg_template.h` with its valid range documented.
6. Do not change public API names or signatures without stating the compatibility impact first.
7. A KV/TSL node format change must bump the version and describe how old data migrates.
8. New files keep the Apache-2.0 license header (copy an existing source file).
9. Comment only where "why" needs explaining; do not restate the code. Source comments and
   commit messages are in English.

## Definition of done

1. The case covers the changed behaviour and its boundaries: sector full, KV nodes full
   triggering GC, blob length crossing the write granularity, TSDB full, init check failure.
2. Power-loss related changes are verified with a `fdb_xxx_deinit` -> `fdb_xxx_init` reboot
   simulation.
3. Cases never hard-code the write granularity; derive lengths with `FDB_WG_ALIGN()` /
   `_TKV_W`, because CI runs `FDB_WRITE_GRAN` = 1/8/32/64/128/256.
4. When header layout changes, update the `_TKV_*` macros in the tests too, otherwise cases
   compute wrong addresses and pass silently.
5. Before committing, make sure no case was left unregistered (missing `UTEST_UNIT_RUN`).

## Code style

4-space indent, no tabs, Allman braces; `fdb_xxx` public, `_fdb_xxx` cross-file internal,
`static` for file-local. See `.github/copilot-instructions.md`.

## Commit messages

Conventional Commits, following existing history:

```
feat(kvdb): add a transient sector read cache for the loading phases (#433)
fix(tsdb): fix deadlock when not-format init check fails (#427)
docs: sync the docs with the recent code changes
```

Format: `type(scope): lowercase imperative description`, scope usually `kvdb`, `tsdb`, `fal`,
`docs` or `test`. One commit does one thing; do not reformat untouched code along the way.