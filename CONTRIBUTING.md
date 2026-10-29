# Contributing

This project is a playable alpha. Priorities are hardware acceptance, full-run stability, portable builds and measured rendering/loading improvements.

## Bug reports

Include package version, game build identity (hash rather than an APK attachment), Vita model and firmware, relevant plugins, actual clock settings, steps to reproduce and expected versus observed behaviour. Mention whether the problem survives a fresh launch.

Provide a short, reviewed log excerpt or screenshot when useful. Remove personal paths, network addresses and profile information. Do not upload game binaries, assets, full memory dumps or personal saves by default.

## Changes

Keep changes focused and explain the evidence behind them. Preserve original behaviour when a patch cannot verify its expected binary layout. Changes to cached text must account for dynamic values, colors, scale, positioning and invalidation.

For performance work, compare the same scene, save, clocks and build with the feature disabled and enabled. Report frame-time distributions and memory costs, not only average FPS. Repeat measurements before claiming a small improvement. Collect screenshots outside timed measurement windows.

Distinguish host/ARM harness coverage from real hardware coverage. Modelled operating-system calls and passing geometry tests do not establish complete game or GPU correctness.

Do not buy items or advance a user's test save during automated benchmark work. Use explicitly designated test data and verify restoration.

Describe AI-assisted changes accurately. Generated code still requires review, provenance checks and meaningful tests.

Run tools/run_public_checks.py and tools/test_publication.py before submitting.
Use explicit external inputs for tools/run_private_checks.py. Never run console
automation from tests. Keep the current source/data boundary and exact-APK guards.
