//
// forked from kayteh/RocketLeagueCUE
// which is forked from CUE SDK Example progress.cpp
//
#define CORSAIR_LIGHTING_SDK_DISABLE_DEPRECATION_WARNINGS

#ifdef __APPLE__
#include <CUESDK/CUESDK.h>
#else
#include <CUESDK.h>
#endif

#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <vector>
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <psapi.h>

const int redZone = 80; // darkOrangeStop
const int yellowStop = 60;
const int lightOrangeStop = 70;
const char *windowName = "Rocket League (64-bit, DX11, Cooked)";

const char *toString(CorsairError error)
{
    switch (error)
    {
    case CE_Success:
        return "CE_Success";
    case CE_ServerNotFound:
        return "CE_ServerNotFound";
    case CE_NoControl:
        return "CE_NoControl";
    case CE_ProtocolHandshakeMissing:
        return "CE_ProtocolHandshakeMissing";
    case CE_IncompatibleProtocol:
        return "CE_IncompatibleProtocol";
    case CE_InvalidArguments:
        return "CE_InvalidArguments";
    default:
        return "unknown error";
    }
}

double getKeyboardWidth(CorsairLedPositions *ledPositions)
{
    const auto minmaxLeds = std::minmax_element(
        ledPositions->pLedPosition, ledPositions->pLedPosition + ledPositions->numberOfLed,
        [](const CorsairLedPosition &clp1, const CorsairLedPosition &clp2) {
            return clp1.left < clp2.left;
        });

    // std::cout << minmaxLeds.second->left << " + "
    //           << minmaxLeds.second->width << " - "
    //           << minmaxLeds.first->left << "\n";
    // For a corsair K68, the values are:
    // 429 + 13 - 21
    return minmaxLeds.second->left + minmaxLeds.second->width;
    // - minmaxLeds.first->left;
}

DWORD getProcessId()
{
    HWND hWnd = FindWindowA(NULL, windowName);

    if (hWnd == NULL)
        return NULL;

    DWORD processID;
    GetWindowThreadProcessId(hWnd, &processID);

    return processID;
}

HANDLE initProcessHandle()
{
    DWORD processID = getProcessId();
    return OpenProcess(PROCESS_VM_READ, true, processID);
}

DWORD_PTR GetProcessBaseAddress()
{
    DWORD processID = getProcessId();
    DWORD_PTR baseAddress;
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    LPBYTE moduleArrayBytes;
    DWORD bytesRequired;

    if (processHandle && EnumProcessModules(processHandle, NULL, 0, &bytesRequired) && bytesRequired)
    {
        moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

        if (moduleArrayBytes)
        {
            HMODULE *moduleArray = (HMODULE *)moduleArrayBytes;

            if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
                baseAddress = (DWORD_PTR)moduleArray[0];

            LocalFree(moduleArrayBytes);
        }

        CloseHandle(processHandle);
    }

    return baseAddress;
}

HANDLE getRocketLeagueHandle()
{
    HANDLE handle = NULL;

    while (handle == NULL && !GetAsyncKeyState(VK_ESCAPE))
    {
        handle = initProcessHandle();
        if (handle == NULL)
        {
            std::cout << "Can't find Rocket League. Please start the gaaaaame!\n";
        }
        else
        {
            std::cout << "Found Rocket League!\n";
            return handle;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int getRocketLeagueTurbo(HANDLE handle, INT_PTR p)
{
    int value;
    ReadProcessMemory(handle, (LPVOID)p, &value, 4, NULL);

    return value;
}

UINT_PTR getRocketLeagueTurboPointer(HANDLE handle)
{
    INT_PTR base = (INT_PTR)(GetProcessBaseAddress());
    std::cout << "ProcessBaseAddress: " << std::hex << base << std::endl
              << "handle: " << std::hex << handle << std::endl;
    base += 0x02350D30;

    int addr;
    ReadProcessMemory(handle, (LPVOID)base, &addr, 4, NULL);
    std::cout << "addr: " << addr << std::endl;

    // int offsets[6] = {0x4A0, 0x54, 0x4C, 0x520, 0x54};
    // int offsets[] = {0x38C};

    // for (int offset : offsets)
    // {
    //     int b;
    //     ReadProcessMemory(handle, (LPVOID)(addr + offset), &b, 4, 0);
    //     addr += b;
    //     std::cout << std::hex << b
    //               << "+" << std::hex << offset
    //               << " so addr is now " << std::hex << addr << std::endl;
    // }

    return addr + 0x38C;
    // return addr;
}

void setLeds(CorsairLedPositions *ledPositions, double percentage)
{
    // Should store keyboardWidth instead of calculating it each time
    const double keyboardWidth = getKeyboardWidth(ledPositions);

    const double boostWidth = keyboardWidth * percentage / 100;
    std::vector<CorsairLedColor> vec;

    for (int i = 0, n = ledPositions->numberOfLed; i < n; i++)
    {
        const auto ledPos = ledPositions->pLedPosition[i];
        CorsairLedColor ledColor = CorsairLedColor();
        ledColor.ledId = ledPos.ledId;

        if (ledPos.left < boostWidth)
        {
            double posPct = ledPos.left / keyboardWidth * 100;

            ledColor.r = 255;
            ledColor.g = 145;

            if (posPct >= yellowStop)
                ledColor.g = 87;
            if (posPct >= lightOrangeStop)
                ledColor.g = 37;
            if (posPct >= redZone)
                ledColor.g = 0;
        }
        vec.push_back(ledColor);
    }
    CorsairSetLedsColors(static_cast<int>(vec.size()), vec.data());
}

void loop(CorsairLedPositions *ledPositions, HANDLE handle, UINT_PTR pointer)
{
    std::cout << "pointer: " << pointer << std::endl;
    // while (!GetAsyncKeyState(VK_ESCAPE))
    while (true)
    {
        // XXX: Get this value repeatably. I suck at it.
        // For demos, I used cheat engine to grab the memory locations.
        // auto percentage = getRocketLeagueTurbo(handle, 0x1C1F428198C);
        auto percentage = getRocketLeagueTurbo(handle, pointer);

        setLeds(ledPositions, percentage);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main()
{
    CorsairPerformProtocolHandshake();
    if (const auto error = CorsairGetLastError())
    {
        std::cout << "Handshake failed: " << toString(error) << std::endl;
        getchar();
        return -1;
    }

    // const auto ledPositions = CorsairGetLedPositions();
    CorsairLedPositions *ledPositions = CorsairGetLedPositions();
    if (ledPositions && ledPositions->numberOfLed <= 0)
    {
        std::cout << "No Corsair Leds detected! Exiting...\n";
        return -1;
    }

    std::cout << "Working... Press Escape to close program...\n\n";

    HANDLE handle = getRocketLeagueHandle();
    UINT_PTR pointer = getRocketLeagueTurboPointer(handle);
    loop(ledPositions, handle, pointer);

    return 0;
}
