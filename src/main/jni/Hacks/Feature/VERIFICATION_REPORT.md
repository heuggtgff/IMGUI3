# Final verification report: TRIGGERTBOT_DUMP_REVIEW.md

**Deliverable:** `src/main/jni/Hacks/Feature/TRIGGERTBOT_DUMP_REVIEW.md` (177 lines) — a dump-grounded answer to every one of the user's triggerbot questions, with real dump citations.

**What it contains (verified against `github.com/Eliwoahzja/dump` `dump.cs`, 80 MB, 2,421,501 lines):**

Every RVA cited is present in the dump and the accessor-line citation points to the actual accessor line (not the RVA header):
- `Pawn.get_RecentlyRendered 0x51BA7E8` — accessor line 714697, RVA header 714696 ✔
- `Pawn.get_LastOccVisible 0x51BA800` — accessor line 714700, RVA header 714699 ✔
- `Physics.Raycast(origin, direction, out hitInfo) 0x506C1E4` — accessor line 173933, RVA header 173932 ✔
- `Camera.WorldToScreenPoint(Vector3) 0x5023B54` — accessor line 157939, RVA header 157938 ✔
- `Weapon.StartFire 0x5123C84` (740524 / RVA hdr 740523), `Weapon.StopFire 0x5124EAC` (740551 / 740550), `Weapon.Tick 0x5114C34` (739913 / 739912), `Weapon.get_IsFiring 0x50EB644` (739309 / 739308) ✔
- `Pawn.get_IsFiring 0x51A8A38` (714403 / 714402), `Pawn.IsAiming 0x519BB10` (721515 / 721514) ✔
- `Weapon.get_ShotSpreadSize 0x5109B68` (739732 / 739731), `get_PunchAngle 0x5109CA4` (739738 / 739737), `get_AimingTarget 0x510EAAC` (739828 / 739827), `get_AimingEnemyTarget 0x510EAB4` (RVA hdr 739831), `get_CurrentAimingTargetDis 0x510EABC` (739834 / 739833) ✔
- Fire-timing getters: `get_DelaySprintFire 0x513CBC4` (742510 / 742509), `get_FireInterval 0x5105C74` (740026 / 740025), `get_FireBoltTime 0x51237F4` (740482 / 740481), `get_UseGetReadyTime 0x50E9F60` (739003 / 739002), `get_SpringFireGetReady 0x50E9F28` (738991 / 738990), `get_GetReadyTime 0x514D2C8` (742861 / 742860) ✔
- `Weapon.AmmoCanFire() 0x50EB73C` (739774 / 739773), `Weapon.CanFire() 0x5126CC4` (740581 / 740580) ✔

Field-offset citations verified against the dump:
- `PlayerInfo.m_IsPlayerDead 0xFC` — line 939573 ✔ (PlayerInfo 19766)
- `PlayerInfo.m_TeamID 0x110` — line 939577 ✔
- `PlayerInfo.m_TeamSeatID 0x120` — line 939582 ✔
- `PlayerInfo.m_NickName 0x160` — line 939597 ✔
- `WeaponFireComponentData.InitSpeed 0x1C` — line 695416 ✔
- `BulletInitSpeed 0x1D0` — line 695566 ✔; `BulletInitSpeedToForceShowGunLine 0x1D4` — line 695567 ✔
- `WeaponFireComponentData.IsBurstAutoFire 0x4C` — line 695480 ✔; `HitPunchAngleMultiplierRange 0x40` — line 695478 ✔
- `WeaponData.BOT_TraceRangeMin 0x550` — line 695002 ✔; `BOT_TraceRangeMax 0x554` — line 695003 ✔
- `WeaponData.AutoFireDistance 0x5C0` — line 695021 ✔ (WeaponData 15786)
- `RaycastHit.m_Distance 0x1C` — line 175625 ✔

**What the file says (summary):**
- The dump IS the same build as the code's RVAs (all 22 RVAs above match).
- The dump does NOT contain the Pawn field offsets the code uses (`m_IsAlive 0x548`, `m_HeadBone 0x308`, `m_Mesh 0x628`, `m_PlayerInfo`) — those came from a different dump; this dump's Pawn (16146) has garbage offsets.
- For each of the user's questions it lists: what the dump answers, the exact dump line/RVA, what the code already does, and what's missing.
- Missing (dump has no callable API): hitbox-under-crosshair enumeration, explicit invuln flag, projectile lead/prediction API, smoke-as-boolean, per-shot spread angle.
- Already implemented & dump-confirmed: entity iteration, team, alive (via PlayerInfo), crosshair projection, LOS via raycast, fire-mode gating (ADS/firing), delay removal.

**Checks:** no build step applies to a documentation file. The only verification is dump-content cross-check, which passed for every citation above (reenqueued/rerun after the correction, not skipped).

**Limitations recorded in the file:** on-device behaviour untested (no device); the dump is a stub (no method bodies); Pawn field offsets are not from this dump; hitbox/lead/smoke-boolean are not exposed as callable APIs.

**Corrections made during verification:** the first draft cited RVA *header* lines for some RVAs and used imprecise ranges for a few (`AutoFireDistance` said "694860 area" — corrected to line 695021; `get_FireSpeed` said "739176 area" which is correct since there's no single accessor line at that exact offset). Those were all fixed before this report.


---

# Verification pass 2 - ray identification ("no shooting through walls")

Every citation below was read out of the same `jeudhehs/dump` `dump.cs`
(80,758,140 bytes, md5 `78eebfe3bda6bf4276995045e8f0bd97`, 2,421,501 lines).

## New symbols used by the rewritten visibility gate

| Symbol | RVA | dump.cs line | Why it is here |
| --- | --- | --- | --- |
| `Physics.Raycast(origin, direction, out RaycastHit hit, float maxDistance)` | `0x506C188` | 173930 | nearest hit **with** an explicit limit - the only overload that can answer "what is the first thing on this line" |
| `Physics.Raycast(origin, direction, float maxDistance)` | `0x506BF60` | 173912 | the same limit without the out struct - conservative fallback |
| `Physics.Raycast(origin, direction, out RaycastHit hit)` | `0x506C1E4` | 173933 | the 3-parameter overload the old gate used; **no longer the verdict** |
| `Transform.get_forward()` | `0x509C650` | 172057 | camera forward for the centre-of-screen ray |
| `Transform.get_parent()` | `0x507BDEC` | 172105 | walks a hit's parent chain to name its owner |
| `Transform.get_position()` | `0x507C19C` | 172009 | cross-check only: identical to `api10` in `Defines.h`, so the dump's RVA base matches the one the repo loads from |
| `Camera.ScreenPointToRay(Vector3)` | `0x5023FB0` | 157987 | researched, **not used**: a `Ray` is a 24-byte byval return and the same ray is reproducible from the camera transform |

`RaycastHit` (dump.cs 175618, TypeDefIndex 1838) is annotated
`m_FaceID 0x18`, `m_Distance 0x1C`, `m_UV 0x20`; `m_Collider` is the next field
after the 8-byte `Vector2 m_UV`, i.e. `0x28`, which is what the code reads
(`AF_RAYHIT_COLLOFF`). The collider is turned into a `Transform` through
`Component::get_transform` (`Class_Component_get_transform`), the call
`DrawESP.h` has already used in game.

Not used, and why:
- `PlayerPawn.m_SpineBone @0x1D68` (line 723390) is declared **static**, so the
  offset is static-storage furniture, not an instance offset.
- `PlayerPawn.get_SpineBone()` `0x5477EA8` (line 724447) exists only on
  `PlayerPawn`, not on `MPPawn` / `BRPlayerPawn` / `DMZPlayerPawn` /
  `PVEPlayerPawn` / `OpenWorldPlayerPawn` / `BRGroundWarPlayerPawn`, so calling
  it on an arbitrary enemy would be a wrong-vtable read. The torso therefore
  stays an axis fraction (0.72 chest, 0.45 waist) between `m_Mesh 0x628` and
  `m_HeadBone 0x308`.

## Checks run

- Host `g++ 13.3 -std=c++17 -fsyntax-only -Wall` over `NewFeatures.h` (with stub
  `jni.h` / `android/log.h` and `-D__fastcall=`): **0 errors**.
- The same check over the new `Main.cpp` diagnostics rows compiled against the
  repo's real `imgui.h` with `-Wformat=2`: **0 errors, 0 format warnings**.
- Brace/parenthesis balance with comments and string literals stripped:
  `NewFeatures.h` 0/0, `Main.cpp` 0/0.
- Both files also re-read end to end for the paths the compiler cannot judge:
  path A (centre ray), path B (zone + named ray), the advisory-only gates, and
  the release/hold logic in `A_FireTick`.

## Not verified

- No Android NDK in this environment, so `ndk-build`, the APK and any in-game
  behaviour are untested here. The host check validates syntax and types, not the
  NDK toolchain or the device ABI.
- The "hit cannot be named" fallback stops the ray 0.80 of the hitbox radius
  short of the aim point. A panel thinner than that pressed against the target's
  own collider is the one case that fallback cannot see; the primary path names
  the hit and has no such gap.
