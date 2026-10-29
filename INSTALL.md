# Private local installation / update

Title ID SPAV00001, version 00.72. The source-preparation VPK has a distinct name,
but uses the existing bubble/data paths; it is not a separate application.
Use the already working homebrew/VitaShell environment with the compatible
kubridge and shader-compiler setup. Record exact firmware/plugin versions in
acceptance results; the preparation tools do not install or alter them.

Close the game. Independently copy and verify the entire
`ux0:/data/spireandroid/` directory and retain the currently working VPK.
Isolate the original data under a unique directory name and run acceptance on a
verified copy at the normal path. Do not overwrite/merge the originals or
uninstall the bubble. Stop if same-version installation is refused.

## Required data mapping

| Local prepared input | Vita destination |
| --- | --- |
| libSpire_ANDROID.so | ux0:/data/spireandroid/libSpire_ANDROID.so |
| assets/ (entire tree) | ux0:/data/spireandroid/assets/ |
| textures/data.bin | ux0:/data/spireandroid/texture-cache-033/data.bin |
| textures/index.bin | ux0:/data/spireandroid/texture-cache-033/index.bin |

The VPK already contains direct-index.bin, loading-pack.bin, launcher-splashes.bin
and the shell images. For an existing installation using the exact supported APK,
the engine/assets need not be recopied if their identity has been verified. The
**new texture pack and index must be installed together with the new VPK** because
pack ordering/cache identity differ from the development build. Do not carry an
old direct-cache receipt into an unverified dataset; the runtime validates its
identity and regenerates receipts. No user settings/preferences/saves are supplied.

See OPTIMIZATIONS.md for the three opt-in flags matching the retained configuration.
Do not copy development diagnostics or enable additional experiments. Keep original
launcher settings and flag files recoverable. No files are transferred automatically.

After tests, close the game, preserve test results separately and restore the
original directory name without merging. Restore the former VPK/cache together
when rolling back. Never upload APK, game .so, assets, caches, VPK or saves publicly.
