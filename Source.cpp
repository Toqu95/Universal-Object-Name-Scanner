#include <iostream>
#include <Windows.h>

#include "MinHook/MinHook.h"
#include <vector>
#include <Psapi.h>
#include "IL2CPP Resolver/Main.hpp"

#if _WIN64 
#pragma comment(lib, "MinHook/libMinHook.x64.lib")
#else
#pragma comment(lib, "MinHook/libMinHook.x86.lib")
#endif

static MODULEINFO GetModuleInfo(const char* szModule)
{
    MODULEINFO modinfo = { 0 };
    HMODULE hModule = GetModuleHandle(szModule);
    if (hModule == 0)
        return modinfo;
    GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    return modinfo;
}

static uintptr_t FindPattern(const char* module, const char* pattern, const char* mask)
{
    MODULEINFO mInfo = GetModuleInfo(module);
    uintptr_t base = (uintptr_t)mInfo.lpBaseOfDll;
    uintptr_t size = (uintptr_t)mInfo.SizeOfImage;
    uintptr_t patternLength = (uintptr_t)strlen(mask);

    for (uintptr_t i = 0; i < size - patternLength; i++)
    {
        bool found = true;
        for (uintptr_t j = 0; j < patternLength; j++)
        {
            found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
        }
        if (found)
        {
            return uintptr_t(base + i);
        }
    }

    return NULL;
}

template <typename T>
static T Read(uintptr_t address)
{
    return *((T*)address);
}

template<typename T>
static void Write(uintptr_t address, T value)
{
    *((T*)address) = value;
}

template<typename T>
static uintptr_t Protect(uintptr_t address, uintptr_t protect)
{
    DWORD oldProt;
    VirtualProtect((LPVOID)address, sizeof(T), protect, &oldProt);
    return oldProt;
}

static uintptr_t GetModuleAddress(const char* moduleName)
{
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (hModule == NULL)
        return NULL;
    return (uintptr_t)hModule;
}

static uintptr_t FindDMAAddy(uintptr_t baseAddress, std::vector<unsigned int> offsets)
{
    for (int i = 0; i < offsets.size(); i++)
        baseAddress = *(uintptr_t*)baseAddress + offsets[i];
    return baseAddress;
}

template<typename... T>
void print(T... args) {
    (std::cout << ... << args) << std::endl;
}

void Exit(FILE* _stream, HMODULE _module)
{
    FreeConsole();
    fclose(_stream);
    FreeLibraryAndExitThread(_module, 0);
}

void MainThread(HMODULE hModule)
{
    // Create Console
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);

    // MinHook Initialize
    if (!MH_Initialize() == MH_OK)
    {
        print("[MinHook] Failed to Initialize");
        Exit(fp, hModule);
    }
    else
        print("[MinHook] Initialized");

    IL2CPP::Initialize();

    print("[IL2CPP Resolver] Initialized");

    Unity::il2cppArray<Unity::CGameObject*>* Players;

    // Loop
    while (true)
    {
        // Delay
        Sleep(10);

        if (GetAsyncKeyState(VK_F1))
        {
            system("cls");
            Unity::il2cppArray<Unity::CGameObject*>* Players = Unity::Object::FindObjectsOfType<Unity::CGameObject>("UnityEngine.GameObject");

            for (int i = 0; i < Players->m_uMaxLength; i++)
            {
                print((void*)Players->At(i), " : ", Players->At(i)->GetName()->ToString());
            }
        }

        // Exit Key -/- Panic Key
        if (GetAsyncKeyState(VK_END))
            break;
    }

    // MinHook UnInitialize
    MH_Uninitialize();
    print("[MinHook] UnInitialized");

    // Delay
    Sleep(100);

    // Exit - Clean up
    Exit(fp, hModule);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr));
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}