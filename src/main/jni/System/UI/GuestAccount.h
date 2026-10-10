#ifndef GUEST_ACCOUNT_RESET_H
#define GUEST_ACCOUNT_RESET_H

#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static const char* guestAccountPaths[] = {
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/lastUserId.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/gsdk_prefs.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/MFILE.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/apm_cfg.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/appsflyer-data.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/buglySdkInfos.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/CentauriHTTPSP.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/CentauriOverseaIP.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.PayCachePreference_crypto.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm_preferences.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.v2.playerprefs.xml",
    "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.msdk.persist.fallback.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/lastUserId.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/gsdk_prefs.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/MFILE.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/apm_cfg.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/appsflyer-data.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/buglySdkInfos.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/CentauriHTTPSP.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/CentauriOverseaIP.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.PayCachePreference_crypto.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm_preferences.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.v2.playerprefs.xml",
    "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs/com.garena.msdk.persist.fallback.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/lastUserId.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/gsdk_prefs.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/MFILE.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/apm_cfg.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/appsflyer-data.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/buglySdkInfos.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/CentauriHTTPSP.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/CentauriOverseaIP.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.PayCachePreference_crypto.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm_preferences.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.v2.playerprefs.xml",
    "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs/com.garena.msdk.persist.fallback.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/lastUserId.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/gsdk_prefs.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/MFILE.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/apm_cfg.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/appsflyer-data.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/buglySdkInfos.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/CentauriHTTPSP.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/CentauriOverseaIP.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.PayCachePreference_crypto.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm_preferences.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/com.garena.game.codm.v2.playerprefs.xml",
    "/data/user/0/com.garena.game.codm/shared_prefs/com.garena.msdk.persist.fallback.xml",
    "/data/data/com.garena.game.codm/shared_prefs/lastUserId.xml",
    "/data/data/com.garena.game.codm/shared_prefs/gsdk_prefs.xml",
    "/data/data/com.garena.game.codm/shared_prefs/MFILE.xml",
    "/data/data/com.garena.game.codm/shared_prefs/apm_cfg.xml",
    "/data/data/com.garena.game.codm/shared_prefs/appsflyer-data.xml",
    "/data/data/com.garena.game.codm/shared_prefs/buglySdkInfos.xml",
    "/data/data/com.garena.game.codm/shared_prefs/CentauriHTTPSP.xml",
    "/data/data/com.garena.game.codm/shared_prefs/CentauriOverseaIP.xml",
    "/data/data/com.garena.game.codm/shared_prefs/com.garena.game.codm.PayCachePreference_crypto.xml",
    "/data/data/com.garena.game.codm/shared_prefs/com.garena.game.codm_preferences.xml",
    "/data/data/com.garena.game.codm/shared_prefs/com.garena.game.codm.v2.playerprefs.xml",
    "/data/data/com.garena.game.codm/shared_prefs/com.garena.msdk.persist.fallback.xml"
};

static inline void DeleteGuestFile(const char* path) {
    if (!path || !path[0]) return;
    unlink(path);
    std::string backup = std::string(path) + ".bak";
    unlink(backup.c_str());
}

static inline void DeleteDirectoryRecursive(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return;

    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string fullPath = std::string(path) + "/" + entry->d_name;
        struct stat info {};
        if (stat(fullPath.c_str(), &info) != 0) {
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            DeleteDirectoryRecursive(fullPath.c_str());
        } else {
            unlink(fullPath.c_str());
        }
    }

    closedir(dir);
    rmdir(path);
}

static inline void ResetBannedGuestAccount() {
    for (const char* path : guestAccountPaths) {
        DeleteGuestFile(path);
    }

    static const char* const prefsDirs[] = {
        "/data/data/com.garena.game.codm/shared_prefs",
        "/data/user/0/com.garena.game.codm/shared_prefs",
        "/data/data/com.mobile.survive/virtual/data/user/0/com.garena.game.codm/shared_prefs",
        "/data/data/com.samsung.mobile/virtual/data/user/0/com.garena.game.codm/shared_prefs",
        "/data/data/com.pengyou.cloneapp/chaos/data/user/0/com.garena.game.codm/shared_prefs"
    };

    for (const char* dir : prefsDirs) {
        DeleteDirectoryRecursive(dir);
    }

    usleep(200000);
}

#endif
