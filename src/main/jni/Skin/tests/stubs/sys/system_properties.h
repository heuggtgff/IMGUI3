// Host stub for the Android property API (harness only, no NDK here).
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int __system_property_get(const char *name, char *value);
#ifdef __cplusplus
}
#endif
