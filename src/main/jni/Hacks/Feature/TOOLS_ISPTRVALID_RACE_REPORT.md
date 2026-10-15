# The "crashes after a while" bug: a data race in `Tools::IsPtrValid`

**Reported symptom:** the game crashes after login, or ~60 s after opening the
app, with no menu interaction. "Must have something to do with recent changes."

**Root cause:** `IL2CppSDKGenerator/IL2Cpp/Tools.cpp` kept *one*
`std::vector<std::pair<uintptr_t, uintptr_t>>` of readable ranges
(`g_readableRanges`). `IsPtrValid()` is called from several threads — the render
thread (`DrawESP`, the trigger scan, `KillDecoration_ApplyLocalInventoryInfo`),
`Skins_Thread`'s loaders and `Init_Thread` — and it looked at that vector on
every call. The refresh path, which *re-parsed* `/proc/self/maps` into the same
vector, was reached routinely:

* on the first call (empty snapshot), and
* on **every dead pointer that is not in the snapshot** — which is the common
  case in a match, where pawn/weapon pointers die between frames.

So a reader was regularly binary-searching the live vector while another thread
did `clear()` + `emplace_back()` on it. When the `emplace_back` crossed the
vector's capacity it reallocated the buffer; the reader's next `ranges[mid]`
then read **freed memory**. On device that is a SIGSEGV inside
`ToolsRangesContain`, at an arbitrary moment, with no user action — which is
exactly the shape of the report. `g_readableCacheMs` and the lazy
`target_pid = getpid()` global in `pvm()` carried the same unsynchronised-access
problem.

## Evidence

This was measured, not reasoned: the real `Tools.cpp` was linked into a host test
and driven from six threads (four hammering a live address through the snapshot,
two taking the refresh path with addresses that are not in `/proc/self/maps`),
under ThreadSanitizer.

**Pre-fix `Tools.cpp` (HEAD) — 21 data-race reports, then a crash:**

```
SUMMARY: ThreadSanitizer: data race .../vector.tcc:119 in std::vector<...>::_M_realloc_insert
    #0 ..._M_realloc_insert
    #1 emplace_back
    #2 ToolsRefreshReadableRanges Tools.cpp:142
    #3 Tools::IsPtrValid(void*)  Tools.cpp:179
  Previous read of size 8 ... by thread T2
SUMMARY: ThreadSanitizer: SEGV src/main/jni/IL2CppSDKGenerator/IL2Cpp/Tools.cpp:153 in ToolsRangesContain
exit 66
```

Line 153 is `if (a < g_readableRanges[mid].first)` — the reader dying in the
binary search, the write/read pair above it is the reallocating refresh. That is
the device crash, reproduced.

**Post-fix — clean:** `OK: 6 checks, 6247240 IsPtrValid calls across 6 threads`,
no ThreadSanitizer output, exit 0.

## The fix

The snapshot is now **immutable for as long as anyone can read it**:

- `ToolsRangeSnapshot g_rangeSlots[8]` — a static pool of snapshots, one of
  which is live at a time (`std::atomic<int> g_rangeLive`).
- A reader takes the live snapshot pointer **once per call** and only reads it;
  the binary search is `ToolsRangesContain(snapshot, addr)`.
- The writer fills the slot published `kRangeSlots - 1` refreshes ago (never the
  live one), then publishes it with a `release` store. `clear()` keeps the
  slot's capacity, so refilling never reallocates, and no slot is ever freed.
  At the 250 ms refresh throttle that is ≥1.5 s of grace for a reader whose
  whole critical section is one binary search over a few hundred entries.
- Exactly one thread parses `/proc/self/maps` at a time
  (`g_rangeRefreshing` + `compare_exchange_strong`, re-checking the throttle
  under the claim). A thread that loses the race answers from the snapshot that
  is already live, which is the one the winner is rebuilding.
- `pvm()` resolves its pid through a thread-safe function-local static instead
  of the lazily-written `target_pid` global.

Hot-path cost is unchanged: one acquire load plus the binary search — no
syscall, no lock, no allocation, which is what the original optimisation was for.

## Checks run

- `IL2CppSDKGenerator/IL2Cpp/tests/tools_isvalid_tsan.cpp` (new, committed, links
  the real `Tools.cpp`): 6 functional checks on the guard contract (live address
  valid, `nullptr` / `< 0x10000` / unmapped hole invalid) plus the six-thread
  hammer. **Pre-fix: 21 races, exit 66. Post-fix: 0 races, 6.2 M calls, exit 0.**
- `Tools.cpp` with `-Wall -Wextra`: 4 warnings before, 4 after, normalised set
  **byte-identical**; 0 errors both.
- `Main.cpp` whole-header-graph `-fsyntax-only`: 0 errors (Tools.h unchanged).
- The other RVAs the loaders call every 1.5 s (`GetRoleConfName 0x90F4214`,
  `GetVehicleSkinName 0xAF74B38`, `GetDropPlaneName 0xB198B08`,
  `GetDeadBoxIDAddress 0x90F2CA0`, the `WeaponPromotionInfo` and `InventoryUtil`
  kill-decoration overloads, the `PhysicsVehicle_Skis` five, the `WeaponAttrData`
  getters and `Pawn.get_HeadPosition`) were all re-checked against the supplied
  `dump.cs` and each resolves to the method the code claims — so the remaining
  crash candidates are not mis-targeted hooks.
- **Not run here:** `ndk-build`, the APK, anything on-device. ThreadSanitizer is
  a host instrument; the device build does not use it. This proves the
  concurrency contract and the crash mechanism, not the device ABI.

## Residual uncertainty, and how to close it

A SIGSEGV in `ToolsRangesContain` is now impossible, but without a device
logcat the link between *this* race and *your* crash is an inference from the
mechanism and the timing (the refresh path only becomes busy once pawns start
dying, i.e. in-match; the loaders and the growing heap start earlier). If it
still crashes after this build, the tombstone will now name a *different*
function — send it and the same treatment applies.

The next most likely candidate of the same class is not in `Tools.cpp`:
`Skins_Thread` appends to `charData` / `itemData` / `watch` / `deadboxF` /
`droplane` / `snowboardData` / `g_targetCharacters` from a background thread
while the render thread iterates those same vectors in the skin tab, with no
lock. Opening the skin tab during the loaders' first seconds is a vector
reallocation under an in-flight `for (const auto& x : data)`. That one needs the
same kind of treatment (publish-immutable-snapshot, or a mutex around the
loaders and the tab read) and its own test.
