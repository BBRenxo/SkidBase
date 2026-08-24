#pragma once
#include "Libs/Http/Http.h"
#include "Libs/Closures/Closures.h"
#include "lapi.h"
#include "lstate.h"
#include "lobject.h"
#include "lgc.h"
#include "../../Core/Execution/Execution.h"
#include <Windows.h>
#include <string>
#include <unordered_set>

inline bool IsOurThread(lua_State* L) {
    return L != nullptr && L->userdata != nullptr && (L->userdata->capabilities & 0b1000000);
}

namespace env
{
    inline lua_CFunction original_index = nullptr;
    inline lua_CFunction original_namecall = nullptr;

    inline int identifyexecutor(lua_State* L) {
        lua_pushstring(L, "SkidBase");
        lua_pushstring(L, "v0.1");
        return 2;
    }

    inline int getexecutorname(lua_State* L) {
        lua_pushstring(L, "SkidBase");
        return 1;
    }

    inline int getgenv(lua_State* L) {
        auto our_state = Execution::skidsstate;
        const auto mainState = our_state;

        if (mainState == L) {
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            return 1;
        }

        if (!mainState->isactive)
            luaC_threadbarrier(mainState);

        lua_pushvalue(mainState, LUA_GLOBALSINDEX);
        lua_xmove(mainState, L, 1);

        return 1;
    }

    inline int index_hook(lua_State* L)
    {
        if (!IsOurThread(L)) return original_index(L);

        if (L->userdata && L->userdata->capabilities == 0xFFFFFFFFFFFFFFFF)
        {
            if (lua_isstring(L, 2)) {
                const char* key = lua_tostring(L, 2);

                if (key && strcmp(key, "HttpGet") == 0)
                {
                    lua_pushcfunction(L, http::HttpGet, "HttpGet");
                    return 1;
                }
                if (key && strcmp(key, "HttpGetAsync") == 0)
                {
                    lua_pushcfunction(L, http::HttpGet, "HttpGetAsync");
                    return 1;
                }
            }
        }
        
        if (original_index)
            return original_index(L);
        return 0;
    }

    inline int namecall_hook(lua_State* L)
    {
        if (!IsOurThread(L)) return original_namecall(L);

        if (L->userdata && L->userdata->capabilities == 0xFFFFFFFFFFFFFFFF)
        {
            const char* key = L->namecall ? getstr(L->namecall) : nullptr;
            if (key) {
                if (strcmp(key, "HttpGet") == 0 || strcmp(key, "HttpGetAsync") == 0)
                    return http::HttpGet(L);
            }
        }
        
        if (original_namecall)
            return original_namecall(L);
        return 0;
    }


    // register funcs here not in libs plz
    inline void Register(lua_State* L)
    {
        luaL_sandboxthread(L);

        lua_pushcfunction(L, http::HttpGet, "HttpGet");
        lua_setglobal(L, "HttpGet");

        lua_pushcfunction(L, closures::loadstring, "loadstring");
        lua_setglobal(L, "loadstring");

        lua_pushcfunction(L, identifyexecutor, "identifyexecutor");
        lua_setglobal(L, "identifyexecutor");

        lua_pushcfunction(L, getexecutorname, "getexecutorname");
        lua_setglobal(L, "getexecutorname");

        lua_pushcfunction(L, getgenv, "getgenv");
        lua_setglobal(L, "getgenv");

        int top = lua_gettop(L);
        lua_getglobal(L, "game");
        
        if (lua_istable(L, -1) || lua_isuserdata(L, -1))
        {
            if (luaL_getmetafield(L, -1, "__index"))
            {
                if (lua_type(L, -1) == LUA_TFUNCTION)
                {
                    Closure* cl = clvalue(const_cast<TValue*>(luaA_toobject(L, -1)));
                    original_index = cl->c.f;
                    cl->c.f = index_hook;
                }
                lua_pop(L, 1);
            }
            
            if (luaL_getmetafield(L, -1, "__namecall"))
            {
                if (lua_type(L, -1) == LUA_TFUNCTION)
                {
                    Closure* cl = clvalue(const_cast<TValue*>(luaA_toobject(L, -1)));
                    original_namecall = cl->c.f;
                    cl->c.f = namecall_hook;
                }
                lua_pop(L, 1);
            }
        }
        
        lua_settop(L, top);
    }
}
