# Modpack updates: behavior proposal

Status: discussion draft, not an agreed design or a description of implemented
behavior. This responds to the request in
[PR #5793](https://github.com/MultiMC/Launcher/pull/5793#issuecomment-6724342565)
to define pack updates before proposing an implementation. The code in that PR
is an experiment; it should not determine the answers below.

## Problem and scope

A player has installed a pack release, changed settings, and possibly added,
disabled, removed, or replaced mods. They want to move to another release of
that pack without manually reconstructing those choices or losing their worlds.

An update follows a pack author's published release. Updating individual mods
to their latest versions is a separate operation. Preserving files does not
guarantee that the resulting mods will work together or interpret settings in
the same way.

The proposed first scope is an explicitly requested update of a Modrinth pack
with a known original release, on the same Minecraft version and loader family.
This is a scope proposal, not a requirement that the underlying instance model
be specific to Modrinth. Other providers, Minecraft migrations, loader-family
changes, automatic updates, and arbitrary configuration merging are deferred.

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
2. Show eligible target releases. Recommend a newer stable release of the same
   project and distinguish “no newer release” from “newer releases require a
   different Minecraft version or loader family.” Validate the actual target
   requirements as well as the provider's listing. Do not compare display version
   strings as if all packs used semantic versioning.
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

Small local fixtures should exercise these cases deterministically. Real-pack
tests supplement them and should be optional and pinned to exact releases.
Execution on Windows, Linux, and macOS and an actual game launch remain separate
validation work; a passing planner test does not demonstrate pack compatibility.

## Decisions needed before another implementation

1. Should updates modify an existing instance or create a replacement instance
   for the user to switch to? What identity and local data must carry over?
2. Where should provenance, local selections, and managed content live within
   MultiMC's instance/component model? What can import and update share?
3. Are the proposed conflict and configuration policies acceptable, particularly
   deleted files, new optional mods, and required pack configuration changes?
4. Is fixing Minecraft and loader family a useful first scope? Should the user
   choose targets, and should prereleases or downgrades ever be offered?
5. What recovery and undo guarantees justify the storage and maintenance costs?

Once those decisions are agreed, implementation can be proposed in small pieces:
shared import/provenance handling, a read-only comparison with tests, then an
instance operation with failure/recovery tests, and finally the UI. The existing
prototype is not a prerequisite for accepting this behavior proposal.
