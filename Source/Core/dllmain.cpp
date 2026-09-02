#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include "lua.h"
#include "lualib.h"
#include "../Roblox/Offsets/Offsets.h"
#include "../Roblox/Offsets/Funcs.h"
#include "../Roblox/Environment/Instances.h"
#include "../Roblox/Environment/Environment.h"
#include "../Core/Execution/Execution.h"

#include "TeleportHandler.hpp"

// Todo
// waitinghybirdscriptsjob
template<typename T>
T read(uintptr_t address, uintptr_t offset = 0) {
    return *reinterpret_cast<T*>(address + offset);
}

void connection(lua_State* L) {
    // Use named pipe instead of raw winsock. Port 9002 was detectable by
    // anti-cheat network monitors. Named pipe \\.\pipe\RenzBase is invisible.
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    char pipeName[] = "\\\\.\\pipe\\RenzBase";

    while (true) {
        hPipe = CreateNamedPipeA(
            pipeName,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096, 4096, 0, nullptr);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(1000);
            continue;
        }

        // Wait for a client (the UI) to connect.
        BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!connected) {
            CloseHandle(hPipe);
            continue;
        }

        // Read script source.
        std::string script;
        char buffer[4096];
        DWORD bytesRead = 0;

        while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            script += buffer;
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);

        if (!script.empty()) {
            Execution::execute(L, script);
        }

        Sleep(10);
    }
}

void main_thread() {
    Teleport->load();
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread(main_thread).detach();
    }
    return TRUE;
}
