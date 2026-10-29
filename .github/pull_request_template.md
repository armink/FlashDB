<!--
  English or Chinese, both fine. Keep it short. Write N/A for items that do not apply
  instead of deleting them, and make sure the ticked boxes match reality.
-->

## What this changes

<!-- What problem does it solve, and why this way? Not just "what changed". -->

## Related issue

Closes #

## Test closure

<!-- A change under src/ must ship tests, see AGENTS.md and .github/copilot-instructions.md -->

- [ ] Added or updated cases in `tests/fdb_kvdb_tc.c` or `tests/fdb_tsdb_tc.c`
- [ ] New cases are registered in the `UTEST_UNIT_RUN(...)` list of `testcase()`
- [ ] Cases cover boundaries (sector full / GC triggered / blob crossing write granularity / init check failure)
- [ ] Bug fix: the case failed before the fix and passes after, and its name carries the issue number
- [ ] No new case needed, reason: (pure comment / docs / formatting, or a behaviour-preserving refactor already covered)

## Write granularity covered

<!-- CI runs all six; say so if the change was only validated at one value -->

- [ ] CI passed for `FDB_WRITE_GRAN` = 1 / 8 / 32 / 64 / 128 / 256
- [ ] Local manual verification only, granularities:
- [ ] Not verified, reason and risk:

## Data layout and compatibility

- [ ] KV/TSL node format unchanged
- [ ] Node format changed: version bumped, old data migrates by
- [ ] Public struct or API in `inc/flashdb.h` changed, compatibility impact:
- [ ] API or config macro changed, and `docs/` plus `docs/zh-cn/` are updated

## Power-loss semantics

- [ ] Not on a write path
- [ ] On a write path, covered by the `fdb_xxx_deinit` -> `fdb_xxx_init` reboot simulation
- [ ] On a write path, not coverable by utest, verification method: