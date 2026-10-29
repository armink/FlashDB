# FlashDB code review instructions

FlashDB is an ultra-lightweight embedded database for MCUs, written in plain C99
(no C++, no compiler extensions, no C11 atomics). It provides two databases on top of
raw NOR/NAND flash or a file:

- KVDB - key/value store, see `src/fdb_kvdb.c`
- TSDB - time series store, see `src/fdb_tsdb.c`

Keep two constraints in mind while reviewing: RAM and latency are hard limits, and any
change touching the flash write path or the KV/TSL node format is a data compatibility
change for devices already in the field.

## Layout

| Path | Contents |
|---|---|
| `inc/flashdb.h` | the only public header; every public API is declared here |
| `inc/fdb_def.h` | public types, error codes, config macro defaults |
| `inc/fdb_low_lvl.h` | internal low-level flash operations |
| `inc/fdb_cfg_template.h` | configuration template, copied to `fdb_cfg.h` by the user |
| `src/fdb.c` | init, control commands, blob helpers |
| `src/fdb_kvdb.c` / `src/fdb_tsdb.c` | the two database implementations |
| `src/fdb_read_cache.c` | optional transient sector read cache |
| `src/fdb_file.c` | file backend |
| `src/fdb_utils.c` | CRC32, string and status-bit helpers |
| `port/fal` | vendored third-party Flash Abstraction Layer - **do not review, do not suggest changes** |
| `tests/*_tc.c` | utest cases, executed by CI under QEMU |
| `docs/` | docsify documentation, `docs/zh-cn` is the Chinese edition |

## Conventions

- C99, 4-space indent, no tabs; functions use Allman braces (opening brace on its own line).
- Public API is `fdb_<object>_<verb>` (`fdb_kv_set`, `fdb_tsl_append`); cross-file internal
  functions start with an underscore (`_fdb_flash_write`); file-local functions are `static`.
- Type names end in `_t` (`fdb_kvdb_t`). Every public function carries a Doxygen block with
  `@param` / `@return`, plus a `@note` whenever power loss, blocking or alignment applies.
- Errors are returned as `fdb_err_t`; never print and ignore.
- Logging goes through `FDB_INFO` / `FDB_DEBUG` / `FDB_ASSERT` and the file's own `FDB_LOG_TAG`.
- Feature code is wrapped in `#ifdef FDB_USING_<FEATURE>`. A new config macro must also be
  added to `inc/fdb_cfg_template.h`, documenting its valid range and constraints.
- New files keep the license header (year of creation):

  ```c
  /*
   * Copyright (c) 20XX, Armink, <armink.ztl@gmail.com>
   *
   * SPDX-License-Identifier: Apache-2.0
   */
  ```

## What to look for (matters more than style)

- **Write granularity.** `FDB_WRITE_GRAN` is 1/8/32/64/128/256 bits. A write whose length is
  not a multiple of the granularity, or two writes into the same granularity unit without an
  erase in between, corrupts flash. CI runs all six values; a change validated only at the
  default value easily breaks the others.
- **Erase before write.** NOR can only turn 1s into 0s. Any path that must turn a 0 bit back
  to 1 needs an erase first.
- **Power-loss consistency.** The status bits in `fdb_utils.c` exist so that an interrupted
  write is detectable. A new write sequence without a recoverable status bit, or a node
  format change without a version bump, is a defect.
- **Alignment and overflow.** Addresses are `uint32_t` and lengths are `size_t`; check
  `addr + size` for overflow and check reads/writes that cross a sector boundary.
- **Concurrency.** Erase/program paths may run with the scheduler locked or from an ISR: no
  blocking calls, no allocation of unbounded size on that path.
- **RAM.** Target devices have only a few KB of RAM. A new sector-sized buffer or cache needs
  a comment explaining why, plus a config macro that can turn it off.
- **Compatibility.** Do not suggest renaming or reordering public struct members; users keep
  `fdb_kvdb` control blocks on the stack. A struct layout change must be called out explicitly.

## Do not suggest

- C++, C11 atomics or compiler builtins: the code must build with `arm-none-eabi-gcc` and old
  vendor toolchains.
- Bypassing FAL and calling the chip HAL directly. All access goes through
  `fal_flash_read/write/erase`, which is what makes the file backend possible.
- Formatting-only churn on untouched code (line re-wrapping, brace moves).
- Changes to `port/fal/**`: report the issue to the FAL upstream instead.
- The project has exactly one heap allocation, the one in `src/fdb_read_cache.c` overridable
  via `FDB_READ_CACHE_MALLOC` / `FDB_READ_CACHE_FREE`. Do not introduce a second allocation
  source in a new module.

## Test closure (must check)

**Every change must ship its tests in the same PR. A `src/` change without a matching test
change must be called out.** This is a hard requirement of this repository, not a suggestion.

How to judge:

- A change under `src/` with no corresponding change in `tests/fdb_kvdb_tc.c` or
  `tests/fdb_tsdb_tc.c` -> raise it.
- Pure comment, pure `docs/`, pure `demos/`, or pure formatting changes may ship without
  tests, but the author must explain why behaviour is unaffected.
- A pure refactor (behaviour unchanged) may add no new case, but the author must name the
  existing cases that cover it, and the reviewer must confirm those cases actually reach the
  changed path, rather than just matching file names.
- A new public API needs a case; a behaviour change needs a case proving the difference from
  the old behaviour.
- Bug fixes follow "test first": the case fails before the fix and passes after, and its name
  carries the issue number (existing convention, see `test_fdb_github_issue_249` in
  `tests/fdb_tsdb_tc.c`).
- A new `static void test_xxx(void)` must be registered in the `UTEST_UNIT_RUN(...)` list of
  `static void testcase(void)` in the same file, otherwise it never runs. This is the most
  common omission - check exactly there.
- Cases must cover boundaries, not only the happy path: sector full, KV nodes full triggering
  GC, blob length crossing the write granularity, TSDB full, DB init check failure.
- Power-loss semantics must be verified with a `fdb_xxx_deinit` -> `fdb_xxx_init` "reboot
  simulation"; this is the existing pattern (see `tests/fdb_tsdb_tc.c`) and new cases should
  follow it.
- Verify that the boxes ticked in `.github/pull_request_template.md` match reality: a claim of
  "tests added" with no `tests/` change, a claim of a green six-granularity CI run with no
  build to back it, or checklist items deleted instead of marked `N/A`, all need to be raised.

## Test case style

- **Never hard-code the write granularity.** Derive lengths with `FDB_WG_ALIGN()` and the
  local `_TKV_W` (`((FDB_WRITE_GRAN + 7) / 8)`), otherwise a case only holds for one
  `FDB_WRITE_GRAN` value.
- The `_TKV_*` layout macros inside the test file (`_TKV_SEC_HDR_SZ`, `_TKV_KV_HDR_SZ`, ...)
  are derived from `FDB_STORE_STATUS_TABLE_SIZE` and `FDB_DIRTY_STATUS_TABLE_SIZE`. When those
  node definitions change they must be updated in lockstep, otherwise a case computes wrong
  addresses and passes silently.
- Each test file is wrapped in `#if defined(RT_USING_UTEST) && defined(FDB_USING_KVDB)` (TSDB
  likewise), and the timeout passed to `UTEST_TC_EXPORT(...)` (20 s for KVDB, 60 s for TSDB) is
  relied on by CI. Check whether a slow new case needs it raised.

## CI today

`.github/workflows/main.yml` cross-compiles six `FDB_WRITE_GRAN` values
(1/8/32/64/128/256) and runs the utest cases from `tests/` under QEMU. Therefore:

- validating a single granularity does not prove correctness; ask the author to confirm all six;
- a KV/TSL node layout change must also state how old data migrates (version bump /
  `FDB_KV_AUTO_UPDATE`);
- if a changed path is not covered by CI (for example a path only reachable through the file
  backend), the author must describe the manual verification.