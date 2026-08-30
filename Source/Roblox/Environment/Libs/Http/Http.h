#pragma once
#include "lua.h"
#include "lualib.h"
#include <Windows.h>
#include <winhttp.h>
#include <string>
#include <map>

#pragma comment(lib, "winhttp.lib")

namespace http
{
    inline std::string req(std::string url, std::map<std::string, std::string> hdrs = {})
    {
        bool sec = (url.size() >= 8 && (_strnicmp(url.c_str(), "https://", 8) == 0));
        std::string clean_url = sec ? url.substr(8) : url.substr(7);

        auto slash = clean_url.find('/');
        auto host = clean_url.substr(0, slash);
        auto path = slash == std::string::npos ? "/" : clean_url.substr(slash);

        auto sess = WinHttpOpen(L"SkidBase", 0, 0, 0, 0);
        if (!sess) return "";

        auto conn = WinHttpConnect(sess, std::wstring(host.begin(), host.end()).c_str(), sec ? 443 : 80, 0);
        if (!conn) { WinHttpCloseHandle(sess); return ""; }

        auto req = WinHttpOpenRequest(conn, L"GET", std::wstring(path.begin(), path.end()).c_str(), 0, 0, 0, sec ? WINHTTP_FLAG_SECURE : 0);
        if (!req) { WinHttpCloseHandle(conn); WinHttpCloseHandle(sess); return ""; }

        std::wstring raw_hdrs;
        for (auto& [k, v] : hdrs)
            raw_hdrs += std::wstring(k.begin(), k.end()) + L": " + std::wstring(v.begin(), v.end()) + L"\r\n";

        WinHttpSendRequest(req, raw_hdrs.empty() ? 0 : raw_hdrs.c_str(), (DWORD)raw_hdrs.length(), 0, 0, 0, 0);
        WinHttpReceiveResponse(req, 0);

        std::string res;
        DWORD sz = 0;
        do {
            WinHttpQueryDataAvailable(req, &sz);
            if (!sz) break;
            auto buf = new char[sz + 1];
            DWORD read = 0;
            WinHttpReadData(req, buf, sz, &read);
            buf[read] = 0;
            res += buf;
            delete[] buf;
        } while (sz > 0);

        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(sess);
        return res;
    }

    inline int HttpGet(lua_State* L)
    {
        int idx = (lua_type(L, 1) == LUA_TUSERDATA || lua_type(L, 1) == LUA_TTABLE) ? 2 : 1;
        const char* raw_url = luaL_checkstring(L, idx);

        std::string url = raw_url;

        if (url.find("http://") != 0 && url.find("https://") != 0) {
            luaL_error(L, "Invalid protocol (expected 'http://' or 'https://')");
            return 0;
        }

        std::map<std::string, std::string> hdrs;
        if (lua_istable(L, idx + 1)) {
            lua_pushnil(L);
            while (lua_next(L, idx + 1)) {
                if (lua_isstring(L, -2) && lua_isstring(L, -1))
                    hdrs[lua_tostring(L, -2)] = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
        }

        hdrs["User-Agent"] = "SkidBase";
        auto res = req(url, hdrs);

        if (res.empty()) {
            lua_pushstring(L, "son");
            return 1;
        }

        lua_pushstring(L, res.c_str());
        return 1;
    }
}
