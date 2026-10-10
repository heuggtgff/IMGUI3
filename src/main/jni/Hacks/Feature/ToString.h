#pragma once

enum EVehicleType {
    eVehicleType_Plane = 31707001,
    eVehicleType_Helicopter = 31707601,
    eVehicleType_HelicopterSecurity = 31707602,
    eVehicleType_HelicopterBlima = 31707118,
    eVehicleType_JetFighter = 31707127,
    eVehicleType_Buick = 31707102,
    eVehicleType_Jeep = 31707105,
    eVehicleType_Pickup = 31707106,
    eVehicleType_BeachMotor = 31707107,
    eVehicleType_Boat = 31707301,
    eVehicleType_Motor = 31707401,
    eVehicleType_Bike = 31707402,
    eVehicleType_Drone = 31707611,
    eVehicleType_InterferDrone = 31707621,
    eVehicleType_Turret = 31707901,
    eVehicleType_ReconCar = 31707108,
    eVehicleType_ATV = 31707101,
    eVehicleType_TRUCK = 31707103,
    eVehicleType_Assault = 31707104,
    eVehicleType_Tank = 31707109,
    eVehicleType_Snowboard = 31707110,
    eVehicleType_LandRover = 31707112,
    eVehicleType_HeavyTank = 31707111,
    eVehicleType_Harley2020 = 31707113,
    eVehicleType_HarleyMod = 31707114,
    eVehicleType_HawkX3 = 31707115,
    eVehicleType_Coupe = 31707116,
    eVehicleType_CoupeGT500 = 31707117,
    eVehicleType_LamborghiniCountach = 31707119,
    eVehicleType_RallyCar = 31707120,
    eVehicleType_Lamborghini350GT = 31707121,
    eVehicleType_Lamborghini400GTS = 31707122,
    eVehicleType_A20NightDrive = 31707123,
    eVehicleType_JeepHoliday = 31707124,
    eVehicleType_AntiHero = 31707125,
    eVehicleType_CyberbrainDrone = 31707126,
    eVehicleType_CodmKG_Vehicle = 31707128,
    eVehicleType_HoverMotor = 31707129,
    eVehicleType_TankWarBeast = 31707130,
};

std::string EVehicleToString(EVehicleType eVehicleType) {
    std::string strVehicle;
    
    switch(eVehicleType) {
    
    case eVehicleType_Plane:
        strVehicle += std::string(OBFUSCATE("Plane"));
        break;
    case eVehicleType_Helicopter:
        strVehicle += std::string(OBFUSCATE("Helicopter"));
        break;
    case eVehicleType_HelicopterSecurity:
        strVehicle += std::string(OBFUSCATE("Helicopter Security"));
        break;
    case eVehicleType_HelicopterBlima:
        strVehicle += std::string(OBFUSCATE("Helicopter Blima"));
        break;
    case eVehicleType_JetFighter:
        strVehicle += std::string(OBFUSCATE("JetFighter"));
        break;
    case eVehicleType_Buick:
        strVehicle += std::string(OBFUSCATE("Buick"));
        break;
    case eVehicleType_Jeep:
        strVehicle += std::string(OBFUSCATE("Jeep"));
        break;
    case eVehicleType_Pickup:
        strVehicle += std::string(OBFUSCATE("Pickup"));
        break;
    case eVehicleType_BeachMotor:
        strVehicle += std::string(OBFUSCATE("BeachMotor"));
        break;
    case eVehicleType_Boat:
        strVehicle += std::string(OBFUSCATE("Boat"));
        break;
    case eVehicleType_Motor:
        strVehicle += std::string(OBFUSCATE("Motor"));
        break;
    case eVehicleType_Bike:
        strVehicle += std::string(OBFUSCATE("Bike"));
        break;
    case eVehicleType_Drone:
        strVehicle += std::string(OBFUSCATE("Drone"));
        break;
    case eVehicleType_InterferDrone:
        strVehicle += std::string(OBFUSCATE("InterferDrone"));
        break;
    case eVehicleType_Turret:
        strVehicle += std::string(OBFUSCATE("Turret"));
        break;
    case eVehicleType_ReconCar:
        strVehicle += std::string(OBFUSCATE("ReconCar"));
        break;
    case eVehicleType_ATV:
        strVehicle += std::string(OBFUSCATE("ATV"));
        break;
    case eVehicleType_TRUCK:
        strVehicle += std::string(OBFUSCATE("TRUCK"));
        break;
    case eVehicleType_Assault:
        strVehicle += std::string(OBFUSCATE("Assault"));
        break;
    case eVehicleType_Tank:
        strVehicle += std::string(OBFUSCATE("Tank"));
        break;
    case eVehicleType_Snowboard:
        strVehicle += std::string(OBFUSCATE("Snowboard"));
        break;
    case eVehicleType_LandRover:
        strVehicle += std::string(OBFUSCATE("LandRover"));
        break;
    case eVehicleType_HeavyTank:
        strVehicle += std::string(OBFUSCATE("HeavyTank"));
        break;
    case eVehicleType_Harley2020:
        strVehicle += std::string(OBFUSCATE("Harley2020"));
        break;
    case eVehicleType_HarleyMod:
        strVehicle += std::string(OBFUSCATE("HarleyMod"));
        break;
    case eVehicleType_HawkX3:
        strVehicle += std::string(OBFUSCATE("HawkX3"));
        break;
    case eVehicleType_Coupe:
        strVehicle += std::string(OBFUSCATE("Coupe"));
        break;
    case eVehicleType_CoupeGT500:
        strVehicle += std::string(OBFUSCATE("CoupeGT500"));
        break;
    case eVehicleType_LamborghiniCountach:
        strVehicle += std::string(OBFUSCATE("LamborghiniCountach"));
        break;
    case eVehicleType_RallyCar:
        strVehicle += std::string(OBFUSCATE("RallyCar"));
        break;
    case eVehicleType_Lamborghini350GT:
        strVehicle += std::string(OBFUSCATE("Lamborghini350GT"));
        break;
    case eVehicleType_Lamborghini400GTS:
        strVehicle += std::string(OBFUSCATE("Lamborghini400GTS"));
        break;
    case eVehicleType_A20NightDrive:
        strVehicle += std::string(OBFUSCATE("A20NightDrive"));
        break;
    case eVehicleType_JeepHoliday:
        strVehicle += std::string(OBFUSCATE("JeepHoliday"));
        break;
    case eVehicleType_AntiHero:
        strVehicle += std::string(OBFUSCATE("AntiHero"));
        break;
    case eVehicleType_CyberbrainDrone:
        strVehicle += std::string(OBFUSCATE("CyberbrainDrone"));
        break;
    case eVehicleType_CodmKG_Vehicle:
        strVehicle += std::string(OBFUSCATE("CodmKG_Vehicle"));
        break;
    case eVehicleType_HoverMotor:
        strVehicle += std::string(OBFUSCATE("HoverMotor"));
        break;
    case eVehicleType_TankWarBeast:
        strVehicle += std::string(OBFUSCATE("TankWarBeast"));
        break;
    default:
        strVehicle += "";
    }
    return strVehicle;
}