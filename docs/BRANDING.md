# Branding: product icon and KernelForgeLabs publisher identity

**ProsperoEden Switch UI** is an independently maintained interface edition
based on BlackBearReloaded's ProsperoEden and the Eden emulator.

## Product identity (startup, rail, missing covers)

The floating opening icon, 64px left-side navigation mark, and missing-game-cover
fallback use one OpenGL texture, `textures_.brand()`. It is read from
`ui/art/brand.tga`. The animation's position, size, duration and motion are
unchanged.

During **every PS5 package build**, the release packager first generates
`sce_sys/icon0.png` using the existing reviewed branding source. Then
`tools/launcher/package-branding.py` resizes **that exact packaged PNG** to
384x384 RGBA TGA and writes `ui/art/brand.tga`. A second invocation verifies
pixel-for-pixel equality. The committed source-tree TGA is only a development
fallback; it is not the release authority.

This means the product icon shown on the PS5's own home screen matches the
launcher icon. No dependency on another project's publisher logo is introduced.

## KernelForgeLabs publisher badge (About)

The forged-K, hammer and anvil image that the maintainer approved is stored
as a compact pinned WebP source at
`assets/branding/kernel-forge-labs.webp.b64`. The package step verifies its
SHA-256, converts it to `ui/art/kernel-forge-labs.tga`, and checks the
result. It has its **own** OpenGL texture, `textures_.forge()`, drawn only
within the project-credits side of About. Switching Themezer backgrounds does
not replace this publisher mark.

The project-credits page continues to credit the **Eden team** for their
emulator, **BlackBearReloaded** for the original PS5 port, and the wider
homebrew ecosystem. KernelForgeLabs is explicitly credited for the independent
Switch UI edition, without implying upstream endorsement.

## Hardware qualification

- [x] Launch the app from the PS5 home screen and compare its icon with the
      floating intro mark and 64px rail badge.
- [x] Confirm the original opening animation still travels to the rail and
      respects Reduce motion.
- [x] Open About and check the forge emblem at the lower right of project
      credits, with no overlap or clipping at normal/large text sizes.
- [x] Verify original Eden/BlackBearReloaded attribution remains readable.
- [x] Switch to a Themezer wallpaper and reopen About; the forge badge should
      remain the same.
- [x] Reinstall using the **whole package** (not just eboot.bin) because art
      assets have changed.

**Hardware result — passed on PS5 (2026-10-15).** The maintainer installed the complete build from commit `8c762510` and confirmed every branding checklist item: the floating app icon and rail icon match the PS5 icon; the original animation and Reduce Motion behavior work; the forge badge displays cleanly on About at the tested text sizes, including with Themezer artwork; and Eden/BlackBearReloaded attribution remains visible. These results qualify the branding change, not the entire v6.0 public release.
