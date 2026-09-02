#include "Unc.h"

namespace unc {
    void apply(lua_State* L, Mode mode) {
        if (!L) return;
        switch (mode) {
            case Mode::Off:
                // Let counter tick normally (leave alone)
                set_sunc_flag(L, false);
                break;
            case Mode::Unc:
                // Zero yield counter so it never trips
                unc_yield_counter(L);
                set_sunc_flag(L, false);
                break;
            case Mode::Sunc:
                // Unc + bypass identity enforcement
                unc_yield_counter(L);
                set_sunc_flag(L, true);
                break;
        }
    }
}
