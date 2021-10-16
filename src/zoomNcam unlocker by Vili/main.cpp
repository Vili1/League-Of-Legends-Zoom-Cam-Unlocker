#include <Windows.h> //HWND, DWORD etc.
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream> // cout
#include <vector> //vector ...
#include <ctime>
#include <string>
#include <sstream>
#include "ntinfo.h"

//global vars
float zoomValue = 1.281169772;
float leftNright = 180;
float upNdown = 56;
float zoomValueReset = 1.281169772;
float leftNrightReset = 180;
float upNdownReset = 56;
float rotationSpeed = 5.5;
float zoomSpeed = 0.05;

auto titleGen = [](int num)
{
    std::string titleName;
    for (int i = 0; i < num; i++)
    {
        titleName += rand() % 100 + 30;
    }
    return titleName;
};

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
                    //printf("PID: %04d Thread ID: 0x%04x\n", te.th32OwnerProcessID, te.th32ThreadID);
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
    DWORD used = 0, ret = 0;
    DWORD stacktop = 0, result = 0;
    MODULEINFO mi;
    GetModuleInformation(processHandle, GetModuleHandle("kernel32.dll"), &mi, sizeof(mi));
    stacktop = (DWORD)GetThreadStackTopAddress_x86(processHandle, hThread);
    CloseHandle(hThread);
    if (stacktop)
    {
        //find the stack entry pointing to the function that calls "ExitXXXXXThread"
        //Fun thing to note: It's the first entry that points to a address in kernel32
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
        delete buf32;
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
        //printf("TID: 0x%04x = THREADSTACK%2d BASE ADDRESS: 0x%04x\n", *it, stackNum, threadStartAddress);
        if (stackNum == stackNumber) return threadStartAddress;
        stackNum++;
    }
}

/*
void reloadFunk()
{
    std::cout << "---------------------------------------------------------------------------\n";
    for (int i = 5; i > 0; i--)
    {
        if (i == 1)
        {
            std::cout << "Auto reloading in " << i << " second\n";
            Sleep(1000);
            system("CLS");
        }
        else
        {
            std::cout << "Auto reloading in " << i << " seconds\n";
            Sleep(1000);
        }
    }
}
*/

//zoom stuff 
DWORD pID = NULL;
HANDLE processHandle = NULL;
DWORD PointerBaseAddress = GetThreadstackStartAddress(0, pID, processHandle);
DWORD offsetGameToBaseAdress = -0x000000D8;
std::vector<DWORD> pointsOffsets{ 0x0, 0x8, 0x10, 0x9F4, 0x20, 0x0, 0x4, 0x260 };
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
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
            //std::cout << "debugginfo: baseaddress = " << std::hex << baseAddress << std::endl;
            DWORD pointsAddress = baseAddress; //the Adress we need -> change now while going through offsets
            for (int i = 0; i < pointsOffsets.size() - 1; i++) // -1 because we dont want the value at the last offset
            {
                ReadProcessMemory(processHandle, (LPVOID)(pointsAddress + pointsOffsets.at(i)), &pointsAddress, sizeof(pointsAddress), NULL);
                //std::cout << "debugginfo: Value at offset = " << std::hex << pointsAddress << std::endl;
            }
            pointsAddress += pointsOffsets.at(pointsOffsets.size() - 1); //Add Last offset -> done!!

            //std::cout << "scroll up\n";
            
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress), &zoomValue, sizeof(float), NULL);
            /*
            zoomValue -= zoomSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            */
            if (zoomValue > 0.78)
            {
                zoomValue -= zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            }
        }
        else
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
            //std::cout << "debugginfo: baseaddress = " << std::hex << baseAddress << std::endl;
            DWORD pointsAddress = baseAddress; //the Adress we need -> change now while going through offsets
            for (int i = 0; i < pointsOffsets.size() - 1; i++) // -1 because we dont want the value at the last offset
            {
                ReadProcessMemory(processHandle, (LPVOID)(pointsAddress + pointsOffsets.at(i)), &pointsAddress, sizeof(pointsAddress), NULL);
                //std::cout << "debugginfo: Value at offset = " << std::hex << pointsAddress << std::endl;
            }
            pointsAddress += pointsOffsets.at(pointsOffsets.size() - 1); //Add Last offset -> done!!

            //std::cout << "scroll down\n";
            
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress), &zoomValue, sizeof(float), NULL);
            /*
            zoomValue += zoomSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            */
            if (zoomValue < 2.7)
            {
                zoomValue += zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            }
        }

    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
    if (hook) {
        UnhookWindowsHookEx(hook);
        hook = NULL;
    }

    return TRUE;
}

void ui()
{
    std::cout << "Up arrow rotates the camera UP" << std::endl;
    std::cout << "Down arrow rotates the camera DOWN" << std::endl;
    std::cout << "Left arrow rotates the camera LEFT" << std::endl;
    std::cout << "Right arrow rotates the camera RIGHT" << std::endl;
    std::cout << "Numpad + zoom in" << std::endl;
    std::cout << "Numpad - zoom out" << std::endl;
    std::cout << "Numpad 0 Reset camera position" << std::endl;
    std::cout << "Numpad 1 Restore camera position" << std::endl;
    std::cout << "Numpad 2 set rotation speed, the default rotation speed is 5.5" << std::endl;
    std::cout << "Numpad 3 set zoom speed, the default zoom speed is 0.05" << std::endl;
    std::cout << "Numpad 4 shows the console" << std::endl;
    std::cout << "Numpad 5 hides the console" << std::endl;
    std::cout << "Numpad 6 kills the unlocker" << std::endl;
    std::cout << "---------------------------------------------------------------------------" << std::endl;
    std::cout << "If the unlocker doesn't work press Delete to fully reload it!" << std::endl;
    std::cout << "---------------------------------------------------------------------------" << std::endl;
    std::cout << "Your current rotation speed is: " << std::hex << rotationSpeed << std::endl;
    std::cout << "Your current zoom speed is: " << std::hex << zoomSpeed << std::endl;
    std::cout << "---------------------------------------------------------------------------" << std::endl;
}


int main()
{
    reload:
    system("CLS");
    SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());

    HWND hGameWindow = FindWindow(NULL, "League of Legends (TM) Client");
    if (hGameWindow != NULL)
    {
        std::cout << "League of Legends found successfully!" << std::endl;
        std::cout << "---------------------------------------------------------------------------" << std::endl;
    }
    else
    {
        std::cout << "Unable to find League of Legends, Please open League of Legends!" << std::endl;
        Sleep(5000);
        //reloadFunk();
        //goto reload;
        return 0;
    }
    //DWORD pID = NULL; // ID of our Game
    GetWindowThreadProcessId(hGameWindow, &pID);
    //HANDLE processHandle = NULL;
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL)
    {
        std::cout << "Try to run the application as administrator.\n";
        std::cout << "---------------------------------------------------------------------------\n";
        Sleep(5000);
        //system("pause");
        return 0;
    }

    DWORD PointerBaseAddress = GetThreadstackStartAddress(0, pID, processHandle);
    //DWORD offsetGameToBaseAdress = -0x000000D8;
    //std::vector<DWORD> pointsOffsets{ 0x0, 0x8, 0x10, 0x9F4, 0x20, 0x0, 0x4, 0x260 };
    //DWORD baseAddress = NULL;
    //Get value at gamebase+offset -> store it in baseAddress
    ReadProcessMemory(processHandle, (LPVOID)(PointerBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);
    //std::cout << "debugginfo: baseaddress = " << std::hex << baseAddress << std::endl;
    DWORD pointsAddress = baseAddress; //the Adress we need -> change now while going through offsets
    for (int i = 0; i < pointsOffsets.size() - 1; i++) // -1 because we dont want the value at the last offset
    {
        ReadProcessMemory(processHandle, (LPVOID)(pointsAddress + pointsOffsets.at(i)), &pointsAddress, sizeof(pointsAddress), NULL);
        //std::cout << "debugginfo: Value at offset = " << std::hex << pointsAddress << std::endl;
    }
    pointsAddress += pointsOffsets.at(pointsOffsets.size() - 1); //Add Last offset -> done!!

    //left and right offset
    std::vector<DWORD> pointsOffsets2{ 0x0, 0x8, 0x10, 0x9F4, 0x20, 0x0, 0x4, 0x17C };

    DWORD pointsAddress2 = baseAddress;

    for (int i = 0; i < pointsOffsets2.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(pointsAddress2 + pointsOffsets2.at(i)), &pointsAddress2, sizeof(pointsAddress2), NULL);
    }
    pointsAddress2 += pointsOffsets2.at(pointsOffsets2.size() - 1);

    //up and down offset
    std::vector<DWORD> pointsOffsets3{ 0x0, 0x8, 0x10, 0x9F4, 0x20, 0x0, 0x4, 0x174 };

    DWORD pointsAddress3 = baseAddress;

    for (int i = 0; i < pointsOffsets3.size() - 1; i++)
    {
        ReadProcessMemory(processHandle, (LPVOID)(pointsAddress3 + pointsOffsets3.at(i)), &pointsAddress3, sizeof(pointsAddress3), NULL);
    }
    pointsAddress3 += pointsOffsets3.at(pointsOffsets3.size() - 1);

    ui(); //call the UI

    //LRESULT CALLBACK WindowProc(In HWND   hwnd), In UINT   uMsg, In WPARAM wParam, In LPARAM lParam);
    
    HWND window;
    AllocConsole();
    window = FindWindowA("ConsoleWindowClass", NULL);
    ShowWindow(window, 0);
    //mouse
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
    hook = SetWindowsHookExW(WH_MOUSE_LL, MouseHook, nullptr, 0);

    if (!hook)
    {
        std::cerr << "SetWindowsHookExW() failed\n";
        return EXIT_FAILURE;
    }

    //std::cout << "Hooked: " << hook << '\n';


    while (true)
    {
        Sleep(5);
        if (GetAsyncKeyState(VK_DELETE))
        {
            goto reload;
        }

        if (GetAsyncKeyState(VK_NUMPAD0)) //reset
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            //WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValueReset, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress2), &leftNrightReset, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress3), &upNdownReset, sizeof(float), 0);
            system("CLS");
            ui();
            std::cout << "Your camera position has been reset!" << std::endl;
        }

        if (GetAsyncKeyState(VK_NUMPAD1)) //restore
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            //WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress2), &leftNright, sizeof(float), 0);
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress3), &upNdown, sizeof(float), 0);
            system("CLS");
            ui();
            std::cout << "Your camera position has been restoerd!" << std::endl;
        }

        if (GetAsyncKeyState(VK_NUMPAD2))
        {
            ShowWindow(window, 1);
            std::cout << "Set rotation speed:" << std::endl;
            std::cin >> rotationSpeed;
            ShowWindow(window, 0);
            system("CLS");
            ui();
            
        }

        if (GetAsyncKeyState(VK_NUMPAD3))
        {
            ShowWindow(window, 1);
            std::cout << "Set zoom speed:" << std::endl;
            std::cin >> zoomSpeed;
            ShowWindow(window, 0);
            system("CLS");
            ui();
            
        }

        if (GetAsyncKeyState(VK_NUMPAD5))
        {
            ShowWindow(window, 0);
        }

        if (GetAsyncKeyState(VK_NUMPAD4))
        {
            ShowWindow(window, 1);
        }

        if (GetAsyncKeyState(VK_NUMPAD6))
        {
            system("CLS");
            ShowWindow(window, 1);
            std::cout << "Goodbye! :)" << std::endl;
            Sleep(5000);
            return 0;
        }

        if (GetAsyncKeyState(VK_ADD)) //numpad +
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress), &zoomValue, sizeof(float), NULL);
            /*
            zoomValue -= zoomSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            */
            if (zoomValue > 0.78)
            {
                zoomValue -= zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            }
            
        }

        if (GetAsyncKeyState(VK_SUBTRACT)) // numpad -
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress), &zoomValue, sizeof(float), NULL);
            /*
            zoomValue += zoomSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            */
            if(zoomValue < 2.7)
            {
                zoomValue += zoomSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &zoomValue, sizeof(float), 0);
            } 
        }

        if (GetAsyncKeyState(VK_LEFT)) // LEFT ARROW
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress2), &leftNright, sizeof(float), NULL);
            leftNright += rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress2), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_RIGHT)) // RIGHT ARROW
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress2), &leftNright, sizeof(float), NULL);
            leftNright -= rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress2), &leftNright, sizeof(float), 0);
        }

        if (GetAsyncKeyState(VK_UP)) // UP ARROW
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress3), &upNdown, sizeof(float), NULL);
            /*
            upNdown += rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress3), &upNdown, sizeof(float), 0);
            */
            if (upNdown < 160)
            {
                upNdown += rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(pointsAddress3), &upNdown, sizeof(float), 0);
            }
        }

        if (GetAsyncKeyState(VK_DOWN)) // DOWN ARROW
        {
            SetConsoleTitleA(titleGen(rand() % 100 + 30).c_str());
            ReadProcessMemory(processHandle, (LPCVOID)(pointsAddress3), &upNdown, sizeof(float), NULL);
            /*
            upNdown -= rotationSpeed;
            WriteProcessMemory(processHandle, (LPVOID)(pointsAddress3), &upNdown, sizeof(float), 0);
            */
            if (upNdown > 20)
            {
                upNdown -= rotationSpeed;
                WriteProcessMemory(processHandle, (LPVOID)(pointsAddress3), &upNdown, sizeof(float), 0);
            }
        }

        HWND hGameWindow = FindWindow(NULL, "League of Legends (TM) Client");
        if (hGameWindow == NULL)
        {
            return 0;
        }

        PeekMessage(nullptr, nullptr, 0, 0, PM_REMOVE);
    }
}