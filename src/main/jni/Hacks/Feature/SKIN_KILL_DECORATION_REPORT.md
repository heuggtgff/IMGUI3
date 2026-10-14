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
`itemFields = res + 0x18`, so `itemFields->ID` read `res + 0x34`
(`ModelAssetIDRaw`) instead of `res + 0x2C`, and `getitem.ItemRes` /
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
- Loader bases corrected to `+0x10` for the six classes above; `RealAssetIDs`
  read at `kill + 0x10`.
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
  and `-D__fastcall=`: **0 errors**; with `-Wall -Wextra -Wformat=2` the only
  warnings are pre-existing (unused locals, narrowing in `charData`), none in the
  new code, and the new `ImGui::TextColored` format rows are clean.
- Brace/paren/bracket balance with comments and string literals stripped:
  `Skin/Thread.h` 0/0/0, `System/UI/SkinTab.h` 0/0/0, `Skin/Fields.h` 0/0/0.
- Every class offset and RVA used above re-read out of `dump.cs`.

## Not verified

- No Android NDK here: `ndk-build`, the APK and in-match behaviour are untested
  by this report. Host checks validate syntax/types, not the device ABI.
- If the game inlines `get_KillIconItemID`, hooking it cannot change the kill
  icon; the diagnostics distinguish that case (`attr=0/0` = never called) from a
  key mismatch (`calls>0, hits=0`). The remaining lever there is writing
  `WeaponAttrData.m_KillIconItem @0x78` on the local loadout's record, which
  needs an existing `SimpleItem` instance for the item.
- `Weapon.get_KillEffectGroupIDOverride` keys on the weapon id, so on the local
  client an enemy holding the same weapon id would also render the forced effect.
- The character (`RoleConfFields`) path still carries its own stale tail
  (`RolePackID` etc.) and was left untouched.
