# Homebrew catalog — Switch UI title-ID reservation

**Status:** Reservation PR [#302](https://github.com/blackbearreloaded/ps5-homebrew-catalog/pull/302) **submitted, open and awaiting maintainer merge**. Submitted by `KernelForgeX` from fork branch `reserve-ppsa99186` at commit `6809c9dc0bbe0e60d635309b7c75774c4a6d3fe7`. GitHub's dedicated **Submission check** passed (Validate submission and Scan the release). A separate general CI workflow reports `action_required` with zero jobs, potentially awaiting repository-maintainer approval; this is not a failed submission check. **The title ID is not reserved until the PR is merged.** No public beta or release has been published.

This edition is a separate PS5 app from the original ProsperoEden.
Its intended title identity is:

- Title ID: `PPSA99186`
- Display title: `ProsperoEden Switch UI`
- Source: https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI
- Maintainer: `KernelForgeX`, published by `KernelForgeLabs`
- Original upstream: https://github.com/blackbearreloaded/ProsperoEden
- License: GPL-3.0 (matches GitHub's detected repository license)

## Reservation record to submit

The upstream catalog guide requires the four release/download link
fields to be `null` until publication. Create **`apps/PPSA99186.json`**
in a fork of https://github.com/blackbearreloaded/ps5-homebrew-catalog
with exactly this JSON:

```json
{
  "titleid": "PPSA99186",
  "name": "ProsperoEden Switch UI",
  "kind": "app",
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

**Do not assume that owning the project GitHub organization automatically
owns the catalog reservation.** The catalog records the **individual
GitHub account that opens the reservation pull request**. It requires
that same account to submit the later release/update. The recommended
persistent submitting account is `KernelForgeX`.

For an **organization-owned source repository**, the catalog submission
guide explicitly requires the submitter's GitHub organization membership
to be **public**. Make `KernelForgeX`'s membership of `KernelForgeLabs`
public before opening a release listing PR; verify that catalog CI
accepts ownership before announcing a catalog installation.

1. Check the catalog's `apps/PPSA99186.json` and open PRs for existing
   claims/conflicts immediately before preparing the reservation.
2. From the same account you will maintain long-term (`KernelForgeX`),
   fork the catalog and add the JSON above.
3. Open a reservation pull request from that account, fixing any
   automated validation messages. The ID is **not reserved** until the
   catalog maintainers merge the PR.
4. Keep the reservation updated and do not let it become stale for
   180 days.
5. For public listing, first publish and verify an approved source
   tagged GitHub release with the full installable app ZIP and exact
   sha256, then **update the reservation in a second PR from the same
   submitting account**.

The catalog accepts GitHub pre-releases, and its automated daily
release-update job may advance listed releases. A Reddit beta can remain
**outside** the catalog while `PPSA99186` is only a Coming soon
reservation. Decide explicit beta/stable versioning before pushing any
`v*` tag: this repository's release workflow publishes a prerelease
automatically on matching tags. Never replace already-public release
ZIPs with different bytes.

Official guide:
https://github.com/blackbearreloaded/ps5-homebrew-catalog/blob/main/docs/submitting.md#reserving-a-title-id

## Outstanding verification

- [ ] Confirm title ID is still free at submission time (including open PRs).
- [ ] Make KernelForgeX's KernelForgeLabs organization membership public.
- [x] Submit reservation PR #302 as KernelForgeX.
- [ ] Await maintainer approval and merge of reservation PR #302.
- [ ] Verify fresh CI build, clean PS5 install, safe rollback and complete
      source/release documentation before inviting public beta users.
- [ ] Validate the new organization's release assets and uploader/attestation.
- [ ] Qualify automatic catalog-based updates independently before claiming
      that updates work in the Switch UI Edition.

Nothing in this guide reserves a title ID or publishes a release.
