# PR #5793: rationale and response to review

This explains the tradeoffs visible in the submitted code. It is a retrospective
reading of the implementation, not a claim to recover the original agent's
private reasoning. The [behavior proposal](modpack-update-proposal.md) is the
starting point for discussion; the prototype is not an agreed architecture.

## Latest feedback and response

In the [November 12 comment](https://github.com/MultiMC/Launcher/pull/5793#issuecomment-6835137839),
peterix supports pursuing pack updates, asks to discuss order on Discord, and
points out both the launcher update work and the need to consider other formats.
The proposal now compares existing import paths, identifies the existing
`BaseInstance::setManagedPack()` metadata, and suggests small reviewable steps
with an agreed experiment boundary. Modrinth is no longer assumed to be the
first implementation. Runtime changes wait for that design discussion.

## Why the prototype took these approaches

| Approach | Problem it addresses | Cost or assumption to revisit |
| --- | --- | --- |
| Update a published pack release, keeping Minecraft and loader family fixed | Bounds the update and avoids independently upgrading every mod. | Excludes many real pack upgrades; this scope was chosen before agreement. |
| Record a baseline and compare hashes | Distinguishes unchanged installed pack files from local modifications. | A separate JSON ownership model duplicates concepts that may belong in the instance model; hashes alone do not express user intent. |
| Preserve existing configuration and extra files | Avoids overwriting the player's settings and additions. | Preserved settings may be incompatible with the target; blanket path rules cannot express every pack's requirements. |
| Stage downloads, take a snapshot, and journal in-place writes | Attempts to make failures recoverable while retaining instance identity. | Introduces backup lifecycle, launch gating, locking, copy/export exclusions, and substantial recovery code. Updating a copy is an alternative to evaluate. |
| Separate synchronous planning/file operations from dialogs | Makes the core callable from tests without driving the UI. | `PackUpdate.cpp` still combines parsing, planning, mod inspection, storage, and recovery; the dialog still owns the operation's orchestration. |
| Use `NetJob`, checksum validators, and Qt background work | Reuses existing download/progress infrastructure and keeps disk work off the UI thread. | Reusing utilities does not establish integration with the instance task/component lifecycle. |
| Inspect mod identities | Tries to retain disabled choices across renames and detect duplicate active mods. | Adds format-specific parsing and blocks unfamiliar JARs; identity and compatibility policy need agreement. |

## Integration and readability concerns

The maintainer's concern is supported by concrete boundaries in this PR:

- `InstanceImportTask::processModrinth()` parses the manifest, while
  `Update::readPack()` implements another interpretation. Import/update behavior
  should share an agreed representation and validation rules.
- `PackUpdateDialog::run()` combines linking, release selection, planning,
  downloads, component changes, installation, and recovery. The UI should present
  choices and results; the operation should fit the existing task lifecycle.
- `updateLoaderComponent()` edits `mmc-pack.json` directly and the dialog reloads
  `PackProfile` afterward. Component ownership and persistence need an agreed
  boundary before implementing this again.
- `BaseInstance`, launch, copy, and export now know about updater-specific state.
  Instance-wide mutation/recovery behavior should be designed together, rather
  than accumulated as special cases for one provider.
- Several helpers and tests compress branches and multiple mutations onto a
  single line. A future implementation should use ordinary control-flow blocks,
  descriptive intermediate values, and small functions with one responsibility.
  Recovery code especially needs visible phases and explicit failure outcomes.

The PR adds one Python file: a 106-line optional real-pack fixture downloader.
The updater and automated unit tests are C++; Python is not a launcher runtime
dependency. The script's purpose is reproducible external fixtures, but it is
still additional tooling to maintain. Small local fixtures should cover the
agreed behavior first; whether to retain this helper can be decided separately.

Formatting alone would not address the review. Nor would rewriting the whole
prototype before agreeing on behavior. The proposed next step is to settle the
questions in the behavior proposal, then make small changes along MultiMC's
existing boundaries. No new architecture is assumed approved by this document.

## Suggested Discord opener (not posted)

Hi peterix, I'm the author of PR #5793. Thanks for inviting me here. I've revised
the proposal to consider the existing ATLauncher, Technic, FTB, and MultiMC import
paths alongside Modrinth. I'd like to agree on a small first step that fits your
launcher update work. Would comparing provenance and optional selections in two
import paths be useful, or is there a different prerequisite you'd prefer first?

Proposal: https://github.com/nikhil-iso/Launcher/blob/codex/modpack-updater/doc/modpack-update-proposal.md
