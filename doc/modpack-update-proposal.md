# Modpack updates: behavior proposal

Status: discussion draft, not an agreed design or a description of implemented
behavior. This responds to the request in
[PR #5793](https://github.com/MultiMC/Launcher/pull/5793#issuecomment-6724342565)
to define pack updates before proposing an implementation. The code in that PR
is an experiment; it should not determine the answers below.

The [November 12 feedback](https://github.com/MultiMC/Launcher/pull/5793#issuecomment-6835137839)
asks for planning across pack formats and a discussion of implementation order
on Discord. The maintainer is also working on a new launcher update mechanism
to support experiments without affecting users who do not want them. Its design
and availability are not established here. Agree on how this work fits that
effort before adding runtime changes or exposing the prototype to users.

## Problem and scope

A player has installed a pack release, changed settings, and possibly added,
disabled, removed, or replaced mods. They want to move to another release of
that pack without manually reconstructing those choices or losing their worlds.

An update follows a pack author's published release. Updating individual mods
to their latest versions is a separate operation. Preserving files does not
guarantee that the resulting mods will work together or interpret settings in
the same way.

The proposal covers explicitly requested updates across pack formats. The first
implementation's formats and order need agreement; Modrinth is the prototype's
format, not a settled first milestone. Keeping Minecraft and loader family fixed
is still a proposed initial limit. Minecraft migrations, loader-family changes,
automatic updates, and arbitrary configuration merging are deferred.

## Differences between pack sources

The existing import paths provide concrete cases to discuss. This is an inventory
of this checkout, not a claim that each source already supports pack updates.

| Existing path | What the shared behavior must account for |
| --- | --- |
| Modrinth (`InstanceImportTask::processModrinth`) | Manifest files, overrides, and client requirements; a local archive need not identify a remote project/release. |
| ATLauncher (`ATLPackInstallTask`, `ATLPackManifest`) | Optional selections, dependencies, separate config archives, and extraction rules; installation is more than copying a list of JARs. |
| Technic (`SolderPackInstallTask`, `SingleZipPackInstallTask`) | Solder builds download and extract multiple archives; a single-ZIP pack follows a different path. Download identity is not installed-file identity. |
| Legacy FTB and FTB App import (`LegacyFTB::PackInstallTask`, `ImportFTB::PackInstallTask`) | Installing a published pack and copying an already installed directory provide different evidence of original content and user changes. |
| MultiMC instance archive (`InstanceImportTask::processMultiMC`) | An instance snapshot does not by itself establish an update source or distinguish pack content from user content. |

Before choosing a common representation, walk through a manifest-based pack and
an archive/extraction-based pack, including optional selections. Identify which
information each importer actually has and what needs recording at installation.
Do not make Modrinth IDs, URLs, hashes, or release labels prerequisites for all
formats. Missing provenance or unsupported install operations must be explicit;
do not invent it from filenames. These cases should guide reuse of existing
import code rather than the creation of another parallel installer framework.

`BaseInstance` already stores managed-pack type, ID, name, and version through
`setManagedPack()`; ATLauncher already uses it. Start the provenance discussion
there. Those fields identify a pack but do not record per-file ownership or
optional selections. Agree on how to extend or complement them before adding
another provider-specific source of truth.

## Information needed to make a decision

The launcher needs to distinguish three things:

1. What the previous pack release supplied and what was actually installed.
2. What is in the instance now, including the user's selections and changes.
3. What the selected target release supplies and requires.

A matching filename alone does not establish ownership. A checksum establishes
content equality, but does not by itself establish that a user-added file belongs
to the pack. A release identifier establishes provenance, not current disk state.
Optional files that were never selected must be distinguishable from required
files the user subsequently deleted.

For new imports, retain enough provenance to make these distinctions. For old
instances with missing or incomplete provenance, explain what is unknown and
offer a separate linking/verification step. Do not silently adopt local files.
If the original release is unavailable or cannot be verified, updating is
unavailable; ordinary use of the unchanged instance remains possible.

The storage format and whether this information belongs in a broader instance
model are open design decisions.

## Proposed user flow

1. The user requests a pack update. The launcher identifies the installed pack
   and release, or explains why they need to be linked first.
2. Show eligible target releases from the same pack source. Where the source has
   release ordering and stable/recommended labels, use their defined meanings;
   otherwise require an explicit target instead of guessing a “latest” release.
   Distinguish “no newer release,” “target cannot be determined,” and “newer
   releases require a different Minecraft version or loader family.” Validate
   actual target requirements as well as any provider listing. Do not assume
   display version strings use semantic versioning or all sources have the same
   release metadata.
3. Compare the old release, local instance, and target. Show pack versions,
   component changes, additions, replacements, removals, preserved local changes,
   conflicts, changelog, expected downloads, and required working/backup space.
   Obtaining the archive for this comparison may itself require a download.
4. Let the user cancel without changing installed content or the installed-release
   record. Linking, if requested, is a separate confirmed metadata operation.
5. After confirmation, prepare and verify the update. Recheck that the reviewed
   instance state still applies before replacing anything. A changed plan needs
   another review; confirmation does not authorize overwriting later edits.
6. Report success only when content and installed-release information agree.
   Explain preserved deviations and how to recover the previous state.

Checking or downloading must not implicitly launch Minecraft. The action's name
should make clear that it updates a pack, rather than every individual mod.

## Proposed rules for local changes

These are defaults for discussion. “Conflict” means explain the affected paths
and reason and leave the installed instance unchanged until resolved. The first
version need not include an interactive merge editor.

| Situation | Proposed result |
| --- | --- |
| Pack-owned file is unchanged locally; target changes or drops it | Replace or remove it as previewed. |
| Target adds a file at an unused path | Add it, subject to optional-file choices and protected user data. |
| User added a file unrelated to the target | Keep it; do not start treating it as pack-owned. |
| Target occupies a user-owned path, even with identical bytes | Report a collision; do not silently adopt or replace it. |
| Pack mod was locally replaced or deleted | Report the local change; do not silently restore it or discard the replacement. |
| Pack mod was disabled | Preserve that choice when it can be mapped unambiguously to the target. Otherwise report a conflict. |
| Optional mod was not selected | Keep it unselected when its identity is known. Newly introduced optional mods need an explicit selection policy. |
| Existing configuration or preferences differ from pack defaults | Preserve them and report that target defaults were not applied. |
| Target introduces a previously absent configuration default | Propose adding it; a known user deletion must be treated as a local change. |
| Other pack-owned support file was locally edited or deleted | Preserve the local change and report the deviation. If required by the target, report a conflict. |
| Save, screenshot, server list, or other user data exists at a target path | Never replace or remove it as part of a pack update. Report conflicting pack content. |

Instance name, group, icon, account selection, Java/memory settings, and launch
preferences are outside the pack's authority. Component changes required by a
target must be visible and must respect locally customized components.

Keeping extra mods cannot guarantee compatibility. Known collisions should be
reported; whether unknown mod formats block an update is an explicit policy
question, not a consequence of whichever parsers happen to exist.

## Failure, cancellation, and recovery

Downloads, archive validation, and checksum failures must leave installed content
and its release record unchanged. Invalid paths must not write outside the
intended instance, and path aliases must not overwrite unrelated files on any
supported filesystem. Insufficient space must be reported before modification;
running out of space later must still be recoverable.

The instance must not launch or undergo a competing mutation during installation
or recovery. Cancellation before installation leaves it usable. During
installation, cancellation must either reach a consistent state or clearly wait
until a safe stopping point; closing a dialog must not abandon writes.

After a crash or write failure, either the previous consistent state is restored
or the launcher clearly reports recovery is required and prevents launching a
partially updated instance. Failed recovery retains the evidence and backup
needed to retry or recover manually. It must never be reported as success.

An explicit undo must state which files it will restore or remove. Subsequent
edits to those files require a warning before replacement; unrelated later work,
especially new or changed worlds, must survive. Undoing pack files cannot undo
world or configuration migrations performed by Minecraft after the update.

Agree on backup scope, retention, cleanup, and disk costs before implementation.
A failed second update must not destroy the previous usable recovery point.
Copy/export behavior must define which provenance is portable and exclude
transient locks and incomplete transactions. These requirements do not prescribe
an in-place journal, full snapshot, or directory replacement strategy.

## Examples that should become acceptance tests

- An unchanged pack moves from A to B: previewed changes are applied, the release
  becomes B, and another check does not reinstall B.
- A renamed disabled mod remains disabled; an ambiguous mapping produces a
  conflict without changing the instance.
- A replaced mod, a deleted required mod, and an unselected optional mod are
  distinguished and receive the outcomes above.
- A user-added mod survives; a target that collides with its path cannot overwrite
  it. Local configuration edits and known deletions are preserved or reported.
- A target requiring another Minecraft version or loader family is not installed
  through the initial update flow. A loader-version change is explicitly shown.
- A download failure, bad checksum, cancelled review, or edit during review leaves
  installed content and the release record unchanged.
- An interruption at each installation boundary recovers to a consistent state;
  an unavailable backup produces a recoverable error, not a successful launch.
- Undo after playing preserves new world progress and warns about later edits to
  files it would replace. Copy/export does not carry an active transaction.
- A manifest-based source and an archive/extraction-based source produce the
  same local-change outcomes for equivalent installed content. Unknown provenance
  produces an explicit limitation; optional selections survive either path.
- Users outside the agreed experiment retain their existing launch/import flow
  without new pack-update prompts or changes to their instances.

Small local fixtures should exercise these cases deterministically. Real-pack
tests supplement them and should be optional and pinned to exact releases.
Execution on Windows, Linux, and macOS and an actual game launch remain separate
validation work; a passing planner test does not demonstrate pack compatibility.

## Decisions needed before another implementation

1. Which formats should be addressed first, and what work can proceed alongside
   the launcher update mechanism? How will users opt into and leave experiments?
2. Should updates modify an existing instance or create a replacement instance
   for the user to switch to? What identity and local data must carry over?
3. Where should provenance, local selections, and managed content live within
   MultiMC's instance/component model? What can import and update share?
4. Are the proposed conflict and configuration policies acceptable, particularly
   deleted files, new optional mods, and required pack configuration changes?
5. Is fixing Minecraft and loader family a useful first scope? Should the user
   choose targets, and should prereleases or downgrades ever be offered?
6. What recovery and undo guarantees justify the storage and maintenance costs?

## Proposed order for discussion

1. Discuss the questions above with the maintainer on
   [MultiMC's Discord](https://discord.gg/multimc), as requested in the review and
   the repository's contribution guidance. Record the agreed first change and
   experiment boundary; this sequence is a suggestion, not an approved roadmap.
2. Compare two different import paths with small local examples. Agree on pack
   identity, installed-file provenance, and optional-selection semantics before
   changing their representation. Existing import behavior must remain intact.
3. Propose the smallest shared import/provenance change that discussion supports,
   with focused tests. Review it independently of the prototype's updater.
4. Add read-only comparison and conflict tests for both formats. Review the
   results before introducing any instance writes.
5. Once the instance operation and recovery approach are agreed, implement and
   test failures, cancellation, and undo in isolation from the UI.
6. Expose the flow through the maintainer's agreed experiment mechanism when it
   is ready. Verify opt-in behavior and Windows/Linux/macOS execution before
   proposing wider availability.

Do not create a competing launcher update mechanism or assume a feature flag,
release channel, or delivery date. The existing prototype remains reference
material; this proposal does not request merging it or implementing every format
at once.
