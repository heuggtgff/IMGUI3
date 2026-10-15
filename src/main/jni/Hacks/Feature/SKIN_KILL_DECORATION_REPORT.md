# Skin / kill-effect / kill-icon fixes — evidence report

Two reported symptoms:

1. **Kill effects and kill icons never appear in match.**
2. **The equipped skin shows as a blank card in the loadout UI** even though the
   skin itself is visible in game.

Both turned out to be *data* bugs, not tuning: `Skin/Thread.h`'s loaders read the
config objects at the wrong base offset and then the menu wrote those (zero)
values back over the game's own data.

## Why the offsets were wrong

`Skin/Fields.h`'s `// 0x..` comments are **absolute** offsets, so a cast must use a
base equal to the offset of the struct's *first* field. Several loaders in
`Skin/Thread.h` used `+0x18` while the class' first field is at `0x10`, shifting
every field read by 8 bytes. Verified against the il2cpp dump (`dump.cs`,
2,421,501 lines) — the dumper masks some offsets with XOR `0x46464646`, so they
are decoded here (`real = dumped ^ 0x46464646`, checked against the classes whose
offsets are *not* masked):

| config class (dump line) | first field | dump | loader base before | after |
| --- | --- | --- | --- | --- |
| `ItemResourceConfig` (811593) | `UIMiniSpriteNameIndex` | `0x10` | `+0x18` | **`+0x10`** |
| `WeaponConfExtraConfig` (824064) | `DefaultMythicArmor` | `0x10` | `+0x18` | **`+0x10`** |
| `KillEffectItemConfConfig` (812074) | `RealAssetIDs` | `0x10` | `+0x18` | **`+0x10`** |
| `CharacterModelConfig` (806793) | `BindEffect1PType` | `0x10` | `+0x18` | **`+0x10`** |
| `MythicSightConfig` (814614) | `ShowInBag` | `0x10` | `+0x18` | **`+0x10`** |
| `BRDeadboxSkinConfig` (802767) | `ColorID` | `0x10` | `+0x18` | **`+0x10`** |
| `BRDropPlaneSkinConfig` (802811) | `ColorID` | `0x10` | `+0x18` | **`+0x10`** |
| `RoleSkinConfig` (819208) | `ClothType` | `0x18` | `+0x18` | `+0x18` (was right) |
| `MythicArmorConfig` (814559) | `AssetID` | `0x14` | `+0x14` | `+0x14` (was right) |
| `WeaponConfConfig` (823910) | `Accuracy` | `0x20` | `+0x20` | `+0x20` (was right) |
| `Item2InventoryConfig` (811194) | `ItemID` | `0x20` | `+0x20` | `+0x20` (was right) |

Unmasked proof cases: `WeaponConfExtraConfig.DefaultMythicArmor 0x10`,
`DefaultDeadReplayEffectId 0x18`, `DefaultKillEffectId 0x1C`, `ID 0x24` and
`WeaponConfConfig.DefWeaponSkinID 0x38`, `ID 0x40`, `BaseWeaponId 0x34` all match
the struct comments exactly — the convention holds, and `WeaponConfFields`' own
tail was stale (see below).

## The bug chain

**Blank skin card.** `LoadWeaponSkins` built `itemResByID` with
`itemFields = res + 0x18`, so `itemFields->ID` read `res + 0x24`
(`ModelAssetIDRaw`) instead of `res + 0x1C` (`ID`), and `getitem.ItemRes` /
`getitem.ItemResInt` were read from the wrong fields. `itemResByID.find(confID)`
therefore missed, `itemData` carried `ItemRes = {0,0,0,0}` for every skin, and
`applyWeaponSkin` copied those zeros into the *real* `ItemResourceConfig` fields
(`UIMiniSpriteNameIndex`/`UISpriteNameIndex`/`InventoryModelID`/…). Zero sprite
index ⇒ blank thumbnail, while the in-game model kept coming from
`DefWeaponSkinID`, which is why the skin itself still showed.

**Dead kill effects.** The same `+0x18` on `WeaponConfExtraFields` made
`weaponExtraByID` key on a wrong id, so `deadReplay`/`killEffect` stayed 0 and
`itemData.WeaponExtra[4]` was 0 — and `applyWeaponSkin` then wrote
`DefaultKillEffectId = 0` **over** the weapon's own kill effect. Independently,
`KillEffectItemConfConfig.RealAssetIDs` was read at `kill + 0x18` (inside
`IsAutoDemolition`/`ColorID`) instead of `kill + 0x10`, so
`killEffectByWeapon` was empty and `activeKillEffects` was never populated.

## Changes

`Skin/Thread.h`
- Loader bases corrected to `+0x10` for the seven classes above; `RealAssetIDs`
  read at `kill + 0x10` and the item id at `kill + 0x2C`.
- `killEffectItemByAssetId` maps the kill-effect asset id to the
  `KillEffectItemConfConfig` item id (`ID @0x2C`); `killEffectByWeapon` is now a
  file-scope map so the diagnostics can report its size.
- New local-player path, all RVAs confirmed in the dump:
  `WeaponAttrData.get_KillIconItemID 0x8479334`,
  `WeaponAttrData.get_KillEffectItemID 0x847924C` (both keyed by
  `m_BlueprintWeaponId @0x20`; dump 49510) and
  `Weapon.get_KillEffectGroupIDOverride 0x5167BDC` (dump 16255, id via
  `Weapon.get_WeaponID 0x51199AC`). They are installed through a
  `KittyMemory::getAbsoluteAddress` + `IsPtrValid` guard, not blind
  `m_unity + rva` arithmetic, and report whether they installed.
- The kill-effect-level hooks now also report level 5 for an applied kill
  *effect* (an effect asset we force is an L5 asset), not only for a kill icon.

`Skin/Fields.h`
- `WeaponConfFields` tail rebuilt to the real layout (`IsDouble 0x44`,
  `ChangeColorStr 0x50`, …, `LOCID_Name 0x68`, `UIProperty 0x78`,
  `UIPropertyUnlockLevel 0x80`). `LOCID_Name` was compiling to `0x58`
  (`EnableCamoBeta`), so the skin-name copy wrote the wrong slot — the working
  value in `Hacks/StructGame/Defines.h` (`Class_WeaponConfConfig_LOCID_Name 0x68`)
  independently agrees.

`System/UI/SkinTab.h`
- Every icon/sprite/model/name/effect write is now guarded: a 0 or an invalid
  pointer never overwrites a value the game already resolved, so a failed read
  can no longer blank a card or delete a kill effect.
- The six copy-pasted `ItemResourceConfig` blocks collapsed into one guarded
  block per matched id (one of them silently omitted `UISpriteNameIndex`).
- `WeaponFireEffectConfigConfig` id read at `0x80` and the two delay floats
  written at `0x84`/`0x88` (the struct comments were 4 bytes out and the old
  write at `0x8C` ran into the following pointer).
- Diagnostics in the Camo tab: `KillIcon cfg/map/active`,
  `KillFx map/items/active`, `Hooks attr=hit/calls item=hit/calls grp=hit/calls`,
  `HookInstalled`, last `WeaponAttrData` key and the loaded item count.

## Checks run

- Host `g++ 13.3 -std=c++17 -fsyntax-only` over a TU including the real
  `System/UI/SkinTab.h` (which pulls `Skin/Fields.h`, `Skin/Thread.h`,
  `Skin/Camo.h`) with stubbed `jni.h`/`android/log.h`/`sys/system_properties.h`
  and `-D__fastcall=`: **0 errors**.
- The same command run twice with `-Wall -Wextra -Wformat=2` — once on this
  revision, once on the pre-fix tree (`10050e5`, same TU, same stubs) — gives
  **45 warnings on both**, and the normalised warning sets (file + message,
  line/column numbers stripped) are **byte-identical**, so the fix introduced no
  new warnings. `-Wall` alone: 37 on both, also identical. Everything reported is
  pre-existing: `-Wunused-*` locals, `-Wnarrowing` in `charData`,
  `-Wsign-compare` in the camo-checkbox loops, one `m_ANOGSLIB` redefinition.
- Runtime offset test (host, x86-64 LP64 — same 8-byte pointers and natural
  alignment as arm64), committed as
  `src/main/jni/Skin/tests/offset_check.cpp` with its host stubs: a byte buffer
  is filled with sentinels at the dump offsets, then read back through the exact
  casts the loaders use. All pass:
  `ItemResourceFields.ID 0x1C`, `WeaponConfExtraFields.ID 0x24`,
  `KillEffectItemFields.ID 0x2C`, `MythicSightFields.ID 0x20`,
  `BRDeadboxSkinFields.ID 0x20`, `BRDropPlaneSkinFields.ID 0x14`,
  `CharacterModelFields.ItemID 0x78`, `WeaponConfFields.LOCID_Name 0x68`,
  `WeaponAssetGroupFields.Id 0x44`, plus the raw reads
  (`fireConf +0x80/+0x84`, `kill + 0x2C`, `inv +0x20/0x24/0x28`).
- The same test driven with the **pre-fix** `+0x18` bases reproduces every bug:
  `ID` returns `ModelAssetIDRaw` (0x24), `WeaponConfExtraFields.ID` returns
  0x24 where `DefaultKillEffectId` (0x1C) was wanted, and the kill-effect id
  reads 0 — the map that stayed empty.
- Config layouts: ~30 classes re-read out of `dump.cs` (the masked classes
  decoded with `real = dumped ^ 0x46464646`), first fields, key ids and pointer
  fields all agreeing with the casts above.
- Brace/paren/bracket balance with comments and string literals stripped:
  `Skin/Thread.h` 0/0/0, `System/UI/SkinTab.h` 0/0/0, `Skin/Fields.h` 0/0/0.
- Every class offset and RVA used above re-read out of `dump.cs`.

## Follow-up pass checks (character tab, kill-icon record, triggerbot filter)

- `Skin/tests/offset_check.cpp` now also covers the two character-tab classes,
  as `static_assert`s on the compiled layout plus runtime sentinels read back
  through the exact casts the loader uses: `RoleConfFields` `ColorID 0x22`,
  `ColorSubID 0x23`, `RoleLvGroupID 0x2C`, `ShowRare 0x2F`, `BlockAppearance
  0x30`, `ID 0x34`, `RolePackID 0x44`, `LOCID_Name 0xA0`, `WinnerCircle 0xF8`;
  `RolePackFields` `EntryAnimID 0x40`, `GestureId 0x54`, `HandEffectUI 0x58`,
  `KillStreakSkinID 0x68`, `LoadingFrame 0x6C`, `LobbySceneType 0x70`,
  `RolePackID 0x90`, `AirPoseBR 0xA8`. The same test drives the pre-fix `+0x18`
  base and shows it reading `KeepInCatalog` as `ID` - the empty Character tab -
  so the fix is measured, not asserted. Passes (`OK: all loader offsets match
  the il2cpp dump`).
- `Skin/tests/skin_headers_check.cpp` (new): a `-fsyntax-only` TU that includes
  `System/UI/SkinTab.h` (and therefore `Skin/Fields.h` + `Skin/Thread.h` +
  `Skin/Camo.h`) and instantiates the render entry point. 0 errors.
- `Main.cpp` compiled with `-fsyntax-only` over the whole header graph (stubs +
  the `__aarch64__` shim): 0 errors.
- `InventoryInfo` offsets (`0x4C` / `0x5C`), `Inventory.m_InventoryInfo 0x98`,
  `Weapon : Inventory` and the ctor RVAs (`RoleConfConfig 0x90F20BC`,
  `RolePackConfConfig 0x90F784C`, `RoleSkinConfig 0x90F95C8`,
  `CharacterModelConfig 0xB1A9738`, `SPKillIconItemConfConfig 0xAF67F1C`)
  re-read out of `dump.cs`.
- Still not run here: `ndk-build`, the APK and anything on-device.

## Not verified

- No Android NDK here: `ndk-build`, the APK and in-match behaviour are untested
  by this report. Host checks validate syntax/types, not the device ABI.
- If the game inlines `get_KillIconItemID`, hooking it cannot change the kill
  icon; the diagnostics distinguish that case (`attr=0/0` = never called) from a
  key mismatch (`calls>0, hits=0`). That case is now covered by the
  `InventoryInfo` write described above (`icon=` in the diagnostics), which does
  not depend on the getter being called. Writing `WeaponAttrData.m_KillIconItem
  @0x78` instead would need an existing `SimpleItem` instance for the icon item
  and is deliberately not attempted.
- `Weapon.get_KillEffectGroupIDOverride` keys on the weapon id, so on the local
  client an enemy holding the same weapon id would also render the forced effect.
## Same bug class, fixed in the follow-up pass

The character tab (`CharacterModelConfig x ItemResourceConfig x RoleConfConfig`,
with `RolePackConfConfig` refining the pack values) had two of these violations,
and both are what kept the list empty - the "character skins still isn't loading"
report:

- `RoleConfFields` was cast at `+0x18` while `RoleConfConfig`'s first field is
  `0x10` (dump 818159), so the `Z1->ID != W` match that populates the list read
  `0x3C` (`KeepInCatalog`) instead of `0x34` (`ID`) and **no character ever
  matched**. The cast is now `+0x10` and the declaration was rebuilt against the
  dump (which is unmasked for this class): the phantom `bool BlockBack` is gone
  and the real `int BlockAppearance 0x30` is in its place, so `ColorID 0x22`,
  `ColorSubID 0x23`, `RoleLvGroupID 0x2C`, `ShowRare 0x2F`, `ID 0x34` and
  `RolePackID 0x44` all land exactly where the class has them; the string tail
  was one pointer short, which put `LOCID_Name` on `0x98` instead of `0xA0`.
- `RolePackFields` was cast at `+0x18` too, and its declaration did not follow
  the live (alphabetically ordered) class at all. Both are fixed: base `+0x10`
  and the declaration rebuilt field for field, so `EntryAnimID 0x40`,
  `GestureId 0x54`, `HandEffectUI 0x58`, `KillStreakSkinID 0x68`,
  `LoadingFrame 0x6C`, `LobbySceneType 0x70` and `RolePackID 0x90` are real.
- `LoadCharacterSkins` also latched `loadCharacter = true` **unconditionally**,
  so the first 1.5 s poll - which almost always beats the config tables - marked
  the list "loaded" forever and no later call ever retried. It now latches only
  once all three tiers the join needs were actually present.

`VehicleSkinConfFields` (snowboard) is still cast at `+0x18` against
`VehicleSkinConfConfig.ColorID 0x10` (dump 822946) with a missing
`DisableInCollection`; unchanged here, it belongs to the snowboard tab.

- `ctorHook/ctorOffset.h` is an older duplicate of `Skin/Thread.h`'s loaders -
  it declares its own `sBool`/`activeKillEffects` and copies of the `+0x18`-based
  loops. Nothing includes it (verified by searching every file in the repo,
  build files included), so it is dead code, but it is the file where the old
  bases still live; a loader fix belongs in `Skin/Thread.h`.

## The mythic kill icon: the record the HUD reads

The getter hooks can only change the icon if the game both calls them **and**
accepts the id they return; an inlined `get_KillIconItemID` cannot be hooked at
all. There is a second, independent lever, and it is now used:

`public class Weapon : Inventory` (dump 737390), and `Inventory` carries
`protected InventoryInfo m_InventoryInfo; // 0x98` (dump 761114,
`get_InventoryInfo 0xC362DCC`). `InventoryInfo` (dump 1088476, TypeDefIndex
23000) is the struct the in-match HUD reads its decorations from - `KillIcon
0x4C`, `KillEffectGroupID 0x40`, `KillBoard 0x48`, `MythicArmorID 0x50`,
`KillEffectLevel 0x5C`. `KillDecoration_ApplyLocalInventoryInfo()` (called once
per rendered frame, local player's held weapon only) writes `KillIcon` when the
applied-skin map has that weapon and raises `KillEffectLevel` to 5. Only when
the value actually differs, and `KillEffectGroupID` is deliberately untouched:
kill effects already work through the getter hooks, and that map's value is an
effect id or an asset id depending on which source won at load time.

Left otherwise correct and unchanged: `RoleSkinFields +0x18`
(`RoleSkinConfig.ClothType 0x18`), `MythicArmorFields +0x14`,
`WeaponConfFields +0x20`, `Item2InventoryFields +0x20`,
`WeaponAssetGroupFields +0x40`, `SkinPtr`/`GetIl2CppArrayLen`.
