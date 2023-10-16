#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <sstream>
#include <fstream>
#include "ntinfo.h"
#include <winbase.h>
#include <string.h>
#include <process.h>
#include <thread>
#include <chrono>
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
const char WindowName[30] = "League of Legends (TM) Client";
BOOL WindowinFocus = false;
HHOOK hook = NULL;
uintptr_t camZAddress = NULL;
uintptr_t leftNrightAdderss = NULL;
uintptr_t upNdownAddress = NULL;
DWORD pID = NULL;
HANDLE processHandle = NULL;
//get screen res
int x = GetSystemMetrics(SM_CXSCREEN);
int y = GetSystemMetrics(SM_CYSCREEN);
//file related
std::ifstream offsetsFile("offsets.txt");
std::vector<uintptr_t> offsets;

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
    if (nCode != HC_ACTION) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }   
    MSLLHOOKSTRUCT* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
    if (GetForegroundWindow() != FindWindow(NULL, WindowName)){
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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (GetForegroundWindow() == FindWindow(NULL, WindowName))
        {
            if (GetAsyncKeyState(VK_MBUTTON)) //lock mouse
            {
                SetCursorPos(x / 2, y / 2);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void mouseLR()
{
    std::thread(mouseLock).detach();
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
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
  
    }
}

void keyboard()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

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
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

    }
}

uintptr_t initPRT(uintptr_t offsetGameToBaseAdress, std::vector<uintptr_t> AddrOffsets)
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

void readFile()
{
    if (offsetsFile.is_open())
    {
        std::string line;
        while (std::getline(offsetsFile, line))
        {
            std::istringstream iss(line);
            uintptr_t offset;
            while (iss >> std::hex >> offset)
            {
                offsets.push_back(offset);
            }
        }
        offsetsFile.close();
    }
    else
    {
        std::cout << "Unable to open offsets.txt" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        exit(EXIT_FAILURE);
    }

    if (offsets.size() < 1)
    {
        std::cout << "Offsets file is empty or missing the base address offset." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        exit(EXIT_FAILURE);
    }
}

void findGame()
{
    readFile();
    uintptr_t offsetGameToBaseAddress = offsets[0];
    offsets.erase(offsets.begin());//remove the base address offset from the vector

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (FindWindow(NULL, WindowName) != NULL)
        {
            system("cls");
            std::cout << "Found!" << std::endl;
            GetWindowThreadProcessId(FindWindow(NULL, WindowName), &pID);
            processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
            if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
            {
                std::cout << "Try to run the application as administrator." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                exit(EXIT_FAILURE);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));

            camZAddress = initPRT(offsetGameToBaseAddress, offsets);
            leftNrightAdderss = camZAddress - 264;
            upNdownAddress = camZAddress - 268;
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress), &zoomValue, sizeof(float), NULL);
            ReadProcessMemory(processHandle, (LPCVOID)(leftNrightAdderss), &leftNrightValue, sizeof(float), NULL);
            ReadProcessMemory(processHandle, (LPCVOID)(upNdownAddress), &upNdownValue, sizeof(float), NULL);

            while (true)
            {
                Sleep(1000);
                if (FindWindow(NULL, WindowName) == NULL)
                {
                    CloseHandle(processHandle);
                    break;
                }
            }
        }
        else
        {
            system("cls");
            std::cout << "Looking for League of Legends!" << std::endl;
        }
    }
}

int main()
{
    SetConsoleTitleA(titleGen(rand() % 300 + 300).c_str());

    std::thread(setupHook).detach();
    std::thread(mouseLR).detach();
    std::thread(keyboard).detach();

    findGame();
}
