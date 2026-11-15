#include "ConfigManager.h"

#include "ConfigManager.h"

#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <sstream>
#include <stdexcept>

namespace
{
    std::wstring Trim(const std::wstring& value)
    {
        std::wstring result = value;

        result.erase(result.begin(), std::find_if(
            result.begin(),
            result.end(),
            [](wchar_t character)
            {
                return !std::iswspace(character);
            }));

        result.erase(std::find_if(
            result.rbegin(),
            result.rend(),
            [](wchar_t character)
            {
                return !std::iswspace(character);
            }).base(), result.end());

        return result;
    }

    std::wstring Unquote(const std::wstring& value)
    {
        const std::wstring result = Trim(value);

        if (result.size() >= 2 &&
            ((result.front() == L'"' && result.back() == L'"') ||
             (result.front() == L'\'' && result.back() == L'\'')))
        {
            return result.substr(1, result.size() - 2);
        }

        return result;
    }

    bool SplitKeyValue(
        const std::wstring& line,
        std::wstring& key,
        std::wstring& value)
    {
        const std::size_t separator = line.find(L':');

        if (separator == std::wstring::npos)
        {
            return false;
        }

        key = Trim(line.substr(0, separator));
        value = Unquote(line.substr(separator + 1));
        return !key.empty();
    }

    std::wstring WideFromUtf8(const std::string& value)
    {
        if (value.empty())
        {
            return std::wstring();
        }

        const int length = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0);

        if (length <= 0)
        {
            return std::wstring();
        }

        std::wstring result(static_cast<std::size_t>(length), L'\0');
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            &result[0],
            length);
        return result;
    }

    std::string Utf8FromWide(const std::wstring& value)
    {
        if (value.empty())
        {
            return std::string();
        }

        const int length = WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (length <= 0)
        {
            return std::string();
        }

        std::string result(static_cast<std::size_t>(length), '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            &result[0],
            length,
            nullptr,
            nullptr);
        return result;
    }

    bool IsTopology(const std::wstring& value)
    {
        return value == L"internal" ||
            value == L"external" ||
            value == L"extend";
    }
}

ConfigManager::ConfigManager()
    : baseResolutionX_(1920.0)
    , baseResolutionY_(1080.0)
{
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(
        nullptr,
        buffer,
        MAX_PATH);

    if (length == 0 || length == MAX_PATH)
    {
        return;
    }

    path_ = std::wstring(buffer, length);
    const std::size_t separator = path_.find_last_of(L"\\/");

    if (separator == std::wstring::npos)
    {
        path_ = L"config.yaml";
    }
    else
    {
        path_.resize(separator + 1);
        path_ += L"config.yaml";
    }
}

bool ConfigManager::Load(Config& config, std::wstring& error) const
{
    std::wstring content;
    toggleHotkey_.clear();
    baseResolutionX_ = 1920.0;
    baseResolutionY_ = 1080.0;

    if (!ReadFile(content, error))
    {
        std::wstring createError;

        if (WriteDefaultTemplate(createError))
        {
            error = L"Config file was missing. A template was created at: " +
                path_ +
                L". Populate the hardware adapter IDs and retry.";
        }
        else
        {
            error += L" Template creation failed: " + createError;
        }

        return false;
    }

    return Parse(content, config, error);
}

bool ConfigManager::WriteDefaultTemplate(std::wstring& error) const
{
    const std::wstring content =
        L"general:\n"
        L"  toggle_hotkey: \"Shift+F8\"\n"
        L"  base_resolution_x: 1920\n"
        L"  base_resolution_y: 1080\n"
        L"gpus:\n"
        L"  primary:\n"
        L"    adapter_id: \"HighPerfAdapter=0xABC;\"\n"
        L"    topology: \"internal\"\n"
        L"  secondary:\n"
        L"    adapter_id: \"HighPerfAdapter=0xDEF;\"\n"
        L"    topology: \"external\"\n";

    return WriteFile(content, error);
}

const std::wstring& ConfigManager::GetPath() const
{
    return path_;
}

const std::wstring& ConfigManager::GetToggleHotkey() const
{
    return toggleHotkey_;
}

double ConfigManager::GetBaseResolutionX() const
{
    return baseResolutionX_;
}

double ConfigManager::GetBaseResolutionY() const
{
    return baseResolutionY_;
}

bool ConfigManager::ReadFile(
    std::wstring& content,
    std::wstring& error) const
{
    if (path_.empty())
    {
        error = L"Unable to determine the executable directory.";
        return false;
    }

    HANDLE file = CreateFileW(
        path_.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        error = L"Unable to open config file: " + path_;
        return false;
    }

    LARGE_INTEGER size = {};
    const bool sizeRead = GetFileSizeEx(file, &size) != FALSE;

    if (!sizeRead || size.QuadPart < 0 || size.QuadPart > 16 * 1024 * 1024)
    {
        CloseHandle(file);
        error = L"Config file has an invalid size.";
        return false;
    }

    std::string bytes(static_cast<std::size_t>(size.QuadPart), '\0');
    DWORD bytesRead = 0;
    const bool read = bytes.empty() || ::ReadFile(
        file,
        &bytes[0],
        static_cast<DWORD>(bytes.size()),
        &bytesRead,
        nullptr) != FALSE;

    CloseHandle(file);

    if (!read || bytesRead != bytes.size())
    {
        error = L"Unable to read config file: " + path_;
        return false;
    }

    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF)
    {
        bytes.erase(0, 3);
    }

    content = WideFromUtf8(bytes);

    if (!bytes.empty() && content.empty())
    {
        error = L"Config file is not valid UTF-8.";
        return false;
    }

    return true;
}

bool ConfigManager::WriteFile(
    const std::wstring& content,
    std::wstring& error) const
{
    if (path_.empty())
    {
        error = L"Unable to determine the executable directory.";
        return false;
    }

    const std::string bytes = Utf8FromWide(content);
    HANDLE file = CreateFileW(
        path_.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        error = L"Unable to create config file: " + path_;
        return false;
    }

    DWORD bytesWritten = 0;
    const bool written = bytes.empty() || ::WriteFile(
        file,
        bytes.data(),
        static_cast<DWORD>(bytes.size()),
        &bytesWritten,
        nullptr) != FALSE;

    CloseHandle(file);

    if (!written || bytesWritten != bytes.size())
    {
        error = L"Unable to write config file: " + path_;
        return false;
    }

    return true;
}

bool ConfigManager::Parse(
    const std::wstring& content,
    Config& config,
    std::wstring& error) const
{
    config.clear();

    std::wistringstream stream(content);
    std::wstring line;
    std::wstring currentProfile;
    bool inGpus = false;
    bool inGeneral = false;
    bool hasGpus = false;
    int lineNumber = 0;

    while (std::getline(stream, line))
    {
        ++lineNumber;

        if (!line.empty() && line.back() == L'\r')
        {
            line.pop_back();
        }

        const std::wstring trimmed = Trim(line);

        if (trimmed.empty() || trimmed.front() == L'#')
        {
            continue;
        }

        const std::size_t indentation = line.find_first_not_of(L" \t");
        const std::size_t spaces = indentation == std::wstring::npos
            ? line.size()
            : indentation;

        std::wstring key;
        std::wstring value;

        if (spaces == 0)
        {
            const std::wstring section = Trim(line);

            if (section == L"gpus:")
            {
                inGpus = true;
                inGeneral = false;
                hasGpus = true;
                currentProfile.clear();
                continue;
            }

            if (section == L"general:")
            {
                inGpus = false;
                inGeneral = true;
                currentProfile.clear();
                continue;
            }

            if (section != L"gpus:" && section != L"general:")
            {
                error = L"Expected 'gpus:' at line " +
                    std::to_wstring(lineNumber) + L".";
                return false;
            }

        }

        if (!inGpus && !inGeneral)
        {
            error = L"Unexpected content before 'gpus:' at line " +
                std::to_wstring(lineNumber) + L".";
            return false;
        }

        if (spaces == 2)
        {
            if (inGeneral)
            {
                if (!SplitKeyValue(trimmed, key, value) ||
                    (key != L"toggle_hotkey" &&
                     key != L"base_resolution_x" &&
                     key != L"base_resolution_y"))
                {
                    error = L"Invalid General property at line " +
                        std::to_wstring(lineNumber) + L".";
                    return false;
                }

                if (key == L"toggle_hotkey")
                {
                    toggleHotkey_ = value.empty()
                        ? L"Shift+F8"
                        : value;
                }
                else
                {
                    try
                    {
                        const double resolution = std::stod(value);

                        if (resolution <= 0.0)
                        {
                            throw std::invalid_argument("non-positive resolution");
                        }

                        if (key == L"base_resolution_x")
                            baseResolutionX_ = resolution;
                        else
                            baseResolutionY_ = resolution;
                    }
                    catch (const std::exception&)
                    {
                        error = L"Invalid base resolution at line " +
                            std::to_wstring(lineNumber) + L".";
                        return false;
                    }
                }
                continue;
            }

            if (trimmed.back() != L':' ||
                !SplitKeyValue(trimmed, key, value) ||
                !value.empty())
            {
                error = L"Invalid GPU profile at line " +
                    std::to_wstring(lineNumber) + L".";
                return false;
            }

            currentProfile = key;

            if (config.find(currentProfile) != config.end())
            {
                error = L"Duplicate GPU profile: " + currentProfile;
                return false;
            }

            config[currentProfile] = GpuProfile();
            continue;
        }

        if (spaces == 4 && inGpus && !currentProfile.empty())
        {
            if (!SplitKeyValue(trimmed, key, value))
            {
                error = L"Invalid profile property at line " +
                    std::to_wstring(lineNumber) + L".";
                return false;
            }

            GpuProfile& profile = config[currentProfile];

            if (key == L"adapter_id")
            {
                profile.adapterId = value;
            }
            else if (key == L"topology")
            {
                profile.topology = value;
            }
            else
            {
                error = L"Unknown profile property: " + key;
                return false;
            }

            continue;
        }

        error = L"Invalid indentation at line " +
            std::to_wstring(lineNumber) + L".";
        return false;
    }

    if (!hasGpus || config.empty())
    {
        error = L"Config must contain at least one GPU profile under 'gpus:'.";
        return false;
    }

    for (Config::const_iterator iterator = config.begin();
         iterator != config.end();
         ++iterator)
    {
        if (iterator->second.adapterId.empty())
        {
            error = L"Profile has an empty adapter_id: " + iterator->first;
            return false;
        }

        if (!IsTopology(iterator->second.topology))
        {
            error = L"Invalid topology in profile: " + iterator->first;
            return false;
        }
    }

    return true;
}
