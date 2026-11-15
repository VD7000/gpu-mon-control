#include <windows.h>
#include <cmath>
#include <iostream>

// Fixed reference-resolution constants.
constexpr double BASE_X = 2560.0;
constexpr double BASE_Y = 1080.0;

// Mouse-hook state.
HHOOK hMouseHook = nullptr;
double g_ScaleX = 1.0;
double g_ScaleY = 1.0;
POINT lastPoint = { 0, 0 };
bool isModifying = false;

void RecalculateScales()
{
    const double curX = static_cast<double>(GetSystemMetrics(SM_CXSCREEN));
    const double curY = static_cast<double>(GetSystemMetrics(SM_CYSCREEN));

    if (curX > 0.0 && curY > 0.0)
    {
        double rawX = (curX / BASE_X) * (BASE_Y / curY);
        double rawY = 1.0;

        // Direct sensitivity compensation based on the axis ratio.
        double speedCompensation = rawY / rawX;

        // Final correction without reducing the intended mouse speed.
        g_ScaleX = rawX * speedCompensation; // Always evaluates to 1.0.
        g_ScaleY = rawY * speedCompensation; // Adjusts Y to the target proportion.


        std::cout << "[SYSTEM] RESOLUTION: "
            << curX << "x" << curY
            << " | X: " << g_ScaleX
            << " | Y: " << g_ScaleY
            << std::endl;
    }

}


// Checks whether the system cursor is visible.
bool IsCursorVisible()
{
    CURSORINFO cursorInfo = {};
    cursorInfo.cbSize = sizeof(cursorInfo);

    if (GetCursorInfo(&cursorInfo))
    {
        return (cursorInfo.flags & CURSOR_SHOWING) != 0;
    }

    return false;
}

// Low-level mouse-hook callback.
LRESULT CALLBACK LowLevelMouseProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_MOUSEMOVE)
    {
        if (g_ScaleX == 1.0 && g_ScaleY == 1.0)
        {
            return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
        }

        const MSLLHOOKSTRUCT* mouseStruct =
            reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);

        // Pass input through unchanged while the cursor is hidden.
        if (isModifying || !IsCursorVisible())
        {
            lastPoint = mouseStruct->pt;
            return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
        }

        const int dx = mouseStruct->pt.x - lastPoint.x;
        const int dy = mouseStruct->pt.y - lastPoint.y;

        if (dx != 0 || dy != 0)
        {
            isModifying = true;

            const int newX = lastPoint.x +
                static_cast<int>(std::round(dx * g_ScaleX));

            const int newY = lastPoint.y +
                static_cast<int>(std::round(dy * g_ScaleY));

            SetCursorPos(newX, newY);

            lastPoint.x = newX;
            lastPoint.y = newY;

            isModifying = false;

            // Suppress the original unmodified movement.
            return 1;
        }

        lastPoint = mouseStruct->pt;
    }

    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

// Window message procedure.
LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (msg == WM_DISPLAYCHANGE)
    {
        RecalculateScales();
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main()
{
    // The source file should be saved as UTF-8.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    RecalculateScales();

    if (!GetCursorPos(&lastPoint))
    {
        std::cerr << "[ERROR] Не удалось получить позицию курсора."
                  << std::endl;
        return 1;
    }

    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = GetModuleHandle(nullptr);
    windowClass.lpszClassName = L"UltraWideMouseFixer";

    if (!RegisterClass(&windowClass) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::cerr << "[ERROR] Не удалось зарегистрировать класс окна."
                  << std::endl;
        return 1;
    }

    HWND hwnd = CreateWindow(
        L"UltraWideMouseFixer",
        nullptr,
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        nullptr,
        nullptr);

    if (!hwnd)
    {
        std::cerr << "[ERROR] Не удалось создать message-only окно."
                  << std::endl;
        return 1;
    }

    hMouseHook = SetWindowsHookEx(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        GetModuleHandle(nullptr),
        0);

    if (!hMouseHook)
    {
        std::cerr << "[ERROR] Не удалось запустить хук."
                  << std::endl;
        return 1;
    }

    MSG msg = {};

    while (true)
    {
        const BOOL result = GetMessage(&msg, nullptr, 0, 0);

        if (result == 0)
        {
            break;
        }

        if (result == -1)
        {
            std::cerr << "[ERROR] Ошибка обработки очереди сообщений."
                      << std::endl;
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hMouseHook);
    return 0;
}