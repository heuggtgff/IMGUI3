// Host syntax check for the whole skin / kill-decoration graph.
//
// System/UI/SkinTab.h pulls in Skin/Fields.h + Skin/Thread.h + Skin/Camo.h - i.e.
// every file the skin work lives in - and this TU instantiates the render entry
// point, so a change that only breaks in that include graph fails here instead of
// in ndk-build.
//
// This file is compiled with -fsyntax-only (nothing is linked, so every game
// symbol stays an unresolved stub) and it is never part of the APK: Android.mk's
// wildcards only cover ImGui, IL2CppSDKGenerator/IL2Cpp, KittyMemory,
// System/Texture and the jni root.
//
// Run it from the repo root:
//   g++ -std=c++17 -fsyntax-only -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Skin/tests/skin_headers_check.cpp
// Exits 0 with no output on success.
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include <thread>
#include <mutex>
#include "Hacks/StructGame/StructGame.h"

struct Skin_ExtraMenuStub { bool CamoTest; int CamoTestMode; };
struct Skin_ConfigStub { Skin_ExtraMenuStub ExtraMenu; };
static Skin_ConfigStub Config;

// Externs SkinTab.h expects from Main.cpp.
float menu[4];
bool showKeyboard = false;
ImFont* F50 = nullptr;
ImFont* JAAT = nullptr;
namespace font { ImFont* inter_semibold = nullptr; }

#include "System/UI/SkinTab.h"

void tu_use(void) { RenderSkinCategoryContent(0, true); }
