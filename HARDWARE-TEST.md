# Hardware acceptance — source-preparation candidate

Status: PENDING. No console access or personal save access during preparation.
Record source commit, VPK SHA-256, data pack hashes, model/firmware/plugins,
clock settings and enabled flags. Use only isolated copies described in INSTALL.md.

1. Install the matched VPK/cache pair. Test cold startup and background launcher,
   native logos, readiness, menu entry and LiveArea art. Record loading duration.
2. Check music and sound effects, volume, transitions, sleep/resume and full restart.
3. Enable the three retained flags (OPTIMIZATIONS.md). Check startup logs for
   configuration failures or mismatch lockout; remove experimental profiling flags.
4. Load a copy of an existing run. Inspect a five-card hand: text, rotation,
   colors, dynamic numbers and hover. Play a card with touch and normal controls,
   end the turn and verify subsequent cards/enemy intent. Check upgrade grid,
   map, shop and collection; compare visual correctness before performance.
5. Verify each unlocked character remains selectable and preserves unlocks.
   Test profile changes, relic collection and save/continue across full restart.
6. Play at least 30 minutes. Note crashes, stalls, memory pressure, sound loss,
   visual corruption or input issues. No full-run guarantee follows from this check.
7. For a performance comparison, use the same scene/save/clocks/preferences,
   identical flags except the optimization under test, and multiple complete
   windows. Keep screenshots outside timing. Report slower runs as well.

Stop on unexpected save/profile behavior. Retain test data privately; restore
originals without merging. Public bug reports should contain reproduction steps,
version and reviewed log excerpts, not saves, APKs or full memory dumps.
