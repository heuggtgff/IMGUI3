#pragma once
namespace BoneMapOffsets {
    static constexpr uintptr_t m_BoneMappings = 0xA8;
    static constexpr uintptr_t m_LeftToe = 0x20;
    static constexpr uintptr_t m_LeftBetis = 0x28;
    static constexpr uintptr_t m_LeftPaha = 0x30;
    static constexpr uintptr_t m_RightToe = 0x38;
    static constexpr uintptr_t m_RightBetis = 0x40;
    static constexpr uintptr_t m_RightPaha = 0x48;
    static constexpr uintptr_t m_Hips = 0x50;
    static constexpr uintptr_t m_LeftHand = 0x58;
    static constexpr uintptr_t m_LeftArm = 0x60;
    static constexpr uintptr_t m_LeftUpperArm = 0x68;
    static constexpr uintptr_t m_Head = 0x70;
    static constexpr uintptr_t m_RightHand = 0x78;
    static constexpr uintptr_t m_RightArm = 0x80;
    static constexpr uintptr_t m_RightUpperArm = 0x88;
    static constexpr uintptr_t m_Neck = 0x90;
    static constexpr uintptr_t m_Body = 0x98;
}

static inline bool IsZeroVec3(const Vector3 &v) {
    return (v.x == 0.0f && v.y == 0.0f && v.z == 0.0f);
}

static inline Vector3 Normalized(Vector3 v) {
    float length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0.0f) {
        return Vector3(v.x / length, v.y / length, v.z / length);
    }
    return v;
}

static inline int SkeletonColorToU8(float v) {
    if (v <= 1.0f) v *= 255.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    return (int)(v + 0.5f);
}

static inline ImU32 SkeletonToColor(float *col) {
    if (!col) return IM_COL32(255, 255, 255, 255);
    return IM_COL32(
        SkeletonColorToU8(col[0]),
        SkeletonColorToU8(col[1]),
        SkeletonColorToU8(col[2]),
        SkeletonColorToU8(col[3])
    );
}

static inline uintptr_t SafeReadU64(uintptr_t address) {
    if (!Tools::IsPtrValid(reinterpret_cast<void *>(address))) {
        return 0;
    }
    return *reinterpret_cast<uintptr_t *>(address);
}

static inline Vector3 GetBoneMapPos(uintptr_t boneMap, uintptr_t boneSlotOffset) {
    if (!Tools::IsPtrValid(reinterpret_cast<void *>(boneMap))) {
        return Vector3::zero();
    }

    uintptr_t table = SafeReadU64(boneMap + 0x10);
    if (!Tools::IsPtrValid(reinterpret_cast<void *>(table))) {
        return Vector3::zero();
    }

    uintptr_t boneNode = SafeReadU64(table + boneSlotOffset);
    if (!Tools::IsPtrValid(reinterpret_cast<void *>(boneNode))) {
        return Vector3::zero();
    }

    uintptr_t transformPtr = SafeReadU64(boneNode + 0x40);
    if (!Tools::IsPtrValid(reinterpret_cast<void *>(transformPtr))) {
        return Vector3::zero();
    }

    Transform *boneTransform = reinterpret_cast<Transform *>(transformPtr);
    if (!Tools::IsPtrValid(boneTransform)) {
        return Vector3::zero();
    }

    return boneTransform->get_position();
}

static inline void DrawSkeletonLine(ImDrawList* draw, Vector3 from, Vector3 to, ImU32 color, float thickness, int sHeight) {
    auto mainCamera = Camera::get_main();
    if (!mainCamera) return;

    Vector3 fromScreen = mainCamera->WorldToScreenPoint(from);
    Vector3 toScreen = mainCamera->WorldToScreenPoint(to);

    if (fromScreen.z > 0 && toScreen.z > 0) {
        draw->AddLine(
            ImVec2(fromScreen.x, sHeight - fromScreen.y),
            ImVec2(toScreen.x, sHeight - toScreen.y),
            color,
            thickness
        );
    }
}

static bool DrawBoneMapSkeleton(ImDrawList *draw, Pawn *pawn, ImU32 color, float thickness, int sHeight) {
    uintptr_t pawnAddr = reinterpret_cast<uintptr_t>(pawn);
    uintptr_t boneMap = SafeReadU64(pawnAddr + BoneMapOffsets::m_BoneMappings);
    if (!Tools::IsPtrValid(reinterpret_cast<void *>(boneMap))) {
        return false;
    }

    Vector3 bones[16];
    bones[0] = GetBoneMapPos(boneMap, BoneMapOffsets::m_Head);
    bones[1] = GetBoneMapPos(boneMap, BoneMapOffsets::m_Neck);
    bones[2] = GetBoneMapPos(boneMap, BoneMapOffsets::m_Body);
    bones[3] = GetBoneMapPos(boneMap, BoneMapOffsets::m_Hips);
    bones[4] = GetBoneMapPos(boneMap, BoneMapOffsets::m_LeftUpperArm);
    bones[5] = GetBoneMapPos(boneMap, BoneMapOffsets::m_LeftArm);
    bones[6] = GetBoneMapPos(boneMap, BoneMapOffsets::m_LeftHand);
    bones[7] = GetBoneMapPos(boneMap, BoneMapOffsets::m_RightUpperArm);
    bones[8] = GetBoneMapPos(boneMap, BoneMapOffsets::m_RightArm);
    bones[9] = GetBoneMapPos(boneMap, BoneMapOffsets::m_RightHand);
    bones[10] = GetBoneMapPos(boneMap, BoneMapOffsets::m_LeftPaha);
    bones[11] = GetBoneMapPos(boneMap, BoneMapOffsets::m_LeftBetis);
    bones[12] = GetBoneMapPos(boneMap, BoneMapOffsets::m_LeftToe);
    bones[13] = GetBoneMapPos(boneMap, BoneMapOffsets::m_RightPaha);
    bones[14] = GetBoneMapPos(boneMap, BoneMapOffsets::m_RightBetis);
    bones[15] = GetBoneMapPos(boneMap, BoneMapOffsets::m_RightToe);

    if (IsZeroVec3(bones[0]) && IsZeroVec3(bones[1]) && IsZeroVec3(bones[2]) && IsZeroVec3(bones[3])) {
        return false;
    }

    auto mainCamera = Camera::get_main();
    if (!mainCamera) {
        return false;
    }

    Vector3 screenBones[16];
    bool visible[16];
    int visibleCount = 0;
    for (int i = 0; i < 16; ++i) {
        screenBones[i] = mainCamera->WorldToScreenPoint(bones[i]);
        visible[i] = (screenBones[i].z > 0.0f);
        if (visible[i]) {
            visibleCount++;
        }
    }
    if (visibleCount == 0) {
        return false;
    }

    float y[16];
    for (int i = 0; i < 16; ++i) {
        y[i] = sHeight - screenBones[i].y;
    }

    auto DrawSeg = [&](int a, int b) {
        if (visible[a] && visible[b]) {
            draw->AddLine(ImVec2(screenBones[a].x, y[a]), ImVec2(screenBones[b].x, y[b]), color, thickness);
        }
    };

    DrawSeg(0, 1);
    DrawSeg(1, 2);
    DrawSeg(2, 3);
    DrawSeg(1, 4);
    DrawSeg(4, 5);
    DrawSeg(5, 6);
    DrawSeg(1, 7);
    DrawSeg(7, 8);
    DrawSeg(8, 9);
    DrawSeg(3, 10);
    DrawSeg(10, 11);
    DrawSeg(11, 12);
    DrawSeg(3, 13);
    DrawSeg(13, 14);
    DrawSeg(14, 15);

    return true;
}

static inline void DrawCompleteSkeleton(ImDrawList *draw, Pawn *pawn, ImU32 color, float thickness, int sHeight) {
    DrawBoneMapSkeleton(draw, pawn, color, thickness, sHeight);
}

static inline void AddSkeletonToDrawESP(ImDrawList* draw, Pawn* pawn, bool isBot, int sHeight) {
    if (!Config.ESPMenu.Skeleton) return;

    ImU32 skeletonColor;
    float skeletonThickness;

    if (isBot) {
        skeletonColor = SkeletonToColor(Config.sColorsESPBOT.SkeletonBOT);
        skeletonThickness = Config.BskelLine;
    } else {
        skeletonColor = SkeletonToColor(Config.sColorsESPPLAYER.SkeletonPLAYER);
        skeletonThickness = Config.PskelLine;
    }

    DrawCompleteSkeleton(draw, pawn, skeletonColor, skeletonThickness, sHeight);
}
