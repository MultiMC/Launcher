# Updating a Modrinth instance

Select an instance and click **Update mods**, next to **View Mods**. The launcher
checks for a newer stable release of its Modrinth modpack. Minecraft's version
and loader family stay fixed; a required loader-version change appears in the
preview. Confirm **Update** to download changed files, back up the instance,
and install the release in place.

This updates the pack author's published selection of mods. It does not select
the newest release of every individual mod, and it does not update CurseForge
packs or change Minecraft versions.

## Existing instances

Supported Modrinth imports record their project and release automatically. If
baseline capture is unsupported, import still succeeds and logs the reason; the
updater requires explicit linking. For an
older instance, the first update asks for the Modrinth project URL/ID and the
release originally installed. Pack mods must match that release's checksums.
Extra mods are preserved. If a pack mod was manually replaced or deleted, the
dialog identifies it: restore the original file or choose the correct original
release, then retry. Linking does not infer ownership from filenames.

A raw `.mrpack` import retains a baseline but still requires this one-time
project/release selection, because the archive format does not guarantee a
Modrinth project/version ID.

## Preserving preferences and recovering

- Existing `config/`, `defaultconfigs/`, `options*`, `servers.dat`, saves,
  screenshots, logs, and crash reports are never overwritten by the updater.
- Missing defaults can be installed. Existing user-created files are not claimed
  as pack-owned. Locally modified non-mod pack files are preserved and listed.
- Unmodified pack-owned resource packs and support files can be updated or
  removed. Obsolete pack-owned mods are removed; extra user mods remain.
- Enabled/disabled states follow tracked mods across filename changes when a
  stable identity is available. Ambiguous disabled mods stop the update rather
  than silently changing their selection.
- Active top-level JARs are checked for duplicate Fabric, Quilt, Forge,
  NeoForge, or legacy `mcmod.info` IDs. Unrecognized active JARs block the update
  with an actionable message. Nested loader dependencies are left to the loader.
- The updater never merges arbitrary configuration formats. A new mod can still
  interpret or migrate an existing configuration differently when Minecraft is
  launched. Preserving bytes is not a compatibility guarantee.

**Restore previous version** restores files changed by the latest update and
removes files it added. Other files, including new saves, are left alone. The
confirmation explains that any subsequent edits to those affected files will
be replaced. The full pre-update snapshot under `.pack-update/backup-<id>` (identified by the recovery record) is available
for manual recovery of configurations a mod migrated after launch.

An interrupted installation leaves a recovery journal and blocks launch. Click
**Update mods** or **Restore previous version** to recover before playing.
Downloads can be cancelled. Installation and restoration cannot be cancelled
mid-transaction; errors trigger rollback. A damaged backup stops recovery
without discarding its journal.

Snapshots and the verified download cache are stored inside the
instance's `.pack-update` directory. Each update gets a distinct backup so a
failed subsequent update cannot destroy the previous restore point. Snapshots
are retained for manual recovery and consume disk space. Copying or exporting
an instance excludes this private updater directory. Do not edit an instance from another program
while an update or restore is running. Symlinked instance content is rejected.

## Developer design

`modplatform/modrinth/PackUpdate` is a synchronous, UI-independent core with:

- archive/manifest validation and game/loader compatibility checking;
- version selection, baseline capture, deterministic planning, and collision
  checks;
- content verification, disk-space preflight, snapshotting, journaled commits,
  rollback, and recovery.

`ui/dialogs/PackUpdateDialog` coordinates the Modrinth API, existing `NetJob`
downloads/checksum validators, review dialogs, and background disk tasks. A
content-addressed cache reuses verified payloads. The `.mrpack` archive is
fetched before review because it contains the manifest and overrides; individual
changed payload downloads begin after confirmation.

`modrinth-update.json` is versioned internal per-instance state (`schema: 1`):
project ID, installed version ID, original manifest, and managed-file records
(path, expected hash/algorithm, stable identity, optional flag, override flag).
Enabled/disabled state is recorded by the actual installed path and rechecked
against disk. Unknown state versions fail closed. Only a successful installation
commits the target release metadata.

All write targets are relative to validated instance/game roots. Archive paths,
case-insensitive duplicate paths, symlinks, hashes, and client environments are
validated before use. `QSaveFile` performs individual replacements, and
`.pack-update/journal.json` makes a multi-file interruption recoverable. Launch
is gated on the active update lock/journal as well as in-memory busy state.

## Build and test

Use the platform prerequisites in `BUILD.md`: CMake, Qt 5 development packages,
zlib, a C++ compiler, and JDK 8 or 11. JDK 21 cannot compile the existing Java 7
launcher helpers. Initialize both Git submodules first.

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
```

On macOS, select the JDK explicitly if a newer JDK is the system default:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/Qt \
  -DJava_JAVA_EXECUTABLE=/path/to/jdk-11/Contents/Home/bin/java \
  -DJava_JAVAC_EXECUTABLE=/path/to/jdk-11/Contents/Home/bin/javac \
  -DJava_JAR_EXECUTABLE=/path/to/jdk-11/Contents/Home/bin/jar
```

The same tests are registered on Windows (MinGW), Linux, and macOS. For headless
runs set `QT_QPA_PLATFORM=offscreen`. `PackDownload` and the upstream QuaZip suite
need permission to bind a localhost mock HTTP server. They make no external
network requests. To run only the new deterministic tests:

```sh
ctest --test-dir build -R 'Pack(Update|Download)' --output-on-failure
```

The tests cover protected settings, custom files, disabled mods, unchanged files,
modified/missing originals, collisions, support-file updates, stable release
selection, incompatible game/loaders, traversal, duplicate IDs, bad archives,
insufficient space, failed commits, corrupt backups, interrupted recovery,
verified HTTP downloads, bad checksums, server errors, and cancellation.

### Optional real-pack acceptance

Python 3 and curl fetch public fixtures only when explicitly invoked. The files
are downloaded into a disposable cache; this does not inspect an installed
instance or launch Minecraft. Budget about 300 MiB for downloads and additional
space for temporary instance/staging/backup copies.

```sh
python3 tests/modpack-updater/fetch_skyblock_fixtures.py /tmp/skyblock-fixtures \
  --old ZLHoupi8 --new 3kzGWMEi --download-files
MULTIMC_SKYBLOCK_FIXTURES=/tmp/skyblock-fixtures \
  ctest --test-dir build -R '^PackUpdate$' --output-on-failure
```

Without explicit IDs, the script selects the newest distinct stable pair sharing
a game version and loader. The test validates the manifests' exact dependencies,
updates a disposable instance, checks settings/user-mod/disabled-state retention,
checks a second update is a no-op, and restores the previous release.

Validated locally on macOS with Qt 5.15.2 and JDK 11: full native launcher build,
mocked download tests, and SkyBlock Enhanced **5.4.0 → 5.4.1** (Minecraft 26.2,
Fabric 0.19.5), with **14 writes and 11 removals**. Windows/Linux execution and an
authenticated Minecraft launch are not verified by this development run. Before
shipping, repeat the build/tests on both platforms and launch a disposable copy
of the updated pack using a valid Minecraft account.

This is a source implementation. Upstream acceptance, release signing, and
public binary distribution are separate from these changes.
