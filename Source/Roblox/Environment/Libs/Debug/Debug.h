#pragma once
// Stub for debug lib. Requires lua.h to be included before this.
// Real impl needs Luau bytecode parsing.

#include <cstring>

namespace dbg {
    inline int getconstant(lua_State* L) {
        luaL_checkany(L, 1);
        (void)luaL_checkinteger(L, 2);
        lua_pushnil(L);
        return 1;
    }

    inline int getconstants(lua_State* L) {
        luaL_checkany(L, 1);
        lua_newtable(L);
        return 1;
    }

    inline int getupvalue(lua_State* L) {
        luaL_checkany(L, 1);
        int idx = (int)luaL_checkinteger(L, 2);
        const char* name = lua_getupvalue(L, 1, idx);
        if (name) {
            // stack has: function (1), upvalue-name (2), upvalue-value (3)
            return 2;
        }
        lua_pushnil(L);
        return 1;
    }

    inline int setupvalue(lua_State* L) {
        luaL_checkany(L, 1);
        int idx = (int)luaL_checkinteger(L, 2);
        luaL_checkany(L, 3);
        const char* name = lua_setupvalue(L, 1, idx);
        if (name) lua_pushstring(L, name);
        else lua_pushnil(L);
        return 1;
    }

    inline int getinfo(lua_State* L) {
        luaL_checkany(L, 1);
        const char* what = luaL_optstring(L, 2, "n");
        lua_Debug ar;
        if (!lua_getinfo(L, 1, what, &ar)) {
            lua_pushnil(L);
            return 1;
        }
        lua_newtable(L);
        lua_pushstring(L, ar.source); lua_setfield(L, -2, "source");
        lua_pushstring(L, ar.short_src); lua_setfield(L, -2, "short_src");
        lua_pushinteger(L, ar.linedefined); lua_setfield(L, -2, "linedefined");
        lua_pushinteger(L, ar.currentline); lua_setfield(L, -2, "currentline");
        if (ar.name) lua_pushstring(L, ar.name); else lua_pushstring(L, "");
        lua_setfield(L, -2, "name");
        lua_pushstring(L, ar.what); lua_setfield(L, -2, "what");
        return 1;
    }

    inline int getproto(lua_State* L) {
        luaL_checkany(L, 1);
        (void)luaL_checkinteger(L, 2);
        lua_pushnil(L);
        return 1;
    }

    inline int getprotos(lua_State* L) {
        luaL_checkany(L, 1);
        lua_newtable(L);
        return 1;
    }

    inline int getstack(lua_State* L) {
        int level = (int)luaL_checkinteger(L, 1);
        lua_Debug ar;
        if (lua_getstack(L, level, &ar)) {
            lua_getinfo(L, level, "n", &ar);
            lua_newtable(L);
            if (ar.name) lua_pushstring(L, ar.name); else lua_pushstring(L, "");
            lua_setfield(L, -2, "name");
            return 1;
        }
        lua_pushnil(L);
        return 1;
    }

    inline int setstack(lua_State* L) {
        lua_pushnil(L);
        return 1;
    }

    inline int setconstant(lua_State* L) {
        lua_pushnil(L);
        return 1;
    }

    inline int getfenv(lua_State* L) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        return 1;
    }
}
