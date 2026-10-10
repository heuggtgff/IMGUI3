#pragma once

bool (*orig_bypass)(void *ins);
bool hook_bypass(void *ins) {
    return false;
}

#if defined(__aarch64__)

inline const char *armFalse = "00 00 80 D2 C0 03 5F D6";

#endif

struct range {
    uintptr_t Irt, Ind;
    struct Iter {
      uintptr_t val;
      bool operator!=(const Iter& it) const {
        return val <= it.val; 
      }
      uintptr_t operator*() const {
        return val; 
      }
      void operator++() {
        val += 4; 
      }
    };
    Iter begin() const { 
      return {Irt}; 
    }
    Iter end() const { 
      return {Ind}; 
    }
};

inline void InitializeProtection() {
// Direct Memory Patch ────────────────
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x202680), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x204218), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x258B6C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x259670), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3055A0), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3075C4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x30E234), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x35140C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x303A68), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x47D0C8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x245F34), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2B8CDC), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4AB5EC), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4E70A4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x39CD30), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x259010), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x44BC90), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4A9A00), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4952E8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4951C8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x49521C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x42BA18), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x420578), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1FF3A4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x455A80), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4F03D4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x39AE94), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x41BA40), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1EE554), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1F815C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x203460), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x24D638), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x24EDD0), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x257B90), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x28F894), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2A9338), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2ABBC8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x438154), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x44A714), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x48CF20), (void*)hook_bypass, (void**)&orig_bypass);
for (auto offs : range{0x1, 0x1000}) {
MemoryPatch::createWithHex("libanogs.so", offs, armFalse).Modify();
    }
    
}