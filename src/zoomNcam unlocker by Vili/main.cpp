#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <sstream>
#include "ntinfo.h"
#include <winbase.h>
#include <string.h>
#include <process.h>

//global vars
float zoomValue = 1.281169772;
float leftNrightValue = 180;
float upNdownValue = 56;
const float leftNrightReset = 180;
const float upNdownReset = 56;
const float rotationSpeed = 5.5;
const float Msensitivity = 1.5;
const float zoomSpeed = 0.05;
int scroll = 0;
char WindowName[30] = "League of Legends (TM) Client";
BOOL WindowinFocus = false;
HWND hGameWindow = FindWindow(NULL, WindowName);
HHOOK hook = NULL;
uintptr_t camZAddress = NULL;
uintptr_t leftNrightAdderss = NULL;
uintptr_t upNdownAddress = NULL;
DWORD pID = NULL;
HANDLE processHandle = NULL;
//get screen res
int x = GetSystemMetrics(SM_CXSCREEN);
int y = GetSystemMetrics(SM_CYSCREEN);

auto titleGen = [](int num)
{
    std::srand(std::time(0));
    std::string titleName;
    for (int i = 0; i < num; i++)
    {
        titleName += rand() % 300 + 300;
    }
    return titleName;
};

std::vector<uintptr_t> threadList(uintptr_t pid)
{
    std::vector<uintptr_t> vect = std::vector<uintptr_t>();
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h == INVALID_HANDLE_VALUE)
    return vect;
    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    if (Thread32First(h, &te))
    {
        do
        {
            if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
            {
                if (te.th32OwnerProcessID == pid)
                {
                    vect.push_back(te.th32ThreadID);
                }
            }
            te.dwSize = sizeof(te);
        }
        while (Thread32Next(h, &te));
    }
    return vect;
}

uintptr_t GetThreadStartAddress(HANDLE processHandle, HANDLE hThread)
{
    uintptr_t stacktop = 0, result = 0;
    MODULEINFO mi;
    GetModuleInformation(processHandle, GetModuleHandle("kernel32.dll"), &mi, sizeof(mi));
    stacktop = (uintptr_t)GetThreadStackTopAddress_x86(processHandle, hThread);
    CloseHandle(hThread);
    if (stacktop) {

        uintptr_t* buf32 = new uintptr_t[8192];

        if (ReadProcessMemory(processHandle, (LPCVOID)(stacktop - 8192), buf32, 8192, NULL))
        {
            for (int i = 8192 / 8 - 1; i >= 0; --i)
            {
                if (buf32[i] >= (uintptr_t)mi.lpBaseOfDll && buf32[i] <= (uintptr_t)mi.lpBaseOfDll + mi.SizeOfImage)
                {
                    result = stacktop - 8192 + i * 8;
                    break;
                }
            }
        }

        delete[] buf32;
    }
    return result;
}

uintptr_t GetThreadstackStartAddress(int stackNumber, uintptr_t pID, HANDLE processHandle)
{
    std::vector<uintptr_t> threadId = threadList(pID);
    int stackNum = 0;
    for (auto it = threadId.begin(); it != threadId.end(); ++it)
    {
        HANDLE threadHandle = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, *it);
        uintptr_t threadStartAddress = GetThreadStartAddress(processHandle, threadHandle);
        if (stackNum == stackNumber) return threadStartAddress;
        stackNum++;
    }
    return 0;
}

LRESULT CALLBACK MouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION)
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    MSLLHOOKSTRUCT* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
    if (GetForegroundWindow() != FindWindow(NULL, WindowName))
    {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    if (wParam == WM_MOUSEWHEEL)
    {
        if (info->mouseData == 0x780000)
        {
            if (zoomValue > 0.78)
            {
                zoomValue -= zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress), &zoomValue, sizeof(float), 0);
            }
        }
        else
        {
            if (zoomValue < 2.7)
            {
                zoomValue += zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress), &zoomValue, sizeof(float), 0);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
    if (hook)
    {
        UnhookWindowsHookEx(hook);
        hook = NULL;
    }
    return TRUE;
}

void setupHook()
{
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
    hook = SetWindowsHookExW(WH_MOUSE_LL, MouseHook, nullptr, 0);
    if (!hook)
    {
        exit(EXIT_FAILURE);
    }
    GetMessageW(nullptr, nullptr, 0, 0);
}

void mouseLock()
{
    while (true)
    {
        Sleep(10);

        if (GetForegroundWindow() == FindWindow(NULL, WindowName))
        {
            if (GetAsyncKeyState(VK_MBUTTON)) //lock mouse
            {
                SetCursorPos(x / 2, y / 2);
                Sleep(5);
            }
        }
        else
        {
            Sleep(500);
        }
    }
}

void mouseLR()
{
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)mouseLock, 0, 0, 0);//mouseLock thread
    POINT p; //for cursor pos
    while (true)
    {
        Sleep(5);
        if (GetForegroundWindow() == FindWindow(NULL, WindowName))
        {
            if (GetAsyncKeyState(VK_MBUTTON)) //lock mouse
            {
                if (GetCursorPos(&p))
                {
                    if (p.x < x / 2)
                    {
                        leftNrightValue += Msensitivity;
                        WriteProcessMemory(processHandle, (LPVOID)(leftNrightAdderss), &leftNrightValue, sizeof(float), 0);
                    }

                    if (p.x > x / 2)
                    {
                        leftNrightValue -= Msensitivity;
                        WriteProcessMemory(processHandle, (LPVOID)(leftNrightAdderss), &leftNrightValue, sizeof(float), 0);
                    }

                    if (p.y < y / 2)
                    {
                        if (upNdownValue < 88)
                        {
                            upNdownValue += Msensitivity;
                            WriteProcessMemory(processHandle, (LPVOID)(upNdownAddress), &upNdownValue, sizeof(float), 0);
                        }
                    }

                    if (p.y > y / 2)
                    {
                        if (upNdownValue > 17.5)
                        {
                            upNdownValue -= Msensitivity;
                            WriteProcessMemory(processHandle, (LPVOID)(upNdownAddress), &upNdownValue, sizeof(float), 0);
                        }
                    }

                }
            }
        }
        else
        {
            Sleep(500);
        }
  
    }
}

void keyboard()
{
    while (true)
    {
        Sleep(10);

        if (GetForegroundWindow() == FindWindow(NULL, WindowName))
        {
            if (GetAsyncKeyState(VK_NUMPAD0)) //reset
            {
                WriteProcessMemory(processHandle, (LPVOID)(leftNrightAdderss), &leftNrightReset, sizeof(float), 0);
                leftNrightValue = leftNrightReset;
                WriteProcessMemory(processHandle, (LPVOID)(upNdownAddress), &upNdownReset, sizeof(float), 0);
                upNdownValue = upNdownReset;
            }

            if (GetAsyncKeyState(VK_ADD)) //numpad +
            {
                if (zoomValue > 0.78)
                {
                    zoomValue -= zoomSpeed;
                    WriteProcessMemory(processHandle, (LPVOID)(camZAddress), &zoomValue, sizeof(float), 0);
                }
            }

            if (GetAsyncKeyState(VK_SUBTRACT)) // numpad -
            {
                if (zoomValue < 2.7)
                {
                    zoomValue += zoomSpeed;
                    WriteProcessMemory(processHandle, (LPVOID)(camZAddress), &zoomValue, sizeof(float), 0);
                }
            }

            if (GetAsyncKeyState(VK_LEFT)) // LEFT ARROW
            {
                leftNrightValue += rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(leftNrightAdderss), &leftNrightValue, sizeof(float), 0);
            }

            if (GetAsyncKeyState(VK_RIGHT)) // RIGHT ARROW
            {
                leftNrightValue -= rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(leftNrightAdderss), &leftNrightValue, sizeof(float), 0);
            }

            if (GetAsyncKeyState(VK_UP)) // UP ARROW
            {
                if (upNdownValue < 88)
                {
                    upNdownValue += rotationSpeed;
                    WriteProcessMemory(processHandle, (LPVOID)(upNdownAddress), &upNdownValue, sizeof(float), 0);
                }
            }

            if (GetAsyncKeyState(VK_DOWN)) // DOWN ARROW
            {
                if (upNdownValue > 17.5)
                {
                    upNdownValue -= rotationSpeed;
                    WriteProcessMemory(processHandle, (LPVOID)(upNdownAddress), &upNdownValue, sizeof(float), 0);
                }
            }
        }
        else
        {
            Sleep(500);
        }

    }
}

void findGameWindowToHook()
{
    if (hGameWindow != NULL)
    {
        std::cout << "League of Legends found successfully!" << std::endl;
        GetWindowThreadProcessId(hGameWindow, &pID);
        processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
        if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
        {
            std::cout << "Try to run the application as administrator." << std::endl;
            Sleep(3000);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        std::cout << "Unable to find League of Legends, Please make sure that you are in a game!" << std::endl;
        Sleep(3000);
        exit(EXIT_FAILURE);
    }
}

uintptr_t iniPRT(uintptr_t offsetGameToBaseAdress, std::vector<uintptr_t> AddrOffsets)
{
    uintptr_t baseAddress = NULL;
    uintptr_t PointerBaseAddress = GetThreadstackStartAddress(0, pID, processHandle);
    ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
    uintptr_t Address = baseAddress;
    for (int i = 0; i < AddrOffsets.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(Address + AddrOffsets.at(i)), &Address, sizeof(Address), NULL);
    }
    Address += AddrOffsets.at(AddrOffsets.size() - 1);
    return Address;
}

int main()
{
    SetConsoleTitleA(titleGen(rand() % 300 + 300).c_str());

    findGameWindowToHook();

    camZAddress = iniPRT(-0x00000298, { 0x8, 0x18, 0x1A0, 0x30, 0x0, 0x8, 0x2B0 });
    leftNrightAdderss = camZAddress - 264;
    upNdownAddress = camZAddress - 268;
    ReadProcessMemory(processHandle, (LPCVOID)(camZAddress), &zoomValue, sizeof(float), NULL);
    ReadProcessMemory(processHandle, (LPCVOID)(leftNrightAdderss), &leftNrightValue, sizeof(float), NULL);
    ReadProcessMemory(processHandle, (LPCVOID)(upNdownAddress), &upNdownValue, sizeof(float), NULL);

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)setupHook, 0, 0, 0);//mouse scroll thread

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)mouseLR, 0, 0, 0);//mouseLR thread

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)keyboard, 0, 0, 0);//keyboar thread

    while(true)
    {
        Sleep(1000);
        if (FindWindow(NULL, WindowName) == NULL)
        {
            CloseHandle(processHandle);
            return 0;
        }
    }
}
