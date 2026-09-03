#pragma once
// SkidBase offsets for version-e7d81637d42c4b23 (Sep 2 2026)
// Source: supQ Runtime Dumper v2 (latest offsets.hpp from Downloads)

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
        // Print function (Lua VM print)
        inline const uintptr_t Print = Rebase(0x1C8A050);
        // LuaVMLoad (the lua_State* loader)
        inline const uintptr_t GetGlobalState = Rebase(0x406E600);
        // luaD_throw (Lua error throw)
        inline const uintptr_t LuaDThrow = Rebase(0x26D4690);
        // Opcode lookup table
        inline const uintptr_t OpcodeLookupTable = Rebase(0x61D1700);
        // Note: LuauExecute = 0x0 in source dump (unknown).
        // SkidBase calls it via "LuaD_Call" in newer dump = 0x26D4770.
        // This is what SkidBase calls "ExecuteVM" in Funcs.h.
        inline const uintptr_t LuauExecute = Rebase(0x26D4770);
    }

    namespace Miscellaneous {
        // FakeDataModel.Pointer
        inline const uintptr_t FakeDatamodelPOINTER = Rebase(0x8D22868);
        // LuaO_NilObject (Lua nil singleton)
        inline const uintptr_t LuaNil = Rebase(0x63516D8);
        // LuaH_DummyNode (Lua dummy hash node)
        inline const uintptr_t LuaDummy = Rebase(0x6351188);
        // TaskSchedulerTargetFps
        inline const uintptr_t TargetFPS = Rebase(0x8109DE8);
    }

    namespace Offsets {
        // DataModel.FakeDataModelToDataModel (DataModel at offset 0x1F8)
        inline const uintptr_t DataModel = 0x1F8;
        // DataModel.ScriptContext
        inline const uintptr_t ScriptContext = 0x440;
        // DataModel.Children
        inline const uintptr_t Children = 0x78;
        // DataModel.GameLoaded
        inline const uintptr_t GameLoaded = 0x630;
    }

    namespace Identity1 {
        // RobloxThread.IdentityPtr
        inline const uintptr_t IdentityPointer = Rebase(0x80C10C8);
        // RobloxThread.GetTlsPointer
        inline const uintptr_t GetTlsPointer = Rebase(0x39A0);
    }

    namespace Identity2 {
        // Capabilities.GetCapabilities
        inline const uintptr_t GetCapabilities = Rebase(0x1CC5820);
        // Capabilities.Capabilities (unknown, keep old)
        inline const uintptr_t Capabilities = 0x28;
    }

    namespace TaskScheduler {
        inline const uintptr_t Pointer = Rebase(0x8ABD728);
        inline const uintptr_t JobStart = 0xC8;
        inline const uintptr_t JobEnd = 0xD0;
        inline const uintptr_t JobName = 0x18;
    }

    namespace FireMouse {
        // These were missing in new dump, kept from old offsets
        inline const uintptr_t FireMouseClick = Rebase(0x3AC4300);
        inline const uintptr_t FireRightMouseClick = Rebase(0x3B07820);  // updated from new dump
        inline const uintptr_t FireMouseHoverEnter = Rebase(0x3B08E10); // updated from new dump
        inline const uintptr_t FireMouseHoverLeave = Rebase(0x3B08FB0); // updated from new dump
        inline const uintptr_t FireProximityPrompt = Rebase(0x30CA950); // updated from new dump
    }

    namespace InstanceBridge {
        inline const uintptr_t PushInstance = Rebase(0x40608A0); // updated from new dump
        inline const uintptr_t CastArgs = Rebase(0x3FDE600); // updated from new dump
    }
}
