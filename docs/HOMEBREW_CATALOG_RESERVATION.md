# Homebrew catalog — Switch UI title-ID reservation

**Current status (2026-10-20): RESERVED.** Catalog
[PR #302](https://github.com/darkxex/ps5-homebrew-catalog/pull/302)
was initially closed on October 16, then reconsidered, approved and
**merged October 18, 2026**. `PPSA99186` is reserved for the KernelForgeLabs
Switch UI edition. The ProsperoStore entry currently says Coming soon;
no public release artifact has been published. A separate catalog update
PR is required to activate downloads.

This edition is a separate PS5 app from the original ProsperoEden.
Its reserved title identity is:

- Title ID: `PPSA99186`
- Display title: `ProsperoEden Switch UI`
- Source: https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI
- Maintainer: `KernelForgeX`, published by `KernelForgeLabs`
- Original upstream: https://github.com/blackbearreloaded/ProsperoEden
- License: GPL-3.0 (matches GitHub's detected repository license)

## Approved reservation record

The catalog currently contains this reservation. The `null` release
fields must be filled in through a subsequent catalog PR; they are not
download links.

```json
{
  "titleid": "PPSA99186",
  "name": "ProsperoEden Switch UI",
  "kind": "app",
  "category": "emulators",
  "description": "An independent Switch-inspired PS5 interface edition of BlackBearReloaded's ProsperoEden emulator.",
  "license": "GPL-3.0",
  "author": "KernelForgeLabs",
  "version": null,
  "source_repo": null,
  "artifact_url": null,
  "sha256": null,
  "icon_url": null
}
```

## Submission/ownership rules

The reservation belongs to the GitHub account `KernelForgeX`, which
submitted PR #302. The release-listing PR must be submitted from that
same account, with public membership in the source repository's
`KernelForgeLabs` organization.

1. Publish and verify a tagged GitHub prerelease from the approved source.
2. Attach the complete installable ZIP and record its SHA-256.
3. Verify the release icon matches the installed application icon.
4. Update `apps/PPSA99186.json` in a new catalog PR as `KernelForgeX`.
5. Wait for catalog validation and maintainer approval before claiming
   the application is downloadable through ProsperoStore.

The initial October 16 rejection and subsequent October 18 approval
are recorded in PR #302. Preserve that discussion as project history.
Never replace the bytes of an already published release asset.

Official instructions:
https://github.com/darkxex/ps5-homebrew-catalog/blob/main/docs/submitting.md

## Outstanding verification

- [x] Reserve `PPSA99186` through approved, merged PR #302.
- [x] Preserve a PS5-tested v1.005.000 integration baseline.
- [ ] Confirm public `KernelForgeX` membership in `KernelForgeLabs`.
- [ ] Complete remaining release hardware and rollback gates.
- [ ] Verify CI release ZIP, provenance, licensing and checksums.
- [ ] Publish the first GitHub prerelease.
- [ ] Convert the reservation into a downloadable catalog listing.
- [ ] Qualify optional in-app update notifications/installations later.

The reservation is approved; publication remains pending.
