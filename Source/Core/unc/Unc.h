#pragma once
#include "lua.h"
#include <cstdint>
#include <Windows.h>

namespace unc {
    // Disable Luau's per-thread yield counter.
    // Normally incremented every VM tick. Above ~10ms = "yield limit exceeded".
    // We zero it in our executor threads so scripts can wait forever.
    //
    // Also: sunc mode bypasses identity check enforcement in luau_execute so
    // sandboxed threads can do privileged ops.

    enum class Mode {
        Off,            // normal (yield counter ticks normally)
        Unc,            // yield counter zeroed (no yield limit)
        Sunc            // unc + identity enforcement bypassed
    };

    // Apply mode to a lua_State. Idempotent — safe to call repeatedly.
    void apply(lua_State* L, Mode mode);

    // Yield counter location in lua_State (Hyperion f5a60436d48947d3)
    // Located by RE — the counter is at +0xC8 in the main thread struct
    // and per-thread counters are at thread+0x88
    constexpr uintptr_t MAIN_STATE_YIELD_COUNTER = 0xC8;
    constexpr uintptr_t THREAD_YIELD_COUNTER = 0x88;

    // Set yield counter on a thread to UINT_MAX so it never trips.
    inline void unc_yield_counter(lua_State* L) {
        if (!L) return;
        *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(L) + THREAD_YIELD_COUNTER) = UINTPTR_MAX;
    }

    // Set sunc flag (bypass identity enforcement) — sets a special flag
    // in lua_State that luau_execute checks before applying identity
    // restrictions. Located at +0x1A0 in Hyperion's lua_State struct.
    constexpr uintptr_t SUNC_FLAG_OFFSET = 0x1A0;
    inline void set_sunc_flag(lua_State* L, bool enabled) {
        if (!L) return;
        *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(L) + SUNC_FLAG_OFFSET) = enabled ? 1 : 0;
    }
}
