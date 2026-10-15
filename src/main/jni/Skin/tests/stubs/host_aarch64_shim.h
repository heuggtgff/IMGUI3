// Host-only prelude for the desktop syntax checks (-include).
//
// A handful of real-build symbols live behind `#if defined(__aarch64__)`; on a
// desktop g++ they simply do not exist, which stops the compile of headers
// that are perfectly fine on device. Defining __aarch64__ instead would change
// real code paths (hook thunks, syscall numbers), so this shim declares just
// the names those guarded blocks expose. It never reaches the APK.
#pragma once

// System/Core/Bypass.h (guarded by __aarch64__).
inline const char *armFalse = "00 00 80 D2 C0 03 5F D6";
