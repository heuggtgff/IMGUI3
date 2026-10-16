#pragma once

float SlideRange;
bool isExecute;

enum TouchPhase {
    Began = 0,
    Moved = 1,
    Stationary = 2,
    Ended = 3,
    Canceled = 4,
};

enum TouchType {
    Direct = 0,
    Indirect = 1,
    Stylus = 2,
};

struct Touch {
    int m_FingerId;
    Vector2 m_Position;
    Vector2 m_RawPosition;
    Vector2 m_PositionDelta;
    float m_TimeDelta;
    int m_TapCount;
    TouchPhase m_Phase;
    TouchType m_Type;
    float m_Pressure;
    float m_maximumPossiblePressure;
    float m_Radius;
    float m_RadiusVariance;
    float m_AltitudeAngle;
    float m_AzimuthAngle;
};

enum LineTarget {
    Top = 0,
    Center = 1,
    Bottom = 2
};

enum EspBoxType {
    Fill = 0,
    Outline = 1,
    Corner = 2,
    ThreeD = 3
};

enum EspHealthPosition {
    HealthTop = 0,
    HealthSide = 1
};

enum CrosshairTarget {
    Normal = 0,
    Circle = 1,
    Cross = 2
};

enum EspStyleTarget {
    EspStyleNone = 0,
    EspStyle3DSphere = 1,
    EspStylePlayerSignal = 2
};

enum EAim {
  Distance = 0,
  Crosshair = 1
};

enum EAimTarget {
    Heads = 0,
    Chests = 1,
    Body = 2
};

enum EAimTrigger {
    None = 0,
    Shooting = 1,
    Scoping = 2
};

struct sConfig {
        float Pline;
		float Bline;
		float PskelLine;
		float BskelLine;
        bool TeleportEnemy = false;
        bool UnlimitedAmmo;
    struct sInitImGui {
        uintptr_t thiz;
    };
    sInitImGui ImGuiMenu{0};
    
struct sWeaponAim {
        bool Aimbot360;
        float AimAssistSize;
        bool AimSilent;
        EAimTarget Target;
        EAimTrigger Trigger;
        EAim By;
        float size;
        float Cross;
        
         // Frame Rate Controls
        bool FpsLevel;
        float FpsLevel1;
        bool showFPSLevelSlider;

        bool FpsLevelUltra;              // Ultra FPS toggle
        float FpsLevelUltra1 = 240.0f;   // Default Ultra FPS value
        bool showFPSLevelUltraSlider;    // Show the Ultra FPS slider
    };
    sWeaponAim Aim{0};
    
    struct sESPMenuLineScale {
        float lineSize;
    };
    sESPMenuLineScale sESPMenuLineScale{0};
    
    struct sESPMenu {
        bool Signal;
        bool Alert;
        bool Count;
        bool Name;
        bool isPlayerLine;
        LineTarget Target;
        EspBoxType BoxType;
        EspHealthPosition HealthPosition;
        CrosshairTarget CrosshairType;
        EspStyleTarget EspStyle;
        bool Armor;
        bool GrenadeWarn;
        bool BRClass;
        bool Vehicle;
        bool VehicleHealth;
        bool Box;
        bool Health;
        bool Distance;
        bool Skeleton;
        bool ShowFov;
        bool Crosshair;
        bool Aimline;
    };
    sESPMenu ESPMenu{0};
    
    struct sColorsESPPLAYER {
    float *LinePLAYER;
    float *BoxPLAYER;
    float *NamePLAYER;
    float *HealthPLAYER;
    float *DistancePLAYER;
    float *SkeletonPLAYER;
};
sColorsESPPLAYER sColorsESPPLAYER{0};

struct sColorsESPBOT {
    float *LineBOT;
    float *BoxBOT;
    float *NameBOT;
    float *HealthBOT;
    float *DistanceBOT;
    float *SkeletonBOT;
};
sColorsESPBOT sColorsESPBOT{0};
    
    struct sColorsESPOTHERS {
        float *PovOTHERS;
    };
    sColorsESPOTHERS sColorsESPOTHERS{0};
    
    struct sExtraMenu {
        bool SmartReload;
        bool NoGravity;
        float NoGravityScale = 1.0f;
        bool ClearTerrain;
        bool BlackSky;
        bool Execute;
        bool NoSmoke;
        bool UnliAmmo;
        float VehicleScale = 1.0f;
        bool SpeedhackX;
        bool CameraPov;
        float CameraPovSize;
        bool LongSlide;
        bool UnliSlide;
        bool NoWingsuit;
        bool WalkUnderWater;
        bool NoCrouch;
        bool Frame;
        bool HighFps;
        bool Fps;
        bool Grap;
        bool Grapss;
        bool EnhanceFPS;
        bool Spectatex;
        bool Attachment;
        // "Mythic Attachments": with an applied weapon skin, the client answers
        // YES to the skin's own unlock question and to every attachment unlock
        // question (see System/Hooks/MythicAttachments.h). ON by default - this
        // is what makes an applied mythic behave as if it were really owned.
        bool MythicAttachments = true;
		bool Blueprints;
		bool ClearDisplay;
		bool ResetGuest;
		bool RedWallhack;
    	bool Spread;
    	bool Fire;
    	bool Diving;
        bool Recoil;
        bool Reload;
        bool Shake;
        bool Scope;
        bool Switch;
        bool Flash;
        bool Hit;
        bool Rpd;
        bool Parachute;
		bool WallHack;
		bool Kinetic;
        bool CamoTest;
        int CamoTestMode;
    };
    sExtraMenu ExtraMenu{0};

	struct sColorsESP {
	
        float *Vehicle;
    };
    sColorsESP ColorsESP{0};

    // New Features menu (VAR3)
    struct sNewFeaturesMenu
    {
        bool NoCrouch = false;
        bool Triggerbot = false;
        int TriggerbotMode = 0;        // 0 = Always, 1 = ADS only, 2 = Firing only
        int TriggerbotDelay = 0;       // ms (humanized reaction delay)
        int TriggerbotRadius = 40;     // LEGACY, unused: the zone IS the dot crosshair now
                                       // (Crosshair.Size below). Kept only so old
                                       // config files keep loading.
        bool NoHitFlinch = false;
        bool NoDrift = false;
        bool NoSlideFatigue = false;
        // Firing timing - each removes exactly one delay, nothing else
        bool NoSprintToFireDelay = false;  // sprint-out timer before the gun fires
        bool NoHipfireDelay = false;       // hipfire get-ready / wind-up gate
    };
    sNewFeaturesMenu NewFeaturesMenu{0};

    // The menu's own dot crosshair - and the triggerbot's crosshair. There is
    // exactly ONE crosshair in this build: the dot sits at the render centre plus
    // OffsetX/OffsetY in pixels, that pixel is what the trigger aims through, and
    // the dot's own radius IS the trigger's zone. Nothing else feeds the trigger's
    // geometry, so shrinking the dot tightens the trigger by exactly the same
    // amount and the two can never drift apart. On by default: this IS the
    // crosshair now.
    struct sCrosshair
    {
        bool  Enabled = true;           // draw the dot (the trigger aims here either way)
        float Color[4] = {1.0f, 1.0f, 1.0f, 0.95f};  // RGB + opacity (alpha)
        float Size = 6.0f;              // dot diameter in px == the trigger zone diameter
        float OffsetX = 0.0f;           // px right of centre (0 = centre)
        float OffsetY = 0.0f;           // px below centre (0 = centre)
    };
    sCrosshair Crosshair{};
};

sConfig Config{0};

