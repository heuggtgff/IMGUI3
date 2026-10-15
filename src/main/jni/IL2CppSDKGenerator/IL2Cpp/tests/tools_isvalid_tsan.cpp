// Host test for Tools::IsPtrValid - the readable-range snapshot, and the data
// race that made the game crash "after a while" instead of at a fixed point.
//
// It links the REAL src/main/jni/IL2CppSDKGenerator/IL2Cpp/Tools.cpp (no copy
// of the logic) and drives it the way the device does: the render thread (ESP,
// trigger scan, kill-decoration pass) hammers IsPtrValid every frame while the
// loader thread and Init_Thread hit the same function for their own guards. The
// refresher threads call it with addresses that are NOT in /proc/self/maps, so
// they take the path that re-parses the snapshot - the write side of the race.
//
// Run it from the repo root:
//   g++ -std=c++17 -fsanitize=thread -g -O1 -ffunction-sections -fdata-sections \
//       -D__fastcall= -I src/main/jni/Skin/tests/stubs -I src/main/jni \
//       -I src/main/jni/ImGui -I src/main/jni/Includes \
//       src/main/jni/IL2CppSDKGenerator/IL2Cpp/tests/tools_isvalid_tsan.cpp \
//       src/main/jni/IL2CppSDKGenerator/IL2Cpp/Tools.cpp \
//       -Wl,--gc-sections -o /tmp/tools_isvalid_tsan && /tmp/tools_isvalid_tsan
//
// Expected on a fixed tree: the functional checks pass, ~5 million IsPtrValid
// calls finish across 6 threads, no ThreadSanitizer output, exit 0.
//
// On the pre-fix tree the same command reports 13 data races and then dies with
//   SUMMARY: ThreadSanitizer: SEGV ... Tools.cpp in ToolsRangesContain
// because the refreshing thread reallocated the vector the reading thread was
// binary-searching: the reader walked freed memory, which is the device crash.
//
// No NDK is needed (the Android/JNI headers are the stubs in Skin/tests) and
// Android.mk does not wildcard this directory, so the file never reaches the
// APK. ThreadSanitizer is a host-only instrument: the device build does not use
// it, so this test proves the *concurrency contract* of the snapshot, not the
// device ABI.
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s\n", msg);                                         \
        }                                                                      \
    } while (0)

int main()
{
    // ---- functional contract ------------------------------------------
    // Every guarded hook, ESP pass and trigger scan depends on these three
    // answers, and they must still hold with the snapshot published atomically.
    static int liveObject = 0;
    CHECK(Tools::IsPtrValid(&liveObject), "a live heap address is valid");
    CHECK(!Tools::IsPtrValid(nullptr), "nullptr is not valid");
    CHECK(!Tools::IsPtrValid((void *)0x1000), "an address below 0x10000 is not valid");
    CHECK(!Tools::IsPtrValid((void *)0x0000'1000'0000ull),
          "an unmapped hole is not valid");
    CHECK(Tools::IsPtrValid(&liveObject),
          "the live address is still valid after an unmapped one was probed");

    // ---- the race ------------------------------------------------------
    // Readers: sampled from several threads at once, exactly like the render
    // thread and the loaders on device.
    std::atomic<bool> stop{false};
    std::atomic<long> calls{0};

    auto reader = [&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            for (int i = 0; i < 200; i++) {
                Tools::IsPtrValid(&liveObject);
                calls.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };
    auto refresher = [&]() {
        // Addresses that are not in the snapshot, so process_vm_readv fails and
        // the snapshot is re-parsed (the write side).
        const uintptr_t holes[] = { 0x0000'1000'0000ull, 0x0000'2000'0000ull,
                                    0x0000'3000'0000ull, 0x0000'4000'0000ull };
        while (!stop.load(std::memory_order_relaxed)) {
            for (uintptr_t h : holes) {
                Tools::IsPtrValid((void *)h);
                calls.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) threads.emplace_back(reader);
    threads.emplace_back(refresher);
    threads.emplace_back(refresher);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    stop.store(true);
    for (auto &t : threads) t.join();

    CHECK(calls.load() > 100000, "the hammering actually ran");

    printf("\n");
    if (g_failures == 0) {
        printf("OK: %d checks, %ld IsPtrValid calls across 6 threads\n",
               g_checks, calls.load());
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
