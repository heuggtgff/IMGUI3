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

    inline void NoteAsk(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) g_diag.asked[gate]++;
    }

    inline void NoteGrant(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) g_diag.granted[gate]++;
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
