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

#include <algorithm>
#include <iostream>
#include <future>
#include <thread>
#include <vector>

#include <stdlib.h>
#include <tchar.h>
#include <windows.h>

// I have no clue why, but this include needs to be the last one,
// or else the code won't compile!
#include <psapi.h>

const int redZone = 80;  // darkOrangeStop
const int yellowStop = 60;
const int lightOrangeStop = 70;

//* CHANGE THE FOLLOWING VARIABLES IN ORDER TO MAKE IT WORK FOR YOU
const char *windowName = "Rocket League (64-bit, DX11, Cooked)";
INT_PTR baseAddress = 0x2368DD0;
INT_PTR offsets[] = {0x70, 0x2F0, 0x88, 0xC0, 0x08, 0x38C};

double keyboardWidth;

const char *toString(CorsairError error) {
    switch (error) {
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

double getKeyboardWidth(CorsairLedPositions *ledPositions) {
    const auto minmaxLeds = std::minmax_element(
        ledPositions->pLedPosition,
        ledPositions->pLedPosition + ledPositions->numberOfLed,
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

DWORD getProcessId() {
    HWND hWnd = FindWindowA(NULL, windowName);

    if (hWnd == NULL) return NULL;

    DWORD processID;
    GetWindowThreadProcessId(hWnd, &processID);

    return processID;
}

HANDLE initProcessHandle() {
    DWORD processID = getProcessId();
    return OpenProcess(PROCESS_VM_READ, true, processID);
}

DWORD_PTR GetProcessBaseAddress() {
    DWORD processID = getProcessId();
    DWORD_PTR baseAddress = NULL;
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    LPBYTE moduleArrayBytes;
    DWORD bytesRequired;

    if (processHandle &&
        EnumProcessModules(processHandle, NULL, 0, &bytesRequired) &&
        bytesRequired) {
        moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

        if (moduleArrayBytes) {
            HMODULE *moduleArray = (HMODULE *)moduleArrayBytes;

            if (EnumProcessModules(processHandle, moduleArray, bytesRequired,
                                   &bytesRequired))
                baseAddress = (DWORD_PTR)moduleArray[0];

            LocalFree(moduleArrayBytes);
        }

        CloseHandle(processHandle);
    }

    return baseAddress;
}

HANDLE getRocketLeagueHandle() {
    HANDLE handle = NULL;

    while (handle == NULL && !GetAsyncKeyState(VK_ESCAPE)) {
        handle = initProcessHandle();
        if (handle == NULL) {
            std::cout
                << "Can't find Rocket League. Please start the gaaaaame!\n";
        } else {
            std::cout << "Found Rocket League!\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return handle;
}

int getRocketLeagueTurbo(HANDLE handle, INT_PTR p) {
    int value;
    ReadProcessMemory(handle, (LPVOID)p, &value, 4, NULL);

    return value;
}

INT_PTR getRocketLeagueTurboPointer(HANDLE handle) {
    INT_PTR addr = (INT_PTR)(GetProcessBaseAddress());
    addr += baseAddress;

    std::cout << "BaseAddress: " << std::hex << addr << std::endl;

    for (INT_PTR offset : offsets) {
        // std::cout << "before addr: " << addr << std::endl;
        ReadProcessMemory(handle, (LPCVOID)addr, &addr, sizeof(addr), 0);
        // std::cout << "after  addr: " << addr << std::endl;
        addr += offset;
    }

    std::cout << "addr: " << addr << std::endl;
    return addr;
}

void setLeds(CorsairLedPositions *ledPositions, double percentage) {
    const double boostWidth = keyboardWidth * percentage / 100;
    std::vector<CorsairLedColor> vec;

    for (int i = 0, n = ledPositions->numberOfLed; i < n; i++) {
        const auto ledPos = ledPositions->pLedPosition[i];
        CorsairLedColor ledColor = CorsairLedColor();
        ledColor.ledId = ledPos.ledId;

        if (ledPos.left < boostWidth) {
            double posPct = ledPos.left / keyboardWidth * 100;

            ledColor.r = 255;
            ledColor.g = 145;

            if (posPct >= yellowStop) ledColor.g = 87;
            if (posPct >= lightOrangeStop) ledColor.g = 37;
            if (posPct >= redZone) ledColor.g = 0;
        }
        vec.push_back(ledColor);
    }
    CorsairSetLedsColors(static_cast<int>(vec.size()), vec.data());
}

void loop(CorsairLedPositions *ledPositions, HANDLE handle) {
    UINT_PTR pointer = getRocketLeagueTurboPointer(handle);
    std::cout << "pointer: " << pointer << std::endl;

    while (true) {
        if (getProcessId() == NULL) return;

        // XXX: Get this value repeatably. I suck at it.
        // For demos, I used cheat engine to grab the memory locations.
        // auto percentage = getRocketLeagueTurbo(handle, 0X1C1913EFC8C);
        pointer = getRocketLeagueTurboPointer(handle);
        auto percentage = getRocketLeagueTurbo(handle, pointer);

        std::cout << percentage << std::endl;

        setLeds(ledPositions, percentage);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main() {
    CorsairPerformProtocolHandshake();
    if (const auto error = CorsairGetLastError()) {
        std::cout << "Handshake failed: " << toString(error) << std::endl;
        (void)getchar();
        return -1;
    }

    CorsairLedPositions *ledPositions = CorsairGetLedPositions();
    if (ledPositions && ledPositions->numberOfLed <= 0) {
        std::cout << "No Corsair Leds detected! Exiting...\n";
        return -1;
    }
    keyboardWidth = getKeyboardWidth(ledPositions);

    std::cout << "Working... Press Ctrl+C to close program...\n\n";

    HANDLE handle = getRocketLeagueHandle();
    loop(ledPositions, handle);

    return 0;
}
