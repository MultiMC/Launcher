# MENKIA Launcher

MENKIA is an independent Windows-first Minecraft Java launcher project based on the open-source MultiMC Launcher codebase.

The goal is to build a polished, reliable desktop launcher with MENKIA branding and its own configuration, services, and release process. This repository is a work in progress; it is not yet a tested or distributable executable.

## Current status

- [x] MENKIA source fork established
- [x] Initial application identity and user-agent changed to MENKIA
- [x] MENKIA SVG application mark added
- [x] Upstream Microsoft, paste.ee, and Imgur client credentials removed from the build configuration
- [x] Windows executable metadata changed to MENKIA
- [ ] Replace remaining legacy artwork and audit all upstream branding references
- [ ] Complete dependency and license notices for every bundled component
- [ ] Establish a reproducible Windows x64 build environment
- [ ] Register MENKIA's own Microsoft Entra application before enabling Microsoft sign-in
- [ ] Implement and test MENKIA-specific interface improvements
- [ ] Package, sign, and test a Windows release

## Building

See [BUILD.md](BUILD.md) for the upstream build requirements. Those instructions are not yet a verified recipe for current Windows x64 systems. Do not assume that a successful source checkout means a working build.

## Authentication and game access

Microsoft authentication must use an application registration owned by MENKIA. The upstream MultiMC client ID has deliberately been removed and Microsoft sign-in remains unconfigured until that registration and its redirect settings are completed.

A local/offline profile is not a Microsoft account and does not grant ownership of Minecraft. It can only be used where the server explicitly permits offline-mode authentication.

## Upstream origin and license

MENKIA is an independent fork of [MultiMC/Launcher](https://github.com/MultiMC/Launcher). The upstream project is licensed under Apache License 2.0; retain the upstream license and required notices when redistributing. Third-party libraries and submodules may have separate licenses, which must be reviewed individually before a release.

MENKIA is not affiliated with or endorsed by the MultiMC contributors. The original project name, logos, service credentials, and branding are not MENKIA assets. This fork is being rebranded independently.
