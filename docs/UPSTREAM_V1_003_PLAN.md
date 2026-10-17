# ProsperoEden v1.003.000 — repeatable integration plan

**Status (2026-10-16):** Batch 01 passed [CI #38916616359](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38916616359) and general PS5 smoke testing; the targeted large-RomFS case remains untested. **Metroid Prime 4's severe FPS slowdown was reproduced on Batch 02a, Batch 01, AND the pre-integration Switch UI build #37** (`50c54c1c`); it was also observed on official upstream v1.003.000. Therefore the issue **cannot be attributed to either integration batch**. Its cause and the precise older 50–60 FPS baseline remain unknown. Batch 01's checkpoint remains `d1d331f5`; Batch 02a's diagnostic code is preserved at `6dd98c15`. Do not promote unqualified code or mix further changes until per-game diagnostics are documented. Nothing merged into release-prep.

## Immutable reference points

| Input | Pinned identity |
| --- | --- |
| Source | blackbearreloaded/ProsperoEden |
| Previous release | v1.001.000 at `d7f2764f8ecf8bdbd979eece2b402621edceee2f` |
| Target release | v1.003.000 at `368294ee4b5fb1b38c6cb88854c53ae46e9b8a93` |
| Our integration branch | `integration/upstream-v1.003.000` |
| Previously tested release-prep base | `f0234eb3212df716abbe8306e0474fc746f3a846` |
| Underlying Eden core | `67bada77f8a43a90da2e94e89b8e7da73c256989` — unchanged |
| Our package identity | `PPSA99186`, contentVersion `06.000.000` — unchanged |
| Machine-readable manifest | [tools/upstream/v1.003.000.json](../tools/upstream/v1.003.000.json) |

Never merge upstream `main` blindly, overwrite our Switch UI, bump the Eden pin as a side effect, change title IDs, enable automatic updates or push a release tag. The existing `v*` GitHub Actions trigger publishes a GitHub prerelease, so tags are strictly out of scope.

## Run the same audit every release

The audit is **read-only**: it verifies the manifest, local title ID, Eden pin, archive dependency and update-disable guard, then compares exact upstream tag SHAs against all our modified paths. No network access or remote Git is required for the CI guard; the full diff uses an explicitly provided local checkout.

```bash
git clone https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI.git
git clone https://github.com/blackbearreloaded/ProsperoEden.git upstream-reference
cd ProsperoEden-Switch-UI
git switch integration/upstream-v1.003.000

# Always run these first. They do not fetch or modify application code.
python3 tools/upstream/audit_changes.py --verify-only
python3 -m unittest discover -s tools/upstream -p 'test_*.py' -v

# Produce an immutable, machine-readable review of paths and commit SHAs.
python3 tools/upstream/audit_changes.py \
  --upstream ../upstream-reference --fork . \
  --output /tmp/upstream-v1.003.000-review.json
```

Source comparison as of October 16: **53 commits, 256 changed upstream files, 95 overlaps with files changed by our existing fork**. The overlap count is informational, not an assertion that 95 merge conflicts will occur. Review all changed paths. Protected visual/theme/identity/packaging files are **never automatic merges**.

For any future release: pin both release tag commit SHAs in a fresh manifest, then pass `--manifest tools/upstream/vNEXT.json` to **the same audit script**. Compare against the previously integrated release tag, generate a new review, and only then schedule integration batches. Do not track a moving branch head as the target.

## Small integration batches

| Group | Scope | State |
| --- | --- | --- |
| 01 | RomFS stack overflow during destruction of huge modded directory trees | Adapted from upstream [4f813a35](https://github.com/blackbearreloaded/ProsperoEden/commit/4f813a352816662fb86183bb1d108425a507881f); **CI and general PS5 smoke passed**; Prime 4 issue also exists on **pre-integration #37**, so not uniquely introduced by RomFS change. Targeted large-RomFS case untested |
| 02a | Texture-collector scan liveness: count actual evictions with bounded scans | **CI passed, game-specific compatibility not qualified.** Same Prime 4 slowdown occurs in original pre-integration #37, so not evidence this backport introduced it. Collector benefit/overhead under real memory pressure needs separate testing. Upstream [46b269fc](https://github.com/blackbearreloaded/ProsperoEden/commit/46b269fc18218572b7c7d72d17d90994c646c1c4). |
| 02b | Memory pool/JIT memory-on-demand, PS5 Pro memory mode, 4/6/8 GiB per-game memory | **Not yet integrated**; requires additional host checks, PS5 model testing and UI adaptations |
| 03 | CPU/JIT, fastmem, code caches, scheduling, lower GPU/CPU overhead | Pending performance/compatibility review |
| 04 | Vulkan/RADV graphics correctness and partial texture updates | Pending graphics review |
| 05 | Controller haptics, vibration control and split touchpad | Pending per-game mapping and UX review |
| 06 | Lua runtime launcher, Slate theme, upstream launcher appearance | **Deferred**, not necessary for emulator core improvements |

Each batch has to:

1. Name exact upstream source SHAs and contributors, list changed files and identify fork overlaps.
2. Cherry-pick *only clean and independent commits* with `git cherry-pick -x`. If adaptation is required, cite the original commit in our patch and leave a traceable porting note. Never mislabel an adaptation as a verbatim cherry-pick.
3. Include a targeted host regression test for each major fix and require a full successful GitHub Actions build on the integration branch.
4. Preserve Switch Dark/Themezer, Tactile Leaf sound assets, startup/forge art, flat Settings, distinct PS5 title/installer behavior and disabled automatic update installation.
5. Backup `/data/prosperoeden/` and the previous installed `PPSA99186` app folder, then smoke-test game launch, graphics, memory, mods, input, themes, saves and restart on real PS5 hardware.
6. Record build URL/SHA, test console, results and regressions; only bring validated batches into release-prep after explicit review. Roll back that batch if a regression appears.

## Hardware regression report — Batch 02a (2026-10-16)

- **Failing artifact:** [CI run #38946440182](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38946440182), source `6dd98c15`, stored at branch `diagnostic/upstream-v1.003.000-batch02a-fps-regression`.
- **Metroid Prime 4: Beyond:** previously roughly 50–60 FPS; now 7–20 FPS most of the time, intermittently near 50 FPS. This is a blocking gameplay regression. Cause **not yet isolated**; CPU work from collector is one hypothesis, not a finding.
- **Zelda: Breath of the Wild:** initial area ~30 FPS with one brief drop to ~24 FPS; no visible texture corruption. No save file/fast travel available, so a memory-stress test was **not completed**.
- **Official upstream corroboration:** Maintainer also installed official ProsperoEden v1.003.000 and observed similarly poor Metroid Prime 4 performance. Independently, [upstream issue #195](https://github.com/blackbearreloaded/ProsperoEden/issues/195) was reported against **v1.002.000** on 2026-10-14: FPS near 60 when memory usage is about 9 GB, falling to 2–9 FPS or crashing near 12 GB. Firmware, game location and settings have not been matched, so this is correlation, **not proof of the same cause**.
- **Chronology correction:** The collector source fix [46b269fc](https://github.com/blackbearreloaded/ProsperoEden/commit/46b269fc18218572b7c7d72d17d90994c646c1c4) was **already included in upstream v1.002.000**, not just v1.003.000. The official issue does NOT exonerate that change: both upstream and Batch 02a contain it. Nor does it demonstrate the collector is to blame; other memory/runtime changes also occurred. Avoid making attribution claims until a matching-settings v1.001-based A/B test.
- **A/B result (2026-10-16):** Maintainer retested Metroid Prime 4 using the full Batch 01 (#38, `d1d331f5`) build and confirmed **the severe FPS problem is still present**. Therefore Batch 02a is **not required** for this slowdown. Batch 01's general PS5 smoke test was successful, but Prime 4 performance fails on that checkpoint too. This does not establish whether the RomFS backport, an earlier baseline issue, shared state, or PS5 conditions are responsible.
- **Pre-integration A/B result (2026-10-16):** Maintainer confirmed the **same low-FPS problem on Release build #37**, the last pre-integration Switch UI package ([run #38801787792](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38801787792), commit `50c54c1c`). This means the slowdown occurs **without Batch 01's RomFS backport and without Batch 02a's texture-collector backport**. The latest upstream v1.003.000 also reportedly struggles, and [upstream issue #195](https://github.com/blackbearreloaded/ProsperoEden/issues/195) documents comparable Prime 4 symptoms. These results do not isolate a shared GPU-memory issue, game update/mod change, or console/runtime setting. There is currently **no verified matching-scene build demonstrating 50–60 FPS** in this comparison series. Do not call either migration batch the cause of this observed slowdown.
- **Next diagnostic:** Before more builds, identify the precise older emulator build/scene on which Prime 4 previously achieved 50–60 FPS, game update and graphics settings, and whether this was Switch UI or upstream. In a current build, use the in-game performance overlay (Touchpad + R1), observe FPS changes over several minutes from a cold launch, and record memory metrics if exposed. Avoid clearing shader caches, deleting shared settings, or changing several variables at once. If the FPS collapse occurs after memory usage rises, compare with the upstream issue #195 and capture sanitized logs. If Prime 4 is immediately slow from cold launch, prioritize settings/game-update/PS5-runtime differences. Keep this as a **game-specific compatibility blocker**, not a blocker on basic Batch 01 RomFS smoke validity.
- **Diagnostic limitation:** `gc_scan=upstream` is only parsed inside `#ifdef EDEN_DEV_PROFILE`; standard Release build sets that profile off. Editing `dev-settings.txt` for this particular rollback switch **will not work** with the normal GitHub Actions ZIP. An intentional A/B development build would be necessary.
- **Gate:** Do not merge Batch 02a or begin 02b until cause is determined, corrective changes pass CI and Metroid Prime 4 performance is recovered on PS5.

## Historical Metroid Prime 4 regression baseline (2026-10-16)

The user's 50–60 FPS experience was on **official ProsperoEden before
v1.001.000**, not on any version of Switch UI. Subsequent low FPS reproduces
on pre-integration Switch UI build #37, Batches 01/02a, and official upstream
v1.003.000. Thus neither integration batch is required to reproduce it.

The exact original working build and whether reported FPS included frame
generation are **not yet known**. The most useful controlled upstream
comparison is [v1.000.110](https://github.com/blackbearreloaded/ProsperoEden/releases/tag/v1.000.110)
versus [v1.001.000](https://github.com/blackbearreloaded/ProsperoEden/releases/tag/v1.001.000).
Both pin the same Eden core `67bada77`; 27 upstream commits separate them,
including changes to guest clock, thread placement, ASTC texture processing,
and memory management. The comparison does not prove any of those changes
caused the Prime 4 issue. If v1.000.110 also has the issue, examine an
earlier version matching the user's previously working setup.

Before testing a historical original release, back up the complete original
`/data/homebrew/PPSA99008/` app folder as well as shared
`/data/prosperoeden/` configuration and saves. Keep game version, mods,
graphics backend, resolution, scene, frame generation and PS5 environment
as constant as possible; never erase shared data just to run a comparison.
A historical version is a diagnostic only, not a recommendation to downgrade
public installations.

Avoid blocking otherwise safe emulator integrations **solely** because
Prime 4 is slow on our baseline. Maintain a separate game-compatibility
investigation while recording per-batch memory/graphics qualification.

### Official v1.000.110 working performance reference (2026-10-16)

- **User hardware confirmation:** After restoring **official ProsperoEden v1.000.110**, Metroid Prime 4 feels much more fluid, generally stable with occasional dips near **43 FPS**, and appears sharper. This is our **first observed working upstream reference** in the investigation; precise average FPS and frame-generation state were not measured.
- Frame generation was introduced in **v1.000.100** and is available in v1.000.110. In that release, enabling it requires **Settings > Video** (or the game's Video settings) plus a user-provided `Lossless.dll` on Vulkan. **The maintainer confirmed that v1.000.110 displays “Frame Generation: unavailable; needs Lossless.dll to enable.”** Therefore this run was **not using ProsperoEden frame generation**, and the observed fluidity and ~43 FPS dips cannot be attributed to Lossless Scaling interpolation. The FPS overlay distinguishes native (**N**) and generated/display (**F**) rates when available.
- The next controlled comparison is **official v1.001.000** with the same game version/scene, graphics settings, frame-generation state, PS5 enabler and shared data. Do not reset shader caches or change unrelated settings. If v1.001.000 is markedly slower on a matched test, the release-to-release difference narrows the likely regression window to **27 upstream commits** between v1.000.110 and v1.001.000, not any of our Switch UI migration batches.
- Keep backups of `/data/homebrew/PPSA99008/` and shared `/data/prosperoeden/` before replacing official app versions. This historical experiment should not influence public-bundle content without a reviewed fix.

### Confirmed v1.000.110 → v1.001.000 Metroid Prime 4 regression (2026-10-16)

The maintainer performed a controlled comparison with the **original upstream
application**, not a Switch UI build:

| Version | Observed performance |
| --- | --- |
| Official v1.000.110 | Fluid, occasional dips to ~43 FPS; noticeably sharper/fluid appearance |
| Official v1.001.000 | Choppy and erratic, ~7, 14, and 30 FPS |
| Switch UI #37, Batch 01, Batch 02a, official v1.003.000 | Similar severe choppiness |

**Consistent reported video settings in every test:** global **2160p at 60 Hz**,
game **2× internal resolution**. In v1.000.110 the Frame generation option
explicitly says **unavailable (needs Lossless.dll)**, so that successful run
was *not* using Lossless Scaling frame interpolation. We have not verified the
game's exact update, render backend, ASTC policy, cache state, or PS5 memory
readout across every run; preserve those caveats.

The upstream diff [v1.000.110...v1.001.000](https://github.com/blackbearreloaded/ProsperoEden/compare/v1.000.110...v1.001.000)
contains **27 commits**. Both versions pin the same underlying Eden core
`67bada77`. Do **not** diagnose the slowdown as specifically caused by the
Switch UI fork or Batch 01/02a backports.

**Prioritized hypotheses to test, not established causes:**

1. Vulkan/ASTC texture handling changed (new compressed-vs-decoded setting,
   per-frame background ASTC conversion and texture reclaim paths). At 2×
   resolution this may be memory-sensitive. Check **per-game Video > ASTC
   textures = Compressed**, because Full quality keeps decoded textures and
   can consume ~4× as much memory for those textures.
2. The new guest clock/frame pacing and CPU topology detection changed
   timing and worker placement. Both require controlled source-level
   comparison if a 1× render test does not isolate graphics pressure.
3. The v1.001 PS5 graphics/memory budget, image-page locking and GC
   prefetch changes could affect resource contention under load.

**Next short diagnostic:** On official v1.001.000, record Prime 4's effective
ASTC option including the per-game override; confirm **Compressed**. Then
temporarily test **1× internal resolution** at the same 2160p/60 Hz output
and same scene for ~5 minutes; restore 2× afterwards. 1× improvement would
point toward render-scale-dependent work or memory use; no improvement
would increase priority of timing/scheduling and other non-scale-dependent
changes. Neither result alone proves a root cause. Avoid shader-cache
deletion or multiple simultaneous setting changes.

After narrowing the affected subsystem, create one reversible diagnostic
change on a **separate upstream-regression branch**, keep the original
v1.000.110 package as control, and run a matching-scene A/B. Do not
merge a speculative workaround or block unrelated v1.003 feature
integrations merely because Prime 4 predates the fork's changes.

### ASTC/1× graphics-scale follow-up (2026-10-16)

On **official v1.001.000**, the maintainer confirmed **ASTC textures =
Compressed** and changed Prime 4's internal resolution from 2× to **1×**
while leaving global 2160p/60 Hz output. The game **remained badly choppy**.
This reduces the likelihood that excessive 2× GPU render work or decoded
ASTC quality is the primary cause. It does not exclude graphics
synchronization, texture cache or memory pressure at 1×.

A minimal controlled guest-clock ablation has been prepared on a **separate**
branch [`diagnostic/prime4-guest-clock-off`](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/tree/diagnostic/prime4-guest-clock-off),
forked from the **pre-integration Switch UI #37** code (`50c54c1c`).
It bypasses only the two v1.001 `GuestClock` calls in generated Eden
sources while preserving CPU worker placement, graphics code, title ID
and shared data formats. CI/PS5 qualification is **pending**; it is not a
release candidate or accepted upstream fix.

The next PS5 test should restore the original 2× internal resolution,
compare the same Prime 4 scene to known poor Switch UI #37, and note
FPS immediately and after 5–10 minutes. A successful result would
implicate the clock behavior; a failure would prioritize worker placement
and GPU/presentation synchronization. Neither result proves causality
without a repeatable controlled comparison.

### Prime 4 guest-clock negative result and CPU-topology test (2026-10-16)

- [Switch UI diagnostic build #41](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38983986326),
  source `3a890991`, removed only the `GuestClock::Scale` and
  `GuestClock::FrameQueued` injections from the **pre-integration #37**
  source. CI succeeded, but user reported **the same very low FPS in
  Metroid Prime 4 at 2×**, possibly slightly worse. Therefore the
  new guest-clock behavior **alone** is not sufficient to explain the
  original performance regression.
- Old upstream v1.000.110 **already pinned CPU workers** by checking CPUID;
  v1.001.000 added an SMT timing-based **fallback** when fewer than five
  physical cores are found. Turning off *all* worker placement would be
  a different and misleading test.
- A separate minimal experiment
  [`diagnostic/prime4-topology-fallback-off`](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/tree/diagnostic/prime4-topology-fallback-off)
  starts again from untouched **pre-integration #37** and disables
  **only that new v1.001 fallback**; it preserves existing CPUID-based
  pinning and guest-clock behavior. Build/PS5 result pending.
- If the fallback was not exercised (`EDEN_WORKER_TOPOLOGY ... measured=0`
  in the original build), this experiment cannot materially change
  thread placement; a negative result should move us to reviewing other
  v1.000.110→v1.001.000 commits, not random affinity tweaks.
- Neither diagnostic is a patch for release, a v1.003 integration
  batch, or a reason to advance a source version claim.

## Exit conditions

- [ ] All planned high-priority emulator improvements selected and attributed.
- [ ] Host checks, packaging, ZIP safety and attestation pass.
- [ ] On-console graphics/gameplay and Switch UI regression testing pass.
- [ ] Core pin, original user data, UI identity and manual update safety remain intact.
- [ ] Reproducible change report, regression results and complete upstream attribution recorded.
- [ ] Actual foundation/version claims updated **only after** the integrated set is qualified.
- [ ] Release-prep merge separately approved. No public tag or release created here.

The initial backport is an isolated fix, **not** a claim that v1.003.000 is fully integrated.
