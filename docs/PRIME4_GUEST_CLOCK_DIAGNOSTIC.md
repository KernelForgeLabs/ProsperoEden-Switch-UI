# Prime 4 guest-clock-off experiment — diagnostic build only

**Not a public beta, upstream fix, or production candidate.** CI and PS5
hardware results are pending.

## Evidence

- On **official ProsperoEden v1.000.110**, Metroid Prime 4 was fluid, with
  occasional dips to about 43 FPS.
- On **official v1.001.000**, the same game became erratic, approximately
  7/14/30 FPS. Our pre-integration Switch UI #37 and later batches show
  the same symptom. The official v1.003 build does too.
- Settings tested: global 2160p at 60 Hz; game 2× internal scale. On
  v1.001.000, ASTC textures were already **Compressed**, and lowering the
  game to 1× **did not restore FPS**. Frame generation was unavailable
  on the working v1.000.110 because Lossless.dll was missing.
- v1.000.110 and v1.001.000 share Eden core `67bada77`; 27 upstream
  commits separate them.

## Isolated experimental change

Forked directly from the pre-integration Switch UI #37 source,
`50c54c1c7242d108a5169598c233a25dfd503731`.

In `headless/CMakeLists.txt` we bypass **only** the two v1.001
`GuestClock` code injections:

1. Do not inject `GuestClock::Scale(fres)` into
   `CoreTiming::GetClockTicks`.
2. Do not inject `GuestClock::FrameQueued(swap_interval)` into
   `BufferQueueProducer::QueueBuffer`.

This leaves Eden's native guest clock path intact. The pinned-source
anchor checks remain; no renderer, ASTC, worker-topology, title-ID,
packaging or save-data changes have been introduced in this diagnostic
branch. The existing host `check-guest-clock.py` tests the normal
clock algorithm in isolation, **not** the special compiled-out injection
in this build.

## Hardware test protocol

1. Manually trigger a Release build on branch
   `diagnostic/prime4-guest-clock-off` with **Cache snapshot unchecked**.
   Confirm CI success, ZIP validation, checksum and attestation.
2. Stop both ProsperoEden editions. Back up the complete
   `/data/homebrew/PPSA99186/` directory and shared
   `/data/prosperoeden/` user settings and saves off-console.
3. Install the **whole** `PPSA99186/` application folder extracted
   from the inner release ZIP. Do not touch original `PPSA99008/`,
   delete shader caches, or replace only `eboot.bin`.
4. Retest Metroid Prime 4 from the same early-game area at **2× internal
   resolution, 2160p/60 Hz output, Vulkan, ASTC Compressed**, with the same
   game patch, settings, mods, and frame-generation state. Record FPS
   immediately and after 5–10 minutes.
5. **If FPS recovers:** the new guest-clock behavior is implicated.
   Reproduce and refine the fix before proposing a change upstream.
6. **If FPS remains poor:** changing guest-clock scaling alone is
   insufficient. Restore the prior app if desired, and next isolate CPU
   topology or presentation/texture synchronization separately.

The result should be recorded with the full Actions run ID and tested SHA.
**Do not merge this branch into release-prep or publish it as a fix.**
