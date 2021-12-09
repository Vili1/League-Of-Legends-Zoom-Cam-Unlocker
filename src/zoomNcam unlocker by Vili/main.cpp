#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <tchar.h> 
#include <array>
#include <ctime>
#include <string>
#include <sstream>


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

// zoom stuff for the scroll
DWORD pID = NULL;
HANDLE processHandle = NULL;
char moduleName[] = "League of Legends.exe";
DWORD offsetGameToBaseAddress = 0x0186CD9C;
std::array<DWORD, 2> camZOffsets{ 0xC, 0x260 };
DWORD baseAddress = NULL;
HHOOK hook = NULL;
DWORD camZAddress = baseAddress;

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
            DWORD PointerBaseAddress = dwGetModuleBaseAddress(_T(moduleName), pID);
            ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAddress), &baseAddress, sizeof(baseAddress), NULL);
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
        system("start https://sellix.io/Holyness");
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

int main()
{
    SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());

    polymorphic();
    
    integrityCheck();
   
    findGameWindowToHook();

    GetWindowThreadProcessId(hGameWindow, &pID);

    checkProcessHandle();

    //camZAddress - 236bytes = camYAddress - 8bytes = camXAddress
    DWORD PointerBaseAddress = dwGetModuleBaseAddress(_T(moduleName), pID);

    ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAddress), &baseAddress, sizeof(baseAddress), NULL);
    DWORD camZAddress = baseAddress;
    for (int i = 0; i < camZOffsets.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(camZAddress + camZOffsets.at(i)), &camZAddress, sizeof(camZAddress), NULL);
    }
    camZAddress += camZOffsets.at(camZOffsets.size() - 1);

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
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 228), &leftNrightReset, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 236), &upNdownReset, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_NUMPAD1)) //restore
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 228), &leftNright, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 236), &upNdown, sizeof(float), 0);
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
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress - 228), &leftNright, sizeof(float), NULL);
            leftNright += rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 228), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_RIGHT)) // RIGHT ARROW
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress - 228), &leftNright, sizeof(float), NULL);
            leftNright -= rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 228), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_UP)) // UP ARROW
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress- 236), &upNdown, sizeof(float), NULL);

            if (upNdown < 89)
            {
                upNdown += rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 236), &upNdown, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_DOWN)) // DOWN ARROW
        {
            polymorphic();
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(camZAddress - 236), &upNdown, sizeof(float), NULL);

            if (upNdown > 17.5)
            {
                upNdown -= rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddress - 236), &upNdown, sizeof(float), 0);
            }
        }

        checkGameToExit();
    }
}
