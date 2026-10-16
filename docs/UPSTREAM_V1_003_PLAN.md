# ProsperoEden v1.003.000 — repeatable integration plan

**Status (2026-10-16):** Batch 01 passed [CI #38916616359](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38916616359) and general PS5 smoke testing, but not the specific wide-RomFS reproduction. Its tested checkpoint is `checkpoint/upstream-v1.003.000-batch01` (`d1d331f5`). **Batch 02a is blocked due to a severe PS5 gameplay FPS regression** in Metroid Prime 4: Beyond, despite passing [CI #38946440182](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38946440182). Do not promote or compound it with further changes. The exact failing build is frozen as `diagnostic/upstream-v1.003.000-batch02a-fps-regression` (`6dd98c15`). Await matching-settings A/B against Batch 01 before attributing the cause. Nothing merged into release-prep.

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
| 01 | RomFS stack overflow during destruction of huge modded directory trees | Adapted from upstream [4f813a35](https://github.com/blackbearreloaded/ProsperoEden/commit/4f813a352816662fb86183bb1d108425a507881f) into our [3b43c491](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/commit/3b43c49127d5cb007970ad3dec42e4b0f3115364); **CI and general PS5 smoke passed 2026-10-16**, specific large RomFS case untested |
| 02a | Texture-collector scan liveness: count actual evictions with bounded scans | **BLOCKED — severe Metroid Prime 4 FPS regression in PS5 test** (7–20 FPS versus previous 50–60 FPS); CI passed, but gameplay did not. Review upstream [46b269fc](https://github.com/blackbearreloaded/ProsperoEden/commit/46b269fc18218572b7c7d72d17d90994c646c1c4). |
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
- **A/B next:** Quit the app; back up the full installed `PPSA99186` application folder and preserve shared `/data/prosperoeden/` files. Download the **ProsperoEden** artifact from [Batch 01 run #38916616359](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38916616359); install the *whole* app folder, not just `eboot.bin`, and **do not delete shared caches or settings**. Retest Metroid Prime 4 at the same game location, graphics backend, resolution, mods, GPU accuracy, and approximate runtime. If it returns to ~50–60 FPS, that supports the Batch 02a code as the cause; if still poor, investigate shared configuration/cache or other external variables before changing code.
- **Diagnostic limitation:** `gc_scan=upstream` is only parsed inside `#ifdef EDEN_DEV_PROFILE`; standard Release build sets that profile off. Editing `dev-settings.txt` for this particular rollback switch **will not work** with the normal GitHub Actions ZIP. An intentional A/B development build would be necessary.
- **Gate:** Do not merge Batch 02a or begin 02b until cause is determined, corrective changes pass CI and Metroid Prime 4 performance is recovered on PS5.

## Exit conditions

- [ ] All planned high-priority emulator improvements selected and attributed.
- [ ] Host checks, packaging, ZIP safety and attestation pass.
- [ ] On-console graphics/gameplay and Switch UI regression testing pass.
- [ ] Core pin, original user data, UI identity and manual update safety remain intact.
- [ ] Reproducible change report, regression results and complete upstream attribution recorded.
- [ ] Actual foundation/version claims updated **only after** the integrated set is qualified.
- [ ] Release-prep merge separately approved. No public tag or release created here.

The initial backport is an isolated fix, **not** a claim that v1.003.000 is fully integrated.
