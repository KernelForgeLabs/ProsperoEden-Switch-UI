# Prime 4 CPU topology fallback ablation — diagnostic only

**Status:** Prepared for CI and PS5 testing. Not a product fix, release,
or a change to our upstream migration branch.

## Confirmed comparison

- Official ProsperoEden **v1.000.110**: Metroid Prime 4 felt fluid;
  occasional dips to approximately 43 FPS.
- Official **v1.001.000**: choppy, fluctuating 7/14/30 FPS.
- Global output **2160p/60 Hz**; game originally **2×** resolution.
  On v1.001, **ASTC Compressed** already selected; changing to **1×**
  did not improve FPS. Working v1.000.110 did not have Lossless.dll,
  so Lossless Scaling frame generation was unavailable.
- Both official releases pin the same Eden core. Switch UI pre-integration
  build #37 and both migration batches share the poor Prime 4 behavior.
- An isolated **guest-clock-off** diagnostic on build #41
  ([CI run](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38983986326))
  **did not improve FPS** at 2×; user thought it might even be worse.
  We have not disproved interactions between timing and other subsystems.

## Precisely what this changes

This branch forks **pre-integration Switch UI build #37**
(`50c54c1c7242d108a5169598c233a25dfd503731`), rather than
combining multiple diagnostic edits.

The only executable source change is in `headless/performance.cpp`:
it prevents execution of the **v1.001.000-specific CPU topology fallback**
that measures whether adjacent logical CPUs share a physical core when
CPUID cannot identify five distinct cores. The original CPUID-based
detection and all its existing worker pinning remain unchanged,
as do the guest clock, Vulkan, ASTC, emulator pin, PS5 title ID,
audio/UI/themes and user data.

**Crucial limitation:** If CPUID on your console correctly enumerates
five distinct cores, the fallback never ran in the normal build.
Disabling it will then have no meaningful effect on CPU placement.
A negative result is inconclusive about other scheduling changes,
graphics processing or the full v1.001 release.

The expected diagnostic line from a normal build is similar to
`EDEN_WORKER_TOPOLOGY ready=... measured=...`.
In this special build, `measured` should always be `0`.
If accessible from a redacted log, the original `measured` field
helps determine whether this change was active on your PS5.

## Testing

1. Trigger manual GitHub Actions **Release build** from
   `diagnostic/prime4-topology-fallback-off` with Cache snapshot **unchecked**.
   Check that build, ZIP preflight, checksums and attestation succeed.
2. Close both editions and back up the complete
   `/data/homebrew/PPSA99186/` app folder and shared
   `/data/prosperoeden/` settings/saves. Do not modify original
   `PPSA99008/`.
3. Install the complete inner ZIP `PPSA99186/` application
   (not just `eboot.bin`); preserve all shared user data and caches.
4. Test **Metroid Prime 4: Beyond** with the same game version and scene,
   **2160p/60 Hz** global output, **2×** internal scale, **Vulkan**,
   **ASTC Compressed**, and frame generation unavailable.
5. Compare FPS at game launch and 5–10 minutes later. Report whether
   FPS is still ~7/14/30 or improves toward the original v1.000.110
   performance. Note if the normal build's log has `measured=1`.
6. If there is no improvement, stop changing affinity knobs blindly:
   the fallback itself was either not active or not enough to explain
   the issue. Next consider a **commit-level release-window bisect**
   using the original upstream source at selected points between
   v1.000.110 and v1.001.000.

No application release, version tag, default-branch switch,
or promotion to release-prep is authorized by this test.
