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
#include <tchar.h> 

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
HWND window;

auto titleGen = [](int num)
{
    std::string titleName;
    for (int i = 0; i < num; i++)
    {
        titleName += rand() % 100 + 30;
    }
    return titleName;
};

void polymorphic()
{
    std::srand(std::time(0));
    for (int count = 0; count < 10; count++)
    {
        int index = rand() % (6 - 0 + 1) + 0;
        switch (index)
        {
            case 0:
                __asm __volatile
                {
                    sub eax, 3
                    add eax, 1
                    add eax, 2
                }
            case 1:
                __asm __volatile
                {
                    push eax
                    pop eax
                }
            case 2:
                __asm __volatile
                {
                    inc eax
                    dec eax
                }
            case 3:
                __asm __volatile
                {
                    dec eax
                    add eax, 1
                }
            case 4:
                __asm __volatile
                {
                    pop eax
                    push eax
                }
            case 5:
                __asm __volatile
                {
                    mov eax, eax
                    sub eax, 1
                    add eax, 1
                }
            case 6:
                __asm __volatile
                {
                    xor eax, eax
                    mov eax, eax
                }
        }
    }
}

DWORD dwGetModuleBaseAddress(TCHAR* lpszModuleName, DWORD pID)
{
    DWORD dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
    MODULEENTRY32 ModuleEntry32 = { 0 };
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &ModuleEntry32))
    {
        do
        {
            if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0)
            {
                dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32));

    }
    CloseHandle(hSnapshot);
    return dwModuleBaseAddress;
}


std::vector<DWORD> threadList(DWORD pid)
{
    std::vector<DWORD> vect = std::vector<DWORD>();
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

DWORD GetThreadStartAddress(HANDLE processHandle, HANDLE hThread)
{
    DWORD stacktop = 0, result = 0;
    MODULEINFO mi;
    GetModuleInformation(processHandle, GetModuleHandle("kernel32.dll"), &mi, sizeof(mi));
    stacktop = (DWORD)GetThreadStackTopAddress_x86(processHandle, hThread);
    CloseHandle(hThread);
    if (stacktop)
    {
        DWORD* buf32 = new DWORD[4096];
        if (ReadProcessMemory(processHandle, (LPCVOID)(stacktop - 4096), buf32, 4096, NULL))
        {
            for (int i = 4096 / 4 - 1; i >= 0; --i)
            {
                if (buf32[i] >= (DWORD)mi.lpBaseOfDll && buf32[i] <= (DWORD)mi.lpBaseOfDll + mi.SizeOfImage)
                {
                    result = stacktop - 4096 + i * 4;
                    break;
                }
            }
        }
        delete[] buf32;
    }
    return result;
}

DWORD GetThreadstackStartAddress(int stackNumber, DWORD pID, HANDLE processHandle)
{
    std::vector<DWORD> threadId = threadList(pID);
    int stackNum = 0;
    for (auto it = threadId.begin(); it != threadId.end(); ++it)
    {
        HANDLE threadHandle = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, *it);
        DWORD threadStartAddress = GetThreadStartAddress(processHandle, threadHandle);
        if (stackNum == stackNumber) return threadStartAddress;
        stackNum++;
    }
    return 0;
}

// zoom stuff for the scroll
DWORD pID = NULL;
HANDLE processHandle = NULL;
DWORD PointerBaseAddress = GetThreadstackStartAddress(0, pID, processHandle);
DWORD offsetGameToBaseAdress = -0x000000D8;
std::array<DWORD, 8> camZOffsets{ 0x0, 0x8, 0x10, 0xDC0, 0x20, 0x0, 0x4, 0x260 };
DWORD baseAddress = NULL;
HHOOK hook = NULL;

LRESULT CALLBACK MouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION)
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    MSLLHOOKSTRUCT* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

    if (wParam == WM_MOUSEWHEEL)
    {
        if (info->mouseData == 0x780000)
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
            DWORD camZAddress = baseAddress;
            for (int i = 0; i < camZOffsets.size() - 1; i++)
            {
                ReadProcessMemory(processHandle, (LPVOID)(camZAddress + camZOffsets.at(i)), &camZAddress, sizeof(camZAddress), NULL);
            }
            camZAddress += camZOffsets.at(camZOffsets.size() - 1);
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress), &zoomValue, sizeof(float), NULL);
            if (zoomValue > 0.78)
            {
                zoomValue -= zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress), &zoomValue, sizeof(float), 0);
            }
        }
        else
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
            DWORD camZAddress = baseAddress;
            for (int i = 0; i < camZOffsets.size() - 1; i++)
            {
                ReadProcessMemory(processHandle, (LPVOID)(camZAddress + camZOffsets.at(i)), &camZAddress, sizeof(camZAddress), NULL);
            }
            camZAddress += camZOffsets.at(camZOffsets.size() - 1);
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress), &zoomValue, sizeof(float), NULL);
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

void integrityCheck()
{
    HWND clientWindow = FindWindow(NULL, "e11240f4fe281c9eee3c015550f4bb97103270f9d12a7dcdf2c740b795e2cab8");
    if (clientWindow == NULL)
    {
        MessageBox(NULL, "Buy a subscription!", "Don't be GAY!", MB_OK | MB_ICONQUESTION);
        system("start https://holyness.shop/product-list/four-columns");
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
        std::cout << "Unable to find League of Legends, Please open League of Legends!" << std::endl;
        Sleep(3000);
        exit(EXIT_FAILURE);
    }
}

void checkProcessHandle()
{
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
    {
        ShowWindow(window, 1);
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
        exit(EXIT_FAILURE);
    }
}

void cam3dPerson()
{

    char moduleName[] = "League of Legends.exe";
    DWORD gameBaseAddress = dwGetModuleBaseAddress(_T(moduleName), pID);
    DWORD offsetGameToBaseAddress = 0x0310990C;
    std::array<DWORD, 1> localPlayerDirectionOffsets{ 0x9c };
    //DWORD baseAddress;

    ReadProcessMemory(processHandle, (LPVOID)(gameBaseAddress + offsetGameToBaseAddress), &baseAddress, sizeof(baseAddress), NULL);
    std::cout << "Debugginfo: Baseaddress = " << std::hex << baseAddress << std::endl;
    DWORD localPlayerDirectionAddress = baseAddress;
    for (int i = 0; i < localPlayerDirectionOffsets.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(localPlayerDirectionAddress + localPlayerDirectionOffsets.at(i)), &localPlayerDirectionAddress, sizeof(localPlayerDirectionAddress), NULL);
        std::cout << "Debugginfo: address at offset = " << std::hex << localPlayerDirectionAddress << std::endl;
    }
    localPlayerDirectionAddress += localPlayerDirectionOffsets.at(localPlayerDirectionOffsets.size() - 1);
    std::cout << "Debugginfo: address at final offset = " << std::hex << localPlayerDirectionAddress << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;

    float localPlayerDirection = 1.57;
;

    int randomNumber = 0;
    while (true)
    {
        //Sleep(30);
        for (int index = 0; index < 5; index++) {
            //int randomNumber = 0;
            randomNumber = (rand() % 100) + 1;
            // cout << randomNumber << endl;
        }
        // a,d,w,s keys
        if (GetAsyncKeyState(0x41)) // A left
        {
    
            localPlayerDirection += randomNumber;
            WriteProcessMemory(processHandle,(LPVOID)(localPlayerDirectionAddress), &localPlayerDirection, sizeof(float), 0);
        }



    }

}

int main()
{
    SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());

    polymorphic();
    
    integrityCheck();
   
    findGameWindowToHook();

    GetWindowThreadProcessId(hGameWindow, &pID);

    checkProcessHandle();

    

    DWORD PointerBaseAddress = GetThreadstackStartAddress(0, pID, processHandle);

    ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
    DWORD camZAddress = baseAddress;
    for (int i = 0; i < camZOffsets.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(camZAddress + camZOffsets.at(i)), &camZAddress, sizeof(camZAddress), NULL);
    }
    camZAddress += camZOffsets.at(camZOffsets.size() - 1);

    //left and right offset
    std::array<DWORD,8> camXOffsets{ 0x0, 0x8, 0x10, 0xDC0, 0x20, 0x0, 0x4, 0x17C };
    DWORD camXAddress = baseAddress;
    for (int i = 0; i < camXOffsets.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(camXAddress + camXOffsets.at(i)), &camXAddress, sizeof(camXAddress), NULL);
    }
    camXAddress += camXOffsets.at(camXOffsets.size() - 1);

    //up and down offset
    std::array<DWORD,8> camYOffsets{ 0x0, 0x8, 0x10, 0xDC0, 0x20, 0x0, 0x4, 0x174 };
    DWORD camYAddress = baseAddress;

    for (int i = 0; i < camYOffsets.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(camYAddress + camYOffsets.at(i)), &camYAddress, sizeof(camYAddress), NULL);
    }
    camYAddress += camYOffsets.at(camYOffsets.size() - 1);

    //mouse
    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)setupHook, NULL, 0, NULL);
    polymorphic();

    while (true)
    {
        Sleep(10);

        if (GetAsyncKeyState(VK_NUMPAD0)) //reset
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            WriteProcessMemory(processHandle, (LPVOID)(camXAddress), &leftNrightReset, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(camYAddress), &upNdownReset, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_NUMPAD1)) //restore
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            WriteProcessMemory(processHandle, (LPVOID)(camXAddress), &leftNright, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(camYAddress), &upNdown, sizeof(float), 0);

        }

        if (GetAsyncKeyState(VK_NUMPAD6))
        {
            MessageBox(NULL,"The zoom unlocker will close when you press OK!", "You have pressed NUMPAD6 to kill the zoom unlocker!", MB_OK | MB_ICONQUESTION);
            return 0;
        }

        if (GetAsyncKeyState(VK_ADD)) //numpad +
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress), &zoomValue, sizeof(float), NULL);
            
            if (zoomValue > 0.78)
            {
                zoomValue -= zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress), &zoomValue, sizeof(float), 0);
            }
            
        }

        if (GetAsyncKeyState(VK_SUBTRACT)) // numpad -
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress), &zoomValue, sizeof(float), NULL);
           
            if(zoomValue < 2.7)
            {
                zoomValue += zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress), &zoomValue, sizeof(float), 0);
            } 
        }

        if (GetAsyncKeyState(VK_LEFT)) // LEFT ARROW
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camXAddress), &leftNright, sizeof(float), NULL);
            leftNright += rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camXAddress), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_RIGHT)) // RIGHT ARROW
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camXAddress), &leftNright, sizeof(float), NULL);
            leftNright -= rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camXAddress), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_UP)) // UP ARROW
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camYAddress), &upNdown, sizeof(float), NULL);

            if (upNdown < 89)
            {
                upNdown += rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camYAddress), &upNdown, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_DOWN)) // DOWN ARROW
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camYAddress), &upNdown, sizeof(float), NULL);

            if (upNdown > 17.5)
            {
                upNdown -= rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camYAddress), &upNdown, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_F5)) //3d person
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            WriteProcessMemory(processHandle, (LPVOID)(camXAddress), &leftNrightReset, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(camYAddress), &upNdownReset, sizeof(float), 0);
            cam3dPerson();
        }

        checkGameToExit();
    }
}
