# Soft Orbit — original Switch UI audio pack (preview v1)

**Date:** 2026-10-14  
**Edition:** ProsperoEden Switch UI Edition / KernelForgeX  
**Status:** Approved for a PS5 *trial build*, not yet hardware-qualified.

## Provenance

The 20 files in `headless/prosperoeden/ui/sounds/` were synthesized
specifically for this edition using mathematical oscillators, envelopes,
deterministic filtered noise, and simple mixing. They were not sampled from
Nintendo, ElevenLabs, BlackBearReloaded, a game console, a commercial music
recording, or an existing sound-effect collection. They evoke a *restrained,
tactile* console interface without reproducing Nintendo's jingles or recordings.

The generated preview pack was supplied for maintainer audition on 2026-10-14.
The maintainer approved integrating that **exact** pack for on-console
evaluation, allowing individual cues to be swapped later if desired.

All 20 filenames remain compatible with the inherited ProsperoEden
`SoundBank` interface, including numbered focus, slider and toggle variants.

## Format / packaging

- 48,000 samples/second, 16-bit PCM, 1 channel (the PS5 decoder duplicates
  mono clips to stereo).
- All 20 WAV assets are committed into the release-prep tree; no third-party
  sound files remain in the current `ui/sounds` directory.
- `tools/check-soft-orbit-audio.py` verifies the **exact approved** SHA-256
  values, filenames and WAV properties, and is invoked automatically after
  the app's `ui/` folder is copied into the release package.
- In `pe/audio/sounds.cpp`, installed WAV files now take priority over the
  soft-navigation synthesizer; its original tones remain only as fallback for
  missing clips.
- The PS5 home-screen soundtrack (`snd0.at9`) remains absent.
- To update one sound later, intentionally replace its asset and update the
  SHA-256 entry in the checker after listening and testing; avoid using upstream
  clips as a shortcut.

## PS5 trial and rollback

1. Build the `feature/switch-ui-v6.0-release-prep` branch using a manual
   **Release build** GitHub Actions run, with **Cache snapshot** unchecked.
2. Back up the working app's `eboot.bin` and `ui/sounds/` directory.
3. Replace **both** the installed `eboot.bin` **and** the entire
   `ui/sounds/` folder from the `PPSA99186/` release ZIP. Replacing only
   `eboot.bin` is **not** sufficient to test the new sounds.
4. Navigate HOME, Library, Settings, Themes, Downloads and Save Sync. Test
   focus movement, select, back, screen/dialog open/close, toggles, slider,
   save, notifications, and game launch/return where possible.
5. Compare perceived loudness at normal TV volume. Missing/low sounds or
   clipping should be logged for further adjustment.
6. Roll back by closing the app, restoring the backed-up `eboot.bin` and
   the complete previous `ui/sounds/` together. Do **not** alter
   `/data/prosperoeden/`, saves or downloaded Themezer themes.

A console boot and game-launch test is still required after packaging.
