# ESP32Marauder — Session Notes: merge `origin/master` → `new_merge`

_Resume-ready handoff. Repo: `github.com/evilpete/ESP32Marauder`, branch `new_merge`._

## 1. Current repo state (all pushed to `origin/new_merge`)
```
<top>    Fix ESP32-C5 memguard starvation: board-tunable low-RAM thresholds
9cc492d  Add session-notes.md: merge handoff / resume notes
a8e1b1b  Fix ESP32-C5 SoftAP crash: disable 11ax on the AP interface
85ac162  Fix boot crash: build "Admin Tools" in DEVICE_ROOT lazy case
10f7108  Merge origin/master (v1.19.2) into new_merge   <- the merge commit (2 parents)
```
(`<top>` = the memguard commit made this session; `git log --oneline -6` for exact hash.)
- Working tree clean; `HEAD == origin/new_merge == a8e1b1b`.
- No PR opened (not requested).

## 2. What is DONE and verified
- **The merge is complete and correct.** `origin/master` (v1.19.2) merged into `new_merge` as a merge commit (`10f7108`), using `git merge --no-ff -Xignore-cr-at-eol -Xignore-space-change`. All 15 conflicts resolved.
- **Conflict resolution decisions:**
  - Union where both sides added features (board blocks `MARAUDER_CYD_HMI` + `XIAO_ESP32_C5`; settings cache `TimeStPCAP`/sound/PM/GPS + master's `Censor`; CORE2 SD as an `#elif` in the existing chain).
  - `ft6336.h`, `AXP192.cpp`: adopted `T-HMI-WS_C5`'s resolved versions (**CTP_** naming; Core2 `Wire1` macro rework). Fixed malformed `#endif HAS_AXP192`.
  - `Display.h/.cpp`, `Buffer.h`: union (touch includes + master Core2 members / thread-safety).
  - `WiFiScan.cpp` sysinfo: kept new_merge's `p_info` approach (MACs already printed via `p_info`; dropped master's duplicate screen-MAC block).
  - `esp32_marauder.ino`: kept BackLight.hpp architecture + `init_system_time`, Probe-GPS-at-Boot, I2C-GPS; added master's CSI/Core2-power/brightness. Backlight pin guard = `#if defined(HAS_SCREEN) && !defined(HAS_SDMMC)` (per user). Removed `log_d("PSRAM not available")` and `esp_spiram_init()` (per user) to pass master's PSRAM regression tests.
  - **`MenuFunctions.cpp`: adopted master's lazy `buildKind` menu architecture** (user's explicit choice), re-inserting new_merge's **Zigbee menu**, **GPS entries**, **Power/DeepSleep**, and sound/GPS **setting-filters**; kept master's keyboard rename (`marauder_keyboard`). Removed a pre-existing stray `>>>>>>> T-HMI-WS_C5` marker (was inside a comment in `#ifdef NOT_USED`).
  - "Sync Clock with WiFi" re-added to master's `buildWiFiGeneralMenu()` (clean `do_menu_sync_ntp()` version).
  - CI workflows: unioned build-target matrices (all targets from both).
  - `tools/test_installer_manifest.py` + `tools/test_release_notes.py`: **target counts only** → 35 targets / 32 boards.
- **All 10 `tools/` unittest files pass.** No conflict markers; `#if/#endif` balanced (matches master).
- **Boot crash fixed (`85ac162`):** an eager `addNodes(&deviceMenu, "Admin Tools")` survived in `InitMenu`, but `deviceMenu` is now lazy (`MenuBuildKind::DEVICE_ROOT`, null `list` during setup) → null-deref crash right after menu `RunSetup`. Moved "Admin Tools" into the `DEVICE_ROOT` case of `buildLazyMenu()`. Verified it was the ONLY eager-access-on-a-lazy-menu in InitMenu.
  - **Result: ESP32‑S3 (CYD_HMI) and ESP32‑WROOM boot and work.**

## 3. OPEN ISSUE (resume here) — ESP32‑C5 `MARAUDER_WS_C5_28` WiFi
**Symptoms**
- WiFi scan returns nothing; log spam: `[memguard] AP not added: free≈17096 largest≈15348 rejected=N aps=0`.
- SoftAP start crashed earlier (`Load access fault`, `ieee80211_hostap_attach` / `softap_choose_bss_color_without_conflict`).

**Root cause (confirmed)**
- Master's v1.19.2 **`RuntimeMemoryGuard`** (`esp32_marauder/MemoryGuard.{h,cpp}`; reached via `WiFiScan::checkMem()` → `RuntimeMemoryGuard::instance().allow()`) measures **internal heap only** (`MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`). It latches **paused** once free ≤ `pauseFreeBytes = 24 KB` (or largest block ≤ 8 KB) and only resumes above 40 KB / 12 KB.
- WS_C5_28 = ESP32‑C5 (small internal SRAM) + 320×480 display buffers + full feature memory profile → only ~17 KB internal free during scan → guard paused forever → all APs rejected. The SoftAP crash was the same starvation (OOM → null in the WiFi driver), not strictly 11ax.
- Works on S3/WROOM because they keep more internal SRAM free (above 24 KB). Headless C5 boards (DevKit/XIAO, `tft:false`) have no display buffers, so master correctly kept C5 on the full profile — WS_C5_28/WS_C5_35 (C5 **with** display) are the gap master couldn't know about.
- This behavior is **new from the merge** (new_merge had no memguard, so scanning worked, possibly risking OOM).
- The pushed `a8e1b1b` 11ax-AP fix (restrict AP to 11b/g/n on `HAS_DUAL_BAND`) is reasonable to keep but was a symptom fix.

**Fix APPLIED** (committed to `new_merge`, see Section 1) — memguard thresholds are now board-tunable and lowered for C5+display:
1. `esp32_marauder/MemoryGuard.h`: now `#include "configs.h"` under `#ifdef ARDUINO`; `MemoryGuardThresholds` defaults come from `MARAUDER_MEMGUARD_*` macros. With `MARAUDER_LOW_INTERNAL_RAM` defined → pause/resume free = **10 KB / 14 KB**, pause/resume largest = **6 KB / 8 KB**; otherwise the original **24 / 40 / 8 / 12 KB**. Non‑ARDUINO (host) has no board macro → original defaults, so the host Unity test is unchanged.
2. `esp32_marauder/configs.h` (just after `//// END BOARD FEATURES`): defines `MARAUDER_LOW_INTERNAL_RAM` for `MARAUDER_WS_C5_28` / `MARAUDER_WS_C5_35`.
- **STILL NEEDS HARDWARE VERIFICATION:** the 10/14 KB values are a starting guess. Flash WS_C5_28, retry WiFi scan + SoftAP, read the `[memguard] free=… largest=…` log, and tune via the `MARAUDER_MEMGUARD_*` macros (overridable per board in configs.h before MemoryGuard.h is first included). If allocations now proceed but the WiFi driver still OOM-crashes, the floor is too low / the board genuinely needs reduced buffers (consider also a reduced feature profile).
- All 11 `tools/` python tests pass. `test/test_memory_guard/test_main.cpp` (host, Unity) couldn't be compiled here (no toolchain) but is logically unaffected — host defaults are identical.

## 4. Constraints / environment (important for next session)
- **No Arduino/PlatformIO toolchain in this container** — cannot compile or flash. All firmware verification is on the user's hardware. Rely on: conflict-marker checks, `#if/#endif` balance, `tools/` unittests, and user flashing.
- **Running `tools/` tests:** from repo root → `PYTHONPATH=tools python3 -m unittest tools.<name>`. (Some import `from tools.x`; `test_whole_source_coverage` imports bare `whole_source_coverage`; `pytest` is NOT installed.)
- **User guidance to honor:**
  - Only change `tools/` unit tests for **target counts**, nothing structural.
  - Use **CTP_** pin naming on touch code.
  - Commit attribution footer:
    ```
    Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
    Claude-Session: https://claude.ai/code/session_01WPgyG2TWytU5fC4Qf96HrG
    ```
  - Merge with `-Xignore-cr-at-eol -Xignore-space-change` to avoid whitespace conflicts.
- **Reference branches (for resolution intent):**
  - `origin/T-HMI-WS_C5` — fully contains current master; touch/C5/display resolutions (authoritative for pure-touch files).
  - `origin/MSC_USB` — older master; USB/buffer work.

## 5. To do next session (in order)
1. **Flash WS_C5_28 with the applied memguard fix (Section 3)** and retry WiFi scan + SoftAP. Read the `[memguard] free=… largest=…` log and **tune** `MARAUDER_MEMGUARD_*` if needed. If WiFi now works, mark the C5 done.
2. Re-confirm WS_C5_28 **SoftAP** after the memory fix (the 11ax fix may then suffice, or revisit).
3. **Confirm with user:** new_merge had a *duplicate* "Sync Clock with WiFi" under `#ifdef HAS_DIRECT_UPLOAD`; only the clean `do_menu_sync_ntp()` one was ported — is that desired?
4. Optional: open a PR for `new_merge` if the user wants one.

## 6. Boards tested so far
- ✅ ESP32‑S3 (CYD_HMI / LilyGo T‑HMI) — boots & works after the Admin Tools fix.
- ✅ ESP32‑WROOM — works.
- ⏳ ESP32‑C5 `MARAUDER_WS_C5_28` — boots; WiFi scan/AP was blocked by memguard. Memguard floor fix applied (Section 3) — **awaiting hardware re-flash to confirm / tune**.
- Not yet reported: WS_C5_35, other C5 variants.
