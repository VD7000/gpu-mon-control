#include "CliDispatcher.h"

#include "CliDispatcher.h"

#include <cwchar>

#include "ConfigManager.h"
#include "DisplayController.h"
#include "GpuSwitcher.h"

namespace
{
    bool IsArgument(const wchar_t* argument, const wchar_t* value)
    {
        return argument != nullptr && value != nullptr &&
            wcscmp(argument, value) == 0;
    }

    int FindCommandIndex(int argc, wchar_t* argv[])
    {
        for (int index = 1; index < argc; ++index)
        {
            if (!IsArgument(argv[index], L"--verbose"))
            {
                return index;
            }
        }

        return -1;
    }
}

CliDispatcher::CliDispatcher(int argc, wchar_t* argv[])
    : argc_(argc)
    , argv_(argv)
    , verbose_(false)
{
}

int CliDispatcher::Execute()
{
    if (argc_ <= 1 || argv_ == nullptr)
    {
        error_ = L"No CLI command was specified.";
        return 1;
    }

    for (int index = 1; index < argc_; ++index)
    {
        if (IsArgument(argv_[index], L"--verbose"))
        {
            verbose_ = true;
        }
    }

    const int commandIndex = FindCommandIndex(argc_, argv_);
    const wchar_t* command = commandIndex >= 0
        ? argv_[commandIndex]
        : nullptr;

    if (IsArgument(command, L"--help") ||
        IsArgument(command, L"-h") ||
        IsArgument(command, L"/?"))
    {
        output_ =
            L"gpu-mon-control\n"
            L"\n"
            L"Usage:\n"
            L"  gpu-mon-control.exe --init-config\n"
            L"  gpu-mon-control.exe switch <profile_name>\n"
            L"  gpu-mon-control.exe toggle\n"
            L"  gpu-mon-control.exe --help\n"
            L"  gpu-mon-control.exe --verbose --help\n";
        return 0;
    }

    ConfigManager configManager;

    if (IsArgument(command, L"--init-config"))
    {
        if (!configManager.WriteDefaultTemplate(error_))
        {
            return 1;
        }

        output_ = L"Created config file: " + configManager.GetPath();
        return 0;
    }

    Config config;

    if (!configManager.Load(config, error_))
    {
        return 1;
    }

    GpuSwitcher gpuSwitcher;
    DisplayController displayController;
    std::wstring selectedProfile;

    if (IsArgument(command, L"switch"))
    {
        if (commandIndex + 1 >= argc_ ||
            IsArgument(argv_[commandIndex + 1], L"--verbose"))
        {
            error_ = L"Usage: gpu-mon-control.exe switch <profile_name>";
            return 1;
        }

        selectedProfile = argv_[commandIndex + 1];
        const Config::const_iterator profile = config.find(selectedProfile);

        if (profile == config.end())
        {
            error_ = L"Unknown GPU profile: " + selectedProfile;
            return 1;
        }

        TopologyMode topology;

        if (!DisplayController::ParseTopology(
                profile->second.topology,
                topology))
        {
            error_ = L"Invalid topology in profile: " + selectedProfile;
            return 1;
        }

        if (!gpuSwitcher.SetAdapterId(profile->second.adapterId, error_))
        {
            return 1;
        }

        if (!displayController.ApplyTopology(topology, error_))
        {
            return 1;
        }

        output_ = L"Switched to GPU profile: " + selectedProfile;
        return 0;
    }

    if (IsArgument(command, L"toggle"))
    {
        if (!gpuSwitcher.ToggleProfile(
                config,
                selectedProfile,
                error_))
        {
            return 1;
        }

        const Config::const_iterator profile = config.find(selectedProfile);
        TopologyMode topology;

        if (profile == config.end() ||
            !DisplayController::ParseTopology(
                profile->second.topology,
                topology))
        {
            error_ = L"Invalid topology in profile: " + selectedProfile;
            return 1;
        }

        if (!displayController.ApplyTopology(topology, error_))
        {
            return 1;
        }

        output_ = L"Toggled to GPU profile: " + selectedProfile;
        return 0;
    }

    error_ = L"Unknown command: ";
    error_ += command != nullptr ? command : L"<null>";
    return 1;
}

bool CliDispatcher::IsVerbose() const
{
    return verbose_;
}

bool CliDispatcher::HasOutput() const
{
    return !output_.empty();
}

const std::wstring& CliDispatcher::GetOutput() const
{
    return output_;
}

const std::wstring& CliDispatcher::GetError() const
{
    return error_;
}
