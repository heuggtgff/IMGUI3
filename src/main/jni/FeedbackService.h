#pragma once

#include <jni.h>
#include <string>

struct FeedbackState {
    bool enabled = true;
    bool consented = true;
    bool includeScreenshot = true;
    bool hasMatchStats = false;
    int kills = 0;
    int deaths = 0;
    int assists = 0;
    char botToken[192] = "8964417942:AAGWxFYjza0yaUNudSoVtW0CfGbiJLr8NXA";
    char chatId[96] = "-1003885110512";
    char note[384] = "";
};

namespace feedback {

FeedbackState& State();

bool HasHardcodedBotToken();
bool HasHardcodedChatId();
bool IsSending();

void GetStatus(std::string& outText, bool& outIsError);
void StartSendAsync(const FeedbackState& snapshot, const std::string& filesDirPath);
void PumpAfterRender();

}
