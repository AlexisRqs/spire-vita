# Build and validation

Validated on macOS ARM64. Use Python 3.11+, host C compiler, Git, Make, CMake and
a hard-float VitaSDK. Other host platforms remain unverified. Paths used by the
upstream SDK Makefiles should not contain spaces.

## Dedicated SDK and dependencies

The tested SDK archive SHA-256 is
`b081aaae2a9e870f8cdb8899f9fbd6e8da08d9f207832b953d7e277a0507bcbc`.
SDK-VERSION.txt records its components. SDK releases are available from
https://github.com/vitasdk/autobuilds/releases; a moving latest archive is not a
pinned equivalent. The compiler toolchain itself is not rebuilt from source.

```sh
export VITASDK=/absolute/path/to/dedicated/vitasdk
export PATH="$VITASDK/bin:$PATH"
python3 tools/bootstrap_dependencies.py --sdk "$VITASDK" --work /external/spire-dependencies
```

This installs libraries only into the specified SDK. Exact commits are in
DEPENDENCIES.json. taiHEN import stubs, GNU-C11 SceShaccCgExt, math-neon,
vitaShaRK and patched vitaGL are built. The complete vitaGL-local.patch includes
all development modifications; apply it once. No kernel plugin is installed.
An optional `--source-cache` directory supplies local Git repositories; pins are
still checked in fresh checkouts. The validated reconstruction used that mode.

## Private resources

Use your own compatible APK. Its engine must have SHA-256
`619e71cd19c9209dff17d943f94a6e14f9f6e899add6f3b33c14f909594f9d18`.
Other game binaries fail extraction; version labels alone are insufficient.
No game file is downloaded. Inputs and outputs must be outside every Git checkout.

```sh
python3 -m venv /external/spire-tools
/external/spire-tools/bin/python -m pip install -r requirements-assets.txt
/external/spire-tools/bin/python tools/prepare_private_resources.py   --apk /external/your-copy.apk --output /external/spire-private
```

Always choose a new output directory. This extracts the engine and assets,
converts 2,308 textures, generates a deterministic pack/index and matching C
identity header, builds the 486-entry loading subset, renders native startup
logos, and creates the LiveArea images from the local title layers. The image
composer uses Pillow/texture2ddecoder, installed separately at pinned versions.
The completed manifest hashes all required packaging inputs.

`--assets /external/extracted/assets` is an alternative to `--apk` for already
extracted assets; that mode does not supply/validate the engine. Use the APK
workflow for a complete installation. All generated files are private.

## Compile from a clean clone

```sh
cmake -S . -B /external/spire-build   -DSPIRE_PRIVATE_DIR=/external/spire-private
cmake --build /external/spire-build --parallel 6
```

CMake checks the resource manifest, cache identity, loading-subset identity,
logo layout and shell image dimensions, then copies the generated identity header
into the build directory. No manual source edit or old object is needed.
Output: `spire_android_probe` (ELF), `eboot.bin` (SELF),
`spire-vita-sourceprep-0.72.vpk`. Keep these and the generated resources private.
The VPK contains derived art/cache subsets; the original engine and bulk assets
remain external. A complete game installation also needs the matching texture
pack/index; see INSTALL.md. Never mix a new VPK index with an old pack.

## Checks without the game

```sh
python3 tools/run_public_checks.py
python3 tools/test_publication.py
python3 tools/check_publication.py --history
```

The public suite covers resource rejection, UTF-8, input, file adapters, text
configuration/submission, profiling contracts, grid commands, audio mixer and
wave/IMA decoding under ASan/UBSan. It uses only synthetic data. The large-text-cache
experiment remains compiled out of the production candidate; the test verifies
that the flag cannot silently enlarge the retained cache.

## Optional private checks

Install requirements-tests.txt in your external Python environment. Then:

```sh
/external/spire-tools/bin/python tools/run_private_checks.py   --engine /external/spire-private/libSpire_ANDROID.so   --elf /external/spire-build/spire_android_probe   --resources /external/spire-private   --output /external/spire-test-results
```

Seven exact-engine/ARM/configuration harnesses cover Buffer ownership/GC guards,
33 profiling trampolines, rotated text, layout variants and text submission.
Cache tests verify actual regenerated payloads, corrupted inputs, fallback and
single-preload lifecycle using scratch files. Output must be new and external.
These tests do not use personal saves or contact a console.

Local reconstruction does not certify hardware audio, GPU output, suspend,
performance, all characters or full runs. Follow HARDWARE-TEST.md afterward.
