#pragma once

#define m_IL2CPPLIB OBFUSCATE("libanogs.so")
#define m_ANOGSLIB OBFUSCATE("libanogs.so")
#include "../Feature/ToString.h"

#define SLEEP_TIME 1000LL / 60LL

uintptr_t Class_Camera_getAllCamera;

class Transform;

class Component {
public:
    Transform *get_transform() {
        auto _Component__get_transform = (Transform *(*)(Component *)) (Class_Component_get_transform);
        return _Component__get_transform(this);
    }
};

class Transform : public Component {
public:
    Vector3 get_position() {
        auto _Transform__get_position = (Vector3 (*)(Transform *)) (Class_Transform_get_position);
        return _Transform__get_position(this);
    }

	//-- Vehicle Size
    void set_localScale(Vector3 scale) {
        auto _Transform__set_localScale = (void (*)(Transform *, Vector3)) (Class_Transform_set_localScale);
        if (_Transform__set_localScale) {
            _Transform__set_localScale(this, scale);
        }
    }
};

class Camera : public Component {
public:
    static Camera *get_main() {
        auto _Camera__get_main = (Camera *(*)()) (Class_Camera_get_main);
        return _Camera__get_main();
    }

    Vector3 WorldToScreenPoint(Vector3 position) {
        auto main = Camera::get_main();
        if (main) {
            auto _Camera__WorldToScreenPoint = (Vector3 (*)(Camera *, Vector3)) (Class_Camera_WorldToScreenPoint);
            return _Camera__WorldToScreenPoint(main, position);
        }
        return {0, 0, 0};
    }
};

class AttackableTargetInfo {
public:
    float m_Health() {
        return *(float *) ((uintptr_t) this + Class_AttackableTarget_m_Health);
    }

    float m_MaxHealth() {
        return *(float *) ((uintptr_t) this + Class_AttackableTarget_m_MaxHealth);
    }

    float m_ExtraHealth() {
        return *(float *) ((uintptr_t) this + Class_AttackableTarget_m_ExtraHealth);
    }

    float m_MaxExtraHealth() {
        return *(float *) ((uintptr_t) this + Class_AttackableTarget_m_MaxExtraHealth);
    }
	
	int m_LadderLevel() {
        return *(int *) ((uintptr_t) this + api26);
    }
    
    int m_LadderScore() {
        return *(int *) ((uintptr_t) this + api27);
    }
    
    int m_Level() {
        return *(int *) ((uintptr_t) this + api28);
    }
    
    int m_KillEnemyCount() {
        return *(int *) ((uintptr_t) this + api29);
    }
    
    int m_DeathCount() {
        return *(int *) ((uintptr_t) this + api30);
    }
    
    int m_AssistCount() {
        return *(int *) ((uintptr_t) this + api31);
    }
};

class AttackableTarget {
public:
    AttackableTargetInfo *m_AttackableInfo() {
        return *(AttackableTargetInfo **) ((uintptr_t) this + Class_AttackableTarget_m_AttackableInfo);
    }
};

class Weapon {
public:
    int get_WeaponID() {
        using GetWeaponIDFunc = int (*)(Weapon*);
        static GetWeaponIDFunc _Weapon__get_WeaponID = (GetWeaponIDFunc)(Weapon_GetWeaponId);
        return _Weapon__get_WeaponID ? _Weapon__get_WeaponID(this) : 0;
    }
};

class Pawn : public AttackableTarget {
public:

    Transform *get_transform() {
        auto _Component__get_transform = (Transform *(*)(Component *)) (Class_Component_get_transform);
        return _Component__get_transform((Component *)this);
    }
    
    Vector3 get_LastPawnPos() {
        auto _Pawn__get_LastPawnPos = (Vector3 (*)(Pawn *)) (Class_Pawn_get_LastPawnPos);
        return _Pawn__get_LastPawnPos(this);
    }

    Vector3 get_HeadPosition() {
        auto _Pawn__get_HeadPosition = (Vector3 (*)(Pawn *)) (Class_Pawn_get_HeadPosition);
        return _Pawn__get_HeadPosition(this);
    }

    bool m_IsBot() {
        return *(bool *) ((uintptr_t) this + Class_Pawn_m_IsBot);
    }

    bool m_IsAlive() {
        return *(bool *) ((uintptr_t) this + Class_Pawn_m_IsAlive);
    }

    String *get_PlayerName() {
        auto _Pawn__get_PlayerName = (String * (*)(Pawn *)) (Class_Pawn_get_PlayerName);
        return _Pawn__get_PlayerName(this);
    }

    unsigned int *get_TeamSeatId() {
        auto _Pawn__get_HeadPosition = (unsigned int *(*)(Pawn *)) (Class_Pawn_get_TeamSeatId);
        return _Pawn__get_HeadPosition(this);
    }

    int m_LastFireWeaponName() {
        return *(int *) ((uintptr_t) this + Class_Pawn_m_LastFireWeaponName);
    }

    Weapon* get_CurrentWeapon() {
        using GetCurrentWeaponFunc = Weapon* (*)(Pawn*);
        static GetCurrentWeaponFunc _Pawn__get_CurrentWeapon = (GetCurrentWeaponFunc)(Pawn_get_CurrentWeapon);
        return _Pawn__get_CurrentWeapon ? _Pawn__get_CurrentWeapon(this) : nullptr;
    }
};

class PlayerInfo {
public:
    PlayerInfo *m_PlayerInfo(Pawn *thiz) {
        return *(PlayerInfo **) ((uintptr_t) thiz + Class_Pawn_m_PlayerInfo);
    }

    String *m_NickName(PlayerInfo *thiz) {
        return *(String **) ((uintptr_t) thiz + Class_PlayerInfo_m_NickName);
    }
    
    int m_VestValue() {
        return *(int *) ((uintptr_t) this + Class_PlayerInfo_m_VestValue);
    }

    int m_VestMaxValue() {
        return *(int *) ((uintptr_t) this + Class_PlayerInfo_m_VestMaxValue);
    }
};

class BaseGame {
public:
    List<Pawn *> *EnemyPawns() {
        return *(List<Pawn *> **) ((uintptr_t) this + Class_BaseGame_EnemyPawns);
    }

	//-- Vehicle Scale
    List<uintptr_t> *AllVehicles() {
        return *(List<uintptr_t> **) ((uintptr_t) this + Class_BaseGame_m_AllVehicles);
    }
};

//-- ESP Items
class WeaponConfConfig {
public:
    String *MarkOfName() {
        return *(String **)((uintptr_t)this + Class_LethalAndTacticalConfConfig_MarkOfName);
    }
};

class LethalAndTacticalConfConfig {
public:
    String *MarkOfName() {
        return *(String **)((uintptr_t)this + Class_LethalAndTacticalConfConfig_MarkOfName);
    }
};

class BRDroppedPickUpNormal : public Component {
public:
    LethalAndTacticalConfConfig *mThrowConf() {
        return *(LethalAndTacticalConfConfig **)((uintptr_t)this + Class_BRDroppedPickUpNormal_mThrowConf);
    }
};

class BRDroppedPickUpMgr {
public:
    Il2CppDictionary<int, BRDroppedPickUpNormal *> *mNormalItemEntityDic() {
        return *(Il2CppDictionary<int, BRDroppedPickUpNormal *> **)((uintptr_t)this + Class_BRDroppedPickUpMgr_mNormalItemEntityDic);
    }
};

class MatchGame : public BaseGame {
public:
    BRDroppedPickUpMgr *get_DroppedPickUpMgr() {
        auto _MatchGame__get_DroppedPickUpMgr = (BRDroppedPickUpMgr *(*)(MatchGame *))(Class_MatchGame_get_DroppedPickUpMgr);
        return _MatchGame__get_DroppedPickUpMgr(this);
    }
}; //--

class GamePlay {
public:
    static Pawn *get_LocalPawn() {
        auto _GamePlay__get_LocalPawn = (Pawn *(*)()) (Class_Gameplay_get_LocalPawn);
        return _GamePlay__get_LocalPawn();
    }

    static MatchGame *get_MatchGame() {
        auto _GamePlay__get_MatchGame = (MatchGame *(*)()) (Class_Gameplay_get_MatchGame);
        return _GamePlay__get_MatchGame();
    }
};

class PhysicsVehicle : public Component {
public:
    EVehicleType vehicleType() {
        return *reinterpret_cast<EVehicleType *>((uintptr_t)this + Class_PhysicsVehicle__vehicleType);
    }
 
 float get_Health() {
        auto _PhysicsVehicle_get_Health = (float (*)(PhysicsVehicle *))(Class_PhysicsVehicle_get_Health);
        return _PhysicsVehicle_get_Health(this);
    }
};

#define SLEEP_TIME 1000LL / 60LL
