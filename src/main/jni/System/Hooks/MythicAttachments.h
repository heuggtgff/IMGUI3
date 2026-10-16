#pragma once

// ============================================================================
//  MythicAttachments.h  -  "Mythic Attachments": an applied weapon skin behaves
//  as if it were REALLY OWNED, so the weapon's attachments (the mythic's own
//  ones included) are unlocked instead of shown locked.
//
//  The mod already applies the skin itself - the Skin tab writes the weapon's
//  config rows (WeaponConfExtraConfig.DefaultMythicArmor / DefaultMythicSig,
//  the skin id, the camo / kill-decoration records). What is left of "as if I
//  owned it" are the four CLIENT-SIDE unlock answers the loadout asks:
//
//   1. GameBase.WeaponPromotionInfo.IsWeaponAttachmentUnlock(int attachment)
//        RVA 0xA61AA00  (dump.cs 1390332, field WeaponAttachmentUnlockMap 0x18)
//        "is this attachment unlocked at this weapon's own level" - the map the
//        weapon's promotion level fills in. Also the address this repo already
//        hooked for the old Config.ExtraMenu.Attachment flag.
//   2. GameBase.WeaponPromotionInfo.IsWeaponSkinUnlock(uint skinId)
//        RVA 0xA61AD54  (dump.cs 1390344)
//        "is this weapon skin unlocked for this weapon" - the very question
//        owning the skin answers, which is what makes the applied mythic count
//        as owned here.
//   3. LoadoutWeaponsUIHelper.IsThisAttachmentUnlock(uint blueprintWeaponId,
//      uint attachID, bool isCheckWeaponUnlock)
//        RVA 0x5D40528  (dump.cs 2072765)
//        the LOADOUT's own gate: an attachment it says is not unlocked is drawn
//        with the lock. This is the one that greys out / locks the attachment
//        list for a weapon the skin was applied to.
//   4. WeaponAttr.IsAttachUnlock(uint weaponID, uint attachID)
//        RVA 0xA188634  (dump.cs 2045791)
//        the confirm-time validator of the attachment popup (WeaponAttr.
//        OnConfirm -> SetData/EquipWeaponSyncAttch), i.e. the last client gate
//        before the equip request is sent.
//
//  LAYER 2 - the part that behaves like the skin: dump-driven WRITES into the
//  client's own objects, so no "is it unlocked?" answer has to be asked for the
//  lock to disappear.
//
//   5. LoadoutWeaponsAttachmentController.GetAttachmentList(int)
//        RVA 0xB6323C0  (dump.cs 1163747)
//        BUILDS the attachment rows the popup draws. The returned
//        List<AttachmentGroupNew> is walked and every row whose EAttachmentOwnState
//        is ELock (0) is rewritten to EUnlock (1) - the same kind of field write
//        the Skin tab performs when it applies a mythic.
//   6. LoadoutWeaponsAttachmentItemController.SetData(AttachmentGroupNew,
//      out object panel)
//        RVA 0xB83F1C0  (dump.cs 1164290)
//        the row is handed to its view here, so AttachmentGroupNew.OwnState
//        (0x3C) is forced BEFORE the game reads it.
//   7. LoadoutWeaponsAttachmentCell.SetData(..., int cellState, ...)
//        RVA 0xB83D5E0  (dump.cs 1319236)
//        the gunsmith's own slots (Muzzle / Barrel / Optic / ...) are cells with
//        an EAttachmentCellState: ELocked (3) is rewritten to ENone (0), so the
//        slot draws unlocked and its click is not treated as a locked slot
//        (dump.cs 2076253 for the enum).
//
//  Why layer 2 exists: on a phone layer 1 reported asked/granted 0/0 - this
//  build never asks those four (they are tiny and get inlined into their
//  callers). The three below are multi-kilobyte functions ON the render path of
//  the very screen that draws the lock, so they cannot be skipped that way.
//
//  All four answer the honest value while the toggle is off, and the original
//  game function is always the fallback (never a hard-coded false), so turning
//  the option off restores stock behaviour exactly.
//
//  WHAT THIS IS NOT: it is a CLIENT answer, not a server grant. The loadout
//  still has to survive the server's own inventory check when it is saved, so
//  an attachment that is genuinely not unlocked can be rejected there. The
//  mythic's *look* for each attachment (AttachItem.m_mythicSkinID 0x18 /
//  AttachmentGroupOld.MythicSkinID 0x18, dump.cs 2073122 / 2287685) is data
//  resolved from the weapon's mythic armor and is deliberately NOT faked here -
//  inventing an attachment-skin id would put the wrong object on the gun.
//
//  Config: Config.ExtraMenu.MythicAttachments (Hacks/Feature/Init_Hacks.h),
//  default ON. The pre-existing Config.ExtraMenu.Attachment flag is ORed into
//  the attachment half, so an existing config that set it keeps working.
//
//  Include contract: this header uses sConfig Config, so it is included after
//  the game structs (Hacks/Hacks.h -> Feature/Feature.h -> Init_Hacks.h), the
//  same contract Feats.h itself has. It includes Init_Hacks.h so a test TU can
//  pull just this file after the structs.
// ============================================================================

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace mythic_attachments
{
    // The four answers, straight from the dump (RVA / Offset pairs as printed by
    // dump.cs, i.e. what getAbsoluteAddress() wants).
    namespace Rva
    {
        constexpr uintptr_t WeaponPromotion_IsWeaponAttachmentUnlock = 0xA61AA00;
        constexpr uintptr_t WeaponPromotion_IsWeaponSkinUnlock       = 0xA61AD54;
        constexpr uintptr_t Loadout_IsThisAttachmentUnlock           = 0x5D40528;
        constexpr uintptr_t WeaponAttr_IsAttachUnlock                = 0xA188634;

        // Layer 2 - the client-side data writes (dump.cs 1163747 / 1164290 /
        // 1319236), installed next to the four answers in Feats.h.
        constexpr uintptr_t AttachmentList_GetAttachmentList         = 0xB6323C0;
        constexpr uintptr_t AttachmentItem_SetData                   = 0xB83F1C0;
        constexpr uintptr_t AttachmentCell_SetData                   = 0xB83D5E0;
    }

    // The attachment half: the new toggle, plus the flag this repo shipped
    // before it (which had no menu row of its own, so it can only be set from a
    // config file).
    inline bool AttachmentsUnlocked()
    {
        return Config.ExtraMenu.MythicAttachments || Config.ExtraMenu.Attachment;
    }

    // The ownership half - "the skin I applied counts as unlocked".
    inline bool SkinsUnlocked()
    {
        return Config.ExtraMenu.MythicAttachments;
    }

    // The four questions, as an index. Used by the counters below and by the
    // SKINS tab's diagnostic line.
    enum Gate
    {
        GateAttachment = 0,   // WeaponPromotionInfo.IsWeaponAttachmentUnlock
        GateSkin,             // WeaponPromotionInfo.IsWeaponSkinUnlock
        GateLoadout,          // LoadoutWeaponsUIHelper.IsThisAttachmentUnlock
        GateConfirm,          // WeaponAttr.IsAttachUnlock
        GateList,             // LoadoutWeaponsAttachmentController.GetAttachmentList
        GateItem,             // LoadoutWeaponsAttachmentItemController.SetData
        GateCell,             // LoadoutWeaponsAttachmentCell.SetData
        GateCount
    };

    // On-device instrument. The loadout and skin UI ask these questions by
    // themselves whenever they draw, so the SKINS tab prints how often each gate
    // was asked and how often this feature granted it - that is how the feature
    // is confirmed on a phone (nothing here gates a hook; the hook bodies below
    // write the counters and the menu reads them).
    struct DiagStats
    {
        int asked[GateCount];
        int granted[GateCount];
    };
    inline DiagStats g_diag{};

    // Install-time truth, one flag per gate: Feats.h sets it right after the
    // hook call from whether the hooker filled in the original pointer. A 0 here
    // is the reason the counters above stay at 0/0 - the address never took the
    // hook (a dump that no longer matches the installed game, or a failed
    // install), which is the first thing to check after a game update.
    inline bool g_landed[GateCount];
    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    inline void NoteAsk(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) g_diag.asked[gate]++;
    }

    inline void NoteGrant(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) g_diag.granted[gate]++;
    }

    // ---- layer 2: field layout of the client's own objects -----------------
    // IL2CPP objects carry a 16-byte header, so every dump offset below is the
    // ABSOLUTE offset from the object pointer - the same convention the Skin tab
    // already uses when it casts a config instance to its field struct.
    constexpr uintptr_t kAttachGroupNew_OwnState = 0x3C;   // int, dump.cs 2073151
    constexpr int       kOwnStateELock   = 0;              // EAttachmentOwnState
    constexpr int       kOwnStateEUnlock = 1;
    constexpr int       kCellStateELocked = 3;             // EAttachmentCellState
    constexpr int       kCellStateENone   = 0;
    constexpr uintptr_t kList_Items    = 0x10;             // List<T>._items
    constexpr uintptr_t kList_Size     = 0x18;             // List<T>._size
    constexpr uintptr_t kArray_MaxLen  = 0x18;             // Il2CppArray.max_length
    constexpr uintptr_t kArray_Data    = 0x20;             // Il2CppArray[0]

    // Force every ELock row of a List<AttachmentGroupNew> to EUnlock and return
    // how many were rewritten. The walk is bounded by the backing array's OWN
    // length (and a hard 4096-entry ceiling, far above any real attachment
    // list), so a list that is not what the dump says it is stops at the first
    // impossible size instead of walking off into the heap.
    inline int UnlockOwnStateInGroupList(void *list)
    {
        if (list == nullptr) return 0;
        void *items = *(void **)((char *)list + kList_Items);
        if (items == nullptr) return 0;

        const int64_t size = *(int *)((char *)list + kList_Size);
        const uint64_t maxLen = *(uint64_t *)((char *)items + kArray_MaxLen);
        if (size <= 0 || maxLen > 4096 || (uint64_t)size > maxLen) return 0;

        char *data = (char *)items + kArray_Data;
        int forced = 0;
        for (int i = 0; i < (int)size; i++)
        {
            void *group = ((void **)data)[i];
            if (group == nullptr) continue;
            int *own = (int *)((char *)group + kAttachGroupNew_OwnState);
            if (*own == kOwnStateELock) { *own = kOwnStateEUnlock; forced++; }
        }
        return forced;
    }

    // The same write for one row (the SetData hooks). True only when a lock was
    // actually replaced, so a row that is already EEquip / EDefault / EUnlock is
    // left exactly as the game set it.
    inline bool UnlockOwnStateGroup(void *group)
    {
        if (group == nullptr) return false;
        int *own = (int *)((char *)group + kAttachGroupNew_OwnState);
        if (*own != kOwnStateELock) return false;
        *own = kOwnStateEUnlock;
        return true;
    }
}

// ---- 1. the weapon's own attachment-unlock set ----------------------------
// dump.cs 1390332: bool IsWeaponAttachmentUnlock(int attachment) - an instance
// method, so the replacement takes (this, attachment) and hands BOTH back to the
// original. (The pre-mythic build declared the original as taking only the
// instance, so every pass-through call fed the real function an uninitialised
// argument - the fallback path is now signature-correct.)
inline bool (*orig_IsWeaponAttachmentUnlock)(void *instance, int attachment) = nullptr;
inline bool IsWeaponAttachmentUnlock(void *instance, int attachment)
{
    mythic_attachments::NoteAsk(mythic_attachments::GateAttachment);
    if (instance != nullptr && mythic_attachments::AttachmentsUnlocked()) {
        mythic_attachments::NoteGrant(mythic_attachments::GateAttachment);
        return true;
    }
    return orig_IsWeaponAttachmentUnlock != nullptr
         ? orig_IsWeaponAttachmentUnlock(instance, attachment)
         : false;
}

// ---- 2. the skin's own unlock question ------------------------------------
// dump.cs 1390344: bool IsWeaponSkinUnlock(uint skinId).
inline bool (*orig_IsWeaponSkinUnlock)(void *instance, uint32_t skinId) = nullptr;
inline bool IsWeaponSkinUnlock(void *instance, uint32_t skinId)
{
    mythic_attachments::NoteAsk(mythic_attachments::GateSkin);
    if (instance != nullptr && mythic_attachments::SkinsUnlocked()) {
        mythic_attachments::NoteGrant(mythic_attachments::GateSkin);
        return true;
    }
    return orig_IsWeaponSkinUnlock != nullptr
         ? orig_IsWeaponSkinUnlock(instance, skinId)
         : false;
}

// ---- 3. the loadout's attachment gate ------------------------------------
// dump.cs 2072765: bool IsThisAttachmentUnlock(uint blueprintWeaponId,
// uint attachID, bool isCheckWeaponUnlock = false). The third argument is the
// caller's "also check the weapon itself" switch and is passed straight through
// to the original, so only the unlock verdict is replaced.
inline bool (*orig_IsThisAttachmentUnlock)(void *instance, uint32_t blueprintWeaponId,
                                           uint32_t attachID, bool isCheckWeaponUnlock) = nullptr;
inline bool IsThisAttachmentUnlock(void *instance, uint32_t blueprintWeaponId,
                                   uint32_t attachID, bool isCheckWeaponUnlock)
{
    mythic_attachments::NoteAsk(mythic_attachments::GateLoadout);
    if (instance != nullptr && mythic_attachments::AttachmentsUnlocked()) {
        mythic_attachments::NoteGrant(mythic_attachments::GateLoadout);
        return true;
    }
    return orig_IsThisAttachmentUnlock != nullptr
         ? orig_IsThisAttachmentUnlock(instance, blueprintWeaponId, attachID, isCheckWeaponUnlock)
         : false;
}

// ---- 4. the confirm-time validator ---------------------------------------
// dump.cs 2045791: bool IsAttachUnlock(uint weaponID, uint attachID).
inline bool (*orig_IsAttachUnlock)(void *instance, uint32_t weaponID, uint32_t attachID) = nullptr;
inline bool IsAttachUnlock(void *instance, uint32_t weaponID, uint32_t attachID)
{
    mythic_attachments::NoteAsk(mythic_attachments::GateConfirm);
    if (instance != nullptr && mythic_attachments::AttachmentsUnlocked()) {
        mythic_attachments::NoteGrant(mythic_attachments::GateConfirm);
        return true;
    }
    return orig_IsAttachUnlock != nullptr
         ? orig_IsAttachUnlock(instance, weaponID, attachID)
         : false;
}

// ---- 5. the attachment list the popup draws (data write) -----------------
// dump.cs 1163747: private List<AttachmentGroupNew> GetAttachmentList(int).
// The rows are BUILT here with their EAttachmentOwnState, so rewriting them on
// the way out is the client-side equivalent of what the Skin tab does: the
// object the game will read next simply says "unlocked".
inline void *(*orig_GetAttachmentList)(void *instance, int attachmentType) = nullptr;
inline void *GetAttachmentList(void *instance, int attachmentType)
{
    void *list = orig_GetAttachmentList != nullptr
               ? orig_GetAttachmentList(instance, attachmentType)
               : nullptr;
    mythic_attachments::NoteAsk(mythic_attachments::GateList);
    if (list != nullptr && mythic_attachments::AttachmentsUnlocked() &&
        mythic_attachments::UnlockOwnStateInGroupList(list) > 0) {
        mythic_attachments::NoteGrant(mythic_attachments::GateList);
    }
    return list;
}

// ---- 6. the row handed to its view (data write) --------------------------
// dump.cs 1164290: public void SetData(AttachmentGroupNew attachmentInfo,
// out object panel). OwnState is forced BEFORE the original runs, so the view
// that is about to read it sees the unlocked value.
inline void (*orig_AttachmentItemSetData)(void *instance, void *group, void *panel) = nullptr;
inline void AttachmentItemSetData(void *instance, void *group, void *panel)
{
    mythic_attachments::NoteAsk(mythic_attachments::GateItem);
    if (mythic_attachments::AttachmentsUnlocked() &&
        mythic_attachments::UnlockOwnStateGroup(group)) {
        mythic_attachments::NoteGrant(mythic_attachments::GateItem);
    }
    if (orig_AttachmentItemSetData != nullptr)
        orig_AttachmentItemSetData(instance, group, panel);
}

// ---- 7. the gunsmith slot cell (data write) ------------------------------
// dump.cs 1319236: public void SetData(uint weaponID, int attachmentType,
// AttachItem attachItem, OnClickCallbackHandler callback, int cellState,
// string conflictAttachmentStr, string posName, bool isTransmogrification,
// bool enableReddot = true, bool isGuarActiveTipVisible = false). The signature
// is declared exactly like the dump so every argument - the two defaulted bools
// included - travels through to the real function untouched; only cellState is
// changed, and only ELocked -> ENone (a conflict / invalid slot keeps the state
// the game gave it).
inline void (*orig_AttachmentCellSetData)(void *instance, uint32_t weaponID,
                                          int32_t attachmentType, void *attachItem,
                                          void *callback, int32_t cellState,
                                          void *conflictAttachmentStr, void *posName,
                                          bool isTransmogrification, bool enableReddot,
                                          bool isGuarActiveTipVisible) = nullptr;
inline void AttachmentCellSetData(void *instance, uint32_t weaponID,
                                  int32_t attachmentType, void *attachItem,
                                  void *callback, int32_t cellState,
                                  void *conflictAttachmentStr, void *posName,
                                  bool isTransmogrification, bool enableReddot,
                                  bool isGuarActiveTipVisible)
{
    mythic_attachments::NoteAsk(mythic_attachments::GateCell);
    if (cellState == mythic_attachments::kCellStateELocked &&
        mythic_attachments::AttachmentsUnlocked()) {
        cellState = mythic_attachments::kCellStateENone;
        mythic_attachments::NoteGrant(mythic_attachments::GateCell);
    }
    if (orig_AttachmentCellSetData != nullptr)
        orig_AttachmentCellSetData(instance, weaponID, attachmentType, attachItem, callback,
                                   cellState, conflictAttachmentStr, posName,
                                   isTransmogrification, enableReddot, isGuarActiveTipVisible);
}
