/*


This program is a battery monitor utility that alerts the user when the battery
level reaches certain thresholds and displays the current battery percentage
as a large message on the screen. The program also attempts to add itself to
the user's startup applications to run automatically upon system boot.
The main functionalities of this program are:
Monitoring battery status and charging state using the Windows API.
Displaying a large, temporary, semi-transparent message on the screen
with the current battery percentage.
Emitting a beep sound when the battery level reaches 95% and the
device is connected to AC power.
Adding the program to the user's startup applications using the
Windows registry (if not already added).
The program uses a combination of WINAPI functions, multithreading, and
window management techniques to achieve its functionality.
Note: To compile and run this program, make sure to include the required   
Windows header files and libraries (e.g., windows.h, shlwapi.h, shlwapi.lib).

Author: Xus

*/

#include <iostream>
#include <string>
#include <windows.h>
#include <thread>
#include <chrono>
#include <shlwapi.h>



bool GetBatteryStatus(SYSTEM_POWER_STATUS& sps) {
    return GetSystemPowerStatus(&sps) != 0;
}

void Beeps(int j) {
    for (int i = 0; i < j; ++i) {
        Beep(2000, 700);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::wstring* msg = reinterpret_cast<std::wstring*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (message) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect;
        GetClientRect(hWnd, &rect);
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        HFONT hFont = CreateFont(100, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");
        SelectObject(hdc, hFont);
        if (msg) {
            DrawText(hdc, msg->c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void ShowBigMessage(const std::wstring& message) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"BigMessage";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        return;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, wc.lpszClassName, L"", WS_POPUP,
        0, 0, 1, 1, nullptr, nullptr, hInstance, nullptr);

    SetLayeredWindowAttributes(hWnd, 0, 200, LWA_ALPHA);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&message));

    HDC hdc = GetDC(hWnd);
    HFONT hFont = CreateFont(100, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");
    SelectObject(hdc, hFont);

    SIZE textSize;
    GetTextExtentPoint32(hdc, message.c_str(), message.length(), &textSize);
    ReleaseDC(hWnd, hdc);

    int windowWidth = textSize.cx;
    int windowHeight = textSize.cy;

    SetWindowPos(hWnd, HWND_TOPMOST, (screenWidth - windowWidth) / 50, (screenHeight - windowHeight) / 50, windowWidth, windowHeight, SWP_SHOWWINDOW);

    UpdateWindow(hWnd);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, hInstance); // Unregister the window class
}

bool AddToStartup() {
    HKEY hKey;
    LPCWSTR szSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    LPCWSTR szValueName = L"BtryMonitor";

    if (RegOpenKeyExW(HKEY_CURRENT_USER, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR szExistingPath[MAX_PATH];
        DWORD dwType = REG_SZ;
        DWORD dwSize = MAX_PATH * sizeof(WCHAR);

        if (RegQueryValueExW(hKey, szValueName, 0, &dwType, (LPBYTE)szExistingPath, &dwSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
    }
    RegCloseKey(hKey);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, szSubKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        WCHAR szExePath[MAX_PATH];
        GetModuleFileNameW(NULL, szExePath, MAX_PATH);
        PathQuoteSpacesW(szExePath);

        if (RegSetValueExW(hKey, szValueName, 0, REG_SZ, (LPBYTE)szExePath, (lstrlenW(szExePath) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
    }

    RegCloseKey(hKey);
    return false;
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {    
    if (!AddToStartup()) {
        MessageBox(NULL, L"Failed to add the program to startup. Please run this program as an administrator.", L"Error", MB_OK | MB_ICONERROR);
    }
    int prevBatteryPercent = 0;
    while (true) {
        SYSTEM_POWER_STATUS sps;
        if (GetBatteryStatus(sps)) {
            int currentBatteryPercent = sps.BatteryLifePercent;
            bool isConnectedToAC = sps.ACLineStatus == 1;

            while ((std::abs(currentBatteryPercent - prevBatteryPercent) < 5 &&
                currentBatteryPercent < 95 &&  currentBatteryPercent > 31) ||
                (isConnectedToAC &&  currentBatteryPercent < 32 &&
                    std::abs(currentBatteryPercent - prevBatteryPercent) < 5)) {

                std::this_thread::sleep_for(std::chrono::minutes(2));
                GetBatteryStatus(sps);
                currentBatteryPercent = sps.BatteryLifePercent;
                isConnectedToAC = sps.ACLineStatus == 1;
            }
            prevBatteryPercent = currentBatteryPercent;

            if (isConnectedToAC && currentBatteryPercent > 94) {
                Beeps(1);
            }
            std::wstring batteryPercentStr = std::to_wstring(currentBatteryPercent) + L"%";
            ShowBigMessage(batteryPercentStr);
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
        else {
            MessageBox(NULL, L"Failed to Get Battery Status", L"Error", MB_OK | MB_ICONERROR);            
            return 1;
        }
    }

    return 0;
}