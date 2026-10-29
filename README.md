# Slay the Spire Vita — source preparation candidate

Unofficial experimental Android ARMv7 loader/adaptation for PlayStation Vita,
maintained by AlexisRqs with AI assistance. The original game is not included.
The repository remains private while preparing source publication.

This candidate integrates the latest development snapshot
`7e7f12c4c7c84ff19dbc8fc5b940e4acba4c51f1` with the portable resource/dependency
pipeline from the earlier 0.66 publication preparation. Its application version
remains 00.72 and title ID SPAV00001; identify this source-preparation build by
commit and VPK SHA-256, not the version number alone.

## Included recent work

- Native streaming sound/music and lifecycle handling.
- Background startup launcher, local logos and guarded loading texture subset.
- Text layout/vertex reuse and guarded rotated card text rendering.
- Guarded grid command replay and Buffer backing-pointer fast path.
- Exact-binary guards, fallback paths and optional diagnostic profiling.

All current runtime/vendor files were retained. The only runtime source edit for
publication is the generated direct-cache header's include lookup; no gameplay,
optimization, clock or save behavior was rewritten for publication.
Optimization flags retain their development semantics. See OPTIMIZATIONS.md;
merely installing the VPK does not enable every opt-in optimization.

## Build

See [BUILDING.md](BUILDING.md). Supply a compatible local APK, prepare resources
outside Git, build pinned dependencies into a dedicated VitaSDK, and compile.
No game APK/.so, art, cache, save, SDK or VPK is tracked. Game-derived LiveArea
images, logos and cache packs are generated locally. The output VPK is private
and is not a proposed public binary release.

## Validation and limits

The development snapshot was tested on Vita; a reference five-card Guardian
scene measured about 38.7–39.9 FPS versus 36.9 without the Buffer hook. These are
historical scene-specific observations, not a new benchmark or minimum FPS.
The newly reconstructed publication candidate needs separate hardware acceptance.
No complete campaign, all-character run or universal firmware compatibility is claimed.

ROM/APK-free checks: `python3 tools/run_public_checks.py` and
`python3 tools/test_publication.py`. Exact-engine/ARM checks are optional and use
explicit external inputs (BUILDING.md). They simulate OS/GPU services and do not
substitute for actual rendering, audio, sleep/resume and save tests on hardware.

See [INSTALL.md](INSTALL.md), [HARDWARE-TEST.md](HARDWARE-TEST.md),
[PUBLICATION-STATUS.md](PUBLICATION-STATUS.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

## License and credits

MIT covers independently authored original additions only. Vendored components
retain their own licenses; see [LICENSING.md](LICENSING.md) and
[THIRD_PARTY.txt](THIRD_PARTY.txt). No rights to Slay the Spire or its assets are
granted. This project is not affiliated with its original developers/publishers.
