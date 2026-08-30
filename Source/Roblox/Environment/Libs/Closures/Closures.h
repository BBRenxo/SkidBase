#pragma once
#include "lua.h"
#include "lualib.h"
#include "lobject.h"
#include "lstate.h"
#include "lapi.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "../../../../Core/Execution/Execution.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

bool isitourskidthread(lua_State* L);

namespace closures
{
    inline int loadstring(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        const char* src = lua_tostring(L, 1);
        const char* chunkname = luaL_optstring(L, 2, "SkidBase");

        std::string bytecode = Execution::aexecute(src);
        if (bytecode.empty() || bytecode[0] == '\0')
        {
            lua_pushnil(L);
            lua_pushstring(L, bytecode.c_str() + 1);
            return 2;
        }

        if (luau_load(L, chunkname, bytecode.c_str(), bytecode.size(), 0) != LUA_OK)
        {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        Closure* fn = clvalue(const_cast<TValue*>(luaA_toobject(L, -1)));
        if (fn && fn->l.p)
            Execution::setprotocapabilities(fn->l.p, const_cast<uintptr_t*>(&Execution::caps));

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
        return 1;
    }

    inline int checkcaller(lua_State* L)
    {
        lua_pushboolean(L, isitourskidthread(L));
        return 1;
    }

    inline int iscclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_pushboolean(L, lua_iscfunction(L, 1));
        return 1;
    }

    inline int islclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_pushboolean(L, !lua_iscfunction(L, 1));
        return 1;
    }

    inline int clonefunction(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_clonefunction(L, 1);
        return 1;
    }

    inline int identifyexecutor(lua_State* L)
    {
        lua_pushstring(L, "SkidBase");
        lua_pushstring(L, "1.0.0");
        return 2;
    }

    inline int isexecutorclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        Closure* cl = clvalue(const_cast<TValue*>(luaA_toobject(L, 1)));
        lua_pushboolean(L, isitourskidthread(L) && cl != nullptr);
        return 1;
    }
}