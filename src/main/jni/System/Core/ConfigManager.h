#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <jni.h>
#include "FileUtils.h"
#include "Hacks/Hacks.h"

extern JavaVM* VM;

void LoadConfig() {
    std::string textfile = GetFilesDirPath(VM) + "/codmconfig.ini";
    std::ifstream file(textfile.c_str());

    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        size_t lastSpace = line.find_last_of(' ');
        if (lastSpace == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, lastSpace);
        std::string valueStr = line.substr(lastSpace + 1);

        if (key == "ESPMenu.isPlayerLine") Config.ESPMenu.isPlayerLine = (valueStr == "1");
        else if (key == "ESPMenu.Box") Config.ESPMenu.Box = (valueStr == "1");
        else if (key == "ESPMenu.Health") Config.ESPMenu.Health = (valueStr == "1");
        else if (key == "ESPMenu.Name") Config.ESPMenu.Name = (valueStr == "1");
        else if (key == "ESPMenu.Distance") Config.ESPMenu.Distance = (valueStr == "1");
        else if (key == "ESPMenu.Count") Config.ESPMenu.Count = (valueStr == "1");
        else if (key == "ESPMenu.Crosshair") Config.ESPMenu.Crosshair = (valueStr == "1");
        else if (key == "ESPMenu.Aimline") Config.ESPMenu.Aimline = (valueStr == "1");
        else if (key == "ESPMenu.Target") Config.ESPMenu.Target = static_cast<LineTarget>(std::stoi(valueStr));
        else if (key == "ESPMenu.BoxType") Config.ESPMenu.BoxType = static_cast<EspBoxType>(std::stoi(valueStr));
        else if (key == "ESPMenu.HealthPosition") Config.ESPMenu.HealthPosition = static_cast<EspHealthPosition>(std::stoi(valueStr));
        else if (key == "ESPMenu.CrosshairType") Config.ESPMenu.CrosshairType = static_cast<CrosshairTarget>(std::stoi(valueStr));
        else if (key == "ESPMenu.EspStyle") Config.ESPMenu.EspStyle = static_cast<EspStyleTarget>(std::stoi(valueStr));
        else if (key == "Aim.Size") Config.Aim.size = std::stof(valueStr);
        else if (key == "Aim.AimAssistSize") Config.Aim.AimAssistSize = std::stof(valueStr);
        else if (key == "Aim.Aimbot360") Config.Aim.Aimbot360 = (valueStr == "1");
        else if (key == "Aim.AimSilent") Config.Aim.AimSilent = (valueStr == "1");
        else if (key == "Aim.Target") Config.Aim.Target = static_cast<EAimTarget>(std::stoi(valueStr));
        else if (key == "Aim.Trigger") Config.Aim.Trigger = static_cast<EAimTrigger>(std::stoi(valueStr));
        else if (key == "Aim.By") Config.Aim.By = static_cast<EAim>(std::stoi(valueStr));
        else if (key == "ExtraMenu.Kinetic") Config.ExtraMenu.Kinetic = (valueStr == "1");
        else if (key == "ExtraMenu.MythicAttachments") Config.ExtraMenu.MythicAttachments = (valueStr == "1");
        else if (key == "ExtraMenu.Blueprints") Config.ExtraMenu.Blueprints = (valueStr == "1");
        else if (key == "ExtraMenu.Recoil") Config.ExtraMenu.Recoil = (valueStr == "1");
        else if (key == "ExtraMenu.Spread") Config.ExtraMenu.Spread = (valueStr == "1");
        else if (key == "ExtraMenu.Reload") Config.ExtraMenu.Reload = (valueStr == "1");
        else if (key == "ExtraMenu.Scope") Config.ExtraMenu.Scope = (valueStr == "1");
        else if (key == "ExtraMenu.Switch") Config.ExtraMenu.Switch = (valueStr == "1");
        else if (key == "ExtraMenu.Shake") Config.ExtraMenu.Shake = (valueStr == "1");
        else if (key == "ExtraMenu.Flash") Config.ExtraMenu.Flash = (valueStr == "1");
        else if (key == "ExtraMenu.Rpd") Config.ExtraMenu.Rpd = (valueStr == "1");
        else if (key == "ExtraMenu.Hit") Config.ExtraMenu.Hit = (valueStr == "1");
        else if (key == "ExtraMenu.Fire") Config.ExtraMenu.Fire = (valueStr == "1");
        else if (key == "ExtraMenu.Parachute") Config.ExtraMenu.Parachute = (valueStr == "1");
        else if (key == "ExtraMenu.Diving") Config.ExtraMenu.Diving = (valueStr == "1");
        else if (key == "NewFeatures.NoCrouch") Config.NewFeaturesMenu.NoCrouch = (valueStr == "1");
        else if (key == "NewFeatures.Triggerbot") Config.NewFeaturesMenu.Triggerbot = (valueStr == "1");
        else if (key == "NewFeatures.TriggerbotMode") Config.NewFeaturesMenu.TriggerbotMode = std::stoi(valueStr);
        else if (key == "NewFeatures.TriggerbotDelay") Config.NewFeaturesMenu.TriggerbotDelay = std::stoi(valueStr);
        else if (key == "NewFeatures.TriggerbotRadius") Config.NewFeaturesMenu.TriggerbotRadius = std::stoi(valueStr);
        else if (key == "NewFeatures.NoHitFlinch") Config.NewFeaturesMenu.NoHitFlinch = (valueStr == "1");
        else if (key == "NewFeatures.NoDrift") Config.NewFeaturesMenu.NoDrift = (valueStr == "1");
        else if (key == "NewFeatures.NoSlideFatigue") Config.NewFeaturesMenu.NoSlideFatigue = (valueStr == "1");
        else if (key == "NewFeatures.NoSprintToFireDelay") Config.NewFeaturesMenu.NoSprintToFireDelay = (valueStr == "1");
        else if (key == "NewFeatures.NoHipfireDelay") Config.NewFeaturesMenu.NoHipfireDelay = (valueStr == "1");
        // Own dot crosshair. Opacity is the colour's own alpha, so it is stored
        // once (Crosshair.Opacity -> Color[3]) and cannot disagree with the dot.
        else if (key == "Crosshair.Enabled") Config.Crosshair.Enabled = (valueStr == "1");
        else if (key == "Crosshair.Size") Config.Crosshair.Size = std::stof(valueStr);
        else if (key == "Crosshair.OffsetX") Config.Crosshair.OffsetX = std::stof(valueStr);
        else if (key == "Crosshair.OffsetY") Config.Crosshair.OffsetY = std::stof(valueStr);
        else if (key == "Crosshair.Opacity") Config.Crosshair.Color[3] = std::stof(valueStr);
        else if (key == "Crosshair.ColorR") Config.Crosshair.Color[0] = std::stof(valueStr);
        else if (key == "Crosshair.ColorG") Config.Crosshair.Color[1] = std::stof(valueStr);
        else if (key == "Crosshair.ColorB") Config.Crosshair.Color[2] = std::stof(valueStr);
        // DevBypass removed (login bypass).
        else if (key == "ExtraMenu.WallHack") Config.ExtraMenu.WallHack = (valueStr == "1");
    }
    file.close();
}

void SaveConfig() {
    std::string textfile = GetFilesDirPath(VM) + "/codmconfig.ini";
    std::ofstream file(textfile.c_str());

    if (!file.is_open()) {
        return;
    }

    file << "ESPMenu.isPlayerLine " << Config.ESPMenu.isPlayerLine << "\n";
    file << "ESPMenu.Box " << Config.ESPMenu.Box << "\n";
    file << "ESPMenu.Health " << Config.ESPMenu.Health << "\n";
    file << "ESPMenu.Name " << Config.ESPMenu.Name << "\n";
    file << "ESPMenu.Distance " << Config.ESPMenu.Distance << "\n";
    file << "ESPMenu.Count " << Config.ESPMenu.Count << "\n";
    file << "ESPMenu.Crosshair " << Config.ESPMenu.Crosshair << "\n";
    file << "ESPMenu.Aimline " << Config.ESPMenu.Aimline << "\n";
    file << "ESPMenu.Target " << Config.ESPMenu.Target << "\n";
    file << "ESPMenu.BoxType " << Config.ESPMenu.BoxType << "\n";
    file << "ESPMenu.HealthPosition " << Config.ESPMenu.HealthPosition << "\n";
    file << "ESPMenu.CrosshairType " << Config.ESPMenu.CrosshairType << "\n";
    file << "ESPMenu.EspStyle " << Config.ESPMenu.EspStyle << "\n";
    file << "Aim.size " << Config.Aim.size << "\n";
    file << "Aim.AimAssistSize " << Config.Aim.AimAssistSize << "\n";
    file << "Aim.Aimbot360 " << Config.Aim.Aimbot360 << "\n";
    file << "Aim.AimSilent " << Config.Aim.AimSilent << "\n";
    file << "Aim.Target " << Config.Aim.Target << "\n";
    file << "Aim.Trigger " << Config.Aim.Trigger << "\n";
    file << "Aim.By " << Config.Aim.By << "\n";
    file << "ExtraMenu.Kinetic " << Config.ExtraMenu.Kinetic << "\n";
    file << "ExtraMenu.MythicAttachments " << Config.ExtraMenu.MythicAttachments << "\n";
    file << "ExtraMenu.Blueprints " << Config.ExtraMenu.Blueprints << "\n";
    file << "ExtraMenu.Recoil " << Config.ExtraMenu.Recoil << "\n";
    file << "ExtraMenu.Spread " << Config.ExtraMenu.Spread << "\n";
    file << "ExtraMenu.Reload " << Config.ExtraMenu.Reload << "\n";
    file << "ExtraMenu.Scope " << Config.ExtraMenu.Scope << "\n";
    file << "ExtraMenu.Switch " << Config.ExtraMenu.Switch << "\n";
    file << "ExtraMenu.Shake " << Config.ExtraMenu.Shake << "\n";
    file << "ExtraMenu.Flash " << Config.ExtraMenu.Flash << "\n";
    file << "ExtraMenu.Rpd " << Config.ExtraMenu.Rpd << "\n";
    file << "ExtraMenu.Hit " << Config.ExtraMenu.Hit << "\n";
    file << "ExtraMenu.Fire " << Config.ExtraMenu.Fire << "\n";
    file << "ExtraMenu.Parachute " << Config.ExtraMenu.Parachute << "\n";
    file << "ExtraMenu.Diving " << Config.ExtraMenu.Diving << "\n";
    file << "NewFeatures.NoCrouch " << Config.NewFeaturesMenu.NoCrouch << "\n";
    file << "NewFeatures.Triggerbot " << Config.NewFeaturesMenu.Triggerbot << "\n";
    file << "NewFeatures.TriggerbotMode " << Config.NewFeaturesMenu.TriggerbotMode << "\n";
    file << "NewFeatures.TriggerbotDelay " << Config.NewFeaturesMenu.TriggerbotDelay << "\n";
    file << "NewFeatures.TriggerbotRadius " << Config.NewFeaturesMenu.TriggerbotRadius << "\n";
    file << "NewFeatures.NoHitFlinch " << Config.NewFeaturesMenu.NoHitFlinch << "\n";
    file << "NewFeatures.NoDrift " << Config.NewFeaturesMenu.NoDrift << "\n";
    file << "NewFeatures.NoSlideFatigue " << Config.NewFeaturesMenu.NoSlideFatigue << "\n";
    file << "NewFeatures.NoSprintToFireDelay " << Config.NewFeaturesMenu.NoSprintToFireDelay << "\n";
    file << "NewFeatures.NoHipfireDelay " << Config.NewFeaturesMenu.NoHipfireDelay << "\n";
    file << "Crosshair.Enabled " << Config.Crosshair.Enabled << "\n";
    file << "Crosshair.Size " << Config.Crosshair.Size << "\n";
    file << "Crosshair.OffsetX " << Config.Crosshair.OffsetX << "\n";
    file << "Crosshair.OffsetY " << Config.Crosshair.OffsetY << "\n";
    file << "Crosshair.Opacity " << Config.Crosshair.Color[3] << "\n";
    file << "Crosshair.ColorR " << Config.Crosshair.Color[0] << "\n";
    file << "Crosshair.ColorG " << Config.Crosshair.Color[1] << "\n";
    file << "Crosshair.ColorB " << Config.Crosshair.Color[2] << "\n";
    // DevBypass removed (login bypass).
    file << "ExtraMenu.WallHack " << Config.ExtraMenu.WallHack << "\n";
    
    file.close();
}

#endif
