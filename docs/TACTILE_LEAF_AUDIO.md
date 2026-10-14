# Tactile Leaf v3 — original Switch UI sound pack

**Edition:** ProsperoEden Switch UI Edition / KernelForgeX  
**Status:** Approved for PS5 trial build; final hardware volume/quality testing pending.

## Provenance and revisions

All 20 `headless/prosperoeden/ui/sounds/*.wav` were independently synthesized
from mathematical oscillators, shaped transients, filtered noise and envelopes.
No Nintendo audio, recordings, jingles, ElevenLabs clips or other copyrighted
audio samples were imported. The aesthetic is a restrained, tactile
console-style sound, not a copy of an existing product's distinctive cues.

The first `Soft Orbit` experiment was rejected as too electronic. `Tactile
Leaf v2` introduced short ticks, muted button presses and page movement.
The final audition revision `Tactile Leaf v3 — Soft Slide` **changed only
`page_01.wav` and `launch_01.wav` relative to v2**, smoothing the sharp
flips into quiet slides. The remaining 18 WAVs are byte-for-byte identical
to the approved Tactile Leaf v2 audition pack.

## Audio playback and integrity

- All 20 clips are 48,000 Hz **mono 16-bit uncompressed PCM** WAV files,
  compatible with the PS5 launcher decoder, which expands mono to stereo.
- `pe/audio/sounds.cpp` plays these clips in both normal and soft-navigation
  mode; synthesized audio is a fallback only if a recording is missing.
- An intentional gain of **0.45** (about **−6.9 dB**) is applied to all
  recorded UI cues *before* the existing user UI-volume bus. Game audio
  and other mixer buses are unchanged.
- Missing or stale files fail packaging: `tools/check-tactile-leaf-audio.py`
  validates filenames, WAV properties, and the exact approved SHA-256 hashes.
- The PS5 home-screen soundtrack (`snd0.at9`) remains absent.

## On-console verification

1. Launch a manual GitHub Actions release workflow build from
   `feature/switch-ui-v6.0-release-prep`, with the cache snapshot option
   unchecked. Do **not** push a `v*` tag (tags automatically publish a
   prerelease).
2. Back up existing `/data/homebrew/PPSA99186/eboot.bin` and its full
   `ui/sounds/` directory.
3. Copy the newly built **`eboot.bin` and the entire new `ui/sounds/`**
   folder from the release ZIP. A binary-only swap does not install sounds.
4. At your normal TV volume, browse HOME, Library, Settings, Themes,
   Downloads and Save Sync. Listen for quiet but clear focus ticks, selection
   feedback, soft page slides, opening/closing dialogs, and game launch/return.
5. Confirm the Settings > Audio UI volume still behaves as expected; compare
   sound level with game audio, and check that rapid navigation does not
   cause harsh overlapping sounds or clipping.
6. Test the Themezer selector with the **Canopy removed** and verify a theme
   selected before the update still works. If the old selection was Canopy,
   it should safely migrate to Switch Dark.

Rollback by closing the application and restoring the backed-up
`eboot.bin` and entire `ui/sounds/` together. Theme/configuration/saves
remain in `/data/prosperoeden` and should not be removed.

The accompanying sound-generation source script and before/after audition
previews were included in the reviewed Tactile Leaf v3 preview archive.
