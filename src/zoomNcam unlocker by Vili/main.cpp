#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <array>
#include <ctime>
#include <string>
#include <sstream>
#include "ntinfo.h"
#include <winbase.h>
#include <string.h>
#include <process.h>

#define SCROLLUP 1
#define SCROLLDOWN 2

//global vars
float zoomValue = 1.281169772;
float leftNright = 180;
float upNdown = 56;
const float leftNrightReset = 180;
const float upNdownReset = 56;
const float rotationSpeed = 5.5;
const float zoomSpeed = 0.05;
int scroll = 0;
HWND hGameWindow = FindWindow(NULL, "League of Legends (TM) Client");
HHOOK hook = NULL;
uintptr_t camZAddress = NULL;
DWORD pID = NULL;
HANDLE processHandle = NULL;

void killProcessByName(const char* filename)
{
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);
    while (hRes)
    {
        if (strcmp(pEntry.szExeFile, filename) == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (uintptr_t)pEntry.th32ProcessID);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
}

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

void keyboard()
{
    while (true)
    {
        Sleep(10);

        if (GetAsyncKeyState(VK_NUMPAD0)) //reset
        {
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 264), &leftNrightReset, sizeof(float), 0);
            leftNright = leftNrightReset;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 268), &upNdownReset, sizeof(float), 0);
            upNdown = upNdownReset;
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
            leftNright -= rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 264), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_RIGHT)) // RIGHT ARROW
        {
            leftNright += rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 264), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_UP)) // UP ARROW
        {
            if (upNdown < 89)
            {
                upNdown += rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 268), &upNdown, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_DOWN)) // DOWN ARROW
        {
            if (upNdown > 17.5)
            {
                upNdown -= rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 268), &upNdown, sizeof(float), 0);
            }
        }
    }
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

void integrityCheck()
{
    HWND clientWindow = FindWindow(NULL, "e11240f4fe281c9eee3c015550f4bb97103270f9d12a7dcdf2c740b795e2cab8");
    if (clientWindow == NULL)
    {
        MessageBox(NULL, "Buy a subscription!", "Don't be GAY!", MB_OK | MB_ICONQUESTION);
        system("start https://holyness.mysellix.io/");
        exit(EXIT_FAILURE);
    }
}

void findGameWindowToHook()
{
    if (hGameWindow != NULL)
    {
        std::cout << "League of Legends found successfully!" << std::endl;
    }
    else
    {
        std::cout << "Unable to find League of Legends, Please make sure that you are in a game!" << std::endl;
        Sleep(3000);
        exit(EXIT_FAILURE);
    }
}

void checkProcessHandle()
{
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
    {
        std::cout << "Try to run the application as administrator." << std::endl;
        Sleep(3000);
        exit(EXIT_FAILURE);
    }
}

void checkGameToExit()
{
    HWND hGameWindowToExit = FindWindow(NULL, "League of Legends (TM) Client");
    if (hGameWindowToExit == NULL)
    {
        CloseHandle(processHandle);
        exit(EXIT_FAILURE);
    }
}

void antiTamp()
{
    while(true)
    {
        Sleep(10);
        killProcessByName("consent.exe");
    }

}


void callTitle()
{
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

    while (true)
    {
        SetConsoleTitleA(titleGen(rand() % 300 + 300).c_str());
        Sleep(100);
    }
}

uintptr_t iniPRT()
{
    uintptr_t offsetGameToBaseAdress = -0x00000280;
    std::array<uintptr_t, 7> AddrOffsets{ 0x8, 0x18, 0x1A0, 0x30, 0x0, 0x8, 0x2B0 };
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
    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)antiTamp, NULL, 0, NULL);//anti tamp thread
    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)callTitle, NULL, 0, NULL);// call title on a thread

    integrityCheck();
   
    findGameWindowToHook();

    GetWindowThreadProcessId(hGameWindow, &pID);

    checkProcessHandle();

    camZAddress = iniPRT();

    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)setupHook, NULL, 0, NULL);//mouse thread

    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)keyboard, NULL, 0, NULL);//keyboar thread

    while(true)
    {
        Sleep(100);
        checkGameToExit();
    }
}
