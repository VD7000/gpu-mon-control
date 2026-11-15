#pragma once

#pragma once

#include <string>

class CliDispatcher
{
public:
    CliDispatcher(int argc, wchar_t* argv[]);

    int Execute();

    bool IsVerbose() const;
    bool HasOutput() const;

    const std::wstring& GetOutput() const;
    const std::wstring& GetError() const;

private:
    int argc_;
    wchar_t** argv_;
    bool verbose_;
    std::wstring output_;
    std::wstring error_;
};
