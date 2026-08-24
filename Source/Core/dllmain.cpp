#include <windows.h>
#include <iostream>
#include <thread>
#include "lua.h"
#include "lualib.h"
#include "../Roblox/Offsets/Offsets.h"
#include "../Roblox/Offsets/Funcs.h"
#include "../Roblox/Environment/Instances.h"
#include "../Roblox/Environment/Environment.h"
#include "../Core/Execution/Execution.h"

// Todo
// add client modification bypass for BOOSTERS
// add more functions
// waitinghybirdscriptsjob
template<typename T>
T read(uintptr_t address, uintptr_t offset = 0) {
    return *reinterpret_cast<T*>(address + offset);
}

#pragma comment(lib, "ws2_32.lib")

void connection(lua_State* L) {
    WSADATA sss;
    if (WSAStartup(MAKEWORD(2, 2), &sss) != 0) return;

    SOCKET ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ss == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    sockaddr_in s;
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = inet_addr("127.0.0.1");
    s.sin_port = htons(9002); 

    if (bind(ss, (SOCKADDR*)&s, sizeof(s)) == SOCKET_ERROR) {
        closesocket(ss);
        WSACleanup();
        return;
    }

    if (listen(ss, 10) == SOCKET_ERROR) {
        closesocket(ss);
        WSACleanup();
        return;
    }

    while (true) {
        SOCKET client = accept(ss, nullptr, nullptr);
        if (client != INVALID_SOCKET) {
            std::string script;
            char buffer[4096];
            int ssss;

            while ((ssss = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[ssss] = '\0';
                script += buffer;
            }

            closesocket(client);

            if (!script.empty()) {
                Execution::execute(L, script);
            }
        }
        Sleep(10);
    }

    closesocket(ss);
    WSACleanup();
}

void main_thread() {
    uintptr_t fakeDataModel = read<uintptr_t>(Main::Miscellaneous::FakeDatamodelPOINTER);
    while (!fakeDataModel) {
        Sleep(100);
        fakeDataModel = read<uintptr_t>(Main::Miscellaneous::FakeDatamodelPOINTER);
    }
    
    uintptr_t dataModel = read<uintptr_t>(fakeDataModel + Main::Offsets::DataModel);
    while (read<uintptr_t>(dataModel, Main::Offsets::GameLoaded) != 31) {
        Sleep(100);
    }

    uintptr_t children = read<uintptr_t>(dataModel, Main::Offsets::Children);
    uintptr_t scriptContext = read<uintptr_t>(read<uintptr_t>(children), Main::Offsets::ScriptContext);

    lua_State* skidstate = laustate::GetLuasState(scriptContext);
    Execution::skidsstate = skidstate;

    env::Register(skidstate);

    lua_getglobal(skidstate, "print");
    lua_pushstring(skidstate, "(your shitsploit name) Has Loaded!");
    lua_pcall(skidstate, 1, 0, 0);
    std::thread(connection, skidstate).detach();
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread(main_thread).detach();
    }
    return TRUE;
}
