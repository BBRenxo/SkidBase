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

    namespace Functions {
        inline const uintptr_t Print = Rebase(0x92C340);
        inline const uintptr_t GetGlobalState = Rebase(0x2219D10);
        inline const uintptr_t LuauExecute = Rebase(0xB59570);
        inline const uintptr_t LuaDThrow = Rebase(0xB40C30);
        inline const uintptr_t OpcodeLookupTable = Rebase(0x6B83740);
    }

    namespace Miscellaneous {
        inline const uintptr_t FakeDatamodelPOINTER = Rebase(0x8b79b58);
        inline const uintptr_t LuaNil = Rebase(0x610EFF8);
        inline const uintptr_t LuaDummy = Rebase(0x610EEB8);
    }

    namespace Offsets {
        inline const uintptr_t DataModel = 0x1D8;
        inline const uintptr_t ScriptContext = 0x440;
        inline const uintptr_t Children = 0x78;
        inline const uintptr_t GameLoaded = 0x570;
    }
}
