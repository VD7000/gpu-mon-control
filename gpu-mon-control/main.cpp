#include <windows.h>

#include <windows.h>
#include <shellapi.h>

#include <iostream>

#include "CliDispatcher.h"
#include "ConfigManager.h"
#include "DisplayController.h"
#include "GpuSwitcher.h"
#include "MouseScaler.h"
#include "TrayController.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:wWinMainCRTStartup")
#pragma comment(lib, "Shell32.lib")

namespace
{
    constexpr wchar_t WINDOW_CLASS_NAME[] = L"UltraWideMouseFixer";
    constexpr wchar_t MUTEX_NAME[] = L"GpuMonControl_MouseScaler";

    MouseScaler* g_MouseScaler = nullptr;
    ConfigManager* g_ConfigManager = nullptr;
    Config* g_Config = nullptr;
    TrayController* g_Tray = nullptr;
    HWND g_DaemonWindow = nullptr;

    std::wstring ActiveProfile()
    {
        if (g_Config == nullptr)
        {
            return std::wstring();
        }

        GpuSwitcher switcher;
        std::wstring adapter;
        std::wstring error;

        if (!switcher.GetCurrentAdapterId(adapter, error))
        {
            return std::wstring();
        }

        for (Config::const_iterator iterator = g_Config->begin();
             iterator != g_Config->end();
             ++iterator)
        {
            if (iterator->second.adapterId == adapter)
            {
                return iterator->first;
            }
        }

        return std::wstring();
    }

    void UpdateTray()
    {
        if (g_Tray != nullptr && g_MouseScaler != nullptr)
        {
            g_Tray->Update(
                ActiveProfile(),
                g_MouseScaler->IsInstalled());
        }
    }

    bool ApplyProfile(const std::wstring& name)
    {
        if (g_Config == nullptr)
        {
            return false;
        }

        const Config::const_iterator profile = g_Config->find(name);
        if (profile == g_Config->end())
        {
            return false;
        }

        TopologyMode topology;
        if (!DisplayController::ParseTopology(
                profile->second.topology,
                topology))
        {
            return false;
        }

        GpuSwitcher switcher;
        DisplayController display;
        std::wstring error;

        if (!switcher.SetAdapterId(profile->second.adapterId, error) ||
            !display.ApplyTopology(topology, error))
        {
            return false;
        }

        UpdateTray();
        return true;
    }

    bool ToggleProfile()
    {
        if (g_Config == nullptr)
        {
            return false;
        }

        GpuSwitcher switcher;
        std::wstring selected;
        std::wstring error;

        if (!switcher.ToggleProfile(*g_Config, selected, error))
        {
            return false;
        }

        const Config::const_iterator profile = g_Config->find(selected);
        TopologyMode topology;
        DisplayController display;

        if (profile == g_Config->end() ||
            !DisplayController::ParseTopology(
                profile->second.topology,
                topology) ||
            !display.ApplyTopology(topology, error))
        {
            return false;
        }

        UpdateTray();
        return true;
    }

    bool ParseHotkey(
        const std::wstring& value,
        UINT& modifiers,
        UINT& virtualKey)
    {
        modifiers = 0;
        virtualKey = 0;
        std::size_t start = 0;

        while (start < value.size())
        {
            const std::size_t end = value.find(L'+', start);
            const std::wstring part = value.substr(
                start,
                end == std::wstring::npos ? end : end - start);

            if (_wcsicmp(part.c_str(), L"Ctrl") == 0)
                modifiers |= MOD_CONTROL;
            else if (_wcsicmp(part.c_str(), L"Alt") == 0)
                modifiers |= MOD_ALT;
            else if (_wcsicmp(part.c_str(), L"Shift") == 0)
                modifiers |= MOD_SHIFT;
            else if (_wcsicmp(part.c_str(), L"Win") == 0)
                modifiers |= MOD_WIN;
            else if (part.size() == 1)
                virtualKey = VkKeyScanW(part[0]) & 0xFF;
            else if (_wcsnicmp(part.c_str(), L"F", 1) == 0)
            {
                const int functionKey = _wtoi(part.c_str() + 1);
                if (functionKey >= 1 && functionKey <= 24)
                    virtualKey = VK_F1 + functionKey - 1;
            }

            if (end == std::wstring::npos)
                break;
            start = end + 1;
        }

        return modifiers != 0 && virtualKey != 0;
    }

    void RegisterConfiguredHotkey()
    {
        if (g_DaemonWindow == nullptr || g_ConfigManager == nullptr)
            return;

        UnregisterHotKey(g_DaemonWindow, 1);
        const std::wstring& hotkey = g_ConfigManager->GetToggleHotkey();
        UINT modifiers = 0;
        UINT virtualKey = 0;

        if (!hotkey.empty() &&
            ParseHotkey(hotkey, modifiers, virtualKey))
        {
            if (!RegisterHotKey(
                    g_DaemonWindow,
                    1,
                    modifiers,
                    virtualKey))
            {
                OutputDebugStringW(
                    L"gpu-mon-control: unable to register configured hotkey.\n");
            }
        }
    }

    void ShowTrayMenu(HWND hwnd)
    {
        HMENU menu = CreatePopupMenu();
        if (menu == nullptr)
            return;

        AppendMenuW(menu, MF_STRING, 1, L"Toggle GPU");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

        UINT command = 100;
        const std::wstring active = ActiveProfile();
        for (Config::const_iterator iterator = g_Config->begin();
             iterator != g_Config->end();
             ++iterator, ++command)
        {
            AppendMenuW(
                menu,
                MF_STRING | (iterator->first == active ? MF_CHECKED : 0),
                command,
                iterator->first.c_str());
        }

        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(
            menu,
            MF_STRING | (g_MouseScaler->IsInstalled() ? MF_CHECKED : 0),
            200,
            L"Mouse Scaler Active");
        AppendMenuW(menu, MF_STRING, 201, L"Reload config.yaml");
        AppendMenuW(menu, MF_STRING, 202, L"Open config.yaml");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, 203, L"Exit");

        SetForegroundWindow(hwnd);
        POINT point = {};
        GetCursorPos(&point);
        const UINT selected = TrackPopupMenu(
            menu,
            TPM_RETURNCMD | TPM_NONOTIFY,
            point.x,
            point.y,
            0,
            hwnd,
            nullptr);

        if (selected == 1)
            ToggleProfile();
        else if (selected >= 100 && selected < command)
        {
            command = 100;
            for (Config::const_iterator iterator = g_Config->begin();
                 iterator != g_Config->end();
                 ++iterator, ++command)
            {
                if (command == selected)
                {
                    ApplyProfile(iterator->first);
                    break;
                }
            }
        }
        else if (selected == 200)
        {
            if (g_MouseScaler->IsInstalled())
                g_MouseScaler->Uninstall();
            else
                g_MouseScaler->Install();
            UpdateTray();
        }
        else if (selected == 201)
        {
            std::wstring error;
            Config reloaded;
            if (g_ConfigManager->Load(reloaded, error))
            {
                *g_Config = reloaded;
                g_MouseScaler->SetBaseResolution(
                    g_ConfigManager->GetBaseResolutionX(),
                    g_ConfigManager->GetBaseResolutionY());
                RegisterConfiguredHotkey();
                UpdateTray();
            }
        }
        else if (selected == 202)
        {
            ShellExecuteW(
                nullptr,
                L"open",
                g_ConfigManager->GetPath().c_str(),
                nullptr,
                nullptr,
                SW_SHOWNORMAL);
        }
        else if (selected == 203)
        {
            PostQuitMessage(0);
        }

        DestroyMenu(menu);
    }

    bool AttachToParentConsole()
    {
        if (GetConsoleWindow() != nullptr)
        {
            return true;
        }

        return AttachConsole(ATTACH_PARENT_PROCESS) != FALSE;
    }

    void WriteConsoleText(
        const std::wstring& text,
        DWORD standardHandle)
    {
        HANDLE output = GetStdHandle(standardHandle);

        if (output == nullptr || output == INVALID_HANDLE_VALUE)
        {
            return;
        }

        DWORD written = 0;

        if (WriteConsoleW(
                output,
                text.c_str(),
                static_cast<DWORD>(text.size()),
                &written,
                nullptr) != FALSE)
        {
            return;
        }

        const int byteCount = WideCharToMultiByte(
            CP_UTF8,
            0,
            text.c_str(),
            static_cast<int>(text.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (byteCount <= 0)
        {
            return;
        }

        std::string utf8(static_cast<std::size_t>(byteCount), '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            text.c_str(),
            static_cast<int>(text.size()),
            &utf8[0],
            byteCount,
            nullptr,
            nullptr);

        WriteFile(
            output,
            utf8.data(),
            static_cast<DWORD>(utf8.size()),
            &written,
            nullptr);
    }

    int RunCli(int argc, wchar_t* argv[])
    {
        CliDispatcher dispatcher(argc, argv);
        const int result = dispatcher.Execute();

        if (dispatcher.IsVerbose() || dispatcher.HasOutput() || result != 0)
        {
            if (AttachToParentConsole())
            {
                if (dispatcher.HasOutput())
                {
                    WriteConsoleText(
                        dispatcher.GetOutput(),
                        STD_OUTPUT_HANDLE);
                }

                if (result != 0)
                {
                    WriteConsoleText(
                        L"[ERROR] " + dispatcher.GetError() + L"\n",
                        STD_ERROR_HANDLE);
                }
            }
        }

        return result;
    }

    LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        if (message == WM_DISPLAYCHANGE && g_MouseScaler != nullptr)
        {
            g_MouseScaler->RecalculateScales();
            UpdateTray();
        }

        if (message == WM_HOTKEY && wParam == 1)
        {
            ToggleProfile();
        }

        if (message == TrayController::WM_TRAYICON)
        {
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU)
            {
                ShowTrayMenu(hwnd);
            }
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

int RunDaemon()
{
    ConfigManager configManager;
    Config daemonConfig;
    std::wstring configMessage;

    if (!configManager.Load(daemonConfig, configMessage))
    {
        if (configMessage.find(L"Config file was missing.") == 0)
        {
            OutputDebugStringW((configMessage + L"\n").c_str());
        }
        else
        {
            return 1;
        }
    }

    HANDLE mutex = CreateMutexW(
        nullptr,
        TRUE,
        MUTEX_NAME);

    if (mutex == nullptr)
    {
        std::cerr << "[ERROR] Не удалось создать mutex."
                  << std::endl;
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        std::cerr << "[ERROR] Экземпляр daemon уже запущен."
                  << std::endl;

        CloseHandle(mutex);
        return 1;
    }

    MouseScaler mouseScaler;
    g_MouseScaler = &mouseScaler;
    g_ConfigManager = &configManager;
    g_Config = &daemonConfig;
    mouseScaler.SetBaseResolution(
        configManager.GetBaseResolutionX(),
        configManager.GetBaseResolutionY());

    mouseScaler.RecalculateScales();

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassW(&windowClass) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::cerr << "[ERROR] Не удалось зарегистрировать класс окна."
                  << std::endl;

        CloseHandle(mutex);
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        nullptr,
        WS_OVERLAPPED,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    if (hwnd == nullptr)
    {
        std::cerr << "[ERROR] Не удалось создать скрытое окно."
                  << std::endl;

        CloseHandle(mutex);
        return 1;
    }

    g_DaemonWindow = hwnd;

    TrayController tray;
    g_Tray = &tray;

    if (!tray.Initialize(
            hwnd,
            ActiveProfile(),
            mouseScaler.IsInstalled()))
    {
        g_Tray = nullptr;
        g_DaemonWindow = nullptr;
        DestroyWindow(hwnd);
        CloseHandle(mutex);
        return 1;
    }

    RegisterConfiguredHotkey();

    if (!mouseScaler.Install())
    {
        UnregisterHotKey(hwnd, 1);
        tray.Shutdown();
        g_Tray = nullptr;
        g_DaemonWindow = nullptr;
        DestroyWindow(hwnd);
        CloseHandle(mutex);
        return 1;
    }

    UpdateTray();

    MSG message = {};

    while (true)
    {
        const BOOL result = GetMessageW(
            &message,
            nullptr,
            0,
            0);

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

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    mouseScaler.Uninstall();
    UnregisterHotKey(hwnd, 1);
    tray.Shutdown();
    g_Tray = nullptr;
    g_DaemonWindow = nullptr;
    g_Config = nullptr;
    g_ConfigManager = nullptr;
    g_MouseScaler = nullptr;
    DestroyWindow(hwnd);
    CloseHandle(mutex);

    return 0;
}

int WINAPI wWinMain(
    HINSTANCE,
    HINSTANCE,
    PWSTR,
    int)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(
        GetCommandLineW(),
        &argc);

    if (argv == nullptr)
    {
        return 1;
    }

    const int result = argc <= 1
        ? RunDaemon()
        : RunCli(argc, argv);

    LocalFree(argv);
    return result;
}
