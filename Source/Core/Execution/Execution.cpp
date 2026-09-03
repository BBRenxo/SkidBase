#include "Execution.h"
#include "Luau/BytecodeUtils.h"
#include "lstate.h"
#include "lobject.h"
#include "lapi.h"
#include "../unc/Unc.h"

namespace Execution
{
    lua_State* rboloxstate = nullptr;
    lua_State* skidsstate = nullptr;
    std::queue<std::string> queue;
    std::mutex mutexex;

    // ROBLOX_CAP_* constants — only enable what scripts actually need.
    // Setting ALL bits = instant executor detection. Set specific bits only.
    //
    // Level 8 = Plugin / Studio. Real Plugin caps are limited (no GetAsync,
    // no WriteFile, etc). We set identity=8 + executor-mark bit + caps that
    // Roblox's own scripts can use (loadstring, basic).
    //
    // For dangerous ops (WriteFile, ReadFile, HttpGet via HttpService, firetouchinterest)
    // we use identity escalation per-call instead of blanket capabilities.
    constexpr uint64_t CAP_PLUGIN_BASIC = 0x0000000000000100ULL;  // loadstring for plugin
    constexpr uint64_t CAP_EXECUTOR_MARK = 0x0000000000000040ULL;  // marks threads as executor-owned
    constexpr uint64_t CAP_WRITE_SCRIPT = 0x0000000000000008ULL;  // create script instances
    constexpr uint64_t CAP_NETWORK = 0x0000000000000004ULL;  // network ops
    constexpr uint64_t CAP_PLUGIN_FULL = 0x00000000000001FFULL;  // plugin-style full caps (used internally)

    // External-facing caps: what we set on user script threads.
    // Identity 8 + loadstring + write-script + executor-mark.
    // NO: plugin (level 7), game admin (level 6), or full 0xFF..FF.
    const uintptr_t caps = CAP_PLUGIN_BASIC | CAP_EXECUTOR_MARK | CAP_WRITE_SCRIPT | CAP_NETWORK;

    class BytecodeEncoder : public Luau::BytecodeEncoder
    {
        inline void encode(uint32_t* data, size_t count) override
        {
            for (auto i = 0; i < count;)
            {
                uint8_t opcode = LUAU_INSN_OP(data[i]);
                const auto lookuptable = reinterpret_cast<uint8_t*>(Main::Functions::OpcodeLookupTable);
                uint8_t opdiamondsword = opcode * 227;
                opdiamondsword = lookuptable[opdiamondsword];

                data[i] = (opdiamondsword) | (data[i] & ~0xFF);
                i += Luau::getOpLength(static_cast<LuauOpcode>(opcode));
            }
        }
    };

    std::string aexecute(std::string source)
    {
        // === Preprocess script source ===
        // Replace game:HttpGet(...) with HttpGet(...) — Roblox's built-in
        // game:HttpGet returns an HttpRequest Instance, not a string, which
        // breaks loadstring(game:HttpGet(url))(). Our HttpGet returns a
        // string and works the same way. Same for HttpGetAsync, HttpPost,
        // and request.
        size_t pos = 0;
        while ((pos = source.find("game:HttpGet", pos)) != std::string::npos) {
            source.replace(pos, 13, "HttpGet");
            pos += 8;
        }
        pos = 0;
        while ((pos = source.find("game:HttpGetAsync", pos)) != std::string::npos) {
            source.replace(pos, 18, "HttpGetAsync");
            pos += 13;
        }
        pos = 0;
        while ((pos = source.find("game:HttpPost", pos)) != std::string::npos) {
            source.replace(pos, 14, "HttpPost");
            pos += 9;
        }
        pos = 0;
        while ((pos = source.find("game:request", pos)) != std::string::npos) {
            source.replace(pos, 12, "request");
            pos += 7;
        }

        // === Strip non-ASCII bytes ===
        // Luau's loadstring rejects sources with bytes > 127 (UTF-8 emoji etc.).
        // Replace them with spaces so the script remains syntactically valid.
        for (size_t i = 0; i < source.size(); ) {
            unsigned char c = (unsigned char)source[i];
            if (c < 0x80) { i++; continue; }
            if (c >= 0xC0 && c < 0xE0 && i + 1 < source.size()) { source[i] = ' '; source[i+1] = ' '; i += 2; continue; }
            if (c >= 0xE0 && c < 0xF0 && i + 2 < source.size()) { source[i] = ' '; source[i+1] = ' '; source[i+2] = ' '; i += 3; continue; }
            if (c >= 0xF0 && i + 3 < source.size()) { source[i] = ' '; source[i+1] = ' '; source[i+2] = ' '; source[i+3] = ' '; i += 4; continue; }
            source[i] = ' ';
            i++;
        }

        auto bytecide = BytecodeEncoder();
        static const char* globalz[] = { "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", "_G", "_ENV", nullptr };

        Luau::CompileOptions options;
        options.debugLevel = 1;
        options.optimizationLevel = 1;
        options.mutableGlobals = globalz;
        options.vectorLib = "Vector3";
        options.vectorCtor = "new";
        options.vectorType = "Vector3";

        return Luau::compile(source, options, {}, &bytecide);
    }

    void setprotocapabilities(Proto* proto, uintptr_t* capabilities)
    {
        if (!proto) return;
        proto->userdata = capabilities;
        for (int i = 0; i < proto->sizep; ++i)
            if (proto->p[i])
                setprotocapabilities(proto->p[i], capabilities);
    }

    void setthreadcapabilities(lua_State* L, int level, uintptr_t capabilities, bool AddExecutorMark)
    {
        if (!L || !L->userdata) return;
        
        L->userdata->identity = level;
        L->userdata->capabilities = capabilities | (AddExecutorMark ? 0b1000000 : 0);
    }

    void execute(lua_State* L, const std::string& script)
    {
		std::lock_guard<std::mutex> lock(Execution::mutexex);
        if (!L || script.empty())
            return;

        int originalTop = lua_gettop(L);
        lua_State* threadex = lua_newthread(L);
        // checks please
		if (!threadex)
		{
			lua_getglobal(L, "print");
			lua_pushstring(L, "failed to create a thread");
			lua_pcall(L, 1, 0, 0);
			return;
		}
        lua_pop(L, 1);

        luaL_sandboxthread(threadex);

        // Apply unc/sunc to the new thread. Default to Unc (yield counter
        // zeroed) — user can change via setunc/getsunc Lua API.
        unc::apply(threadex, unc::Mode::Unc);

        setthreadcapabilities(threadex, 8, caps, false);

        // === Preprocess script source ===
        // game:HttpGet etc. are rewritten to HttpGet etc. by aexecute(),
        // which is called below. No need to duplicate the work here.

        std::string bytecode = aexecute(script);
        if (luau_load(threadex, "", bytecode.c_str(), bytecode.length(), NULL) != LUA_OK)
        {
            std::string error = lua_tostring(threadex, -1);
            
            lua_getglobal(L, "print");
            lua_pushstring(L, ("script error:" + error).c_str()); // it cool
            lua_pcall(L, 1, 0, 0);

            lua_pop(threadex, 1);
			lua_settop(L, originalTop);
            return;
        }

        Closure* closure = clvalue(const_cast<TValue*>(luaA_toobject(threadex, -1)));
        setprotocapabilities(closure->l.p, const_cast<uintptr_t*>(&caps));

        lua_getglobal(threadex, "task");
        lua_getfield(threadex, -1, "defer");
        lua_remove(threadex, -2);
        lua_insert(threadex, -2);

        if (lua_pcall(threadex, 1, NULL, NULL) != LUA_OK)
        {
            std::string error = lua_tostring(threadex, -1);
            
            lua_getglobal(L, "print");
            lua_pushstring(L, ("(your shitsploit name) runtime error: " + error).c_str());
            lua_pcall(L, 1, 0, 0);

            lua_pop(threadex, 1);
			lua_settop(L, originalTop);
            return;
        }

        lua_settop(L, originalTop);
    }

    void extexecute(const std::string& script) {
        if (script.empty())
            return;

        std::lock_guard<std::mutex> Lock(Execution::mutexex);
        while (Execution::queue.size() >= 512)
            Execution::queue.pop();

        Execution::queue.push(script);
    }

    uintptr_t GetJobByTypeName(const std::string& TypeName) { // todo in update
        return 0;
    }
}
