# Retained optimizations and switches

The runtime comes from development commit 7e7f12c. Production configuration
retains the smaller text caches. No rejected large-cache or integer-text
experiment was enabled during publication preparation.

The last recorded hardware configuration enabled these three files under
`ux0:/data/spireandroid/` (empty files are sufficient):

- `enable-buffer-pointer.flag`: exact-APK Buffer backing-pointer fast path.
- `enable-text-rotation.flag`: guarded rotated text path.
- `enable-grid-replay.flag`: retained grid command replay mode.

The source candidate preserves their opt-in behavior. Install these only on the
isolated test-data copy when reproducing that configuration. Existing flags are
not automatically changed. Remove a flag and fully restart to test without it.
Do not enable the separate `enable-buffer-pointer-test.flag`, high-frequency
`enable-sprite-profile.flag`, large-cache or integer-text experiments as part of
normal acceptance. Preserve a record of the original flags/settings for rollback.

Native audio, direct cache, text submission and the loading pack retain their
existing default-on paths, with the original disable flags. Unsupported binary
layouts preserve fallback or fail configuration according to the existing code.
The Buffer path samples the original pointer and locks itself out on mismatch.

Historical reference measurement: 38.71–39.85 FPS after restart, compared with
36.86 FPS with no Buffer hook on one five-card Guardian scene. The instrumented
A/B figure is not the delivered net gain. No new speed claim is made for this
reconstructed candidate. Measure the same scene/save/clocks/settings on Vita.
