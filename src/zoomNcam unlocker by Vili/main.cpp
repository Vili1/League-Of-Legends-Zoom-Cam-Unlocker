#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <tchar.h> 
#include <array>
#include <thread>

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
uintptr_t camZAddressCPY;
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

uintptr_t dwGetModuleBaseAddress(TCHAR* lpszModuleName, uintptr_t pID)
{
    uintptr_t dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
    MODULEENTRY32 ModuleEntry32 = { 0 };
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &ModuleEntry32))
    {
        do
        {
            if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0)
            {
                dwModuleBaseAddress = (uintptr_t)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32));

    }
    CloseHandle(hSnapshot);
    return dwModuleBaseAddress;
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
                WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY), &zoomValue, sizeof(float), 0);
            }
        }
        else
        {
            if (zoomValue < 2.7)
            {
                zoomValue += zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY), &zoomValue, sizeof(float), 0);
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
            WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY - 264), &leftNrightReset, sizeof(float), 0);
            leftNright = leftNrightReset;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY - 268), &upNdownReset, sizeof(float), 0);
            upNdown = upNdownReset;
        }

        if (GetAsyncKeyState(VK_ADD)) //numpad +
        {
            if (zoomValue > 0.78)
            {
                zoomValue -= zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY), &zoomValue, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_SUBTRACT)) // numpad -
        {
            if (zoomValue < 2.7)
            {
                zoomValue += zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY), &zoomValue, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_LEFT)) // LEFT ARROW
        {
            leftNright -= rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY - 264), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_RIGHT)) // RIGHT ARROW
        {
            leftNright += rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY - 264), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_UP)) // UP ARROW
        {
            if (upNdown < 89)
            {
                upNdown += rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY - 268), &upNdown, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_DOWN)) // DOWN ARROW
        {
            if (upNdown > 17.5)
            {
                upNdown -= rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(camZAddressCPY - 268), &upNdown, sizeof(float), 0);
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
        system("start https://holyness.mysellix.io");
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

void iniPRT()
{
    char moduleName[] = "stub.dll";
    uintptr_t offsetGameToBaseAddress = 0x00B35950;
    std::array<uintptr_t, 8> camZOffsets{ 0x40, 0x10, 0x18, 0x1A0, 0x30, 0x0, 0x8, 0x2B0};
    uintptr_t baseAddress = NULL;
    uintptr_t gameBaseAddress = dwGetModuleBaseAddress(_T(moduleName), pID);
    ReadProcessMemory(processHandle, (LPVOID)(gameBaseAddress + offsetGameToBaseAddress), &baseAddress, sizeof(baseAddress), NULL);
    
    uintptr_t camZAddress = baseAddress;
    for (int i = 0; i < camZOffsets.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(camZAddress + camZOffsets.at(i)), &camZAddress, sizeof(camZAddress), NULL);
    }
    camZAddress += camZOffsets.at(camZOffsets.size() - 1);
    camZAddressCPY = camZAddress;

}

int main()
{
    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)antiTamp, NULL, 0, NULL);//anti tamp thread
    //CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)callPoly, NULL, 0, NULL);//call poly on a thread
    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)callTitle, NULL, 0, NULL);// call title on a thread

    integrityCheck();

    findGameWindowToHook();

    GetWindowThreadProcessId(hGameWindow, &pID);

    checkProcessHandle();

    iniPRT();

    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)setupHook, NULL, 0, NULL);//mouse thread

    CreateThread(NULL, 20, (LPTHREAD_START_ROUTINE)keyboard, NULL, 0, NULL);//keyboar thread

    while(true)
    {
        Sleep(1000);
        checkGameToExit();
    }
}
