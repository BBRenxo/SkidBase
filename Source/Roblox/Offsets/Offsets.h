#pragma once
#include <cstdint>
#include <Windows.h>

namespace Main {
    inline uintptr_t krah() {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    }

    inline uintptr_t Rebase(uintptr_t offset) {
        return krah() + offset;
    }

    // Updated Sep 3 18:01 vs fresh roblox-dumper 3.6 dump against PID 8872
    // Same version on disk (e7d81637d42c4b23) but Roblox hot-patches in
    // memory, so addresses shifted between dumps.
    //
    // Applied offsets (Sep 3):
    //   Print:                 0x1C68FE0 -> 0x1C8A050 (+0x21070)
    //   FakeDataModel:         0x8CA9CC8 -> 0x8D22868 (+0x7B9E0)
    //   GameLoaded:            0x5D0     -> 0x5D8
    //
    // REVERTED (crashed Roblox):
    //   OpcodeLookupTable: 0x7B5CEE0 -> back to 0x6D45820
    //   The snapshot file said 0x7B5CEE0 was ByteCodeVerification::TableA
    //   but SkidBase uses OpcodeLookupTable as a XOR keystream table for
    //   bytecode obfuscation, not as an actual opcode lookup. 0x7B5CEE0
    //   points to memory that didn't have the expected content, so the
    //   encoder produced garbage and Roblox crashed. 0x6D45820 was
    //   working earlier today on the same version — keep it.
    //
    // TODO: re-explore OpcodeLookupTable value carefully. We may want to
    // try ALLOCATED memory (like a static array in SkidBase) so the
    // encoder doesn't depend on Roblox's memory layout at all.
    namespace Functions {
        inline const uintptr_t Print = Rebase(0x1c8a050);
        inline const uintptr_t GetGlobalState = Rebase(0x402B3C0);
        inline const uintptr_t LuauExecute = Rebase(0x26bda30);
        inline const uintptr_t LuaDThrow = Rebase(0x26adad0);
        inline const uintptr_t OpcodeLookupTable = Rebase(0x6d45820);
    }

    namespace Miscellaneous {
        inline const uintptr_t FakeDatamodelPOINTER = Rebase(0x8d22868);
        inline const uintptr_t LuaNil = Rebase(0x62f7418);
        inline const uintptr_t LuaDummy = Rebase(0x62f6ec8);
        inline const uintptr_t TargetFPS = Rebase(0x80993c8);
    }

    namespace Offsets {
        inline const uintptr_t DataModel = 0x1f8;
        inline const uintptr_t ScriptContext = 0x440;
        inline const uintptr_t Children = 0x78;
        inline const uintptr_t GameLoaded = 0x5D8;
    }

    namespace Identity1 {
        inline const uintptr_t IdentityPointer = Rebase(0x8051178);
        inline const uintptr_t GetTlsPointer = Rebase(0x4170);
    }

    namespace Identity2 {
        inline const uintptr_t GetCapabilities = Rebase(0x1ca46d0);
        inline const uintptr_t Capabilities = 0x28;
    }
}
