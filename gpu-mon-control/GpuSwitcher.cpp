#include "GpuSwitcher.h"

#include <windows.h>

namespace
{
    constexpr wchar_t REGISTRY_PATH[] =
        L"Software\\Microsoft\\DirectX\\UserGpuPreferences";

    constexpr wchar_t REGISTRY_VALUE[] =
        L"DirectXUserGlobalSettings";
}

bool GpuSwitcher::GetCurrentAdapterId(
    std::wstring& adapterId,
    std::wstring& error) const
{
    HKEY key = nullptr;
    const LONG openResult = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        REGISTRY_PATH,
        0,
        KEY_QUERY_VALUE,
        &key);

    if (openResult != ERROR_SUCCESS)
    {
        error = L"Unable to open GPU preference registry key.";
        return false;
    }

    DWORD type = 0;
    DWORD byteCount = 0;
    LONG result = RegQueryValueExW(
        key,
        REGISTRY_VALUE,
        nullptr,
        &type,
        nullptr,
        &byteCount);

    if (result != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) ||
        byteCount < sizeof(wchar_t))
    {
        RegCloseKey(key);
        error = L"Unable to read DirectXUserGlobalSettings.";
        return false;
    }

    std::wstring value(byteCount / sizeof(wchar_t), L'\0');
    result = RegQueryValueExW(
        key,
        REGISTRY_VALUE,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(&value[0]),
        &byteCount);

    RegCloseKey(key);

    if (result != ERROR_SUCCESS)
    {
        error = L"Unable to read DirectXUserGlobalSettings.";
        return false;
    }

    if (!value.empty() && value.back() == L'\0')
    {
        value.pop_back();
    }

    adapterId = value;
    return true;
}

bool GpuSwitcher::SetAdapterId(
    const std::wstring& adapterId,
    std::wstring& error) const
{
    if (adapterId.empty())
    {
        error = L"The adapter_id must not be empty.";
        return false;
    }

    HKEY key = nullptr;
    DWORD disposition = 0;
    LONG result = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        REGISTRY_PATH,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &key,
        &disposition);

    if (result != ERROR_SUCCESS)
    {
        error = L"Unable to open GPU preference registry key for writing.";
        return false;
    }

    const DWORD byteCount = static_cast<DWORD>(
        (adapterId.size() + 1) * sizeof(wchar_t));

    result = RegSetValueExW(
        key,
        REGISTRY_VALUE,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(adapterId.c_str()),
        byteCount);

    RegCloseKey(key);

    if (result != ERROR_SUCCESS)
    {
        error = L"Unable to write DirectXUserGlobalSettings.";
        return false;
    }

    return true;
}

bool GpuSwitcher::ToggleProfile(
    const Config& config,
    std::wstring& selectedProfile,
    std::wstring& error) const
{
    if (config.size() < 2)
    {
        error = L"Toggle requires at least two GPU profiles.";
        return false;
    }

    std::wstring currentAdapter;

    if (!GetCurrentAdapterId(currentAdapter, error))
    {
        return false;
    }

    Config::const_iterator current = config.end();

    for (Config::const_iterator iterator = config.begin();
         iterator != config.end();
         ++iterator)
    {
        if (iterator->second.adapterId == currentAdapter)
        {
            current = iterator;
            break;
        }
    }

    if (current == config.end())
    {
        error = L"The current registry adapter_id does not match any configured profile.";
        return false;
    }

    Config::const_iterator next = current;
    ++next;

    if (next == config.end())
    {
        next = config.begin();
    }

    if (!SetAdapterId(next->second.adapterId, error))
    {
        return false;
    }

    selectedProfile = next->first;
    return true;
}
