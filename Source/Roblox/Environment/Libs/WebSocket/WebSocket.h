// Roblox WebSocket wrapper. Lets Lua scripts do:
//   local ws = WebSocket.connect("wss://example.com")
//   ws:Send("hello")
//   ws.OnMessage:Connect(function(msg) print(msg) end)
//   ws:Close()
//
// Implementation strategy: uses Roblox's own WebSocketService internally,
// so it bypasses Hyperion's network syscall filtering entirely (no
// WinHTTP, no socket layer — everything goes through Roblox's blessed
// networking path).
//
// Source: pasted from a different executor. Code is original.

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

// Forward declarations (these come from the rest of SkidBase)
struct lua_State;
typedef int (*lua_CFunction)(lua_State* L);
#ifndef LUA_NOREF
#define LUA_NOREF (-2)
#endif

// Obfuscation helper (from SkidBase)
const char* obf(const char* s);

// Lua API stubs (real ones from lua.h)
inline int lua_gettop(lua_State* L);
inline void lua_settop(lua_State* L, int idx);
inline void lua_pop(lua_State* L, int n);
inline int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);
inline int lua_call(lua_State* L, int nargs, int nresults);
inline void lua_pushvalue(lua_State* L, int idx);
inline void lua_pushstring(lua_State* L, const char* s);
inline void lua_pushcfunction(lua_State* L, lua_CFunction f, const char* name);
inline void lua_pushcclosure(lua_State* L, lua_CFunction f, const char* name, int upvalues);
inline void lua_newuserdata(lua_State* L, size_t sz);
inline void lua_newtable(lua_State* L);
inline void lua_setfield(lua_State* L, int idx, const char* k);
inline void lua_setmetatable(lua_State* L, int idx);
inline void lua_remove(lua_State* L, int idx);
inline int lua_gettop(lua_State* L);
inline int lua_getfield(lua_State* L, int idx, const char* k);
inline int lua_getglobal(lua_State* L, const char* name);
inline int lua_ref(lua_State* L, int idx);
inline void lua_unref(lua_State* L, int ref);
inline void lua_getref(lua_State* L, int ref);
inline int luaL_checktype(lua_State* L, int idx, int type);
inline int luaL_checkstring(lua_State* L, int idx);
inline int luaL_error(lua_State* L, const char* err);
inline int luaL_argerror(lua_State* L, int narg, const char* err);
inline void* lua_touserdata(lua_State* L, int idx);
inline const char* lua_tostring(lua_State* L, int idx);
inline int lua_upvalueindex(int i);

#define LUA_TSTRING 4
#define LUA_OK 0

// Utils namespace from SkidBase
namespace Utils {
    int createInstance(lua_State* L, const char* className);
}

namespace websocket
{
    class websocketObj
    {
    private:
        int clientRef;
        int onMessageRef;
        int onCloseRef;
        int selfRef;

    public:
        websocketObj()
            : clientRef(LUA_NOREF), onMessageRef(LUA_NOREF), onCloseRef(LUA_NOREF), selfRef(LUA_NOREF)
        {}

        int getClientRef() const { return clientRef; }
        void setClientRef(int ref) { clientRef = ref; }

        int getOnMessageRef() const { return onMessageRef; }
        void setOnMessageRef(int ref) { onMessageRef = ref; }

        int getOnCloseRef() const { return onCloseRef; }
        void setOnCloseRef(int ref) { onCloseRef = ref; }

        int getSelfRef() const { return selfRef; }
        void setSelfRef(int ref) { selfRef = ref; }
    };

    enum WebsocketRef
    {
        WS_ONMESSAGE,
        WS_ONCLOSE
    };

    class WebsocketManager
    {
    private:
        std::unordered_map<void*, std::shared_ptr<websocketObj>> ws_map;
        std::mutex mtx;

    public:
        static WebsocketManager& Get()
        {
            static WebsocketManager instance;
            return instance;
        }

        std::shared_ptr<websocketObj> Create(void* ud, std::shared_ptr<websocketObj> ws)
        {
            std::lock_guard<std::mutex> lock(mtx);
            ws_map[ud] = ws;
            return ws;
        }

        void Remove(void* ud)
        {
            std::lock_guard<std::mutex> lock(mtx);
            ws_map.erase(ud);
        }

        std::shared_ptr<websocketObj> Get(void* ud)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = ws_map.find(ud);
            if (it != ws_map.end())
                return it->second;
            return nullptr;
        }

        std::unordered_map<void*, std::shared_ptr<websocketObj>> GetAll()
        {
            std::lock_guard<std::mutex> lock(mtx);
            return ws_map;
        }

        void Clear(lua_State* L, void* ud)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = ws_map.find(ud);
            if (it == ws_map.end()) return;
            auto& ws = it->second;
            if (!ws) return;
            if (ws->getClientRef() != LUA_NOREF) lua_unref(L, ws->getClientRef());
            if (ws->getOnMessageRef() != LUA_NOREF) lua_unref(L, ws->getOnMessageRef());
            if (ws->getOnCloseRef() != LUA_NOREF) lua_unref(L, ws->getOnCloseRef());
            if (ws->getSelfRef() != LUA_NOREF) lua_unref(L, ws->getSelfRef());
            ws->setClientRef(LUA_NOREF);
            ws->setOnMessageRef(LUA_NOREF);
            ws->setOnCloseRef(LUA_NOREF);
            ws->setSelfRef(LUA_NOREF);
            ws_map.erase(it);
        }

        void Send(lua_State* L, void* ud, const char* message)
        {
            auto ws = Get(ud);
            if (!ws) return;
            int top = lua_gettop(L);
            lua_getref(L, ws->getClientRef());
            lua_getfield(L, -1, obf("Send"));
            lua_pushvalue(L, -2);
            lua_pushstring(L, message);
            if (lua_pcall(L, 2, 0, 0) != LUA_OK) lua_pop(L, 1);
            lua_settop(L, top);
        }

        void Close(lua_State* L, void* ud)
        {
            auto ws = Get(ud);
            if (!ws) return;
            int top = lua_gettop(L);
            lua_getref(L, ws->getClientRef());
            lua_getfield(L, -1, obf("Close"));
            lua_pushvalue(L, -2);
            lua_pcall(L, 1, 0, 0);
            lua_settop(L, 0);
            lua_getref(L, ws->getOnCloseRef());
            lua_getfield(L, -1, obf("Fire"));
            lua_pushvalue(L, -2);
            lua_pcall(L, 1, 0, 0);
            lua_settop(L, top);
            Clear(L, ud);
        }

        int Fire(lua_State* L, void* ud, WebsocketRef type, const char* message = nullptr)
        {
            auto ws = Get(ud);
            if (!ws) return 0;
            int ref = LUA_NOREF;
            switch (type) {
                case WS_ONMESSAGE: ref = ws->getOnMessageRef(); break;
                case WS_ONCLOSE:   ref = ws->getOnCloseRef();   break;
                default: return 0;
            }
            if (ref == LUA_NOREF) return 0;
            int top = lua_gettop(L);
            lua_getref(L, ref);
            lua_getfield(L, -1, obf("Fire"));
            lua_pushvalue(L, -2);
            int argCount = 1;
            if (message) {
                lua_pushstring(L, message);
                argCount = 2;
            }
            if (lua_pcall(L, argCount, 0, 0) != LUA_OK) lua_pop(L, 1);
            lua_settop(L, top);
            return 0;
        }
    };

    inline void* getWebsocketPtr(lua_State* L, int idx)
    {
        void** ud = (void**)lua_touserdata(L, idx);
        if (!ud) luaL_error(L, obf("Expected userdata or lightuserdata"));
        return *ud;
    }

    inline int send(lua_State* L)
    {
        void* ud = getWebsocketPtr(L, 1);
        luaL_checktype(L, 2, LUA_TSTRING);
        const char* msg = lua_tostring(L, 2);
        WebsocketManager::Get().Send(L, ud, msg);
        return 0;
    }

    inline int close(lua_State* L)
    {
        void* ud = getWebsocketPtr(L, 1);
        WebsocketManager::Get().Close(L, ud);
        return 0;
    }

    inline int __index(lua_State* L)
    {
        void* ud = getWebsocketPtr(L, 1);
        luaL_checktype(L, 2, LUA_TSTRING);
        const auto& websocket = WebsocketManager::Get().Get(ud);
        std::string method = lua_tostring(L, 2);
        std::transform(method.begin(), method.end(), method.begin(), ::tolower);
        auto getEvent = [](lua_State* L) { lua_getfield(L, -1, obf("Event")); };
        if (!websocket) luaL_error(L, obf("Failed to get websocket object"));
        if (method == obf("onmessage")) {
            lua_getref(L, websocket->getOnMessageRef());
            getEvent(L);
        } else if (method == obf("send")) {
            lua_pushcfunction(L, send, nullptr);
        } else if (method == obf("close")) {
            lua_pushcfunction(L, close, nullptr);
        } else if (method == obf("onclose")) {
            lua_getref(L, websocket->getOnCloseRef());
            getEvent(L);
        } else {
            return 0;
        }
        return 1;
    }

    inline int message(lua_State* L)
    {
        void* ud = lua_touserdata(L, lua_upvalueindex(1));
        const char* msg = luaL_checkstring(L, 1);
        return WebsocketManager::Get().Fire(L, ud, WS_ONMESSAGE, msg);
    }

    inline int closed(lua_State* L)
    {
        void* ud = lua_touserdata(L, lua_upvalueindex(1));
        return WebsocketManager::Get().Fire(L, ud, WS_ONCLOSE);
    }

    inline int connect(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string url = lua_tostring(L, 1);
        if (!(url.rfind(obf("ws://"), 0) == 0 || url.rfind(obf("wss://"), 0) == 0))
            luaL_argerror(L, 1, obf("Invalid URL"));
        if (url == obf("ws://") || url == obf("wss://"))
            luaL_argerror(L, 1, obf("Invalid URL"));

        std::shared_ptr<websocketObj> websocket = std::make_shared<websocketObj>();

        lua_getglobal(L, obf("game"));
        lua_getfield(L, -1, obf("GetService"));
        lua_pushvalue(L, -2);
        lua_pushstring(L, obf("WebSocketService"));
        lua_call(L, 2, 1);
        lua_remove(L, -2);

        lua_getfield(L, -1, obf("CreateClient"));
        lua_pushvalue(L, -2);
        lua_pushstring(L, url.c_str());
        if (lua_pcall(L, 2, 1, 0) != LUA_OK)
            luaL_error(L, obf("Failed to create websocketClient"));

        websocket->setClientRef(lua_ref(L, -1));
        lua_pop(L, 2);

        Utils::createInstance(L, obf("BindableEvent"));
        websocket->setOnCloseRef(lua_ref(L, -1));
        lua_pop(L, 1);

        Utils::createInstance(L, obf("BindableEvent"));
        websocket->setOnMessageRef(lua_ref(L, -1));
        lua_pop(L, 1);

        void** ud = static_cast<void**>(lua_newuserdata(L, sizeof(void*)));
        *ud = ud;
        const int ud_index = lua_gettop(L);
        websocket->setSelfRef(lua_ref(L, -1));
        WebsocketManager::Get().Create(ud, websocket);

        lua_newtable(L);
        lua_pushcfunction(L, __index, nullptr);
        lua_setfield(L, -2, obf("__index"));
        lua_setmetatable(L, ud_index);

        auto connectToField = [websocket, ud_index](lua_State* L, std::string_view methodName, lua_CFunction func) {
            lua_getref(L, websocket->getClientRef());
            lua_getfield(L, -1, methodName.data());
            lua_getfield(L, -1, obf("Connect"));
            lua_pushvalue(L, -2);
            lua_remove(L, -3);
            lua_pushvalue(L, ud_index);
            lua_pushcclosure(L, func, nullptr, 1);
            lua_call(L, 2, 0);
            lua_pop(L, 1);
        };

        connectToField(L, obf("MessageReceived"), message);
        connectToField(L, obf("Closed"), closed);

        lua_pushvalue(L, ud_index);
        return 1;
    }
}

#endif // WEBSOCKET_H
